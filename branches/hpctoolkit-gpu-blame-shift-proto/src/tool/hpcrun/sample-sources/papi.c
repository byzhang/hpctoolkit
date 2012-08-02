// -*-Mode: C++;-*- // technically C99

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2002-2012, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *

//
// PAPI sample source simple oo interface
//

/******************************************************************************
 * system includes
 *****************************************************************************/

#include <alloca.h>
#include <assert.h>
#include <ctype.h>
#include <papi.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ucontext.h>
#include <stdbool.h>

#include <pthread.h>

/******************************************************************************
 * libmonitor
 *****************************************************************************/

#include <monitor.h>


/******************************************************************************
 * local includes
 *****************************************************************************/

#include "simple_oo.h"
#include "sample_source_obj.h"
#include "common.h"

#include <hpcrun/hpcrun_options.h>
#include <hpcrun/hpcrun_stats.h>
#include <hpcrun/metrics.h>
#include <hpcrun/safe-sampling.h>
#include <hpcrun/sample_sources_registered.h>
#include <hpcrun/sample_event.h>
#include <hpcrun/thread_data.h>
#include <utilities/tokenize.h>
#include <messages/messages.h>
#include <lush/lush-backtrace.h>
#include <lib/prof-lean/hpcrun-fmt.h>


/******************************************************************************
 * macros
 *****************************************************************************/


#define OVERFLOW_MODE 0
#define WEIGHT_METRIC 0
#define DEFAULT_THRESHOLD  2000000L

/******************************************************************************
 * forward declarations 
 *****************************************************************************/

static void papi_event_handler(int event_set, void *pc, long long ovec, void *context);
static int  event_is_derived(int ev_code);
static void event_fatal_error(int ev_code, int papi_ret);

/******************************************************************************
 * local variables
 *****************************************************************************/

static void
METHOD_FN(init)
{
  PAPI_set_debug(0x3ff);

  int ret = PAPI_library_init(PAPI_VER_CURRENT);
  TMSG(PAPI,"PAPI_library_init = %d", ret);
  TMSG(PAPI,"PAPI_VER_CURRENT =  %d", PAPI_VER_CURRENT);
  if (ret != PAPI_VER_CURRENT){
    STDERR_MSG("Fatal error: PAPI_library_init() failed with version mismatch.\n"
        "HPCToolkit was compiled with version 0x%x but run on version 0x%x.\n"
        "Check the HPCToolkit installation and try again.",
	PAPI_VER_CURRENT, ret);
    exit(1);
  }

  // Tell PAPI to count events in all contexts (user, kernel, etc).
  // FIXME: PAPI_DOM_ALL causes some syscalls to fail which then
  // breaks some applications.  For example, this breaks some Gemini
  // (GNI) functions called from inside gasnet_init() or MPI_Init() on
  // the Cray XE (hopper).
  if (ENABLED(SYSCALL_RISKY)) {
    ret = PAPI_set_domain(PAPI_DOM_ALL);
    if (ret != PAPI_OK) {
      EMSG("warning: PAPI_set_domain(PAPI_DOM_ALL) failed: %d", ret);
    }
  }

  self->state = INIT;
}

static void
METHOD_FN(thread_init)
{
  TMSG(PAPI, "thread init");
  int retval = PAPI_thread_init(pthread_self);
  if (retval != PAPI_OK) {
    EEMSG("PAPI_thread_init NOT ok, retval = %d", retval);
    monitor_real_abort();
  }
  TMSG(PAPI, "thread init OK");
}

static void
METHOD_FN(thread_init_action)
{
  TMSG(PAPI, "register thread");
  int retval = PAPI_register_thread();
  if (retval != PAPI_OK) {
    EEMSG("PAPI_register_thread NOT ok, retval = %d", retval);
    monitor_real_abort();
  }
  TMSG(PAPI, "register thread ok");
}

static void
METHOD_FN(start)
{
  thread_data_t *td = hpcrun_get_thread_data();
  int eventSet = td->eventSet[self->evset_idx];

  TMSG(PAPI,"starting PAPI w event set %d",eventSet);
  int ret = PAPI_start(eventSet);
  if (ret != PAPI_OK){
    EMSG("PAPI_start failed with %s (%d)", PAPI_strerror(ret), ret);
    hpcrun_ssfail_start("PAPI");
  }

  TD_GET(ss_state)[self->evset_idx] = START;
}

static void
METHOD_FN(thread_fini_action)
{
  TMSG(PAPI, "unregister thread");
  int retval = PAPI_unregister_thread();
  char msg[] = "!!NOT PAPI_OK!! (code = -9999999)\n";
  snprintf(msg, sizeof(msg)-1, "!!NOT PAPI_OK!! (code = %d)", retval);
  TMSG(PAPI, "unregister thread returns %s", retval == PAPI_OK? "PAPI_OK" : msg);
}

static void
METHOD_FN(stop)
{
  thread_data_t *td = hpcrun_get_thread_data();

  int eventSet = td->eventSet[self->evset_idx];
  int nevents  = self->evl.nevents;

  source_state_t my_state = TD_GET(ss_state)[self->evset_idx];

  if (my_state == STOP) {
    TMSG(PAPI,"--stop called on an already stopped event set %d",eventSet);
    return;
  }

  if (my_state != START) {
    TMSG(PAPI,"*WARNING* Stop called on event set that has not been started");
    return;
  }

  TMSG(PAPI,"stop w event set = %d",eventSet);
  long_long *values = (long_long *) alloca(sizeof(long_long) * (nevents+2));
  int ret = PAPI_stop(eventSet, values);
  if (ret != PAPI_OK){
    EMSG("Failed to stop papi f eventset %d. Return code = %d ==> %s",
	 eventSet,ret,PAPI_strerror(ret));
  }

  TD_GET(ss_state)[self->evset_idx] = STOP;
}

static void
METHOD_FN(shutdown)
{
  METHOD_CALL(self, stop); // make sure stop has been called
  PAPI_shutdown();

  self->state = UNINIT;
}

// Return true if PAPI recognizes the name, whether supported or not.
// We'll handle unsupported events later.
static bool
METHOD_FN(supports_event,const char *ev_str)
{
  if (self->state == UNINIT){
    METHOD_CALL(self, init);
  }
  
  char evtmp[1024];
  int ec;
  long th;

  hpcrun_extract_ev_thresh(ev_str, sizeof(evtmp), evtmp, &th, DEFAULT_THRESHOLD);
  return PAPI_event_name_to_code(evtmp, &ec) == PAPI_OK;
}
 
static void
METHOD_FN(process_event_list, int lush_metrics)
{
  char *event;
  int i, ret;
  int num_lush_metrics = 0;

  char* evlist = METHOD_CALL(self, get_event_str);
  for (event = start_tok(evlist); more_tok(); event = next_tok()) {
    char name[1024];
    int evcode;
    long thresh;

    TMSG(PAPI,"checking event spec = %s",event);
    if (! hpcrun_extract_ev_thresh(event, sizeof(name), name, &thresh, DEFAULT_THRESHOLD)) {
      AMSG("WARNING: %s using default threshold %ld, "
	   "better to use an explicit threshold.", name, DEFAULT_THRESHOLD);
    }
    ret = PAPI_event_name_to_code(name, &evcode);
    if (ret != PAPI_OK) {
      EMSG("unexpected failure in PAPI process_event_list(): "
	   "PAPI_event_name_to_code() returned %s (%d)",
	   PAPI_strerror(ret), ret);
      hpcrun_ssfail_unsupported("PAPI", name);
    }
    if (PAPI_query_event(evcode) != PAPI_OK) {
      hpcrun_ssfail_unsupported("PAPI", name);
    }

    // FIXME:LUSH: need a more flexible metric interface
    if (lush_metrics == 1 && strncmp(event, "PAPI_TOT_CYC", 12) == 0) {
      num_lush_metrics++;
    }

    TMSG(PAPI,"got event code = %x, thresh = %ld", evcode, thresh);
    METHOD_CALL(self, store_event, evcode, thresh);
  }
  int nevents = (self->evl).nevents;
  TMSG(PAPI,"nevents = %d", nevents);

  hpcrun_pre_allocate_metrics(nevents + num_lush_metrics);

  for (i = 0; i < nevents; i++) {
    char buffer[PAPI_MAX_STR_LEN];
    int metric_id = hpcrun_new_metric(); /* weight */
    METHOD_CALL(self, store_metric_id, i, metric_id);
    PAPI_event_code_to_name(self->evl.events[i].event, buffer);
    TMSG(PAPI, "metric for event %d = %s", i, buffer);
    hpcrun_set_metric_info_and_period(metric_id, strdup(buffer),
				      MetricFlags_ValFmt_Int,
				      self->evl.events[i].thresh);

    // FIXME:LUSH: need a more flexible metric interface
    if (num_lush_metrics > 0 && strcmp(buffer, "PAPI_TOT_CYC") == 0) {
      // there should be one lush metric; its source is the last event
      assert(num_lush_metrics == 1 && (i == (nevents - 1)));
      int mid_idleness = hpcrun_new_metric();
      lush_agents->metric_time = metric_id;
      lush_agents->metric_idleness = mid_idleness;

      hpcrun_set_metric_info_and_period(mid_idleness, "idleness",
					MetricFlags_ValFmt_Real,
					self->evl.events[i].thresh);
    }
  }
}

static void
METHOD_FN(gen_event_set,int lush_metrics)
{
  int i;
  int ret;
  int eventSet;

  eventSet = PAPI_NULL;
  TMSG(PAPI,"create event set");
  ret = PAPI_create_eventset(&eventSet);
  TMSG(PAPI,"PAPI_create_eventset = %d, eventSet = %d", ret, eventSet);
  if (ret != PAPI_OK) {
    hpcrun_abort("Failure: PAPI_create_eventset.Return code = %d ==> %s", 
		 ret, PAPI_strerror(ret));
  }

  int nevents = (self->evl).nevents;
  for (i = 0; i < nevents; i++) {
    int evcode = self->evl.events[i].event;
    ret = PAPI_add_event(eventSet, evcode);
    TMSG(PAPI, "PAPI_add_event(eventSet=%d, event_code=%x)", eventSet, evcode);
    if (ret != PAPI_OK) {
      EMSG("failure in PAPI gen_event_set(): PAPI_add_event() returned: %s (%d)",
	   PAPI_strerror(ret), ret);
      event_fatal_error(evcode, ret);
    }
  }

  for (i = 0; i < nevents; i++) {
    int evcode = self->evl.events[i].event;
    long thresh = self->evl.events[i].thresh;

    TMSG(PAPI, "PAPI_overflow(eventSet=%d, evcode=%x, thresh=%d)", eventSet, evcode, thresh);
    ret = PAPI_overflow(eventSet, evcode, thresh, OVERFLOW_MODE,
			papi_event_handler);
    TMSG(PAPI,"PAPI_overflow = %d", ret);
    if (ret != PAPI_OK) {
      EMSG("failure in PAPI gen_event_set(): PAPI_overflow() returned: %s (%d)",
	   PAPI_strerror(ret), ret);
      event_fatal_error(evcode, ret);
    }
  }
  thread_data_t *td = hpcrun_get_thread_data();
  td->eventSet[self->evset_idx] = eventSet;
}

static void
METHOD_FN(display_events)
{
  PAPI_event_info_t info;
  char name[200], *prof;
  int ev, ret, num_total, num_prof;

  printf("===========================================================================\n");
  printf("Available PAPI preset events\n");
  printf("===========================================================================\n");
  printf("Name\t    Profilable\tDescription\n");
  printf("---------------------------------------------------------------------------\n");

  num_total = 0;
  num_prof = 0;
  ev = PAPI_PRESET_MASK;
  ret = PAPI_OK;
#ifdef PAPI_ENUM_FIRST
  ret = PAPI_enum_event(&ev, PAPI_ENUM_FIRST);
#endif
  while (ret == PAPI_OK) {
    if (PAPI_query_event(ev) == PAPI_OK) {
      PAPI_event_code_to_name(ev, name);
      PAPI_get_event_info(ev, &info);
      if (event_is_derived(ev)) {
	prof = "No";
      } else {
	prof = "Yes";
	num_prof++;
      }
      num_total++;
      printf("%-10s\t%s\t%s\n", name, prof, info.long_descr);
    }
    ret = PAPI_enum_event(&ev, PAPI_ENUM_EVENTS);
  }
  printf("Total PAPI events: %d, able to profile: %d\n",
	 num_total, num_prof);
  printf("\n");

  printf("===========================================================================\n");
  printf("Available native events\n");
  printf("===========================================================================\n");
  printf("Name\t\t\t\tDescription\n");
  printf("---------------------------------------------------------------------------\n");

  num_total = 0;
  ev = PAPI_NATIVE_MASK;
  ret = PAPI_OK;
#ifdef PAPI_ENUM_FIRST
  ret = PAPI_enum_event(&ev, PAPI_ENUM_FIRST);
#endif
  while (ret == PAPI_OK) {
    if (PAPI_query_event(ev) == PAPI_OK) {
      PAPI_event_code_to_name(ev, name);
      PAPI_get_event_info(ev, &info);
      num_total++;
      printf("%-30s\t%s\n", name, info.long_descr);
    }
    ret = PAPI_enum_event(&ev, PAPI_ENUM_EVENTS);
  }
  printf("Total native events: %d\n", num_total);
  printf("\n");
}

/***************************************************************************
 * object
 ***************************************************************************/

#define ss_name papi
#define ss_cls SS_HARDWARE

#include "ss_obj.h"

/******************************************************************************
 * private operations 
 *****************************************************************************/

// Returns: 1 if the event code is a derived event.
// The papi_avail(1) utility shows how to do this.
static int
event_is_derived(int ev_code)
{
  PAPI_event_info_t info;

  // "Is derived" is kind of a bad thing, so if any unexpected failure
  // occurs, we'll return the "bad" answer.
  if (PAPI_get_event_info(ev_code, &info) != PAPI_OK
      || info.derived == NULL) {
    return 1;
  }
  if (info.count == 1
      || strlen(info.derived) == 0
      || strcmp(info.derived, "NOT_DERIVED") == 0
      || strcmp(info.derived, "DERIVED_CMPD") == 0) {
    return 0;
  }
  return 1;
}

static void
event_fatal_error(int ev_code, int papi_ret)
{
  char name[1024];

  PAPI_event_code_to_name(ev_code, name);
  if (PAPI_query_event(ev_code) != PAPI_OK) {
    hpcrun_ssfail_unsupported("PAPI", name);
  }
  if (event_is_derived(ev_code)) {
    hpcrun_ssfail_derived("PAPI", name);
  }
  if (papi_ret == PAPI_ECNFLCT) {
    hpcrun_ssfail_conflict("PAPI", name);
  }
  hpcrun_ssfail_unsupported("PAPI", name);
}

static void
papi_event_handler(int event_set, void *pc, long long ovec,
                   void *context)
{
  int i;
  int my_events[MAX_EVENTS];
  int my_event_count = MAX_EVENTS;

  // If the interrupt came from inside our code, then drop the sample
  // and return and avoid any MSG.
  if (! hpcrun_safe_enter_async(pc)) {
    hpcrun_stats_num_samples_blocked_async_inc();
    return;
  }

  TMSG(PAPI_SAMPLE,"papi event happened, ovec = %ld",ovec);

  int ret = PAPI_get_overflow_event_index(event_set, ovec, my_events,
					  &my_event_count);
  if (ret != PAPI_OK) {
    hpcrun_abort("Failed inside papi_event_handler at get_overflow_event_index."
		 "Return code = %d ==> %s", ret, PAPI_strerror(ret));
  }
  for (i = 0; i < my_event_count; i++) {
    // FIXME: SUBTLE ERROR: metric_id may not be same from hpcrun_new_metric()!
    // This means lush's 'time' metric should be *last*

    int metric_id = hpcrun_event2metric(&_papi_obj, my_events[i]);

    TMSG(PAPI_SAMPLE,"sampling call path for metric_id = %d", metric_id);

    hpcrun_sample_callpath(context, metric_id, 1/*metricIncr*/, 
			   0/*skipInner*/, 0/*isSync*/);
  }
  hpcrun_safe_exit();
}
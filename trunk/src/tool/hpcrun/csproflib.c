// -*-Mode: C++;-*- // technically C99
// $Id$

// * BeginRiceCopyright *****************************************************
/*
  Copyright ((c)) 2002, Rice University 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  * Neither the name of Rice University (RICE) nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

  This software is provided by RICE and contributors "as is" and any
  express or implied warranties, including, but not limited to, the
  implied warranties of merchantability and fitness for a particular
  purpose are disclaimed. In no event shall RICE or contributors be
  liable for any direct, indirect, incidental, special, exemplary, or
  consequential damages (including, but not limited to, procurement of
  substitute goods or services; loss of use, data, or profits; or
  business interruption) however caused and on any theory of liability,
  whether in contract, strict liability, or tort (including negligence
  or otherwise) arising in any way out of the use of this software, even
  if advised of the possibility of such damage.
*/
// ******************************************************* EndRiceCopyright *

//***************************************************************************
//
// File: 
//    $Source$
//
// Purpose:
//    Initialize, finalize csprof
//
// Description:
//    [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

// call stack profiler: profiles one process running on one processor
//   platforms: alpha tru64 unix
//     -- will profile accross a fork() ??? [this includes system()]
//     -- add support for profiling DSOs (use stuff from papiprof)
//     -- add support for profiling threads
//
//   limitations to implicit interface: 
//     - cannot profile any programs that are restricted from using
//       LD_PRELOAD (for security reasons)

//***************************************************************************
// system include files 
//***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>

//***************************************************************************
// libmonitor include files
//***************************************************************************

#include <monitor.h>

//***************************************************************************
// user include files 
//***************************************************************************

#include <include/uint.h>

#include "sample_sources_all.h"
#include "backtrace.h"
#include "cct.h"
#include "csproflib.h"
#include "csproflib_private.h"
#include "env.h"
#include "files.h"
#include "mem.h"
#include "segv_handler.h"
#include "epoch.h"
#include "metrics.h"
#include "trace.h"
#include "write_data.h"

#include "csprof_options.h"

#include "sample_event.h"
#include "state.h"

#include "thread_data.h"

#include "name.h"

#include "pmsg.h"
#include "unwind.h"
#include "fnbounds_interface.h"
#include "splay-interval.h"
#include "hpcrun_return_codes.h"
#include "lcp.h"

#include <lib/prof-lean/atomic.h>
#include <lib/prof-lean/hpcrun-fmt.h>
#include <lib/prof-lean/hpcio.h>

#include <lush/lush.h>
#include <lush/lush-backtrace.h>
#include <lush/lush-pthread.h>

//***************************************************************************

extern bool csprof_no_samples;

static csprof_options_t opts;

#if !defined(CSPROF_SYNCHRONOUS_PROFILING) 
sigset_t prof_sigset;
#endif

bool hpcrun_is_initialized_private = false;

extern void _start(void);
extern int __stop___libc_freeres_ptrs;

void *static_epoch_offset;
void *static_epoch_end;
long static_epoch_size;

int lush_metrics = 0; // FIXME: global variable for now


// FIXME: tallent: Move this code into main.c

//***************************************************************************
// process level 
//***************************************************************************

void
csprof_init_internal(void)
{
  csprof_epoch_init(csprof_static_epoch());

#if defined(OLD_THREAD)
  csprof_thread_data_init(0,CSPROF_MEM_SZ_DEFAULT,0, NULL);
#endif

  hpcrun_thread_data_new();
  hpcrun_thread_memory_init();
  hpcrun_thread_data_init(0, NULL);

  // WARNING: a perfmon bug requires us to fork off the fnbounds
  // server before we call PAPI_init, which is done in argument
  // processing below. Also, fnbounds_init must be done after the
  // memory allocator is initialized.
  fnbounds_init();
  csprof_options__init(&opts);
  csprof_options__getopts(&opts);

  trace_init(); // this must go after thread initialization
  trace_open();

  // Initialize LUSH agents
  if (opts.lush_agent_paths[0] != '\0') {
    csprof_state_t* state = TD_GET(state);
    TMSG(MALLOC," -init_internal-: lush allocation");
    lush_agents = (lush_agent_pool_t*)csprof_malloc(sizeof(lush_agent_pool_t));
    lush_agent_pool__init(lush_agents, opts.lush_agent_paths);
    EMSG("***> LUSH: %s (%p / %p) ***", opts.lush_agent_paths, 
	 state, lush_agents);
  }

  lush_metrics = (lush_agents) ? 1 : 0;

#ifdef LUSH_PTHREADS
  if (!lush_agents) {
    csprof_abort("LUSH Pthreads monitoring requires LUSH Pthreads agent!");
  }
#endif
  lushPthr_processInit();


  sigemptyset(&prof_sigset);
  sigaddset(&prof_sigset,SIGPROF);

  setup_segv();
  unw_init();

  // sample source setup
  SAMPLE_SOURCES(init);
  SAMPLE_SOURCES(process_event_list,lush_metrics);
  SAMPLE_SOURCES(gen_event_set,lush_metrics);
  SAMPLE_SOURCES(start);

  hpcrun_is_initialized_private = true;
}


void
csprof_fini_internal(void)
{
  NMSG(FINI,"process");
  int ret = monitor_real_sigprocmask(SIG_BLOCK,&prof_sigset,NULL);
  if (ret){
    EMSG("WARNING: process fini could not block SIGPROF, ret = %d",ret);
  }

  csprof_unthreaded_data();
  csprof_state_t *state = TD_GET(state);

  if (hpcrun_is_initialized()) {
    hpcrun_is_initialized_private = false;

    NMSG(FINI,"process attempting sample shutdown");

    SAMPLE_SOURCES(stop);
    SAMPLE_SOURCES(shutdown);

    // shutdown LUSH agents
    if (lush_agents) {
      lush_agent_pool__fini(lush_agents);
      lush_agents = NULL;
    }

    if (csprof_no_samples) return;

    fnbounds_fini();

    hpcrun_finalize_current_epoch();

#if defined(HOST_SYSTEM_IBM_BLUEGENE)
    EMSG("Backtrace for last sample event:\n");
    dump_backtrace(state, state->unwind);
#endif // defined(HOST_SYSTEM_IBM_BLUEGENE)

    hpcrun_write_profile_data(state);

    csprof_display_summary();
    
    pmsg_fini();
  }
}


//***************************************************************************
// thread level
//***************************************************************************

void
csprof_init_thread_support(void)
{
  csprof_init_pthread_key();
  csprof_set_thread0_data();
  csprof_threaded_data();
}


void *
csprof_thread_init(int id, lush_cct_ctxt_t* thr_ctxt)
{
  thread_data_t *td = csprof_allocate_thread_data();
  td->suspend_sampling = 1; // begin: protect against spurious signals


  csprof_set_thread_data(td);

  hpcrun_thread_data_new();
  hpcrun_thread_memory_init();
  hpcrun_thread_data_init(id, NULL);

#if defined(OLD_THREAD)
  csprof_thread_data_init(id,1,0,thr_ctxt);
#endif

  // POSSIBLE MEMORY PROBLEM: if thr_ctxt is reclaimed before copy can be made !!!
  copy_thr_ctxt(thr_ctxt);

  csprof_state_t* state = TD_GET(state);

  // start sampling sources
  TMSG(INIT,"starting sampling sources");

  SAMPLE_SOURCES(gen_event_set,lush_metrics);
  SAMPLE_SOURCES(start);

  int ret = monitor_real_pthread_sigmask(SIG_UNBLOCK,&prof_sigset,NULL);
  if (ret){
    EMSG("WARNING: Thread init could not unblock SIGPROF, ret = %d",ret);
  }
  return (void *)state;
}


void
csprof_thread_fini(csprof_state_t *state)
{
  TMSG(FINI,"thread fini");
  if (hpcrun_is_initialized()) {
    TMSG(FINI,"thread finit stops sampling");
    SAMPLE_SOURCES(stop);
    lushPthr_thread_fini(&TD_GET(pthr_metrics));
    hpcrun_finalize_current_epoch();
#if defined(HOST_SYSTEM_IBM_BLUEGENE)
    EMSG("Backtrace for last sample event:\n");
    dump_backtrace(state, state->unwind);
#endif // defined(HOST_SYSTEM_IBM_BLUEGENE)
    hpcrun_write_profile_data(state);
  }
}


//***************************************************************************

csprof_state_t *
csprof_check_for_new_epoch(csprof_state_t *state)
{
  /* ugh, nasty race condition here:

  1. shared library state has changed since the last profile
  signal, so we enter the if;

  2. somebody else dlclose()'s a library which holds something
  located in our backtrace.  this is not in itself a problem,
  since we don't bother doing anything on dlclose()...;

  3. somebody else (thread in step 2 or a different thread)
  dlopen()'s a new shared object, which begins an entirely
  new epoch--one which does not include the shared object
  which resides in our backtrace;

  4. we create a new state which receives the epoch from step 3,
  not step 1, which is wrong.

  attempt to take baby steps to stop this.  more drastic action
  would involve grabbing the epoch lock, but I believe that would
  be unacceptably slow (both in the atomic instruction overhead
  and the simple fact that most programs are not frequent users
  of dl*). */

  csprof_epoch_t *current = csprof_get_epoch();

  if(state->epoch != current) {
    TMSG(MALLOC," -new_epoch-");
    csprof_state_t *newstate = csprof_malloc(sizeof(csprof_state_t));

    TMSG(EPOCH, "Creating new epoch...");

    /* we don't have to go through the usual csprof_state_{init,alloc}
       business here because most of the stuff we want is already
       in `state' */
    memcpy(newstate, state, sizeof(csprof_state_t));

    /* we do have to reinitialize the tree, though */
    csprof_cct__init(&newstate->csdata, newstate->csdata_ctxt);

    /* and reinsert backtraces */
    if(newstate->bufend - newstate->bufstk != 0) {
      newstate->treenode = NULL;
      csprof_state_insert_backtrace(newstate, 0, /* pick one */
                                    newstate->bufend - 1,
                                    newstate->bufstk,
				    (cct_metric_data_t){ .i = 0 });
    }

    /* and inform the state about its epoch */
    newstate->epoch = current;

    /* and finally, set the new state */
    csprof_set_state(newstate);
    return newstate;
  }
  else {
    return state;
  }
}


// -*-Mode: C++;-*- // technically C99

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// -----------------------------------
// Part of HPCToolkit (hpctoolkit.org)
// -----------------------------------
// 
// Copyright ((c)) 2002-2009, Rice University 
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

/* functions operating on thread-local state */
#ifndef CSPROF_STATE_H
#define CSPROF_STATE_H

//************************* System Include Files ****************************

//*************************** User Include Files ****************************

#include <cct/cct.h>
#include "epoch.h"

#include <lush/lush.h>

#include <messages/messages.h>

//*************************** Datatypes **************************

//***************************************************************************

// ---------------------------------------------------------
// profiling state of a single thread
// ---------------------------------------------------------

typedef struct state_t {

  /* call stack data, stored in private memory */
  hpcrun_cct_t csdata;
  lush_cct_ctxt_t* csdata_ctxt; // creation context

  /* our notion of what the current epoch is */
  hpcrun_epoch_t *epoch;

  /* other profiling states which we have seen */
  struct state_t* next;


} state_t;

//*************************** Forward Declarations **************************

//***************************************************************************

typedef state_t* state_t_f(void);

typedef void state_t_setter(state_t* s);

// ---------------------------------------------------------
// getting and setting states independent of threading support
// ---------------------------------------------------------

extern void hpcrun_reset_state(state_t* state);

state_t* hpcrun_check_for_new_epoch(state_t *);
void hpcrun_state_init(void);
// ---------------------------------------------------------
// expand the internal backtrace buffer
// ---------------------------------------------------------

// tallent: move this macro here from processor/x86-64/backtrace.c.
// Undoubtedly a better solution than this is possible, but this at
// least is a more appropriate location.


cct_node_t* 
hpcrun_state_insert_backtrace(state_t *, int, frame_t *,
			      frame_t *, cct_metric_data_t);

#endif // STATE_H

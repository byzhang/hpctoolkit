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

#ifndef hpcrun_backtrace_h
#define hpcrun_backtrace_h

//***************************************************************************
// file: backtrace.h
//
// purpose:
//     an interface for performing stack unwinding 
//
//***************************************************************************


//***************************************************************************
// system include files 
//***************************************************************************

#include <stddef.h>
#include <stdint.h>

#include <ucontext.h>

//***************************************************************************
// local include files 
//***************************************************************************

#include <cct/cct.h>
#include <hpcrun/epoch.h>
#include "unwind_cursor.h"

//
// backtrace_t type is a struct holding begin, end, current ptrs
//  to frames
//

typedef struct backtrace_t {
  frame_t* beg;
  frame_t* end;
  frame_t* cur;
} backtrace_t;

//***************************************************************************
// interface functions
//***************************************************************************

typedef backtrace_t* bt_fn(ucontext_t* context);

backtrace_t* hpcrun_backtrace_std(ucontext_t* context);

//
// utility routine that does 2 things:
//   1) Generate a std backtrace
//   2) enters the generated backtrace in the cct
//
cct_node_t* hpcrun_backtrace2cct(epoch_t *epoch, ucontext_t* context,
				 int metricId, uint64_t metricIncr,
				 int skipInner, int isSync);

frame_t* hpcrun_skip_chords(frame_t* bt_outer, frame_t* bt_inner, 
			    int skip);

// FIXME: tallent: relocate when 'csprof epoch' trash is untangled
void dump_backtrace(epoch_t* epoch, frame_t* unwind);

//***************************************************************************

#endif // hpcrun_backtrace_h
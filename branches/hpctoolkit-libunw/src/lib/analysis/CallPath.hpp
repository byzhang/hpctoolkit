// -*-Mode: C++;-*-

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

//***************************************************************************
//
// File:
//   $HeadURL$
//
// Purpose:
//   [The purpose of this file]
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

#ifndef Analysis_CallPath_CallPath_hpp 
#define Analysis_CallPath_CallPath_hpp

//************************* System Include Files ****************************

#include <iostream>
#include <vector>
#include <stack>
#include <string>

//*************************** User Include Files ****************************

#include <include/uint.h>

#include "Args.hpp"
#include "Util.hpp"

#include <lib/binutils/LM.hpp>

#include <lib/prof-juicy/CallPath-Profile.hpp>
#include <lib/prof-juicy/Struct-Tree.hpp>

//*************************** Forward Declarations ***************************

//****************************************************************************

namespace Analysis {

namespace CallPath {

// ---------------------------------------------------------
//
// ---------------------------------------------------------

Prof::CallPath::Profile*
read(const Util::StringVec& profileFiles, const Util::UIntVec* groupMap,
     int mergeTy, uint rFlags = 0);

Prof::CallPath::Profile*
read(const char* prof_fnm, uint groupId, uint rFlags = 0);

static inline Prof::CallPath::Profile*
read(const string& prof_fnm, uint groupId, uint rFlags = 0)
{
  return read(prof_fnm.c_str(), groupId, rFlags);
}


void
readStructure(Prof::Struct::Tree* structure, const Analysis::Args& args);


// ---------------------------------------------------------
// 
// ---------------------------------------------------------

void
overlayStaticStructureMain(Prof::CallPath::Profile& prof,
			   string agent, bool doNormalizeTy);

void
overlayStaticStructureMain(Prof::CallPath::Profile& prof,
			   Prof::LoadMap::LM* loadmap_lm,
			   Prof::Struct::LM* lmStrct);


// lm is optional and may be NULL
void 
overlayStaticStructure(Prof::CallPath::Profile& prof,
		       Prof::LoadMap::LM* loadmap_lm,
		       Prof::Struct::LM* lmStrct, BinUtil::LM* lm);

// specialty function for hpcprof-mpi
void
noteStaticStructureOnLeaves(Prof::CallPath::Profile& prof);


void
normalize(Prof::CallPath::Profile& prof, string agent, bool doNormalizeTy);


// ---------------------------------------------------------
//
// ---------------------------------------------------------

void
makeDatabase(Prof::CallPath::Profile& prof, const Analysis::Args& args);

void 
write(Prof::CallPath::Profile& prof, std::ostream& os, 
      const std::string& title, bool prettyPrint = true);

} // namespace CallPath

} // namespace Analysis

//****************************************************************************

#endif // Analysis_CallPath_CallPath_hpp
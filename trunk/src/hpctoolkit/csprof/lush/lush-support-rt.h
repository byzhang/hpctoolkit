// -*-Mode: C++;-*- // technically C99
// $Id$

//***************************************************************************
//
// File: 
//    $Source$
//
// Purpose:
//    LUSH: Logical Unwind Support for HPCToolkit
//
// Description:
//    [The set of functions, macros, etc. defined in the file]
//
// Author:
//    Nathan Tallent, Rice University.
//
//***************************************************************************

#ifndef lush_support_rt_h
#define lush_support_rt_h

//************************* System Include Files ****************************

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

//*************************** User Include Files ****************************

#include <lush-support.h>

#include <prim_unw.h>
#include <mem.h>

//*************************** Forward Declarations **************************

// A macro to automatically declare function pointers types for each
// routine
#define LUSHI_DECL(RET, FN, ARGS)  \
  RET FN ARGS;                     \
  typedef RET (* FN ## _fn_t) ARGS


//***************************************************************************
// LUSH Unwind Types
//***************************************************************************


// ---------------------------------------------------------
// LUSH step
// ---------------------------------------------------------

typedef enum lush_step lush_step_t;

enum lush_step {
  LUSH_STEP_NULL = 0,

  LUSH_STEP_CONT,      // cursor represents valid chord/note
  LUSH_STEP_END_CHORD, // prev note  was the end of the chord
  LUSH_STEP_END_PROJ,  // prev chord was the end of the projection
  LUSH_STEP_ERROR      // error during the step
};


// ---------------------------------------------------------
// LUSH LIP: An opaque logical id
// ---------------------------------------------------------

// N.B.: Currently, this routine belongs here and not in lush-support.h!
static inline lush_lip_t*
lush_lip_clone(lush_lip_t* x)
{
  lush_lip_t* x_clone = csprof_malloc(sizeof(lush_lip_t));
  memcpy(x_clone, x, sizeof(lush_lip_t));
  return x_clone;
}


// ---------------------------------------------------------
// LUSH l-cursor: space for a logical cursor
// ---------------------------------------------------------

typedef struct lush_lcursor lush_lcursor_t;

struct lush_lcursor {
  unsigned char data[64];
};


// ---------------------------------------------------------
// LUSH cursor
// ---------------------------------------------------------

typedef enum lush_cursor_flags lush_cursor_flags_t;

enum lush_cursor_flags {
  LUSH_CURSOR_FLAGS_NONE = 0x00000000,
  
  //fixme: do we want this?
  //LUSH_CURSOR_FLAGS_MASK       = 0x00000000,
  //LUSH_CURSOR_FLAGS_MASK_AGENT = 0x00000000,

  // projections
  LUSH_CURSOR_FLAGS_BEG_PPROJ  = 0x00000001, // cursor @ beg of p-projection
  LUSH_CURSOR_FLAGS_END_PPROJ  = 0x00000002, // cursor @ end of p-projection
  LUSH_CURSOR_FLAGS_END_LPROJ  = 0x00000004, // cursor @ end of l-projection

  // chords
  LUSH_CURSOR_FLAGS_BEG_PCHORD = 0x00000010, // cursor @ beg of p-chord
  LUSH_CURSOR_FLAGS_END_PCHORD = 0x00000020, // cursor @ end of p-chord
  LUSH_CURSOR_FLAGS_END_LCHORD = 0x00000040  // cursor @ end of l-chord
};


typedef struct lush_cursor lush_cursor_t;

struct lush_cursor {
  // meta info
  unsigned flags;            // lush_cursor_flags
  lush_assoc_info_t as_info; // bichord's physical-logical association
  lush_agentid_t aid;        // agent id (if any) owning this cursor
  lush_agentid_t aid_prev;   // previous agent id (excluding identity agent)
  lush_agent_pool_t* apool;  // agent pool

  // physical cursor
  unw_cursor_t pcursor;

  // logical cursor
  lush_lcursor_t lcursor;
  lush_lip_t     lip;
  // active marker FIXME
};


static inline bool 
lush_cursor_is_flag(lush_cursor_t* cursor, lush_cursor_flags_t f)
{ 
  return (cursor->flags & f); 
}


static inline void 
lush_cursor_set_flag(lush_cursor_t* cursor, lush_cursor_flags_t f)
{
  cursor->flags = (cursor->flags | f);
}


static inline void 
lush_cursor_unset_flag(lush_cursor_t* cursor, lush_cursor_flags_t f)
{
  cursor->flags = (cursor->flags & ~f);
}


static inline lush_assoc_t 
lush_cursor_get_assoc(lush_cursor_t* cursor)
{
  return lush_assoc_info__get_assoc(cursor->as_info);
}


static inline void
lush_cursor_set_assoc(lush_cursor_t* cursor, lush_assoc_t as)
{
  lush_assoc_info__set_assoc(cursor->as_info, as);
}


static inline lush_agentid_t
lush_cursor_get_aid(lush_cursor_t* cursor)
{
  return cursor->aid;
}


static inline void
lush_cursor_set_aid(lush_cursor_t* cursor, lush_agentid_t aid)
{
  cursor->aid = aid;
}


static inline lush_agentid_t
lush_cursor_get_aid_prev(lush_cursor_t* cursor)
{
  return cursor->aid_prev;
}


static inline void
lush_cursor_set_aid_prev(lush_cursor_t* cursor, lush_agentid_t aid)
{
  cursor->aid_prev = aid;
}


static inline unw_word_t
lush_cursor_get_ip(lush_cursor_t* cursor)
{
  unw_word_t ip = 0;
  if (unw_get_reg(&cursor->pcursor, UNW_REG_IP, &ip) < 0) {
    // FIXME
  }
  return ip;
}


static inline lush_lip_t*
lush_cursor_get_lip(lush_cursor_t* cursor)
{
  return &cursor->lip;
}


static inline unw_cursor_t*
lush_cursor_get_pcursor(lush_cursor_t* cursor)
{
  return &cursor->pcursor;
}


static inline lush_lcursor_t*
lush_cursor_get_lcursor(lush_cursor_t* cursor)
{
  return &cursor->lcursor;
}


// **************************************************************************

#endif /* lush_support_rt_h */

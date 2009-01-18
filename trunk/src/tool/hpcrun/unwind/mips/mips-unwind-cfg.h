// -*-Mode: C++;-*- // technically C99
// $Id$

#ifndef mips_unwind_cfg_h
#define mips_unwind_cfg_h

//************************* System Include Files ****************************

//*************************** User Include Files ****************************

#include "unwind-cfg.h"

//*************************** Forward Declarations **************************

//***************************************************************************

#if (!HPC_UNW_LITE)
// libmonitor includes
#include <monitor.h>

// hpcrun includes
#  include "splay-interval.h"
#  include "splay.h"
#  include "atomic-ops.h"
#  include <memory/mem.h>
#endif


#if (HPC_UNW_MSG)
#  include "pmsg.h"
#else
#  define TMSG(x) /*remove*/
#  define EMSG(x) /*remove*/
#endif



//***************************************************************************

#endif // mips_unwind_cfg_h

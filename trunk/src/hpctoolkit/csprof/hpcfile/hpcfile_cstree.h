// $Id$
// -*-C-*-
// * BeginRiceCopyright *****************************************************
// ******************************************************* EndRiceCopyright *

//***************************************************************************
//
// File:
//    hpcfile_cstree.h
//
// Purpose:
//    Low-level types and functions for reading/writing a call stack
//    tree from/to a binary file.
//
//    These routines *must not* allocate dynamic memory; if such memory
//    is needed, callbacks to the user's allocator should be used.
//
// Description:
//    [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

#ifndef HPCFILE_CSTREE_H
#define HPCFILE_CSTREE_H

//************************* System Include Files ****************************

//*************************** User Include Files ****************************

#include "hpcfile_general.h"

//*************************** Forward Declarations **************************

#if defined(__cplusplus)
extern "C" {
#endif

//***************************************************************************
//
// Low-level types and functions for reading/writing a call stack tree
// from/to a binary file.
//
// Basic format of HPC_CSTREE: see implementation file for more details
//   HPC_CSTREE header
//   List of tree nodes
//
// Data in the file is stored in little-endian format, the ordering of
// IA-64 instructions and the default mode of an IA-64 processor.
//
//***************************************************************************

#define HPCFILE_CSTREE_MAGIC_STR     "HPC_CSTREE"
#define HPCFILE_CSTREE_MAGIC_STR_LEN 10 /* exclude '\0' */

/* the format of the nodes contained in the file will be different
   depending on whether or not we've using the trampoline in this
   build.  make it so that the library doesn't get confused */
#ifdef CSPROF_TRAMPOLINE_BACKEND
#define HPCFILE_CSTREE_VERSION     "01.0T" /* `T' is for `trampoline' */
#define HPCFILE_CSTREE_VERSION_LEN 5 /* exclude '\0' */
#else
#define HPCFILE_CSTREE_VERSION     "01.00"
#define HPCFILE_CSTREE_VERSION_LEN 5 /* exclude '\0' */
#endif

#define HPCFILE_CSTREE_ENDIAN 'l' /* 'l' for little, 'b' for big */

// ---------------------------------------------------------
// hpcfile_cstree_id_t: file identification.
//
// The size is 16 bytes and valid for both big/little endian ordering.
// Although these files will probably not have very long lives on disk,
// hopefully this will not need to be changed.
// ---------------------------------------------------------
typedef struct hpcfile_cstree_id_s {
  
  char magic_str[HPCFILE_CSTREE_MAGIC_STR_LEN]; 
  char version[HPCFILE_CSTREE_VERSION_LEN];
  char endian;  // 'b' or 'l' (currently, redundant info)
  
} hpcfile_cstree_id_t;

int hpcfile_cstree_id__init(hpcfile_cstree_id_t* x);
int hpcfile_cstree_id__fini(hpcfile_cstree_id_t* x);

int hpcfile_cstree_id__fread(hpcfile_cstree_id_t* x, FILE* fs);
int hpcfile_cstree_id__fwrite(hpcfile_cstree_id_t* x, FILE* fs);
int hpcfile_cstree_id__fprint(hpcfile_cstree_id_t* x, FILE* fs);

// ---------------------------------------------------------
// hpcfile_cstree_hdr_t:
// ---------------------------------------------------------
typedef struct hpcfile_cstree_hdr_s {
    hpcfile_cstree_id_t fid;
  
    // data type sizes (currently, redundant info)
    uint32_t vma_sz;    // 8
    uint32_t uint_sz;   // 8
  
    // data information
    uint64_t num_nodes;         /* number of tree nodes */
    uint32_t epoch;             /* epoch index */
} hpcfile_cstree_hdr_t;

int hpcfile_cstree_hdr__init(hpcfile_cstree_hdr_t* x);
int hpcfile_cstree_hdr__fini(hpcfile_cstree_hdr_t* x);

int hpcfile_cstree_hdr__fread(hpcfile_cstree_hdr_t* x, FILE* fs);
int hpcfile_cstree_hdr__fwrite(hpcfile_cstree_hdr_t* x, FILE* fs);
int hpcfile_cstree_hdr__fprint(hpcfile_cstree_hdr_t* x, FILE* fs);

// ---------------------------------------------------------
// hpcfile_cstree_nodedata_t:
// ---------------------------------------------------------
typedef struct hpcfile_cstree_nodedata_s {

    /* instruction pointer: more accurately, this is an 'operation
       pointer'.  The operation in the instruction packet is represented
       by adding 0, 1, or 2 to the instruction pointer for the first,
       second and third operation, respectively. */
    hpcfile_vma_t ip;
  
    /* 'sp': the stack pointer of this node */
    hpcfile_uint_t sp;

    hpcfile_uint_t num_metrics;
    hpcfile_uint_t *metrics;
} hpcfile_cstree_nodedata_t;

int hpcfile_cstree_nodedata__init(hpcfile_cstree_nodedata_t* x);
int hpcfile_cstree_nodedata__fini(hpcfile_cstree_nodedata_t* x);

int hpcfile_cstree_nodedata__fread(hpcfile_cstree_nodedata_t* x, FILE* fs);
int hpcfile_cstree_nodedata__fwrite(hpcfile_cstree_nodedata_t* x, FILE* fs);
int hpcfile_cstree_nodedata__fprint(hpcfile_cstree_nodedata_t* x, FILE* fs);

// ---------------------------------------------------------
// hpcfile_cstree_node_t: The root node -- the node without a parent -- is
// indicated by identical values for 'id' and 'id_parent'
// ---------------------------------------------------------
typedef struct hpcfile_cstree_node_s {

  hpcfile_cstree_nodedata_t data;

  hpcfile_uint_t id;        // persistent id of self
  hpcfile_uint_t id_parent; // persistent id of parent

} hpcfile_cstree_node_t;

int hpcfile_cstree_node__init(hpcfile_cstree_node_t* x);
int hpcfile_cstree_node__fini(hpcfile_cstree_node_t* x);

int hpcfile_cstree_node__fread(hpcfile_cstree_node_t* x, FILE* fs);
int hpcfile_cstree_node__fwrite(hpcfile_cstree_node_t* x, FILE* fs);
int hpcfile_cstree_node__fprint(hpcfile_cstree_node_t* x, FILE* fs);

//***************************************************************************

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif


// $Id$
// -*-C-*-
// * BeginRiceCopyright *****************************************************
// ******************************************************* EndRiceCopyright *

//***************************************************************************
//
// File:
//    $Source$
//
// Purpose:
//    General and helper functions for reading/writing a HPC data
//    files from/to a binary file.
//
//    These routines *must not* allocate dynamic memory; if such memory
//    is needed, callbacks to the user's allocator should be used.
//
// Description:
//    [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

//************************* System Include Files ****************************

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>

//*************************** User Include Files ****************************

#include "hpcfile_general.h"

//*************************** Forward Declarations **************************

// See header for interface information.
FILE* 
hpcfile_open_for_write(const char* fnm, int overwrite)
{
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  int fd;
  FILE* fs = NULL;

  if (overwrite == 0) {
    // Open file for writing; fail if the file already exists.  
    fd = open(fnm, O_WRONLY | O_CREAT | O_EXCL, mode);
    if (fd < 0) { return NULL; }  
  } else if (overwrite == 1) {
    // Open file for writing; truncate file already exists.
    fd = open(fnm, O_WRONLY | O_CREAT | O_TRUNC, mode); 
  } else {
    return NULL; // blech
  }
  
  // Get a buffered stream since we will be performing many small writes.
  fs = fdopen(fd, "w");
  return fs;
}

// See header for interface information.
FILE* 
hpcfile_open_for_read(const char* fnm)
{
  FILE* fs = fopen(fnm, "r");
  return fs;
}

// See header for interface information.
int   
hpcfile_close(FILE* fs)
{
  if (fs) {
    if (fclose(fs) == EOF) { 
      return HPCFILE_ERR; 
    } 
  }
  return HPCFILE_OK; 
}

//***************************************************************************

int 
hpcfile_str__init(hpcfile_str_t* x)
{
  memset(x, 0, sizeof(*x));
  return HPCFILE_OK;
}

int 
hpcfile_str__fini(hpcfile_str_t* x)
{
  return HPCFILE_OK;
}

int 
hpcfile_str__fread(hpcfile_str_t* x, FILE* fs, 
		   hpcfile_cb__alloc_fn_t alloc_fn)
{
  size_t sz;
  
  // [tag has already been read]

  sz = hpc_fread_le4(&x->length, fs);
  if (sz != sizeof(x->length)) { return HPCFILE_ERR; }

  if (x->length > 0) {
    x->str = alloc_fn(x->length * 1);
    sz = fread(x->str, 1, x->length, fs);
    if (sz != x->length) { return HPCFILE_ERR; }
  }
  
  return HPCFILE_OK;
}

int 
hpcfile_str__fwrite(hpcfile_str_t* x, FILE* fs)
{
  size_t sz;

  sz = hpc_fwrite_le4(&x->tag, fs);
  if (sz != sizeof(x->tag)) { return HPCFILE_ERR; }

  sz = hpc_fwrite_le4(&x->length, fs);
  if (sz != sizeof(x->length)) { return HPCFILE_ERR; }
  
  sz = fwrite(x->str, 1, x->length, fs);
  if (sz != x->length) { return HPCFILE_ERR; }

  return HPCFILE_OK;
}

int 
hpcfile_str__fprint(hpcfile_str_t* x, FILE* fs)
{
  fputs("{hpcfile_str: ", fs);
  
  fprintf(fs, "(tag: %u)\n", x->tag);

  fprintf(fs, "(length: %u)", x->length);
  if (x->str) { fprintf(fs, " (str: %s)", x->str); }
  
  fputs("\n", fs);

  return HPCFILE_OK;
}

//***************************************************************************

int 
hpcfile_num8__init(hpcfile_num8_t* x)
{
  memset(x, 0, sizeof(*x));
  return HPCFILE_OK;
}

int 
hpcfile_num8__fini(hpcfile_num8_t* x)
{
  return HPCFILE_OK;
}

int 
hpcfile_num8__fread(hpcfile_num8_t* x, FILE* fs)
{
  size_t sz;
  
  // [tag has already been read]

  sz = hpc_fread_le8(&x->num, fs);
  if (sz != sizeof(x->num)) { return HPCFILE_ERR; }
    
  return HPCFILE_OK;
}

int 
hpcfile_num8__fwrite(hpcfile_num8_t* x, FILE* fs)
{
  size_t sz;

  sz = hpc_fwrite_le4(&x->tag, fs);
  if (sz != sizeof(x->tag)) { return HPCFILE_ERR; }
  
  sz = hpc_fwrite_le8(&x->num, fs);
  if (sz != sizeof(x->num)) { return HPCFILE_ERR; }
  
  return HPCFILE_OK;
}

int 
hpcfile_num8__fprint(hpcfile_num8_t* x, FILE* fs)
{
  fputs("{hpcfile_num8: ", fs);

  fprintf(fs, "(tag: %u)\n", x->tag); 

  fprintf(fs, "(num: %"PRIu64")\n", x->num);

  return HPCFILE_OK;
}

//***************************************************************************

int 
hpcfile_num8s__init(hpcfile_num8s_t* x)
{
  memset(x, 0, sizeof(*x));
  return HPCFILE_OK;
}

int 
hpcfile_num8s__fini(hpcfile_num8s_t* x)
{
  return HPCFILE_OK;
}

int 
hpcfile_num8s__fread(hpcfile_num8s_t* x, FILE* fs, 
		     hpcfile_cb__alloc_fn_t alloc_fn)
{
  size_t sz;
  
  // [tag has already been read]

  sz = hpc_fread_le4(&x->length, fs);
  if (sz != sizeof(x->length)) { return HPCFILE_ERR; }
  
  if (x->length > 0) {
    uint32_t i;
    x->nums = alloc_fn(x->length * sizeof(uint64_t));
    for (i = 0; i < x->length; ++i) {
      sz = hpc_fread_le8(&x->nums[i], fs);
      if (sz != sizeof(uint64_t)) { return HPCFILE_ERR; }
    }
  }
  
  return HPCFILE_OK;
}

int 
hpcfile_num8s__fwrite(hpcfile_num8s_t* x, FILE* fs)
{
  size_t sz;

  sz = hpc_fwrite_le4(&x->tag, fs);
  if (sz != sizeof(x->tag)) { return HPCFILE_ERR; }
  
  sz = hpc_fwrite_le4(&x->length, fs);
  if (sz != sizeof(x->length)) { return HPCFILE_ERR; }
  
  if (x->length > 0) {
    uint32_t i;
    for (i = 0; i < x->length; ++i) {
      sz = hpc_fwrite_le8(&x->nums[i], fs);
      if (sz != sizeof(uint64_t)) { return HPCFILE_ERR; }
    }
  }

  return HPCFILE_OK;
}

int 
hpcfile_num8s__fprint(hpcfile_num8s_t* x, FILE* fs)
{
  fputs("{hpcfile_num8s: ", fs);

  fprintf(fs, "(tag: %u)\n", x->tag);  

  fprintf(fs, "(length: %u)", x->length);
  if (x->nums) { 
    uint32_t i;
    for (i = 0; i < x->length; ++i) {
      fprintf(fs, " (num: %"PRIu64")", x->nums[i]); 
    }
  }
  
  fputs("\n", fs);

  return HPCFILE_OK;
}


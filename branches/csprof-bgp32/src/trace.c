//*********************************************************************
// global includes 
//*********************************************************************

#include <stdio.h>



//*********************************************************************
// local includes 
//*********************************************************************

#include "env.h"
#include "files.h"
#include "monitor.h"
#include "pmsg.h"
#include "trace.h"
#include "thread_data.h"



//*********************************************************************
// forward declarations 
//*********************************************************************

static void trace_file_validate(int valid, char *op);



//*********************************************************************
// local variables 
//*********************************************************************

static int tracing = 0;



//*********************************************************************
// interface operations
//*********************************************************************

int 
trace_isactive()
{
  return tracing;
}


void 
trace_init()
{
  if (getenv(CSPROF_OPT_TRACE)) {
    tracing = 1;
  }
}


void 
trace_open()
{
  if (tracing) {
    char trace_file[PATH_MAX];
    files_trace_name(trace_file, PATH_MAX);
    thread_data_t *td = csprof_get_thread_data();
    td->trace_file = fopen(trace_file, "w");
    trace_file_validate(td->trace_file != 0, "open");
  }
}


void
trace_append(unsigned int cpid)
{
  if (tracing) {
#define NITEMS 1
    thread_data_t *td = csprof_get_thread_data();
    int written = fwrite(&cpid, sizeof(cpid), NITEMS, td->trace_file);
    trace_file_validate(written == NITEMS, "append");
  }
}


void
trace_close()
{
  if (tracing) {
    int ret;
    thread_data_t *td = csprof_get_thread_data();
    ret = fflush(td->trace_file);
    trace_file_validate(ret == 0, "flush");

    ret = fclose(td->trace_file);
    trace_file_validate(ret == 0, "close");
  }
}



//*********************************************************************
// private operations
//*********************************************************************


static void
trace_file_validate(int valid, char *op)
{
  if (!valid) {
    EMSG("unable to %s trace file\n", op);
    monitor_real_abort();
  }
}
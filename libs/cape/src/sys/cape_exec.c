#include "cape_exec.h"

// cape includes
#include "sys/cape_log.h"
#include "sys/cape_types.h"
#include "stc/cape_stream.h"
#include "stc/cape_list.h"
#include "stc/cape_map.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

// c includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>

//-----------------------------------------------------------------------------

struct CapeExec_s
{
  CapeStream stdout_stream;

  CapeStream stderr_stream;

  CapeList arguments;

  CapeMap environment;
};

//-----------------------------------------------------------------------------

#define CAPE_PIPE_READ     0
#define CAPE_PIPE_WRITE    1

//-----------------------------------------------------------------------------

static void __STDCALL cape_exec__arguments__on_del (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_exec__environment__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    CapeString h = val; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

CapeExec cape_exec_new (void)
{
  CapeExec self = CAPE_NEW (struct CapeExec_s);

  self->stdout_stream = cape_stream_new ();
  self->stderr_stream = cape_stream_new ();

  self->arguments = cape_list_new (cape_exec__arguments__on_del);
  self->environment = cape_map_new (cape_map__compare__s, cape_exec__environment__on_del, NULL);

  return self;
}

//-----------------------------------------------------------------------------

void cape_exec_del (CapeExec* p_self)
{
  if (*p_self)
  {
    CapeExec self = *p_self;

    cape_stream_del (&(self->stdout_stream));
    cape_stream_del (&(self->stderr_stream));

    cape_list_del (&(self->arguments));
    cape_map_del (&(self->environment));

    CAPE_DEL (p_self, struct CapeExec_s);
  }
}

//-----------------------------------------------------------------------------

char** cape_exec_run__argv (CapeExec self, const CapeString executable)
{
  int i = 0;
  char** argv = CAPE_ALLOC (sizeof(char*) * (cape_list_size (self->arguments) + 2));

  if (executable)
  {
    argv[i] = (char*)executable;
    i++;
  }

  {
    CapeListCursor cursor; cape_list_cursor_init (self->arguments, &cursor, CAPE_DIRECTION_FORW);

    while (cape_list_cursor_next (&cursor))
    {
      argv[i] = cape_list_node_data (cursor.node);
      i++;
    }
  }

  argv[i] = NULL;

  return argv;
}

//-----------------------------------------------------------------------------

char** cape_exec_run__envp (CapeExec self)
{
  int i;
  char** envp = NULL;
  number_t len = cape_map_size (self->environment);


  if (len > 0)
  {
    envp = CAPE_ALLOC (sizeof(char*) * (len + 1));

    {
      CapeMapCursor* cursor = cape_map_cursor_new (self->environment, CAPE_DIRECTION_FORW);

      for (i = 0; cape_map_cursor_next (cursor); i++)
      {
        envp[i] = cape_map_node_value (cursor->node);
      }

      cape_map_cursor_del (&cursor);
    }

    envp[i] = NULL;
  }

  return envp;
}

//-----------------------------------------------------------------------------

void cape_exec_run__execute (CapeExec self, const CapeString executable)
{
  int ret;

  // local objects
  char** argv = cape_exec_run__argv (self, executable);
  char** envp = cape_exec_run__envp (self);

  if (envp)
  {
    ret = execve (executable, argv, envp);

    CAPE_FREE (envp);
  }
  else
  {
    ret = execvp (executable, argv);
  }

  if (ret == -1)
  {
    CapeErr err = cape_err_new ();

    cape_err_lastOSError (err);

    printf ("error in execv: %s\n", cape_err_text (err));

    cape_err_del (&err);
  }

  CAPE_FREE (argv);
}

//-----------------------------------------------------------------------------

void cape_exec_run__rec_bytes (int fd, CapeStream s)
{
  char buffer [1024];

  while (TRUE)
  {
    number_t bytes_read = read (fd, buffer, 1024);

    if (bytes_read > 0)
    {
      cape_stream_append_buf (s, buffer, bytes_read);
    }
    else
    {
      return;
    }
  }
}

//-----------------------------------------------------------------------------

int cape_exec_run__recording (CapeExec self, pid_t pid, int fd_stdout, int fd_stderr, CapeErr err)
{
  int res;

  int status = 0;
  pid_t w;

  do
  {
    fd_set fdsread;
    FD_ZERO(&fdsread);

    // add to select set
    FD_SET (fd_stdout, &fdsread);
    FD_SET (fd_stderr, &fdsread);

    //w = waitpid (pid, &status, WUNTRACED | WCONTINUED);

    // run select
    {
      int res_select = select (fd_stderr + 1, &fdsread, NULL, NULL, NULL);

      if (res_select == -1)
      {
        res = cape_err_lastOSError (err);
        return res;
      }
      else
      {
        if (FD_ISSET (fd_stdout, &fdsread))
        {
          cape_exec_run__rec_bytes (fd_stdout, self->stdout_stream);
        }

        if (FD_ISSET (fd_stderr, &fdsread))
        {
          cape_exec_run__rec_bytes (fd_stderr, self->stderr_stream);
        }
      }
    }
  }
  while (!WIFEXITED(status) && !WIFSIGNALED(status));

  if (status != 0)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "exec - run", "exitcode: %i", status);

    res = cape_err_set_fmt (err, CAPE_ERR_RUNTIME, "executed program returned: %i", status);
  }
  else
  {
    res = CAPE_ERR_NONE;
  }

  return res;
}

//-----------------------------------------------------------------------------

int cape_exec_run (CapeExec self, const char* executable, CapeErr err)
{
  int res;
  pid_t pid;

  // initialize file descriptors
  int outfd[2] = {-1, -1}, errfd[2] = {-1, -1};

  if (pipe (outfd) == -1)
  {
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }

  if (pipe (errfd) == -1)
  {
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }

  // fork
  pid = fork();

  switch (pid)
  {
    case -1: // error
    {
      res = cape_err_lastOSError (err);
      goto exit_and_cleanup;
    }
    case  0: // child process
    {
      // close reading ends
      close (outfd[CAPE_PIPE_READ]);
      close (errfd[CAPE_PIPE_READ]);

      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec - run", "execute: %s", executable);

      // redirecting stdout
      dup2 (outfd[CAPE_PIPE_WRITE], STDOUT_FILENO);

      // redirecting stderr
      dup2 (errfd[CAPE_PIPE_WRITE], STDERR_FILENO);

      cape_exec_run__execute (self, executable);

      close (outfd[CAPE_PIPE_WRITE]);
      close (errfd[CAPE_PIPE_WRITE]);

      exit(0);
    }
    default: // parent process
    {
      close (outfd[CAPE_PIPE_WRITE]);
      close (errfd[CAPE_PIPE_WRITE]);

      res = cape_exec_run__recording (self, pid, outfd[CAPE_PIPE_READ], errfd[CAPE_PIPE_READ], err);

      close (outfd[CAPE_PIPE_READ]);
      close (errfd[CAPE_PIPE_READ]);

      return res;
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  close (outfd[CAPE_PIPE_WRITE]);
  close (outfd[CAPE_PIPE_READ]);

  close (errfd[CAPE_PIPE_READ]);
  close (errfd[CAPE_PIPE_WRITE]);

  return res;
}

//-----------------------------------------------------------------------------

int cape_exec_run_direct (CapeExec self, void* fd, const CapeString name, CapeErr err)
{
  int res;
  pid_t pid;
    
  // fork
  pid = fork();
  
  switch (pid)
  {
    case -1: // error
    {
      res = cape_err_lastOSError (err);
      goto exit_and_cleanup;
    }
    case  0: // child process
    {
      int ret;
      
      // local objects
      char** argv = cape_exec_run__argv (self, name);
      char** envp = cape_exec_run__envp (self);
      
      if (envp)
      {
        ret = fexecve ((number_t)fd, argv, envp);
        
        CAPE_FREE (envp);
      }
      else
      {
        const char* const envp[] = {NULL};
        
        ret = fexecve ((number_t)fd, argv, (char* const*)envp);
      }
      
      if (ret == -1)
      {
        CapeErr err = cape_err_new ();
        
        cape_err_lastOSError (err);
        
        printf ("error in execv: %s\n", cape_err_text (err));
        
        cape_err_del (&err);
      }
      
      CAPE_FREE (argv);
      
      exit(0);
    }
    default: // parent process
    {
      wait (NULL);
      
      return CAPE_ERR_NONE;
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
    
  return res;
}

//-----------------------------------------------------------------------------

CapeString cape_exec_env__make_val (const char* name, const char* parameter)
{
  return cape_str_catenate_c (name, '=', parameter);
}

//-----------------------------------------------------------------------------

void cape_exec_env_set (CapeExec self, const char* name, const char* parameter)
{
  CapeMapNode n = cape_map_find (self->environment, name);
  if (n)
  {
    // clear previous value
    CapeString h = cape_map_node_mv (n);
    cape_str_del (&h);

    // override value
    cape_map_node_set (n, (void*)cape_exec_env__make_val (name, parameter));
  }
  else
  {
    cape_map_insert (self->environment, (void*)cape_str_cp (name), (void*)cape_exec_env__make_val (name, parameter));
  }
}

//-----------------------------------------------------------------------------

void cape_exec_append_node (CapeExec self, CapeUdc parameters)
{
  if (cape_udc_type (parameters) == CAPE_UDC_NODE)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (parameters, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      switch (cape_udc_type (cursor->item))
      {
        case CAPE_UDC_STRING:
        {
          cape_exec_append_fmt (self, "-%s", cape_udc_name (cursor->item));
          cape_exec_append_s (self, cape_udc_s (cursor->item, ""));
                   
          break;
        }
      }
    }
    
    cape_udc_cursor_del (&cursor);
  }
}

//-----------------------------------------------------------------------------

void cape_exec_append_s (CapeExec self, const char* parameter)
{
  CapeString h = cape_str_cp (parameter);

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec - param", "append parameter: %s", h);

  cape_list_push_back (self->arguments, h);
}

//-----------------------------------------------------------------------------

void cape_exec_append_fmt (CapeExec self, const char* format, ...)
{
  CapeString parameter = NULL;

  va_list ptr;
  va_start(ptr, format);

  parameter = cape_str_flp (format, ptr);

  va_end(ptr);

  if (parameter)
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec - param", "append parameter: %s", parameter);

    cape_list_push_back (self->arguments, parameter);
  }
}

//-----------------------------------------------------------------------------

const CapeString cape_exec_get_stdout (CapeExec self)
{
  return cape_stream_get (self->stdout_stream);
}

//-----------------------------------------------------------------------------

const CapeString cape_exec_get_stderr (CapeExec self)
{
  return cape_stream_get (self->stderr_stream);
}

//-----------------------------------------------------------------------------

#elif defined __WINDOWS_OS

#include <windows.h>

#define BUFSIZE 4096

typedef struct
{
  HANDLE hr;
  HANDLE hw;

  //OVERLAPPED stOverlapped;

  char buffer[BUFSIZE];

  DWORD dwState;

  CapeStream stream;

  //int initial;

} CapePipe;

//-----------------------------------------------------------------------------

struct CapeExec_s
{
  CapeList arguments;

  CapePipe errPipe;
  CapePipe outPipe;
};

//-----------------------------------------------------------------------------

static void __STDCALL cape_exec__arguments__on_del (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------

CapeExec cape_exec_new (void)
{
  CapeExec self = CAPE_NEW (struct CapeExec_s);

  self->arguments = cape_list_new (cape_exec__arguments__on_del);

  self->errPipe.hr = NULL;
  self->errPipe.hw = NULL;
  self->outPipe.hr = NULL;
  self->outPipe.hw = NULL;

  self->outPipe.stream = cape_stream_new ();
  self->errPipe.stream = cape_stream_new ();

  return self;
}

//-----------------------------------------------------------------------------

void cape_exec_del (CapeExec* p_self)
{
  if (*p_self)
  {
    CapeExec self = *p_self;

    cape_stream_del (&(self->outPipe.stream));
    cape_stream_del (&(self->errPipe.stream));

    CloseHandle (self->outPipe.hr);
    CloseHandle (self->outPipe.hw);
    CloseHandle (self->errPipe.hr);
    CloseHandle (self->errPipe.hw);

    cape_list_del (&(self->arguments));

    CAPE_DEL (p_self, struct CapeExec_s);
  }
}

//-----------------------------------------------------------------------------

void cape_exec_append_s (CapeExec self, const char* parameter)
{
  CapeString h = cape_str_cp (parameter);

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec - param", "append parameter: %s", h);

  cape_list_push_back (self->arguments, h);
}

//-----------------------------------------------------------------------------

void cape_exec_append_fmt (CapeExec self, const char* format, ...)
{
  CapeString parameter = NULL;

  va_list ptr;
  va_start(ptr, format);

  parameter = cape_str_flp (format, ptr);

  va_end(ptr);

  if (parameter)
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec - param", "append parameter: %s", parameter);

    cape_list_push_back (self->arguments, parameter);
  }
}

//-----------------------------------------------------------------------------

int cape_exec_run__loop (CapeExec self, HANDLE processh, CapeErr err)
{
  DWORD dwWait;
  DWORD i;

  while (1)
  {
    DWORD fSuccess, cbRet;
    HANDLE hds[2];
    CapePipe* pipes[2];
    CapePipe* pipe;

    DWORD c = 0;

    /*
     * {
     *  hds[0] = processh;
     *  pipes[0] = NULL;
     *  c++;
  }
  */

    if (self->outPipe.hr != NULL)
    {
      hds[c] = self->outPipe.hr;
      pipes[c] = &(self->outPipe);
      c++;
    }

    if (self->errPipe.hr != NULL)
    {
      hds[c] = self->errPipe.hr;
      pipes[c] = &(self->errPipe);
      c++;
    }

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec", "wait for %i handles", c);

    dwWait = WaitForMultipleObjects(c, hds, FALSE, INFINITE);    // waits indefinitely
    i = dwWait - WAIT_OBJECT_0;

    pipe = pipes[i];

    if (pipe != NULL)
    {
      fSuccess = ReadFile (pipe->hr, pipe->buffer, BUFSIZE, &cbRet, NULL);
      if (!fSuccess)
      {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "exec", "read IO failure");

        CloseHandle (pipe->hr);
        pipe->hr = NULL;

        if (c < 2)
        {
          return cape_err_set (err, CAPE_ERR_NOT_FOUND, "read IO failure");
        }

        continue;
      }

      if (cbRet == 0)
      {
        return cape_err_lastOSError (err);
      }

      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec", "recv data: '%i'", cbRet);

      pipe->buffer[cbRet] = 0;

      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec", "recv '%s'", pipe->buffer);

      cape_stream_append_buf (pipe->stream, pipe->buffer, cbRet);
    }
  }

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_exec_run__create_child_process (CapeExec self, const char* executable, CapeErr err)
{
  int res;
  BOOL bSuccess = FALSE;

  CapeStream s = NULL;

  STARTUPINFO            siStartInfo;
  PROCESS_INFORMATION    piProcInfo;

  // Set up members of the PROCESS_INFORMATION structure.
  ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

  // Set up members of the STARTUPINFO structure.
  // This structure specifies the STDIN and STDOUT handles for redirection.
  ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );

  siStartInfo.cb = sizeof(STARTUPINFO);
  siStartInfo.hStdError = self->errPipe.hw;
  siStartInfo.hStdOutput = self->outPipe.hw;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  s = cape_stream_new ();

  // construct the command line as stream
  {
    CapeListCursor cursor; cape_list_cursor_init (self->arguments, &cursor, CAPE_DIRECTION_FORW);

    cape_stream_append_str (s, executable);
    cape_stream_append_str (s, " /C");

    while (cape_list_cursor_next (&cursor))
    {
      const CapeString h = cape_list_node_data (cursor.node);

      cape_stream_append_c (s, ' ');
      cape_stream_append_str (s, h);
    }
  }

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec:run", "execute: '%s'", cape_stream_get (s));

  bSuccess = CreateProcess(NULL,
                           (char*)cape_stream_get (s),   // command line
                           NULL,                         // process security attributes
                           NULL,                         // primary thread security attributes
                           TRUE,                         // handles are inherited
                           0,                            // creation flags
                           NULL,                         // use parent's environment
                           NULL,                         // use parent's current directory
                           &siStartInfo,                 // STARTUPINFO pointer
                           &piProcInfo);                 // receives PROCESS_INFORMATION

  if (!bSuccess)
  {
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }

  CloseHandle (self->outPipe.hw);
  CloseHandle (self->errPipe.hw);

  res = cape_exec_run__loop (self, piProcInfo.hProcess, err);

  CloseHandle (piProcInfo.hProcess);
  CloseHandle (piProcInfo.hThread);

exit_and_cleanup:

  cape_stream_del (&s);

  return res;
}

//-----------------------------------------------------------------------------

int cape_exec_run (CapeExec self, const char* executable, CapeErr err)
{
  SECURITY_ATTRIBUTES saAttr;

  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  cape_stream_clr (self->outPipe.stream);
  cape_stream_clr (self->errPipe.stream);

  // Create a pipe for the child process's STDOUT.
  if (!CreatePipe (&(self->outPipe.hr), &(self->outPipe.hw), &saAttr, 0))
  {
    return cape_err_lastOSError (err);
  }

  // Ensure the read handle to the pipe for STDOUT is not inherited.
  if (!SetHandleInformation (self->outPipe.hr, HANDLE_FLAG_INHERIT, 0))
  {
    return cape_err_lastOSError (err);
  }

  // Create a pipe for the child process's STDOUT.
  if (!CreatePipe (&(self->errPipe.hr), &(self->errPipe.hw), &saAttr, 0))
  {
    return cape_err_lastOSError (err);
  }

  // Ensure the read handle to the pipe for STDOUT is not inherited.
  if (!SetHandleInformation (self->errPipe.hr, HANDLE_FLAG_INHERIT, 0))
  {
    return cape_err_lastOSError (err);
  }

  // Create the child process.
  return cape_exec_run__create_child_process (self, executable, err);
}

//-----------------------------------------------------------------------------

const CapeString cape_exec_get_stdout (CapeExec self)
{
  return cape_stream_get (self->outPipe.stream);
}

//-----------------------------------------------------------------------------

const CapeString cape_exec_get_stderr (CapeExec self)
{
  return cape_stream_get (self->errPipe.stream);
}

//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------

#include "cape_btrace.h"

#if defined __WINDOWS_OS


#else

#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define _XOPEN_SOURCE
#include <ucontext.h>

//-----------------------------------------------------------------------------

void cape_btrace_handler (int sig)
{
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace (array, 10);

  // print out all the frames to stderr
  fprintf (stderr, "BACKTRACE FROM SEGENTATION FAULT %d\n", sig);
  backtrace_symbols_fd (array, size, STDERR_FILENO);
  
  exit(1);
}

//-----------------------------------------------------------------------------

/* This structure mirrors the one found in /usr/include/asm/ucontext.h */
typedef struct _sig_ucontext {
 unsigned long     uc_flags;
 struct ucontext   *uc_link;
 stack_t           uc_stack;
// struct sigcontext uc_mcontext;
 sigset_t          uc_sigmask;
} sig_ucontext_t;

//-----------------------------------------------------------------------------

void cape_btrace_hdlr (int sig_num, siginfo_t * info, void * ucontext)
{
  void *             array[50];
  void *             caller_address;
  char **            messages;
  int                size, i;
  sig_ucontext_t *   uc;
  
  uc = (sig_ucontext_t *)ucontext;

  
  /* Get the address at the time the signal was raised */
  /*
#if defined(__i386__) // gcc specific
  
  caller_address = (void *) uc->uc_mcontext.eip; // EIP: x86 specific

#elif defined(__x86_64__) // gcc specific
  
  caller_address = (void *) uc->uc_mcontext.rip; // RIP: x86_64 specific

#else
  
  #error Unsupported architecture. // TODO: Add support for other arch.

#endif
   */

  fprintf (stderr, "signal %d, address is %p from %p\n", sig_num, info->si_addr, (void *)caller_address);

  size = backtrace(array, 50);

  /* overwrite sigaction with caller's address */
  array[1] = caller_address;

  messages = backtrace_symbols(array, size);

  /* skip first stack frame (points here) */
  for (i = 1; i < size && messages != NULL; ++i)
  {
    fprintf (stderr, "[bt]: (%d) %s\n", i, messages[i]);
  }

  free (messages);

  exit (EXIT_FAILURE);
}

//-----------------------------------------------------------------------------

int cape_btrace_activate (CapeErr err)
{
  int res;

  /*
  signal (SIGSEGV, cape_btrace_handler);
  */
  
  struct sigaction sigact;

  sigact.sa_sigaction = cape_btrace_hdlr;
  sigact.sa_flags = SA_RESTART | SA_SIGINFO;

  if (sigaction (SIGSEGV, &sigact, (struct sigaction *)NULL) != 0)
  {
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

#endif


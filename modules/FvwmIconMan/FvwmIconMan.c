/*
 * Permission is granted to distribute this software freely
 * for any purpose.
 */
#ifdef USE_BSD
#  define _BSD_SOURCE
#else
#  define USE_POSIX
#endif

#if defined(USE_POSIX) && defined(USE_BSD)
#  error EITHER POSIX.1 or BSD signal semantics - not both!
#endif

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include "FvwmIconMan.h"
#include "readconfig.h"
#include "x.h"
#include "xmanager.h"

#ifdef COMPILE_STANDALONE
#include "module.h"
#else
#include "../../fvwm/module.h"
#endif

static int fd_width;
static volatile sig_atomic_t isTerminated = False;

static char *IM_VERSION = "1.3";

static char const rcsid[] =
  "$Id: FvwmIconMan.c,v 1.10 1999/01/06 10:18:25 domivogt Exp $";


static void TerminateHandler(int);

char *copy_string (char **target, char *src)
{
  int len = strlen (src);
  ConsoleDebug (CORE, "copy_string: 1: 0x%x\n", (unsigned)*target);

  if (*target)
    Free (*target);

  ConsoleDebug (CORE, "copy_string: 2\n");
  *target = (char *)safemalloc ((len + 1) * sizeof (char));
  strcpy (*target, src);
  ConsoleDebug (CORE, "copy_string: 3\n");
  return *target;
}

#ifdef TRACE_MEMUSE

long MemUsed = 0;

void Free (void *p)
{
  struct malloc_header *head = (struct malloc_header *)p;

  if (p != NULL) {
    head--;
    if (head->magic != MALLOC_MAGIC) {
      fprintf (stderr, "Corrupted memory found in Free\n");
      abort();
      return;
    }
    if (head->len > MemUsed) {
      fprintf (stderr, "Free block too big\n");
      return;
    }
    MemUsed -= head->len;
    free (head);
  }
}

void PrintMemuse (void)
{
  ConsoleDebug (CORE, "Memory used: %d\n", MemUsed);
}

#else

void Free (void *p)
{
  if (p != NULL)
    free (p);
}

void PrintMemuse (void)
{
}

#endif


static void TerminateHandler(int sig)
{
  isTerminated = True;
}
 

void ShutMeDown (int flag)
{
  ConsoleDebug (CORE, "Bye Bye\n");
  exit (flag);
}

void DeadPipe (int nothing)
{
  ShutMeDown(0);
}

void SendFvwmPipe (char *message,unsigned long window)
{
  char *hold,*temp,*temp_msg;
  hold=message;

  while(1) {
    temp=strchr(hold,',');
    if (temp!=NULL) {
      temp_msg= (char *)safemalloc(temp-hold+1);
      strncpy(temp_msg,hold,(temp-hold));
      temp_msg[(temp-hold)]='\0';
      hold=temp+1;
    } else temp_msg=hold;

    SendText(Fvwm_fd, temp_msg, window);

    if(temp_msg!=hold) Free(temp_msg);
    else break;
  }
}

static void main_loop (void)
{
  fd_set readset, saveset;

  FD_ZERO (&saveset);
  FD_SET (Fvwm_fd[1], &saveset);
  FD_SET (x_fd, &saveset);

  while( !isTerminated ) {
    /* Check the pipes for anything to read, and block if
     * there is nothing there yet ...
     */
    readset = saveset;
    if (select(fd_width,&readset,NULL,NULL,NULL) < 0) {
      ConsoleMessage ("Internal error with select: errno=%d\n",errno);
    }
    else {

      if (FD_ISSET (x_fd, &readset) || XPending (theDisplay)) {
        xevent_loop();
      }
      if (FD_ISSET(Fvwm_fd[1],&readset)) {
        ReadFvwmPipe();
      }

    }
  } /* while */
}

int main (int argc, char **argv)
{
  char *temp, *s;
  int i;
#ifdef USE_POSIX
  struct sigaction  sigact;
#endif

#ifdef ELECTRIC_FENCE
  extern int EF_PROTECT_BELOW, EF_PROTECT_FREE;

  EF_PROTECT_BELOW = 1;
  EF_PROTECT_FREE = 1;
#endif

#ifdef DEBUG_ATTACH
  {
    char buf[256];
    sprintf (buf, "%d", getpid());
    if (fork() == 0) {
      chdir ("/home/bradym/src/FvwmIconMan");
      execl ("/usr/local/bin/ddd", "/usr/local/bin/ddd", "FvwmIconMan",
	     buf, NULL);
    }
    else {
      int i, done = 0;
      for (i = 0; i < (1 << 27) && !done; i++) ;
    }
  }
#endif

  OpenConsole(OUTPUT_FILE);

#if 0
  ConsoleMessage ("PID = %d\n", getpid());
  ConsoleMessage ("Waiting for GDB to attach\n");
  sleep (10);
#endif

  init_globals();
  init_winlists();

  temp = argv[0];
  s = strrchr (argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  if((argc != 6) && (argc != 7)) {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",Module,
      IM_VERSION);
    ShutMeDown (1);
  }
  if (argc == 7 && !strcasecmp (argv[6], "Transient"))
    globals.transient = 1;

  Fvwm_fd[0] = atoi(argv[1]);
  Fvwm_fd[1] = atoi(argv[2]);
  init_display();
  init_boxes();

#if defined USE_POSIX
  sigemptyset(&sigact.sa_mask);
#ifdef SA_INTERRUPT
  sigact.sa_flags = SA_INTERRUPT;
#else
  sigact.sa_flags = 0;
#endif
  sigact.sa_handler = TerminateHandler;

  sigaction(SIGPIPE, &sigact, NULL);
  sigaction(SIGINT,  &sigact, NULL);
  sigaction(SIGHUP,  &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
#elif defined USE_BSD
  signal(SIGPIPE, TerminateHandler);
  signal(SIGINT,  TerminateHandler);
  signal(SIGHUP,  TerminateHandler);
  signal(SIGTERM, TerminateHandler);
  signal(SIGQUIT, TerminateHandler);
#endif

  read_in_resources (argv[3]);

  for (i = 0; i < globals.num_managers; i++) {
    X_init_manager (i);
  }

  assert (globals.managers);
  fd_width = GetFdWidth();

  SetMessageMask(Fvwm_fd,M_CONFIGURE_WINDOW | M_RES_CLASS | M_RES_NAME |
                 M_ADD_WINDOW | M_DESTROY_WINDOW | M_ICON_NAME |
                 M_DEICONIFY | M_ICONIFY | M_END_WINDOWLIST |
                 M_NEW_DESK | M_NEW_PAGE | M_FOCUS_CHANGE | M_WINDOW_NAME |
#ifdef MINI_ICONS
		 M_MINI_ICON |
#endif
		 M_STRING);

  SendInfo (Fvwm_fd, "Send_WindowList", 0);

  main_loop();

  return 0;
}

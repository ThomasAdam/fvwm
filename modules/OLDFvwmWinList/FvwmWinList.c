/* FvwmWinList Module for Fvwm. 
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The author makes not guarantees or warantees, either express or
 * implied.  Feel free to use any contained here for any purpose, as long
 * and this and any other applicible copyrights are kept intact.

 * The functions in this source file that are based on part of the FvwmIdent
 * module for Fvwm are noted by a small copyright atop that function, all others
 * are copyrighted by Mike Finger.  For those functions modified/used, here is
 *  the full, origonal copyright:
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#define TRUE 1
#define FALSE 

#define NO_CONSOLE

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif

#define YES "Yes"
#define NO  "No"

#include "../../configure.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdarg.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif
#include <unistd.h>
#include <ctype.h>
#ifdef ISC /* Saul */
#include <sys/bsdtypes.h> /* Saul */
#endif /* Saul */
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>

#include "../../fvwm/module.h"
#include "../../version.h"
#include "FvwmWinList.h"
#include "ButtonArray.h"
#include "List.h"
#include "Colors.h"
#include "Mallocs.h"

#define GRAB_EVENTS (ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|EnterWindowMask|LeaveWindowMask)

#define SomeButtonDown(a) ((a)&Button1Mask||(a)&Button2Mask||(a)&Button3Mask)

/* File type information */
FILE *console;
int fd_width;
int Fvwm_fd[2];
int x_fd;

/* X related things */
Display *dpy;
Window Root, win;
int screen,d_depth,ScreenWidth,ScreenHeight;
Pixel back[MAX_COLOUR_SETS], fore[MAX_COLOUR_SETS];
GC  graph[MAX_COLOUR_SETS],shadow[MAX_COLOUR_SETS],hilite[MAX_COLOUR_SETS];
GC  background[MAX_COLOUR_SETS];
XFontStruct *ButtonFont;
int fontheight;
static Atom wm_del_win;
Atom MwmAtom = None;

/* Module related information */
char *Module;
int WindowIsUp=0,win_width=5,win_height=5,win_grav,win_x,win_y,win_title,win_border;
int Clength,Transient=0,Pressed=0,ButPressed,Checked=0;
int MinWidth=DEFMINWIDTH,MaxWidth=DEFMAXWIDTH;
ButtonArray buttons;
List windows;
char *ClickAction[3]={"Iconify -1,Raise","Iconify","Lower"},*EnterAction,
      *BackColor[MAX_COLOUR_SETS] = { "white" },
      *ForeColor[MAX_COLOUR_SETS] = { "black" },
      *geometry="";
char *font_string = "fixed";
int UseSkipList=0,Anchor=1,UseIconNames=0,LeftJustify=0,TruncateLeft=1;

/******************************************************************************
  Main - Setup the XConnection,request the window list and loop forever
    Based on main() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void main(int argc, char **argv)
{
char *temp, *s;

  /* Open the console for messages */
  OpenConsole();

  /* Save the program name for error messages and config parsing */
  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;
  
  /* Setup my name */
  Module = safemalloc(strlen(temp)+2);
  strcpy(Module,"*");
  strcat(Module, temp);
  Clength = strlen(Module);

  if((argc != 6)&&(argc != 7)) {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",Module,
      VERSION);
    ConsoleMessage("%s Version %s should only be executed by fvwm!\n",Module,
      VERSION);
   exit(1);
  }


  if ((argc==7)&&(!mystrcasecmp(argv[6],"Transient"))) Transient=1;

  Fvwm_fd[0] = atoi(argv[1]);
  Fvwm_fd[1] = atoi(argv[2]);

  signal (SIGPIPE, DeadPipe);  

  /* Parse the config file */
  ParseConfig();

  /* Setup the XConnection */
  StartMeUp();

  InitArray(&buttons,0,0,win_width,fontheight+6);
  InitList(&windows);

  fd_width = GetFdWidth();

  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendFvwmPipe("Send_WindowList",0);

/*  SetMessageMask(Fvwm_fd,M_ADD_WINDOW|M_CONFIGURE_WINDOW| M_DESTROY_WINDOW|
	      M_WINDOW_NAME|M_ICON_NAME|M_DEICONIFY|M_ICONIFY|M_END_WINDOWLIST|
	      M_NEW_DESK| M_NEW_PAGE);*/
  /* Recieve all messages from Fvwm */
  EndLessLoop();
}

/******************************************************************************
  EndLessLoop -  Read and redraw until we get killed, blocking when can't read
******************************************************************************/
void EndLessLoop()
{
fd_set readset;
struct timeval tv;

  while(1) {
    FD_ZERO(&readset);
    FD_SET(Fvwm_fd[1],&readset);
    FD_SET(x_fd,&readset);
    tv.tv_sec=0;
    tv.tv_usec=0;
#ifdef __hpux
    if (!select(fd_width,(int *)&readset,NULL,NULL,&tv)) {
      XPending(dpy);
      FD_ZERO(&readset);
      FD_SET(Fvwm_fd[1],&readset);
      FD_SET(x_fd,&readset);
      select(fd_width,(int *)&readset,NULL,NULL,NULL);
    }
#else
    if (!select(fd_width,&readset,NULL,NULL,&tv)) {
      XPending(dpy);
      FD_ZERO(&readset);
      FD_SET(Fvwm_fd[1],&readset);
      FD_SET(x_fd,&readset);
      select(fd_width,&readset,NULL,NULL,NULL);
    }
#endif
    if (FD_ISSET(x_fd,&readset)) LoopOnEvents();
    if (!FD_ISSET(Fvwm_fd[1],&readset)) continue;
    ReadFvwmPipe();
  }
}

/******************************************************************************
  ReadFvwmPipe - Read a single message from the pipe from Fvwm
    Originally Loop() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ReadFvwmPipe()
{
  int count,total,count2=0,body_length;
  unsigned long header[HEADER_SIZE],*body;
  char *cbody;
  if(ReadFvwmPacket(Fvwm_fd[1],header,&body) > 0)
    {
      ProcessMessage(header[1],body);
      free(body);
    }
}

/******************************************************************************
  ProcessMessage - Process the message coming from Fvwm
    Skeleton based on processmessage() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ProcessMessage(unsigned long type,unsigned long *body)
{
int redraw=0,i;
long flags;
char *name,*string;
static current_focus=-1;

  switch(type) {
    case M_ADD_WINDOW:
    case M_CONFIGURE_WINDOW:
      if (body[0]==win) {
        win_x=(int)body[3];
        win_y=(int)body[4];
        win_title=(int)body[9];
        win_border=(int)body[10];
        if (win_border!=0) win_border++;
      }
      if (FindItem(&windows,body[0])!=-1) break;
      if (!(body[8]&WINDOWLISTSKIP) || !UseSkipList)
        AddItem(&windows,body[0],body[8]);
      break;
    case M_DESTROY_WINDOW:
      if ((i=DeleteItem(&windows,body[0]))==-1) break;
      RemoveButton(&buttons,i);
      if (WindowIsUp) AdjustWindow();
      redraw=1;
      break;
    case M_WINDOW_NAME:
    case M_ICON_NAME:
      if ((type==M_ICON_NAME && !UseIconNames) || 
          (type==M_WINDOW_NAME && UseIconNames)) break;
      if ((i=UpdateItemName(&windows,body[0],(char *)&body[3]))==-1) break;
      string=(char *)&body[3];
      name=makename(string,ItemFlags(&windows,body[0]));
      if (UpdateButton(&buttons,i,name,-1)==-1)
      {
        AddButton(&buttons,name,1);
	UpdateButtonSet(&buttons,i,ItemFlags(&windows,body[0])&ICONIFIED?1:0);
      }
      free(name);
      if (WindowIsUp) AdjustWindow();
      redraw=1;
      break;
    case M_DEICONIFY:
    case M_ICONIFY:
      if ((i=FindItem(&windows,body[0]))==-1) break;
      flags=ItemFlags(&windows,body[0]);
      if (type==M_DEICONIFY && !(flags&ICONIFIED)) break;
      if (type==M_ICONIFY && flags&ICONIFIED) break;
      flags^=ICONIFIED;
      UpdateItemFlags(&windows,body[0],flags);
      string=ItemName(&windows,i);
      name=makename(string,flags);
      if (UpdateButton(&buttons,i,name,-1)!=-1) redraw=1;
      if (i!=current_focus||(flags&ICONIFIED))
        if (UpdateButtonSet(&buttons,i,(flags&ICONIFIED) ? 1 : 0)!=-1) redraw=1;
      free(name);
      break;
    case M_FOCUS_CHANGE:
      i=FindItem(&windows,body[0]);
      if (i==current_focus) break;
      if (current_focus!=-1)
	if ((ItemFlags(&windows,ItemID(&windows,current_focus))&ICONIFIED)==0)
	  if (UpdateButtonSet(&buttons,current_focus,0)!=-1) redraw=1;
      current_focus=-1;
      if (i!=-1&&(ItemFlags(&windows,body[0])&ICONIFIED)) break;
      current_focus=i;
      if (current_focus!=-1)
	if (UpdateButtonSet(&buttons,current_focus,2)!=-1) redraw=1;
      break;
    case M_END_WINDOWLIST:
      if (!WindowIsUp) MakeMeWindow();
      redraw=1;
      break;
    case M_NEW_DESK:
      break;
    case M_NEW_PAGE:
      break;
  }

  if (redraw && WindowIsUp==1) RedrawWindow(0);
}

/******************************************************************************
  SendFvwmPipe - Send a message back to fvwm 
    Based on SendInfo() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void SendFvwmPipe(char *message,unsigned long window)
{
int w;
char *hold,*temp,*temp_msg;
  hold=message;

  while(1) {
    temp=strchr(hold,',');
    if (temp!=NULL) {
      temp_msg=malloc(temp-hold+1);
      strncpy(temp_msg,hold,(temp-hold));
      temp_msg[(temp-hold)]='\0';
      hold=temp+1;
    } else temp_msg=hold;

    write(Fvwm_fd[0],&window, sizeof(unsigned long));

    w=strlen(temp_msg);
    write(Fvwm_fd[0],&w,sizeof(int));
    write(Fvwm_fd[0],temp_msg,w);

    /* keep going */
    w=1;
    write(Fvwm_fd[0],&w,sizeof(int));

    if(temp_msg!=hold) free(temp_msg);
    else break;
  }
}

/***********************************************************************
  Detected a broken pipe - time to exit 
    Based on DeadPipe() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
 **********************************************************************/
void DeadPipe(int nonsense)
{
  ShutMeDown(1);
}

/******************************************************************************
  WaitForExpose - Used to wait for expose event so we don't draw too early
******************************************************************************/
void WaitForExpose(void)
{
XEvent Event;
  while(1) {
    XNextEvent(dpy,&Event);
    if (Event.type==Expose) {
      if (Event.xexpose.count==0) break;
    }
  }
}

/******************************************************************************
  RedrawWindow - Update the needed lines and erase any old ones
******************************************************************************/
void RedrawWindow(int force)
{
  DrawButtonArray(&buttons,force);
  if (XQLength(dpy) && !force) LoopOnEvents();
}

/******************************************************************************
  ConsoleMessage - Print a message on the console.  Works like printf.
******************************************************************************/
void ConsoleMessage(char *fmt, ...)
{
#ifndef NO_CONSOLE
va_list args;
FILE *filep;
  if (console==NULL) filep=stderr;
  else filep=console;
  va_start(args,fmt);
  vfprintf(filep,fmt,args);
  va_end(args);
#endif
}

/******************************************************************************
  OpenConsole - Open the console as a way of sending messages
******************************************************************************/
int OpenConsole()
{
#ifndef NO_CONSOLE
  if ((console=fopen("/dev/console","w"))==NULL) {
    fprintf(stderr,"%s: cannot open console\n",Module);
    return 0;
  }
#endif
  return 1;
}

/******************************************************************************
  ParseConfig - Parse the configuration file fvwm to us to use
    Based on part of main() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ParseConfig()
{
  char *tline;

  GetConfigLine(Fvwm_fd,&tline);
  while(tline != (char *)0) 
    {
      if(strlen(tline)>1) 
	{
	  if(mystrncasecmp(tline, CatString3(Module, "Font",""),Clength+4)==0)
	    CopyString(&font_string,&tline[Clength+4]);
	  else if(mystrncasecmp(tline,CatString3(Module,"Fore",""), Clength+4)==0)
	    CopyString(&ForeColor[0],&tline[Clength+4]);
	  else if(mystrncasecmp(tline,CatString3(Module,"IconFore",""), Clength+8)==0)
	    CopyString(&ForeColor[1],&tline[Clength+8]);
	  else if(mystrncasecmp(tline,CatString3(Module,"FocusFore",""), Clength+9)==0)
	    CopyString(&ForeColor[2],&tline[Clength+9]);
	  else if(mystrncasecmp(tline,CatString3(Module, "Geometry",""), Clength+8)==0)
	    CopyString(&geometry,&tline[Clength+8]);
	  else if(mystrncasecmp(tline,CatString3(Module, "Back",""), Clength+4)==0)
	    CopyString(&BackColor[0],&tline[Clength+4]);
	  else if(mystrncasecmp(tline,CatString3(Module,"IconBack",""), Clength+8)==0)
	    CopyString(&BackColor[1],&tline[Clength+8]);
	  else if(mystrncasecmp(tline,CatString3(Module,"FocusBack",""), Clength+9)==0)
	    CopyString(&BackColor[2],&tline[Clength+9]);
	  else if(mystrncasecmp(tline,CatString3(Module, "NoAnchor",""),
				Clength+8)==0) Anchor=0;
	  else if(mystrncasecmp(tline,CatString3(Module, "Action",""), Clength+6)==0)
	    LinkAction(&tline[Clength+6]);
	  else if(mystrncasecmp(tline,CatString3(Module, "UseSkipList",""),
				Clength+11)==0) UseSkipList=1;
	  else if(mystrncasecmp(tline,CatString3(Module, "UseIconNames",""),
				Clength+12)==0) UseIconNames=1;
	  else if(mystrncasecmp(tline,CatString3(Module, "LeftJustify",""),
				Clength+11)==0) LeftJustify=1;
	  else if(mystrncasecmp(tline,CatString3(Module, "TruncateLeft",""),
				Clength+12)==0) TruncateLeft=1;
	  else if(mystrncasecmp(tline,CatString3(Module, "TruncateRight",""),
				Clength+13)==0) TruncateLeft=0;
          else if(mystrncasecmp(tline,CatString3(Module, "MinWidth",""),
                                Clength+8)==0) MinWidth=atoi(&tline[Clength+8]);
          else if(mystrncasecmp(tline,CatString3(Module, "MaxWidth",""),
                                Clength+8)==0) MaxWidth=atoi(&tline[Clength+8]);
	}
      GetConfigLine(Fvwm_fd,&tline);
    }
}

/******************************************************************************
  LoopOnEvents - Process all the X events we get
******************************************************************************/
void LoopOnEvents()
{
int num;
char buffer[10];
XEvent Event;
Window dummyroot,dummychild;
int x,x1,y,y1;
unsigned int dummy1;

  if (Transient && !Checked) {
    XQueryPointer(dpy,win,&dummyroot,&dummychild,&x1,&y1,&x,&y,&dummy1);
    num=WhichButton(&buttons,x,y);
    if (num!=-1) {
      Pressed=1;
      ButPressed=num;
      SwitchButton(&buttons,num);
    } else Pressed=0;
    Checked=1;
  }

  while(XPending(dpy)) {
    XNextEvent(dpy,&Event);

    switch(Event.type) {
      case ButtonRelease:
        if (Pressed) {
          num=WhichButton(&buttons,Event.xbutton.x,Event.xbutton.y);
          if (num!=-1) {
            SendFvwmPipe(ClickAction[(Transient) ? 0:Event.xbutton.button-1],
              ItemID(&windows,num));
            SwitchButton(&buttons,num);
          }
        }
        if (Transient) ShutMeDown(0);
        Pressed=0;
        ButPressed=-1;
        break;
      case ButtonPress:
        num=WhichButton(&buttons,Event.xbutton.x,Event.xbutton.y);
        if (num != -1) {
          SwitchButton(&buttons,num);
          ButPressed=num;
        } else ButPressed=-1;
        Pressed=1;
        break;
      case Expose:
        if (Event.xexpose.count==0)
          RedrawWindow(1);
        break;
      case KeyPress:
        num=XLookupString(&Event.xkey,buffer,10,NULL,0);
        if (num==1) {
          if (buffer[0]=='q' || buffer[0]=='Q') ShutMeDown(0);
          else if (buffer[0]=='i' || buffer[0]=='I') PrintList(&windows);
          else if (buffer[0]=='b' || buffer[0]=='B') PrintButtons(&buttons);
        }
        break;
      case ClientMessage:
        if ((Event.xclient.format==32) && (Event.xclient.data.l[0]==wm_del_win))
          ShutMeDown(0);
      case EnterNotify:
        if (!SomeButtonDown(Event.xcrossing.state)) break;
        num=WhichButton(&buttons,Event.xcrossing.x,Event.xcrossing.y);
        if (num!=-1) {
          SwitchButton(&buttons,num);
          ButPressed=num;
        } else ButPressed=-1;
        Pressed=1;
        break;
      case LeaveNotify:
        if (!SomeButtonDown(Event.xcrossing.state)) break;
        if (ButPressed!=-1) SwitchButton(&buttons,ButPressed);
        Pressed=0;
        break;
      case MotionNotify:
        if (!Pressed) break;
        num=WhichButton(&buttons,Event.xmotion.x,Event.xmotion.y);
        if (num==ButPressed) break;
        if (ButPressed!=-1) SwitchButton(&buttons,ButPressed);
        if (num!=-1) {
          SwitchButton(&buttons,num);
          ButPressed=num;
        } else ButPressed=-1;
        break;
    }
  }
} 

/******************************************************************************
  AdjustWindow - Resize the window according to maxwidth by number of buttons
******************************************************************************/
void AdjustWindow()
{
int new_width=0,new_height=0,tw,i,total;
char *temp;

  total=ItemCount(&windows);
  if (!total) {
    if (WindowIsUp==1) {
      XUnmapWindow(dpy,win);
      WindowIsUp=2;
    }
    return;
  }
  for(i=0;i<total;i++) {
    temp=ItemName(&windows,i);
    if(temp != NULL)
      {
	tw=10+XTextWidth(ButtonFont,temp,strlen(temp));
	tw+=XTextWidth(ButtonFont,"()",2);
	new_width=max(new_width,tw);
      }
  }
  new_width=max(new_width,MinWidth);
  new_width=min(new_width,MaxWidth);
  new_height=(total*(fontheight+6+1)-1);
  if (WindowIsUp && (new_height!=win_height  || new_width!=win_width)) {
    if (Anchor) {
      if (win_grav==SouthEastGravity || win_grav==NorthEastGravity)
        win_x-=(new_width-win_width);
      if (win_grav==SouthEastGravity || win_grav==SouthWestGravity)
        win_y-=(new_height-win_height);
      XMoveResizeWindow(dpy,win,win_x+win_border,win_y+win_title+win_border,
        new_width,new_height);
    } else XResizeWindow(dpy,win,new_width,new_height);
    XSync(dpy,False);
  }
  UpdateArray(&buttons,-1,-1,new_width,-1);
  if (new_height>0) win_height=new_height;
  if (new_width>0) win_width=new_width;
  if (WindowIsUp==2) {
    XMapWindow(dpy,win);
    WindowIsUp=1;
    WaitForExpose();
  }
}

/******************************************************************************
  makename - Based on the flags return me '(name)' or 'name'
******************************************************************************/
char *makename(char *string,long flags)
{
char *ptr;
  ptr=safemalloc(strlen(string)+3);
  if (flags&ICONIFIED) strcpy(ptr,"(");
  else strcpy(ptr,"");
  strcat(ptr,string);
  if (flags&ICONIFIED) strcat(ptr,")");
  return ptr;
}

/******************************************************************************
  LinkAction - Link an response to a users action
******************************************************************************/
void LinkAction(char *string)
{
char *temp;
  temp=string;
  while(isspace(*temp)) temp++;
  if(mystrncasecmp(temp, "Click1", 6)==0)
    CopyString(&ClickAction[0],&temp[6]);
  else if(mystrncasecmp(temp, "Click2", 6)==0)
    CopyString(&ClickAction[1],&temp[6]);
  else if(mystrncasecmp(temp, "Click3", 6)==0)
    CopyString(&ClickAction[2],&temp[6]);
  else if(mystrncasecmp(temp, "Enter", 5)==0)
    CopyString(&EnterAction,&temp[5]);
}

/******************************************************************************
  MakeMeWindow - Create and setup the window we will need
******************************************************************************/
void MakeMeWindow(void)
{
XSizeHints hints;
XGCValues gcval;
unsigned long gcmask;
unsigned int dummy1,dummy2;
int x,y,ret,count;
Window dummyroot,dummychild;
int i;
  
  if ((count=ItemCount(&windows))==0 && Transient) ShutMeDown(0);
  AdjustWindow();

  hints.width=win_width;
  hints.height=win_height;
  hints.win_gravity=NorthWestGravity;
  hints.flags=PSize|PWinGravity|PResizeInc;
  hints.width_inc=0;
  hints.height_inc=0;

  if (geometry!= NULL) {
    ret=XParseGeometry(geometry,&x,&y,&dummy1,&dummy2);

    if (ret&XValue && ret &YValue) {
      hints.x=x;
      if (ret&XNegative) hints.x+=XDisplayWidth(dpy,screen)-win_width;
      hints.y=y;
      if (ret&YNegative) hints.y+=XDisplayHeight(dpy,screen)-win_height;
      hints.flags|=USPosition;
    }

    if (ret&XNegative) {
      if (ret&YNegative) hints.win_gravity=SouthEastGravity;
      else  hints.win_gravity=NorthEastGravity;
    } else {
      if (ret&YNegative) hints.win_gravity=SouthWestGravity;
      else  hints.win_gravity=NorthWestGravity;
    }

  }

  if (Transient) {
    XQueryPointer(dpy,Root,&dummyroot,&dummychild,&hints.x,&hints.y,&x,&y,&dummy1);
    hints.win_gravity=NorthWestGravity;
    hints.flags |= USPosition;
  }
  win_grav=hints.win_gravity;
  win_x=hints.x;
  win_y=hints.y;

  for (i = 0; i != MAX_COLOUR_SETS; i++)
  if(d_depth < 2) {
    back[i] = GetColor("white");
    fore[i] = GetColor("black");
  } else {
    back[i] = GetColor(BackColor[i] == NULL ? BackColor[0] : BackColor[i]);
    fore[i] = GetColor(ForeColor[i] == NULL ? ForeColor[0] : ForeColor[i]);
  }

  win=XCreateSimpleWindow(dpy,Root,hints.x,hints.y,hints.width,hints.height,1,
    fore[0],back[0]);

  wm_del_win=XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(dpy,win,&wm_del_win,1);
  
  XSetWMNormalHints(dpy,win,&hints);

  if (!Transient) {
    XGrabButton(dpy,1,AnyModifier,win,True,GRAB_EVENTS,GrabModeAsync,
      GrabModeAsync,None,None);
    XGrabButton(dpy,2,AnyModifier,win,True,GRAB_EVENTS,GrabModeAsync,
      GrabModeAsync,None,None);
    XGrabButton(dpy,3,AnyModifier,win,True,GRAB_EVENTS,GrabModeAsync,
      GrabModeAsync,None,None);
    SetMwmHints(MWM_DECOR_ALL|MWM_DECOR_RESIZEH|MWM_DECOR_MAXIMIZE|MWM_DECOR_MINIMIZE,
      MWM_FUNC_ALL|MWM_FUNC_RESIZE|MWM_FUNC_MAXIMIZE|MWM_FUNC_MINIMIZE,
      MWM_INPUT_MODELESS);
  } else {
    SetMwmHints(0,MWM_FUNC_ALL,MWM_INPUT_MODELESS);
  }

  for (i = 0; i != MAX_COLOUR_SETS; i++)
  {
    gcval.foreground=fore[i];
    gcval.background=back[i];
    gcval.font=ButtonFont->fid;
    gcmask=GCForeground|GCBackground|GCFont;
    graph[i]=XCreateGC(dpy,Root,gcmask,&gcval);

    if(d_depth < 2) 
      gcval.foreground=GetShadow(fore[i]);
    else 
      gcval.foreground=GetShadow(back[i]);
    gcval.background=back[i];
    gcmask=GCForeground|GCBackground;
    shadow[i]=XCreateGC(dpy,Root,gcmask,&gcval);

    gcval.foreground=GetHilite(back[i]);
    gcval.background=back[i];
    gcmask=GCForeground|GCBackground;
    hilite[i]=XCreateGC(dpy,Root,gcmask,&gcval);

    gcval.foreground=back[i];
    gcmask=GCForeground;
    background[i]=XCreateGC(dpy,Root,gcmask,&gcval);
  }
  
  XSelectInput(dpy,win,(ExposureMask | KeyPressMask));

  ChangeWindowName(&Module[1]);

  if (ItemCount(&windows)>0) {
    XMapRaised(dpy,win);
    WaitForExpose();
    WindowIsUp=1;
  } else WindowIsUp=2;

  if (Transient) {
    if ( XGrabPointer(dpy,win,True,GRAB_EVENTS,GrabModeAsync,GrabModeAsync,
      None,None,CurrentTime)!=GrabSuccess) ShutMeDown(1);
    XQueryPointer(dpy,Root,&dummyroot,&dummychild,&hints.x,&hints.y,&x,&y,&dummy1);
    if (!SomeButtonDown(dummy1)) ShutMeDown(0);
  }

}

/******************************************************************************
  StartMeUp - Do X initialization things
******************************************************************************/
void StartMeUp()
{
  if (!(dpy = XOpenDisplay(""))) {
    fprintf(stderr,"%s: can't open display %s", Module,
      XDisplayName(""));
    exit (1);
  }
  x_fd = XConnectionNumber(dpy);
  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  d_depth = DefaultDepth(dpy, screen);

  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

  if ((ButtonFont=XLoadQueryFont(dpy,font_string))==NULL) {
    if ((ButtonFont=XLoadQueryFont(dpy,"fixed"))==NULL) exit(1);
  }

  fontheight=ButtonFont->ascent+ButtonFont->descent;

  win_width=XTextWidth(ButtonFont,"XXXXXXXXXXXXXXX",10);
 

}

/******************************************************************************
  ShutMeDown - Do X client cleanup
******************************************************************************/
void ShutMeDown(int exitstat)
{
  FreeList(&windows);
  FreeAllButtons(&buttons);
/*  XFreeGC(dpy,graph);*/
  if (WindowIsUp) XDestroyWindow(dpy,win);
  XCloseDisplay(dpy);
  exit(exitstat);
}

/******************************************************************************
  ChangeWindowName - Self explanitory
    Original work from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ChangeWindowName(char *str)
{
XTextProperty name;
  if (XStringListToTextProperty(&str,1,&name) == 0) {
    fprintf(stderr,"%s: cannot allocate window name.\n",Module);
    return;
  }
  XSetWMName(dpy,win,&name);
  XSetWMIconName(dpy,win,&name);
  XFree(name.value);
}

/**************************************************************************
 *
 * Sets mwm hints 
 *
 *************************************************************************/
/* 
 *  Now, if we (hopefully) have MWW - compatible window manager ,
 *  say, mwm, ncdwm, or else, we will set useful decoration style.
 *  Never check for MWM_RUNNING property.May be considered bad.
 */

void SetMwmHints(unsigned int value, unsigned int funcs, unsigned int input)
{
PropMwmHints prop;

  if (MwmAtom==None)
    {
      MwmAtom=XInternAtom(dpy,"_MOTIF_WM_HINTS",False);  
    }
  if (MwmAtom!=None)
    {
      /* sh->mwm.decorations contains OR of the MWM_DECOR_XXXXX */
      prop.decorations= value;
      prop.functions = funcs;
      prop.inputMode = input;
      prop.flags = MWM_HINTS_DECORATIONS| MWM_HINTS_FUNCTIONS | MWM_HINTS_INPUT_MODE;
      
      /* HOP - LA! */
      XChangeProperty (dpy,win,
		       MwmAtom, MwmAtom,
		       32, PropModeReplace,
		       (unsigned char *)&prop,
		       PROP_MWM_HINTS_ELEMENTS);
    }
}



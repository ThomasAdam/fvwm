/****************************************************************************
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/****************************************************************************
 *
 * Assorted odds and ends
 *
 **************************************************************************/


#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>

#include "fvwm.h"
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

FvwmWindow *FocusOnNextTimeStamp = NULL;

char NoName[] = "Untitled"; /* name if no name in XA_WM_NAME */
char NoClass[] = "NoClass"; /* Class if no res_class in class hints */
char NoResource[] = "NoResource"; /* Class if no res_name in class hints */

/**************************************************************************
 * 
 * Releases dynamically allocated space used to store window/icon names
 *
 **************************************************************************/
void free_window_names (FvwmWindow *tmp, Bool nukename, Bool nukeicon)
{
  if (!tmp)
    return;
  
  if (nukename && nukeicon) 
    {
      if (tmp->name == tmp->icon_name) 
	{
	  if (tmp->name != NoName && tmp->name != NULL)
	    XFree (tmp->name);
	  tmp->name = NULL;
	  tmp->icon_name = NULL;
	}
      else
	{
	  if (tmp->name != NoName && tmp->name != NULL)
	    XFree (tmp->name);
	  tmp->name = NULL;
	  if (tmp->icon_name != NoName && tmp->icon_name != NULL)
	    XFree (tmp->icon_name);
	  tmp->icon_name = NULL;
	}
    }   
  else if (nukename) 
    {
      if (tmp->name != tmp->icon_name
          && tmp->name != NoName
          && tmp->name != NULL)
	XFree (tmp->name);
      tmp->name = NULL;
    }
  else
    { /* if (nukeicon) */
      if (tmp->icon_name != tmp->name
          && tmp->icon_name != NoName
          && tmp->icon_name != NULL)
	XFree (tmp->icon_name);
      tmp->icon_name = NULL;
    }
  
  return;
}

/***************************************************************************
 *
 * Handles destruction of a window 
 *
 ****************************************************************************/
void Destroy(FvwmWindow *Tmp_win)
{ 
  int i;
  extern FvwmWindow *ButtonWindow;
  extern FvwmWindow *colormap_win;
  extern Boolean PPosOverride;

  /*
   * Warning, this is also called by HandleUnmapNotify; if it ever needs to
   * look at the event, HandleUnmapNotify will have to mash the UnmapNotify
   * into a DestroyNotify.
   */
  if(!Tmp_win)
    return;

  XUnmapWindow(dpy, Tmp_win->frame);

  if(!PPosOverride)
    XSync(dpy,0);
  
  if(Tmp_win == Scr.Hilite)
    Scr.Hilite = NULL;
  
  Broadcast(M_DESTROY_WINDOW,3,Tmp_win->w,Tmp_win->frame,
	    (unsigned long)Tmp_win,0,0,0,0);

  if(Scr.PreviousFocus == Tmp_win)
    Scr.PreviousFocus = NULL;

  if(ButtonWindow == Tmp_win)
    ButtonWindow = NULL;

  if((Tmp_win == Scr.Focus)&&(Tmp_win->flags & ClickToFocus))
    {
      if(Tmp_win->next)
	{
	  HandleHardFocus(Tmp_win->next);
	}
      else
	SetFocus(Scr.NoFocusWin, NULL,1);
    }
  else if(Scr.Focus == Tmp_win)
    SetFocus(Scr.NoFocusWin, NULL,1);

  if(Tmp_win == FocusOnNextTimeStamp)
    FocusOnNextTimeStamp = NULL;

  if(Tmp_win == Scr.Ungrabbed)
    Scr.Ungrabbed = NULL;

  if(Tmp_win == Scr.pushed_window)
    Scr.pushed_window = NULL;

  if(Tmp_win == colormap_win)
    colormap_win = NULL;

  XDestroyWindow(dpy, Tmp_win->frame);
  XDeleteContext(dpy, Tmp_win->frame, FvwmContext);

  XDestroyWindow(dpy, Tmp_win->Parent);

  XDeleteContext(dpy, Tmp_win->Parent, FvwmContext);

  XDeleteContext(dpy, Tmp_win->w, FvwmContext);
  
  if ((Tmp_win->icon_w)&&(Tmp_win->flags & PIXMAP_OURS))
    XFreePixmap(dpy, Tmp_win->iconPixmap);
  
  if (Tmp_win->icon_w)
    {
      XDestroyWindow(dpy, Tmp_win->icon_w);
      XDeleteContext(dpy, Tmp_win->icon_w, FvwmContext);
    }
  if((Tmp_win->flags &ICON_OURS)&&(Tmp_win->icon_pixmap_w != None))
    XDestroyWindow(dpy, Tmp_win->icon_pixmap_w);
  if(Tmp_win->icon_pixmap_w != None)
    XDeleteContext(dpy, Tmp_win->icon_pixmap_w, FvwmContext);

  if (Tmp_win->flags & TITLE)
    {
      XDeleteContext(dpy, Tmp_win->title_w, FvwmContext);
      for(i=0;i<Scr.nr_left_buttons;i++)
	XDeleteContext(dpy, Tmp_win->left_w[i], FvwmContext);
      for(i=0;i<Scr.nr_right_buttons;i++)
	if(Tmp_win->right_w[i] != None)
	  XDeleteContext(dpy, Tmp_win->right_w[i], FvwmContext);
    }
  if (Tmp_win->flags & BORDER)
    {
      for(i=0;i<4;i++)
	XDeleteContext(dpy, Tmp_win->sides[i], FvwmContext);
      for(i=0;i<4;i++)
	XDeleteContext(dpy, Tmp_win->corners[i], FvwmContext);
    }
  
  Tmp_win->prev->next = Tmp_win->next;
  if (Tmp_win->next != NULL)
    Tmp_win->next->prev = Tmp_win->prev;
  free_window_names (Tmp_win, True, True);		
  if (Tmp_win->wmhints)					
    XFree ((char *)Tmp_win->wmhints);
  /* removing NoClass change for now... */
#if 0
  if (Tmp_win->class.res_name)  
    XFree ((char *)Tmp_win->class.res_name);
  if (Tmp_win->class.res_class) 
    XFree ((char *)Tmp_win->class.res_class);
#else
  if (Tmp_win->class.res_name && Tmp_win->class.res_name != NoResource)
    XFree ((char *)Tmp_win->class.res_name);
  if (Tmp_win->class.res_class && Tmp_win->class.res_class != NoClass)
    XFree ((char *)Tmp_win->class.res_class);
#endif /* 0 */
  if(Tmp_win->mwm_hints)
    XFree((char *)Tmp_win->mwm_hints);

  if(Tmp_win->cmap_windows != (Window *)NULL)
    XFree((void *)Tmp_win->cmap_windows);

  free((char *)Tmp_win);

  if(!PPosOverride)
    XSync(dpy,0);
  return;
}



/**************************************************************************
 *
 * Removes expose events for a specific window from the queue 
 *
 *************************************************************************/
int flush_expose (Window w)
{
  XEvent dummy;
  int i=0;
  
  while (XCheckTypedWindowEvent (dpy, w, Expose, &dummy))i++;
  return i;
}



/***********************************************************************
 *
 *  Procedure:
 *	RestoreWithdrawnLocation
 * 
 *  Puts windows back where they were before fvwm took over 
 *
 ************************************************************************/
void RestoreWithdrawnLocation (FvwmWindow *tmp,Bool restart)
{
  int a,b,w2,h2;
  unsigned int bw,mask;
  XWindowChanges xwc;
  
  if(!tmp)
    return;
  
  if (XGetGeometry (dpy, tmp->w, &JunkRoot, &xwc.x, &xwc.y, 
		    &JunkWidth, &JunkHeight, &bw, &JunkDepth)) 
    {
      XTranslateCoordinates(dpy,tmp->frame,Scr.Root,xwc.x,xwc.y,
			    &a,&b,&JunkChild);
      xwc.x = a + tmp->xdiff;
      xwc.y = b + tmp->ydiff;
      xwc.border_width = tmp->old_bw;
      mask = (CWX | CWY|CWBorderWidth);
      
      /* We can not assume that the window is currently on the screen.
       * Although this is normally the case, it is not always true.  The
       * most common example is when the user does something in an
       * application which will, after some amount of computational delay,
       * cause the window to be unmapped, but then switches screens before
       * this happens.  The XTranslateCoordinates call above will set the
       * window coordinates to either be larger than the screen, or negative.
       * This will result in the window being placed in odd, or even
       * unviewable locations when the window is remapped.  The followin code
       * forces the "relative" location to be within the bounds of the display.
       *
       * gpw -- 11/11/93
       *
       * Unfortunately, this does horrendous things during re-starts, 
       * hence the "if(restart)" clause (RN) 
       *
       * Also, fixed so that it only does this stuff if a window is more than
       * half off the screen. (RN)
       */
      
      if(!restart)
	{
	  /* Don't mess with it if its partially on the screen now */
	  if((tmp->frame_x < 0)||(tmp->frame_y<0)||
	     (tmp->frame_x >= Scr.MyDisplayWidth)||
	     (tmp->frame_y >= Scr.MyDisplayHeight))
	    {
	      w2 = (tmp->frame_width>>1);
	      h2 = (tmp->frame_height>>1);
	      if (( xwc.x < -w2) || (xwc.x > (Scr.MyDisplayWidth-w2 )))
		{
		  xwc.x = xwc.x % Scr.MyDisplayWidth;
		  if ( xwc.x < -w2 )
		    xwc.x += Scr.MyDisplayWidth;
		}
	      if ((xwc.y < -h2) || (xwc.y > (Scr.MyDisplayHeight-h2 )))
		{
		  xwc.y = xwc.y % Scr.MyDisplayHeight;
		  if ( xwc.y < -h2 )
		    xwc.y += Scr.MyDisplayHeight;
		}
	    }
	}
      XReparentWindow (dpy, tmp->w,Scr.Root,xwc.x,xwc.y);
      
      if((tmp->flags & ICONIFIED)&&(!(tmp->flags & SUPPRESSICON)))
	{
	  if (tmp->icon_w) 
	    XUnmapWindow(dpy, tmp->icon_w);
	  if (tmp->icon_pixmap_w) 
	    XUnmapWindow(dpy, tmp->icon_pixmap_w);	  
	}
      
      XConfigureWindow (dpy, tmp->w, mask, &xwc);
      if(!restart)
	XSync(dpy,0);
    }
}


/****************************************************************************
 *
 * Records the time of the last processed event. Used in XSetInputFocus
 *
 ****************************************************************************/
Time lastTimestamp = CurrentTime;	/* until Xlib does this for us */

Bool StashEventTime (XEvent *ev)
{
  Time NewTimestamp = CurrentTime;
  
  switch (ev->type) 
    {
    case KeyPress:
    case KeyRelease:
      NewTimestamp = ev->xkey.time;
      break;
    case ButtonPress:
    case ButtonRelease:
      NewTimestamp = ev->xbutton.time;
      break;
    case MotionNotify:
      NewTimestamp = ev->xmotion.time;
      break;
    case EnterNotify:
    case LeaveNotify:
      NewTimestamp = ev->xcrossing.time;
      break;
    case PropertyNotify:
      NewTimestamp = ev->xproperty.time;
      break;
    case SelectionClear:
      NewTimestamp = ev->xselectionclear.time;
      break;
    case SelectionRequest:
      NewTimestamp = ev->xselectionrequest.time;
      break;
    case SelectionNotify:
      NewTimestamp = ev->xselection.time;
      break;
    default:
      return False;
    }
  /* Only update is the new timestamp is later than the old one, or
   * if the new one is from a time at least 30 seconds earlier than the
   * old one (in which case the system clock may have changed) */
  if((NewTimestamp > lastTimestamp)||((lastTimestamp - NewTimestamp) > 30000))
    lastTimestamp = NewTimestamp;
  if(FocusOnNextTimeStamp)
    {
      SetFocus(FocusOnNextTimeStamp->w,FocusOnNextTimeStamp,1);      
      FocusOnNextTimeStamp = NULL;
    }
  return True;
}





int GetTwoArguments(char *action, int *val1, int *val2, int *val1_unit, int *val2_unit)
{
  char c1, c2;
  int n;

  *val1 = 0;
  *val2 = 0;
  *val1_unit = Scr.MyDisplayWidth;
  *val2_unit = Scr.MyDisplayHeight;

  n = sscanf(action,"%d %d", val1, val2);
  if(n == 2)
    return 2;

  c1 = 's';
  c2 = 's';
  n = sscanf(action,"%d%c %d%c", val1, &c1, val2, &c2);
  if(n == 4)
  {
    if((c1 == 'p')||(c1 == 'P'))
      *val1_unit = 100;
    if((c2 == 'p')||(c2 == 'P'))
      *val2_unit = 100;
    return 2;
  }

  /* now try MxN style number, specifically for DeskTopSize: */
  n = sscanf(action,"%d%*c%d", val1, val2);
  if (n == 2)
    return 2;

  return 0;
}


void ComputeActualPosition(int x,int y,int x_unit,int y_unit,
			   int width,int height,int *pfinalX, int *pfinalY)
{
  *pfinalX = x*x_unit/100;
  *pfinalY = y*y_unit/100;
  if (*pfinalX < 0) 
    *pfinalX += Scr.MyDisplayWidth - width;
  if (*pfinalY < 0)
    *pfinalY += Scr.MyDisplayHeight - height;
}

/* The vars are named for the x-direction, but this is used for both x and y */
static
int GetOnePositionArgument(char *s1,int x,int w,int *pFinalX,float factor,int max)
{
  int val;
  int cch = strlen(s1);

  if (s1[cch-1] == 'p') {
    factor = 1;  /* Use pixels, so don't multiply by factor */
    s1[cch-1] = '\0';
  }
  if (strcmp(s1,"w") == 0) {
    *pFinalX = x;
  } else if (sscanf(s1,"w-%d",&val) == 1) {
    *pFinalX = x-(val*factor);
  } else if (sscanf(s1,"w+%d",&val) == 1) {
    *pFinalX = x+(val*factor);
  } else if (sscanf(s1,"-%d",&val) == 1) {
    *pFinalX = max-w - val*factor;
  } else if (sscanf(s1,"%d",&val) == 1) {
    *pFinalX = val*factor;
  } else {
    return 0;
  }
  /* DEBUG_FPRINTF((stderr,"Got %d\n",*pFinalX)); */
  return 1;
}

/* GetPositionArguments is used for Move, AnimatedMove, maybe more 
 * It lets you specify in all the following ways
 *   20  30          Absolute percent position, from left edge and top
 *  -50  50          Absolute percent position, from right edge and top
 *   10p 5p          Absolute pixel position
 *   10p -0p         Absolute pixel position, from bottom
 *  w+5  w-10p       Relative position, right 5%, up ten pixels
 */
int GetPositionArguments(char *action, int x, int y, int w, int h, int *pFinalX, int *pFinalY)
{
  char *s1, *s2;
  int scrWidth = Scr.MyDisplayWidth;
  int scrHeight = Scr.MyDisplayHeight;

  s1 = strtok(action," \t\n");
  s2 = strtok(NULL," \t\n");
  /* DEBUG_FPRINTF((stderr,"GPA: 1=\"%s\", 2=\"%s\"\n",s1,s2)); */
  
  if (s1 == NULL || s2 == NULL)
    return 0;

  if (GetOnePositionArgument(s1,x,w,pFinalX,(float)scrWidth/100,scrWidth) &&
      GetOnePositionArgument(s2,y,h,pFinalY,(float)scrHeight/100,scrHeight))
    return 2;
  else
    return 0;
}

/*****************************************************************************
 * Used by GetMenuOptions
 *
 * The vars are named for the x-direction, but this is used for both x and y
 *****************************************************************************/
static
char *GetOneMenuPositionArgument(char *action,int x,int w,int *pFinalX,
				 float *width_factor)
{
  char *token, *naction;
  int val;
  int length;
  float factor = (float)w/100;

  naction = GetNextToken(action, &token);
  if (token == NULL)
    return action;
  length = strlen(token);
  if (token[length-1] == 'p') {
    factor = 1;  /* Use pixels, so don't multiply by factor */
    token[length-1] = '\0';
  }
  if (strcmp(token,"c") == 0) {
    *pFinalX = x + ((float)w)/2;
    *width_factor = -0.5;
  } else if (sscanf(token,"c-%d",&val) == 1) {
    *pFinalX = x + ((float)w)/2 - val*factor;
    *width_factor = -0.5;
  } else if (sscanf(token,"c+%d",&val) == 1) {
    *pFinalX = x + ((float)w)/2 + val*factor;
    *width_factor = -0.5;
  } else if (sscanf(token,"-%d",&val) == 1) {
    *pFinalX = x + w - val*factor;
    *width_factor = -1;
  } else if (sscanf(token,"%d",&val) == 1) {
    *pFinalX = x + val*factor;
    *width_factor = 0;
  } else {
    naction = action;
  }
  free(token); 
  return naction;
}

/*****************************************************************************
 * GetMenuOptions is used for Menu, Popup and WindowList
 * It parses strings matching
 *
 *   [ [context-rectangle] x y ] [special-options] [other arguments]
 *
 * and returns a pointer to the first part of the input string that doesn't
 * match this syntax.
 *
 * See documentation for a detailed description.
 ****************************************************************************/
char *GetMenuOptions(char *action, Window w, FvwmWindow *tmp_win,
		     MenuItem *mi, MenuOptions *pops)
{
  char *tok = NULL, *naction = action, *taction;
  int x, y, button, gflags;
  unsigned int width, height;
  Window context_window = 0;
  Bool fHasContext, fUseItemOffset;
  Bool fValidPosHints = fLastMenuPosHintsValid;

  fLastMenuPosHintsValid = FALSE;
  if (pops == NULL) {
    fvwm_msg(ERR,"GetMenuOptions","no MenuOptions pointer passed");
    return action;
  }
  
  taction = action;
  while (action != NULL) {
    /* ^ just to be able to jump to end of loop without 'goto' */
    gflags = NoValue;
    pops->flags = 0;
    pops->pos_hints.fRelative = FALSE;
    /* parse context argument (if present) */
    naction = GetNextToken(taction, &tok);
    
    pops->pos_hints.fRelative = TRUE; /* set to FALSE for absolute hints! */
    fUseItemOffset = FALSE;
    fHasContext = TRUE;
    if (StrEquals(tok, "context")) {
      if (mi && mi->mr) context_window = mi->mr->w;
      else if (tmp_win) {
	if (tmp_win->flags & ICONIFIED) context_window=tmp_win->icon_pixmap_w;
	else context_window = tmp_win->frame;
      } else context_window = w;
      pops->pos_hints.fRelative = TRUE;
    } else if (StrEquals(tok,"menu")) {
      if (mi && mi->mr) context_window = mi->mr->w;
    } else if (StrEquals(tok,"item")) {
      if (mi && mi->mr) {
	context_window = mi->mr->w;
	fUseItemOffset = TRUE;
      }
    } else if (StrEquals(tok,"icon")) {
      if (tmp_win) context_window = tmp_win->icon_pixmap_w;
    } else if (StrEquals(tok,"window")) {
      if (tmp_win) context_window = tmp_win->frame;
    } else if (StrEquals(tok,"interior")) {
      if (tmp_win) context_window = tmp_win->w;
    } else if (StrEquals(tok,"title")) {
      if (tmp_win) {
	if (tmp_win->flags & ICONIFIED) context_window = tmp_win->icon_w;
	else context_window = tmp_win->title_w;
      }
    } else if (mystrncasecmp(tok,"button",6) == 0) {
      if (sscanf(&(tok[6]),"%d",&button) != 1 ||
		 tok[6] == '+' || tok[6] == '-' || button < 0 || button > 9) {
	fHasContext = FALSE;
      } else if (tmp_win) {
	if (button == 0) button = 10;
	if (button & 0x01) context_window = tmp_win->left_w[button/2];
	else context_window = tmp_win->right_w[button/2-1];
      }
    } else if (StrEquals(tok,"root")) {
      context_window = Scr.Root;
      pops->pos_hints.fRelative = FALSE;
    } else if (StrEquals(tok,"mouse")) {
      context_window = 0;
    } else if (StrEquals(tok,"rectangle")) {
      int flags;
      /* parse the rectangle */
      free(tok);
      naction = GetNextToken(taction, &tok);
      if (tok == NULL) {
	fvwm_msg(ERR,"GetMenuOptions","missing rectangle geometry");
	return action;
      }
      flags = XParseGeometry(tok, &x, &y, &width, &height);
      if ((flags & AllValues) != AllValues) {
	free(tok);
	fvwm_msg(ERR,"GetMenuOptions","invalid rectangle geometry");
	return action;
      }
      if (flags & XNegative) x = Scr.MyDisplayWidth - x - width;
      if (flags & YNegative) y = Scr.MyDisplayHeight - y - height;
      pops->pos_hints.fRelative = FALSE;
    } else if (StrEquals(tok,"this")) {
      context_window = w;
    } else {
      /* no context string */
      fHasContext = FALSE;
    }
    
    free(tok);
    if (fHasContext) taction = naction;
    else naction = action;
    
    if (!context_window || !fHasContext
	|| !XGetGeometry(dpy, context_window, &JunkRoot, &JunkX, &JunkY,
			 &width, &height, &JunkBW, &JunkDepth)
	|| !XTranslateCoordinates(
	  dpy, context_window, Scr.Root, 0, 0, &x, &y, &JunkChild)) {
      /* now window or could not get geometry */
      XQueryPointer(dpy,Scr.Root,&JunkRoot,&JunkChild,&x,&y,&JunkX,&JunkY,
		    &JunkMask);
      width = height = 1;
    } else if (fUseItemOffset) {
      y += mi->y_offset;
      height = mi->y_height;
    }
    
    /* parse position arguments */
    taction = GetOneMenuPositionArgument(
      naction, x, width, &(pops->pos_hints.x), &(pops->pos_hints.x_factor));
    naction = GetOneMenuPositionArgument(
      taction, y, height, &(pops->pos_hints.y), &(pops->pos_hints.y_factor));
    if (naction == taction) {
      /* argument is missing or invalid */
      if (fHasContext)
	fvwm_msg(ERR,"GetMenuOptions","invalid position arguments");
      naction = action;
      taction = action;
      break;
    }
    taction = naction;
    pops->flags |= MENU_HAS_POSHINTS;
    if (fValidPosHints == TRUE && pops->pos_hints.fRelative == TRUE) {
      pops->pos_hints = lastMenuPosHints;
    }
    /* we want to do this only once */
    break;
  } /* while (1) */

  if (!(pops->flags & MENU_HAS_POSHINTS) && fValidPosHints) {
    DBUG("GetMenuOptions","recycling position hints");
    pops->flags |= MENU_HAS_POSHINTS;
    pops->pos_hints = lastMenuPosHints;
    pops->pos_hints.fRelative = FALSE;
  }

  action = naction; 
  /* parse additional options */
  while (naction && *naction) {
    naction = GetNextToken(action, &tok);
    if (StrEquals(tok, "WarpTitle")) {
      pops->flags = (pops->flags | MENU_WARPTITLE) & ~MENU_NOWARP;
    } else if (StrEquals(tok, "NoWarp")) {
      pops->flags = (pops->flags | MENU_NOWARP) & ~MENU_WARPTITLE;
    } else if (StrEquals(tok, "Fixed")) {
      pops->flags |= MENU_FIXED;
    } else if (StrEquals(tok, "SelectInPlace")) {
      pops->flags = pops->flags|MENU_SELECTINPLACE;
    } else if (StrEquals(tok, "SelectWarp")) {
      pops->flags = pops->flags|MENU_SELECTWARP;
    } else {
      free(tok);
      break;
    }
    action = naction;
    free (tok);
  }
  if (!(pops->flags|MENU_SELECTINPLACE)) {
    pops->flags &= ~MENU_SELECTWARP;
  }

  return action;
}

int GetOneArgument(char *action, int *val1, int *val1_unit)
{
  char c1;
  int n;

  *val1 = 0;
  *val1_unit = Scr.MyDisplayWidth;

  n = sscanf(action,"%d", val1);
  if(n == 1)
    return 1;

  c1 = '%';
  n = sscanf(action,"%d%c", val1, &c1);

  if(n != 2)
    return 0;
  
  if((c1 == 'p')||(c1 == 'P'))
    *val1_unit = 100;

  return 1;
}


/***************************************************************************
 * 
 * Wait for all mouse buttons to be released 
 * This can ease some confusion on the part of the user sometimes 
 * 
 * Discard superflous button events during this wait period.
 *
 ***************************************************************************/
void WaitForButtonsUp()
{
  Bool AllUp = False;
  XEvent JunkEvent;
  unsigned int mask;

  while(!AllUp)
    {
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &JunkX, &JunkY, &mask);    
      
      if((mask&
	  (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask))==0)
	AllUp = True;
    }
  XSync(dpy,0);
  while(XCheckMaskEvent(dpy,
			ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
			&JunkEvent))
    {
      StashEventTime (&JunkEvent);
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
    }

}

/*****************************************************************************
 *
 * Grab the pointer and keyboard
 *
 ****************************************************************************/
Bool GrabEm(int cursor)
{
  int i=0,val=0;
  unsigned int mask;

  XSync(dpy,0);
  /* move the keyboard focus prior to grabbing the pointer to
   * eliminate the enterNotify and exitNotify events that go
   * to the windows */
  if(Scr.PreviousFocus == NULL)
    Scr.PreviousFocus = Scr.Focus;
  SetFocus(Scr.NoFocusWin,NULL,0);
  mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|PointerMotionMask
    | EnterWindowMask | LeaveWindowMask;
  while((i<1000)&&(val=XGrabPointer(dpy, Scr.Root, True, mask,
				    GrabModeAsync, GrabModeAsync, Scr.Root,
				    Scr.FvwmCursors[cursor], CurrentTime)!=
		   GrabSuccess))
    {
      i++;
      /* If you go too fast, other windows may not get a change to release
       * any grab that they have. */
      sleep_a_little(1000);
    }

  /* If we fall out of the loop without grabbing the pointer, its
     time to give up */
  XSync(dpy,0);
  if(val!=GrabSuccess)
    {
      return False;
    }
  return True;
}


/*****************************************************************************
 *
 * UnGrab the pointer and keyboard
 *
 ****************************************************************************/
void UngrabEm()
{
  Window w;

  XSync(dpy,0);
  XUngrabPointer(dpy,CurrentTime);

  if(Scr.PreviousFocus != NULL)
    {
      w = Scr.PreviousFocus->w;

      /* if the window still exists, focus on it */
      if (w)
	{
	  SetFocus(w,Scr.PreviousFocus,0);
	}
      Scr.PreviousFocus = NULL;
    }
  XSync(dpy,0);
}



/****************************************************************************
 *
 * Keeps the "StaysOnTop" windows on the top of the pile.
 * This is achieved by clearing a flag for OnTop windows here, and waiting
 * for a visibility notify on the windows. Exeption: OnTop windows which are
 * obscured by other OnTop windows, which need to be raised here.
 *
 ****************************************************************************/
void KeepOnTop()
{
  FvwmWindow *t;

  /* flag that on-top windows should be re-raised */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if((t->flags & ONTOP)&&!(t->flags & VISIBLE))
	{
	  RaiseWindow(t);
	  t->flags &= ~RAISED;
	}
      else
	t->flags |= RAISED;
    }
}


/**************************************************************************
 * 
 * Unmaps a window on transition to a new desktop
 *
 *************************************************************************/
void UnmapIt(FvwmWindow *t)
{
  XWindowAttributes winattrs;
  unsigned long eventMask;
  /*
   * Prevent the receipt of an UnmapNotify, since that would
   * cause a transition to the Withdrawn state.
   */
  XGetWindowAttributes(dpy, t->w, &winattrs);
  eventMask = winattrs.your_event_mask;
  XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
  if(t->flags & ICONIFIED)
    {
      if(t->icon_pixmap_w != None)
	XUnmapWindow(dpy,t->icon_pixmap_w);
      if(t->icon_w != None)
	XUnmapWindow(dpy,t->icon_w);
    }
  else if(t->flags & (MAPPED|MAP_PENDING))
    {
      XUnmapWindow(dpy,t->frame);
    }
  XSelectInput(dpy, t->w, eventMask);
}

/**************************************************************************
 * 
 * Maps a window on transition to a new desktop
 *
 *************************************************************************/
void MapIt(FvwmWindow *t)
{
  if(t->flags & ICONIFIED)
    {
      if(t->icon_pixmap_w != None)
	XMapWindow(dpy,t->icon_pixmap_w);
      if(t->icon_w != None)
	XMapWindow(dpy,t->icon_w);
    }
  else if(t->flags & MAPPED)
    {
      XMapWindow(dpy,t->frame);
      t->flags |= MAP_PENDING;
      XMapWindow(dpy, t->Parent);
   }
}




void RaiseWindow(FvwmWindow *t)
{
  FvwmWindow *t2;
  int count, i;
  Window *wins;

  /* raise the target, at least */
  count = 1;
  Broadcast(M_RAISE_WINDOW,3,t->w,t->frame,(unsigned long)t,0,0,0,0);
  
  for (t2 = Scr.FvwmRoot.next; t2 != NULL; t2 = t2->next)
    {
      if(t2->flags & ONTOP)
	count++;
      if((t2->flags & TRANSIENT) &&(t2->transientfor == t->w)&&
	 (t2 != t))
	{
	  count++;
	  Broadcast(M_RAISE_WINDOW,3,t2->w,t2->frame,(unsigned long) t2,
		    0,0,0,0);
	  if ((t2->flags & ICONIFIED)&&(!(t2->flags & SUPPRESSICON)))
	    {
	      count += 2;
	    }	  
	}
    }
  if ((t->flags & ICONIFIED)&&(!(t->flags & SUPPRESSICON)))
    {
      count += 2;
    }

  wins = (Window *)safemalloc(count*sizeof(Window));

  i=0;

  /* ONTOP windows on top */
  for (t2 = Scr.FvwmRoot.next; t2 != NULL; t2 = t2->next)
    {
      if(t2->flags & ONTOP)
	{
	  Broadcast(M_RAISE_WINDOW,3,t2->w,t2->frame,(unsigned long) t2,
		    0,0,0,0);
	  wins[i++] = t2->frame;
	}
    }

  /* now raise transients */
#ifndef DONT_RAISE_TRANSIENTS
  for (t2 = Scr.FvwmRoot.next; t2 != NULL; t2 = t2->next)
  {
    if((t2->flags & TRANSIENT) &&
       (t2->transientfor == t->w) &&
       (t2 != t) &&
       (!(t2->flags & ONTOP)))
    {
      wins[i++] = t2->frame;
      if ((t2->flags & ICONIFIED)&&(!(t2->flags & SUPPRESSICON)))
      {
        if(!(t2->flags & NOICON_TITLE))
          wins[i++] = t2->icon_w;
        if(!(t2->icon_pixmap_w))
          wins[i++] = t2->icon_pixmap_w;
      }
    }
  }
#endif
  if ((t->flags & ICONIFIED)&&(!(t->flags & SUPPRESSICON)))
  {
    if(!(t->flags & NOICON_TITLE)) 
      wins[i++] = t->icon_w;
    if (t->icon_pixmap_w)
      wins[i++] = t->icon_pixmap_w;
  }
  if(!(t->flags & ONTOP))
    wins[i++] = t->frame;
  if(!(t->flags & ONTOP))
    Scr.LastWindowRaised = t;

  if(i > 0)
    XRaiseWindow(dpy,wins[0]);

  XRestackWindows(dpy,wins,i);
  free(wins);
  raisePanFrames();
}


void LowerWindow(FvwmWindow *t)
{
  XLowerWindow(dpy,t->frame);

  Broadcast(M_LOWER_WINDOW,3,t->w,t->frame,(unsigned long)t,0,0,0,0);

  if((t->flags & ICONIFIED)&&(!(t->flags & SUPPRESSICON)))
    {
      XLowerWindow(dpy, t->icon_w);
      XLowerWindow(dpy, t->icon_pixmap_w);
    }
  Scr.LastWindowRaised = (FvwmWindow *)0;
}


void HandleHardFocus(FvwmWindow *t)
{
  int x,y;

  FocusOnNextTimeStamp = t;
  Scr.Focus = NULL;
  /* Do something to guarantee a new time stamp! */
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&JunkX, &JunkY, &x, &y, &JunkMask);
  GrabEm(WAIT);
  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
	       Scr.MyDisplayHeight, 
	       x + 2,y+2);
  XSync(dpy,0);
  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
	       Scr.MyDisplayHeight, 
	       x ,y);
  UngrabEm();
}


/*
** fvwm_msg: used to send output from fvwm to files and or stderr/stdout
**
** type -> DBG == Debug, ERR == Error, INFO == Information, WARN == Warning
** id -> name of function, or other identifier
*/
void fvwm_msg(int type,char *id,char *msg,...)
{
  char *typestr;
  va_list args;

  switch(type)
  {
    case DBG:
#if 0
      if (!debugging)
        return;
#endif /* 0 */
      typestr="<<DEBUG>>";
      break;
    case ERR:
      typestr="<<ERROR>>";
      break;
    case WARN:
      typestr="<<WARNING>>";
      break;
    case INFO:
    default:
      typestr="";
      break;
  }

  va_start(args,msg);

  fprintf(stderr,"[FVWM][%s]: %s ",id,typestr);
  vfprintf(stderr, msg, args);
  fprintf(stderr,"\n");

  if (type == ERR)
  {
    char tmp[1024]; /* I hate to use a fixed length but this will do for now */
    sprintf(tmp,"[FVWM][%s]: %s ",id,typestr);
    vsprintf(tmp+strlen(tmp), msg, args);
    tmp[strlen(tmp)+1]='\0';
    tmp[strlen(tmp)]='\n';
    BroadcastName(M_ERROR,0,0,0,tmp);
  }

  va_end(args);
} /* fvwm_msg */

/* CoerceEnterNotifyOnCurrentWindow()
 * Pretends to get a HandleEnterNotify on the
 * window that the pointer currently is in so that
 * the focus gets set correctly from the beginning
 * Note that this presently only works if the current
 * window is not click_to_focus;  I think that
 * that behaviour is correct and desirable. --11/08/97 gjb */
void
CoerceEnterNotifyOnCurrentWindow()
{
  extern FvwmWindow *Tmp_win; /* from events.c */
  Window child, root;
  int root_x, root_y;
  int win_x, win_y;
  Bool f = XQueryPointer(dpy, Scr.Root, &root,
                         &child, &root_x, &root_y, &win_x, &win_y, &JunkMask);
  if (f && child != None) {
    Event.xany.window = child;
    if (XFindContext(dpy, child, FvwmContext, (caddr_t *) &Tmp_win) ==
XCNOENT)
      Tmp_win = NULL;
    HandleEnterNotify();
    Tmp_win = None;
  }
}

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/****************************************************************************
 * This module is all new
 * by Rob Nation
 * A little of it is borrowed from ctwm.
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/
/***********************************************************************
 *
 * fvwm window-list popup code
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <limits.h>

#include "config.h"
#include "fvwmlib.h"

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

#define SHOW_GEOMETRY (1<<0)
#define SHOW_ALLDESKS (1<<1)
#define SHOW_NORMAL   (1<<2)
#define SHOW_ICONIC   (1<<3)
#define SHOW_STICKY   (1<<4)
#define NO_DESK_SORT  (1<<6)
#define SHOW_ICONNAME (1<<7)
#define SHOW_ALPHABETIC (1<<8)
#define SHOW_EVERYTHING (SHOW_GEOMETRY | SHOW_ALLDESKS | SHOW_NORMAL | SHOW_ICONIC | SHOW_STICKY)

/* Function to compare window title names
 */
static int globalFlags;
int winCompare(const  FvwmWindow **a, const  FvwmWindow **b)
{
  if(globalFlags & SHOW_ICONNAME)
    return strcasecmp((*a)->icon_name,(*b)->icon_name);
  else
    return strcasecmp((*a)->name,(*b)->name);
}


/*
 * Change by PRB (pete@tecc.co.uk), 31/10/93.  Prepend a hot key
 * specifier to each item in the list.  This means allocating the
 * memory for each item (& freeing it) rather than just using the window
 * title directly. */
void do_windowList(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int *Module)
{
  MenuRoot *mr;
  char* ret_action = NULL;
  FvwmWindow *t;
  FvwmWindow **windowList;
  int numWindows;
  int ii;
  char tname[80] = "";
  char loc[40],*name=NULL;
  int dwidth,dheight;
  char tlabel[50]="";
  int last_desk_done = INT_MIN;
  int last_desk_displayed = INT_MIN;
  int next_desk = 0;
  char *t_hot=NULL;		/* Menu label with hotkey added */
  char scut = '0';		/* Current short cut key */
  char *line=NULL,*tok=NULL;
  int desk = Scr.CurrentDesk;
  int flags = SHOW_EVERYTHING;
  char *func=NULL;
  char *tfunc=NULL;
  char *default_action = NULL;
  MenuStatus menu_retval;
  XEvent *teventp;
  MenuOptions mops;
  int low_layer = 0;  /* show all layers by default */
  int high_layer = INT_MAX;

  mops.flags.allflags = 0;
  if (action && *action)
  {
    /* parse postitioning args */
    action = GetMenuOptions(action,w,tmp_win,NULL,&mops);
    line = action;
    /* parse options */
    while (line && *line)
    {
      line = GetNextSimpleOption(line, &tok);
      if (!tok)
	break;

      if (StrEquals(tok,"Function"))
      {
        line = GetNextSimpleOption(line, &func);
      }
      else if (StrEquals(tok,"Desk"))
      {
	free(tok);
        line = GetNextSimpleOption(line, &tok);
	if (tok)
	{
	  desk = atoi(tok);
	  flags &= ~SHOW_ALLDESKS;
	}
      }
      else if (StrEquals(tok,"CurrentDesk"))
      {
        desk = Scr.CurrentDesk;
        flags &= ~SHOW_ALLDESKS;
      }
      else if (StrEquals(tok,"NotAlphabetic"))
        flags &= ~SHOW_ALPHABETIC;
      else if (StrEquals(tok,"Alphabetic"))
        flags |= SHOW_ALPHABETIC;
      else if (StrEquals(tok,"NoDeskSort"))
        flags |= NO_DESK_SORT;
      else if (StrEquals(tok,"UseIconName"))
        flags |= SHOW_ICONNAME;
      else if (StrEquals(tok,"NoGeometry"))
        flags &= ~SHOW_GEOMETRY;
      else if (StrEquals(tok,"Geometry"))
        flags |= SHOW_GEOMETRY;
      else if (StrEquals(tok,"NoIcons"))
        flags &= ~SHOW_ICONIC;
      else if (StrEquals(tok,"Icons"))
        flags |= SHOW_ICONIC;
      else if (StrEquals(tok,"OnlyIcons"))
        flags = SHOW_ICONIC;
      else if (StrEquals(tok,"NoNormal"))
        flags &= ~SHOW_NORMAL;
      else if (StrEquals(tok,"Normal"))
        flags |= SHOW_NORMAL;
      else if (StrEquals(tok,"OnlyNormal"))
        flags = SHOW_NORMAL;
      else if (StrEquals(tok,"NoSticky"))
        flags &= ~SHOW_STICKY;
      else if (StrEquals(tok,"Sticky"))
        flags |= SHOW_STICKY;
      else if (StrEquals(tok,"OnlySticky"))
        flags = SHOW_STICKY;
          /*
             these are a bit dubious, but we
             should keep the OnTop options
             for compatibility
           */
      else if (StrEquals(tok, "NoOnTop"))
	{
	  if (high_layer >= Scr.TopLayer)
            high_layer = Scr.TopLayer - 1;
        }
      else if (StrEquals(tok, "OnTop"))
        {
	  if (high_layer < Scr.TopLayer)
            high_layer = Scr.TopLayer;
	}
      else if (StrEquals(tok, "OnlyOnTop"))
        {
	  high_layer = low_layer = Scr.TopLayer;
        }
      else if (StrEquals(tok, "Layers"))
      {
	free(tok);
        line = GetNextSimpleOption(line, &tok);
	if (tok)
        {
          low_layer = high_layer = atoi(tok);
	  free(tok);
          line = GetNextSimpleOption(line, &tok);
	  if (tok)
          {
            high_layer = atoi(tok);
          }
        }
      }
      else if (!line || !*line)
	default_action = strdup(tok);
      else
      {
        fvwm_msg(ERR,"WindowList","Unknown option '%s'",tok);
      }
      if (tok)
        free(tok);
    }
  }

  globalFlags = flags;
  if (flags & SHOW_GEOMETRY)
  {
    sprintf(tlabel,"Desk: %d\tGeometry",desk);
  }
  else
  {
    sprintf(tlabel,"Desk: %d",desk);
  }
  mr=NewMenuRoot(tlabel);
  AddToMenu(mr, tlabel, "TITLE", FALSE, FALSE);

  numWindows = 0;
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    numWindows++;
  }
  windowList = malloc(numWindows*sizeof(t));
  if (windowList == NULL)
  {
    return;
  }
  /* get the windowlist starting from the current window (if any)*/
  if ((t = Scr.Focus) == NULL) t = Scr.FvwmRoot.next;
  for (ii = 0; ii < numWindows; ii++)
  {
    windowList[ii] = t;
    if (t->next)
      t = t->next;
    else
      t = Scr.FvwmRoot.next;
  }

  /* Do alphabetic sort */
  if (flags & SHOW_ALPHABETIC)
    qsort(windowList,numWindows,sizeof(t),
	  (int(*)(const void*,const void*))winCompare);

  while(next_desk != INT_MAX)
  {
    /* Sort window list by desktop number */
    if((flags & SHOW_ALLDESKS) && !(flags & NO_DESK_SORT))
    {
      /* run through the windowlist finding the first desk not already processed */
      next_desk = INT_MAX;
      for (ii = 0; ii < numWindows; ii++)
      {
        t = windowList[ii];
        if((t->Desk >last_desk_done)&&(t->Desk < next_desk))
          next_desk = t->Desk;
      }
    }
    if(!(flags & SHOW_ALLDESKS))
    {
      /* if only doing one desk and it hasn't been done */
      if(last_desk_done  == INT_MIN)
        next_desk = desk; /* select the desk */
      else
        next_desk = INT_MAX; /* flag completion */
    }
    if(flags & NO_DESK_SORT)
      next_desk = INT_MAX; /* only go through loop once */

    last_desk_done = next_desk;
    for (ii = 0; ii < numWindows; ii++)
    {
      t = windowList[ii];
      if(((t->Desk == next_desk) || (flags & NO_DESK_SORT)) &&
         (!(DO_SKIP_WINDOW_LIST(t))))
      {
        if (!(flags & SHOW_ICONIC) && (IS_ICONIFIED(t)))
          continue; /* don't want icons - skip */
        if (!(flags & SHOW_STICKY) && (IS_STICKY(t)))
          continue; /* don't want sticky ones - skip */
        if (!(flags & SHOW_NORMAL) &&
            !((IS_ICONIFIED(t)) || (IS_STICKY(t))))
          continue; /* don't want "normal" ones - skip */
        if ((t->layer < low_layer) || (t->layer > high_layer))
          continue;  /* don't want this layer */

        /* add separator between desks when geometry shown but not at the top*/
        if (t->Desk != last_desk_displayed)
        {
          if (last_desk_displayed != INT_MIN)
            if (flags & SHOW_GEOMETRY)
            AddToMenu(mr, NULL, NULL, FALSE, FALSE);
          last_desk_displayed = t->Desk;
        }

        if(flags & SHOW_ICONNAME)
          name = t->icon_name;
        else
          name = t->name;
        t_hot = safemalloc(strlen(name) + strlen(tname) + 48);
        sprintf(t_hot, "&%c.  %s", scut, name); /* Generate label */
        if (scut++ == '9') scut = 'A';	/* Next shortcut key */

        if (flags & SHOW_GEOMETRY)
        {
          tname[0]=0;
          if(IS_ICONIFIED(t))
            strcpy(tname, "(");
          sprintf(loc,"%d(%d):",t->Desk, t->layer);
          strcat(tname,loc);

          dheight = t->frame_g.height - t->title_g.height -2*t->boundary_width;
          dwidth = t->frame_g.width - 2*t->boundary_width;

          dwidth -= t->hints.base_width;
          dheight -= t->hints.base_height;

          dwidth /= t->hints.width_inc;
          dheight /= t->hints.height_inc;

          sprintf(loc,"%d",dwidth);
          strcat(tname, loc);
          sprintf(loc,"x%d",dheight);
          strcat(tname, loc);
          if(t->frame_g.x >=0)
            sprintf(loc,"+%d",t->frame_g.x);
          else
            sprintf(loc,"%d",t->frame_g.x);
          strcat(tname, loc);
          if(t->frame_g.y >=0)
            sprintf(loc,"+%d",t->frame_g.y);
          else
            sprintf(loc,"%d",t->frame_g.y);
          strcat(tname, loc);

          if (IS_STICKY(t))
            strcat(tname, " S");
          if (IS_ICONIFIED(t))
            strcat(tname, ")");
          strcat(t_hot,"\t");
          strcat(t_hot,tname);
        }
        if (!func)
        {
          tfunc = safemalloc(40);
          sprintf(tfunc,"WindowListFunc %lu", t->w);
        }
        else
	{
          tfunc = safemalloc(strlen(func) + 32);
          sprintf(tfunc,"%s %lu", func, t->w);
	  free(func);
	  func = NULL;
	}
        AddToMenu(mr, t_hot, tfunc, FALSE, FALSE);
        free(tfunc);
#ifdef MINI_ICONS
        /* Add the title pixmap */
        if (t->mini_icon) {
          mr->last->lpicture = t->mini_icon;
          t->mini_icon->count++; /* increase the cache count!!
                                    otherwise the pixmap will be
                                    eventually removed from the
                                    cache by DestroyMenu */
        }
#endif
        if (t_hot)
          free(t_hot);
      }
    }
  }

  if (func)
    free(func);
  free(windowList);
  MakeMenu(mr);
  if (!default_action && eventp && eventp->type == KeyPress)
    teventp = (XEvent *)1;
  else
    teventp = eventp;
  menu_retval = do_menu(mr, NULL, &ret_action, 0, TRUE, teventp, &mops);
  if (ret_action)
    free(ret_action);
  DestroyMenu(mr, False);
  if (menu_retval == MENU_DOUBLE_CLICKED && default_action && *default_action)
    ExecuteFunction(default_action,tmp_win,eventp,context,*Module,
		    EXPAND_COMMAND);
  if (default_action != NULL)
    free(default_action);
}

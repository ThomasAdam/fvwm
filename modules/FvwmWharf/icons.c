/* icons.c. by Bo Yang.
 * Modifications: Copyright 1995 by Bo Yang.
 *
 * based on icons.c by Robert Nation
 * The icons module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

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

/***********************************************************************
 *
 * Derived from fvwm icon code
 *
 ***********************************************************************/

#include "config.h"
#include "libs/fvwmlib.h"
#include "libs/Picture.h"
#include "libs/Colorset.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "Wharf.h"

#ifdef XPM
#include <X11/xpm.h>
#endif /* XPM */
#include "stepgfx.h"


/****************************************************************************
 *
 * Loads an icon file into a pixmap
 *
 ****************************************************************************/
void LoadIconFile(int button, int ico)
{
#ifndef NO_ICONS
  /* First, check for a monochrome bitmap */
  if(Buttons[button].icons[ico].file != NULL)
    GetBitmapFile(button,ico);
  /* Next, check for a color pixmap */
  if((Buttons[button].icons[ico].file != NULL)&&
     (Buttons[button].icons[ico].w == 0)&&(Buttons[button].icons[ico].h == 0))
    GetXPMFile(button,ico);
#endif
}

/****************************************************************************
 *
 * Creates an Icon Window
 *
 ****************************************************************************/
void CreateIconWindow(int button, Window *win)
{
#ifndef NO_ICONS
  unsigned long valuemask;		/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */

    /* This used to make buttons without icons explode when pushed
  if((Buttons[button].icon_w == 0)&&(Buttons[button].icon_h == 0))
    return;
     */
  attributes.background_pixel = back_pix;
  attributes.border_pixel = 0;
  attributes.colormap = Pcmap;
  attributes.event_mask = ExposureMask;
  valuemask = CWEventMask | CWBackPixel | CWBorderPixel | CWColormap;

  Buttons[button].IconWin =
    XCreateWindow(dpy, *win, 0, 0, BUTTONWIDTH, BUTTONHEIGHT, 0, Pdepth,
		  InputOutput, Pvisual, valuemask, &attributes);

  return;
#endif
}

/****************************************************************************
 *
 * Combines icon shape masks after a resize
 *
 ****************************************************************************/
void ConfigureIconWindow(int button,int row, int column)
{
#ifndef NO_ICONS
  int x,y, w, h;
#ifdef XPM
  int xoff,yoff;
#endif
  int i;

  if (Buttons[button].completeIcon != None)
  {
    XFreePixmap(dpy, Buttons[button].completeIcon);
    Buttons[button].completeIcon = None;
  }

  if(Buttons[button].iconno == 0)
    return;
  if(Buttons[button].swallow != 0) {
    return;
  }
  x = column*BUTTONWIDTH;
  y = row*BUTTONHEIGHT;

  XMoveResizeWindow(dpy, Buttons[button].IconWin, x,y,BUTTONWIDTH,
		    BUTTONHEIGHT);
  Buttons[button].completeIcon =
    XCreatePixmap(dpy,main_win, BUTTONWIDTH,BUTTONHEIGHT,Pdepth);
  XCopyArea(dpy, Buttons[BACK_BUTTON].icons[0].icon,
	    Buttons[button].completeIcon, NormalGC, 0,0,
	    BUTTONWIDTH,BUTTONHEIGHT, 0,0);

  for(i=0;i<Buttons[button].iconno;i++) {
    w = Buttons[button].icons[i].w;
    h = Buttons[button].icons[i].h;
    if (w<1 || h<1) continue;
    if(w > BUTTONWIDTH) w = BUTTONWIDTH;
    if(h > BUTTONHEIGHT) h = BUTTONHEIGHT;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
#ifdef XPM
    xoff = (BUTTONWIDTH - w)/2;
    yoff = (BUTTONHEIGHT - h)/2;
    if (xoff<0) xoff=0;
    if (yoff<0) yoff=0;
    if (Buttons[button].icons[i].mask != None) {
      XSetClipOrigin(dpy, MaskGC, xoff, yoff);
      XSetClipMask(dpy, MaskGC, Buttons[button].icons[i].mask);
    } else {
      XRectangle rect[1];
      rect[0].x=0; rect[0].y=0;
      rect[0].width=w; rect[0].height=h;

      XSetClipRectangles(dpy,MaskGC,xoff,yoff, rect, 1, YSorted);
    }
    XCopyArea(dpy, Buttons[button].icons[i].icon,
	      Buttons[button].completeIcon, MaskGC, 0,0,
	      w, h, xoff,yoff);
#endif
    if(Buttons[button].icons[i].depth == -1) {
      XCopyPlane(dpy,Buttons[button].icons[i].icon,
		 Buttons[button].completeIcon,NormalGC,
		 0,0,w,h, 0,0,1);
      XFreePixmap(dpy, Buttons[button].icons[i].icon);
    }
  }
  XSetWindowBackgroundPixmap(dpy, Buttons[button].IconWin,
			     Buttons[button].completeIcon);
  XClearWindow(dpy,Buttons[button].IconWin);
#endif
}

/***************************************************************************
 *
 * Looks for a monochrome icon bitmap file
 *
 **************************************************************************/
void GetBitmapFile(int button, int ico)
{
#ifndef NO_ICONS
  char *path = NULL;
  int HotX,HotY;

  path = findImageFile(Buttons[button].icons[ico].file, imagePath,R_OK);
  if(path == NULL)return;

  if(XReadBitmapFile (dpy, main_win,path,(unsigned int *)&Buttons[button].icons[ico].w,
		      (unsigned int *)&Buttons[button].icons[ico].h,
		      &Buttons[button].icons[ico].icon,
		      (int *)&HotX,
		      (int *)&HotY) != BitmapSuccess)
    {
      Buttons[button].icons[ico].w = 0;
      Buttons[button].icons[ico].h = 0;
    }
  else
    {
      Buttons[button].icons[ico].depth = -1;
    }
  Buttons[button].icons[ico].mask = None;
  free(path);
#endif
}


/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
int GetXPMFile(int button,int ico)
{
#ifndef NO_ICONS
#ifdef XPM
  XpmAttributes xpm_attributes;
  char *path = NULL;

  path = findImageFile(Buttons[button].icons[ico].file, imagePath,R_OK);
  if(path == NULL)
    return 0;
  xpm_attributes.colormap = Pcmap;
  xpm_attributes.visual = Pvisual;
  xpm_attributes.depth = Pdepth;
  xpm_attributes.closeness = 40000;
  xpm_attributes.valuemask = XpmSize | XpmReturnPixels | XpmColormap
    | XpmVisual | XpmDepth |XpmCloseness;
  if(XpmReadFileToPixmap(dpy, main_win, path,
			 &Buttons[button].icons[ico].icon,
			 &Buttons[button].icons[ico].mask,
			 &xpm_attributes) == XpmSuccess)
  {
    Buttons[button].icons[ico].w = xpm_attributes.width;
    Buttons[button].icons[ico].h = xpm_attributes.height;
    Buttons[button].icons[ico].depth = Pdepth;
    free(path);
  }
  else
  {
    free(path);
    Buttons[button].icons[ico].icon=None;
    return 0;
  }
  if (button==BACK_BUTTON)
  {
    BUTTONWIDTH = xpm_attributes.width;
    BUTTONHEIGHT = xpm_attributes.height;
    if (ForceSize && (BUTTONWIDTH > 64 || BUTTONHEIGHT > 64)) {
      Pixmap resized;

      BUTTONWIDTH = 64;
      BUTTONHEIGHT = 64;
      resized = XCreatePixmap(dpy, main_win, 64, 64, Pdepth);
      XCopyArea(dpy, Buttons[button].icons[ico].icon,
		resized, NormalGC, 0, 0, 64, 64, 0, 0);
      XFreePixmap(dpy, Buttons[button].icons[ico].icon);
      Buttons[button].icons[ico].icon = resized;
    }
    DrawOutline(Buttons[button].icons[ico].icon,BUTTONWIDTH,BUTTONHEIGHT);
  }
  return 1;
#else
  return 0;
#endif /* XPM */
#else
  return 0;
#endif
}

/****************************************************************************
 *
 * read background icons from data
 *
 ****************************************************************************/
int GetXPMData(int button, char **data)
{
#ifndef NO_ICONS
#ifdef XPM
  XpmAttributes xpm_attributes;

  if( button != BACK_BUTTON )
	return 0;
  xpm_attributes.colormap = Pcmap;
  xpm_attributes.visual = Pvisual;
  xpm_attributes.depth = Pdepth;
  xpm_attributes.valuemask = XpmSize | XpmReturnPixels | XpmColormap
			     | XpmVisual | XpmDepth;
  if(XpmCreatePixmapFromData(dpy, main_win, data,
			     &Buttons[button].icons[0].icon,
			     &Buttons[button].icons[0].mask,
			     &xpm_attributes) == XpmSuccess)
  {
    BUTTONWIDTH = Buttons[button].icons[0].w = xpm_attributes.width;
    BUTTONHEIGHT = Buttons[button].icons[0].h = xpm_attributes.height;
    Buttons[button].icons[0].depth = Pdepth;
  }
  else
  {
    return 0;
  }
  DrawOutline(Buttons[button].icons[0].icon,BUTTONWIDTH,BUTTONHEIGHT);

  return 1;
#else
  return 0;
#endif /* XPM */
#else
  return 0;
#endif
}

/*******************************************************************
 *
 * Make a gradient pixmap
 *
 *******************************************************************/

int GetXPMGradient(int button, int from[3], int to[3], int maxcols,
		   int type)
{
    Buttons[button].icons[0].icon=XCreatePixmap(dpy,main_win,64,64, Pdepth);
    Buttons[button].icons[0].mask=None;
    Buttons[button].icons[0].w = 64;
    Buttons[button].icons[0].h = 64;
    if (button==BACK_BUTTON) {
	BUTTONWIDTH = 64;
	BUTTONHEIGHT = 64;
    }
    Buttons[button].icons[0].depth = Pdepth;
    switch (type) {
     case TEXTURE_GRADIENT:
	if (!DrawDegradeRelief(dpy, Buttons[button].icons[0].icon, 0,0,64,64,
			 	from, to, 0, maxcols)) {
	    XFreePixmap(dpy, Buttons[button].icons[0].icon);
	    return 0;
	}
	break;
     case TEXTURE_HGRADIENT:
     case TEXTURE_HCGRADIENT:
	if (!DrawHGradient(dpy, Buttons[button].icons[0].icon, 0, 0, 64,64,
			 from, to, 0, maxcols, type-TEXTURE_HGRADIENT)) {
	    XFreePixmap(dpy, Buttons[button].icons[0].icon);
	    return 0;
	}
	break;
     case TEXTURE_VGRADIENT:
     case TEXTURE_VCGRADIENT:
	if (!DrawVGradient(dpy, Buttons[button].icons[0].icon, 0, 0, 64,64,
			 from, to, 0, maxcols, type-TEXTURE_VGRADIENT)) {
	    XFreePixmap(dpy, Buttons[button].icons[0].icon);
	    return 0;
	}
	break;
     default:
	return 0;
    }
    DrawOutline(Buttons[button].icons[0].icon,64,64);

    return 1;
}

/*******************************************************************
 *
 * Make a colorset pixmap
 *
 *******************************************************************/

void GetXPMColorset(int button, int colorset)
{
  if (Buttons[button].icons[0].icon == None)
  {
    Buttons[button].icons[0].icon=XCreatePixmap(dpy,main_win,64,64, Pdepth);
    Buttons[button].icons[0].mask=None;
    Buttons[button].icons[0].w = 64;
    Buttons[button].icons[0].h = 64;
    if (button==BACK_BUTTON) {
      BUTTONWIDTH = 64;
      BUTTONHEIGHT = 64;
    }
    Buttons[button].icons[0].depth = Pdepth;
  }

  SetRectangleBackground(
    dpy, Buttons[button].icons[0].icon, 0, 0, 64, 64,
    &(Colorset[colorset]), Pdepth, NormalGC);

  DrawOutline(Buttons[button].icons[0].icon,64,64);

  return;
}

/*******************************************************************
 *
 * Make a solid color pixmap
 *
 *******************************************************************/

int GetSolidXPM(int button, Pixel pixel)
{
    GC gc;
    XGCValues gcv;

    gcv.foreground=pixel;
    gc=fvwmlib_XCreateGC(dpy,main_win,GCForeground, &gcv);
    if (button==BACK_BUTTON) {
	BUTTONWIDTH = 64;
	BUTTONHEIGHT = 64;
    }
    Buttons[button].icons[0].icon=XCreatePixmap(dpy,main_win,64,64, Pdepth);
    Buttons[button].icons[0].mask=None;
    XFillRectangle(dpy,Buttons[button].icons[0].icon,gc,0,0,64,64);
    Buttons[button].icons[0].w = 64;
    Buttons[button].icons[0].h = 64;
    Buttons[button].icons[0].depth = Pdepth;
    XFreeGC(dpy,gc);
    DrawOutline(Buttons[button].icons[0].icon,64,64);

    return 1;
}

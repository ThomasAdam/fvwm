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

#include "Tools.h"

#define BdWidth 2		/* Border width */
#define SbWidth 15		/* ScrollBar width */

/*************************************************/
/* Fonction pour Liste / Functions for the List  */
/*************************************************/
void InitList(struct XObj *xobj)
{
  unsigned long mask;
  XSetWindowAttributes Attr;
  int asc,desc,dir;
  XCharStruct struc;
  int minw,minh,resize=0;
  int NbVisCell,NbCell;
#ifdef I18N_MB
  char **ml;
  XFontStruct **fs_list;
#endif
  
  /* Enregistrement des couleurs et de la police / fonts and colors */
  if (xobj->colorset >= 0) {
    xobj->TabColor[fore] = Colorset[xobj->colorset].fg;
    xobj->TabColor[back] = Colorset[xobj->colorset].bg;
    xobj->TabColor[hili] = Colorset[xobj->colorset].hilite;
    xobj->TabColor[shad] = Colorset[xobj->colorset].shadow;
  } else {
    xobj->TabColor[fore] = GetColor(xobj->forecolor);
    xobj->TabColor[back] = GetColor(xobj->backcolor);
    xobj->TabColor[hili] = GetColor(xobj->hilicolor);
    xobj->TabColor[shad] = GetColor(xobj->shadcolor);
  }
  
  mask = 0;
  Attr.background_pixel = xobj->TabColor[back];
  mask |= CWBackPixel;
  Attr.cursor = XCreateFontCursor(dpy,XC_hand2);
  mask |= CWCursor;  /* Curseur pour la fenetre / Window cursor */
  xobj->win = XCreateWindow(dpy, *xobj->ParentWin,
			  xobj->x, xobj->y, xobj->width, xobj->height, 0,
			  CopyFromParent, InputOutput, CopyFromParent,
			  mask, &Attr);

  xobj->gc = fvwmlib_XCreateGC(dpy, xobj->win, 0, NULL);
  XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);
  XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);

#ifdef I18N_MB
  if ((xobj->xfontset = GetFontSetOrFixed(dpy, xobj->font)) == NULL) {
    fprintf(stderr, "FvwmScript: Couldn't load font. Exiting!\n");
    exit(1);
  }
  XFontsOfFontSet(xobj->xfontset, &fs_list, &ml);
  xobj->xfont = fs_list[0];
#else
  if ((xobj->xfont = GetFontOrFixed(dpy,xobj->font)) == NULL) {
    fprintf(stderr, "FvwmScript: Couldn't load font. Exiting!\n");
    exit(1);
  }
#endif
  XSetFont(dpy, xobj->gc, xobj->xfont->fid);

  /* Calcul de la taille du widget *
   * Taille minimum: une ligne ou ascenseur visible */
  /* Computation of the size of the widget *
   * min size: one line or a visible elevator */
  XTextExtents(xobj->xfont, "lp", strlen("lp"), &dir, &asc, &desc, &struc);
  minh = 8 + 3*BdWidth + 3*(asc+desc+3);
  if (xobj->height<minh)
  {
    xobj->height = minh;
    resize = 1;
  }
  minw = 12 + 3*BdWidth +SbWidth +75;
  if (xobj->width<minw)
  {
    xobj->width = minw;
    resize = 1;
  }
  if (resize)
    XResizeWindow(dpy, xobj->win, xobj->width, xobj->height);
  if (xobj->colorset >= 0)
    SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
			&Colorset[xobj->colorset], Pdepth,
			xobj->gc, True);

  /* Calcul de la premiere cellule visible */
  /* Computation of the first visible cell */
  NbVisCell = (xobj->height - 2 - 3*BdWidth) / (asc+desc+3);
  NbCell = CountOption(xobj->title);
  if (NbCell > NbVisCell)
  {
    if (xobj->value2 > (NbCell - NbVisCell + 1))
      xobj->value2 = NbCell - NbVisCell + 1;
    if ((xobj->value2 < 1) || (xobj->value2 < 0))
      xobj->value2 = 1;
  }
  else
    xobj->value2 = 1;
  XSelectInput(dpy, xobj->win, ExposureMask);
}

void DrawVSbList(struct XObj *xobj, int NbCell, int NbVisCell, int press)
{
  XRectangle r;
  int PosTh,SizeTh;

  r.y = 2+BdWidth;
  r.x = xobj->width - (6+BdWidth) -SbWidth;
  r.height = xobj->height - r.y - 2*BdWidth;
  r.width = SbWidth + 4;
  DrawReliefRect(r.x, r.y, r.width, r.height, xobj, shad, hili);
  
  /* Calcul du rectangle pour les fleches *
   * Compute the rectangle for the arrows */
  r.x = r.x + 2;
  r.y = r.y + 2;
  r.width = r.width - 4;
  r.height = r.height - 4;

  /* Dessin de la fleche haute / Draw the up arrow */
  DrawArrowN(xobj, r.x + 1, r.y + 1, press==1);
  DrawArrowS(xobj, r.x + 1, r.y + r.height - 14, press==2);

  /* Calcul du rectangle pour le pouce*/
  r.y = r.y + 13;
  r.height = r.height - 26;

  /* Effacement */
  XClearArea(dpy, xobj->win, r.x, r.y+1, r.width, r.height-2, False);

  /* Dessin du pouce */
  if (NbVisCell < NbCell)
    SizeTh = (NbVisCell * (r.height-8) / NbCell) + 8;
 else
   SizeTh = r.height;
  PosTh = (xobj->value2 - 1) * (r.height - 8) / NbCell;
  DrawReliefRect(r.x, r.y + PosTh, r.width, SizeTh, xobj, hili, shad);
}

void DrawCellule(struct XObj *xobj, int NbCell, int NbVisCell, int HeightCell,
		 int asc)
{
  XRectangle r;
  char *Title;
  int i;

  r.x = 4 + BdWidth;
  r.y = r.x;
  r.width =  xobj->width - 16 - SbWidth - BdWidth;
  r.height = xobj->height -r.y - 4 - 2*BdWidth;

  /* Dessin des cellules / Draw the cells */
  XSetClipRectangles(dpy, xobj->gc, 0, 0, &r, 1, Unsorted);
  for (i = xobj->value2; i < xobj->value2 + NbVisCell; i++)
  {
    Title = (char*)GetMenuTitle(xobj->title, i);
    if (strlen(Title)!=0)
    {
      int x = GetXTextPosition(xobj, r.width, 
			       XTextWidth(xobj->xfont,Title,strlen(Title)),
			       5 + r.x, 0, 0);
      if (xobj->value == i)
      {
	/* hili is better than shad.*/
	XSetForeground(dpy, xobj->gc, xobj->TabColor[hili]);
	XFillRectangle(dpy, xobj->win, xobj->gc, r.x+2,  
		       r.y +(i - xobj->value2)*HeightCell + 2,
		       r.width, HeightCell-2);
	DrawString(dpy, xobj, xobj->win, x,
		   (i - xobj->value2) * HeightCell + asc + 2 + r.y, Title,
		   strlen(Title), fore, back, shad, !xobj->flags[1]);
      }
      else
      {
	XClearArea(dpy, xobj->win, r.x,
		   r.y + (i-xobj->value2)*HeightCell + 2,
		   r.width, HeightCell-2, False);
	DrawString(dpy, xobj, xobj->win, x,
		   (i - xobj->value2) * HeightCell + asc + 2 + r.y, Title,
		   strlen(Title), fore, hili, back, !xobj->flags[1]);
      }
    }
    else
    {
      XClearArea(dpy, xobj->win, r.x + 2, r.y +(i - xobj->value2)*HeightCell +2,
		 r.width, HeightCell - 2, False);
    }
    if (Title != NULL)
      free(Title);
  }
  XSetClipMask(dpy, xobj->gc, None);
}

void DestroyList(struct XObj *xobj)
{
  XFreeGC(dpy, xobj->gc);
  XDestroyWindow(dpy, xobj->win);
}

void DrawList(struct XObj *xobj)
{
  int asc,desc,dir,HeightCell;
  XCharStruct struc;
  int NbVisCell,NbCell;
  XRectangle r;
  
  /* Dessin du contour / */
  DrawReliefRect(0, 0, xobj->width, xobj->height, xobj, hili, shad);

  /* Dessin du contour de la liste */
  r.x = 2 + BdWidth;
  r.y = r.x;
  r.width = xobj->width - r.x - 4 - 2*BdWidth - SbWidth;
  r.height = xobj->height - r.y - 2*BdWidth;
  DrawReliefRect(r.x, r.y, r.width, r.height, xobj, shad, hili);

  /* Calcul du nombre de cellules visibles */
  XTextExtents(xobj->xfont, "lp", strlen("lp"), &dir, &asc, &desc, &struc);
  HeightCell = asc+desc+3;
  NbVisCell = r.height/HeightCell;
  NbCell = CountOption(xobj->title);
  if (NbCell > NbVisCell)
  {
    if (xobj->value2 > (NbCell - NbVisCell + 1))
      xobj->value2 = NbCell - NbVisCell + 1;
    if ((xobj->value2 < 1) || (xobj->value2 < 0))
      xobj->value2 = 1;
  }
  else
    xobj->value2 = 1;


  /* Dessin des cellules */
  DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);

  /* Dessin de l'ascenseur vertical */
  DrawVSbList(xobj, NbCell, NbVisCell, 0);

}

void EvtMouseList(struct XObj *xobj, XButtonEvent *EvtButton)
{
  XRectangle rect,rectT;
  XPoint pt;
  XCharStruct struc;
  int x1,y1,x2,y2;
  Window Win1,Win2;
  unsigned int modif;
  int In = 1;
  static XEvent event;
  int asc,desc,dir;
  int NbVisCell,NbCell,HeightCell,NPosCell,PosMouse;
  fd_set in_fdset;
 
  pt.x = EvtButton->x-xobj->x;
  pt.y = EvtButton->y-xobj->y;

  XTextExtents(xobj->xfont, "lp", strlen("lp"), &dir, &asc, &desc, &struc);
  HeightCell = asc + desc + 3;
  NbVisCell = (xobj->height-6-BdWidth)/HeightCell;
  NbCell = CountOption(xobj->title);

  /* Clic dans une cellule */
  rect.x = 4 + BdWidth;
  rect.y = rect.x;
  rect.width = xobj->width - rect.x - 10 - 2*BdWidth - SbWidth;
  rect.height = xobj->height -rect.y - 4 - 2*BdWidth;

  if(PtInRect(pt,rect))
  {
    /* Determination de la cellule */
    pt.y = pt.y - rect.y;
    NPosCell = xobj->value2 + (pt.y/(asc+desc+3));
    if (NPosCell > CountOption(xobj->title))
      NPosCell = 0;
    else if (NPosCell >= xobj->value2 + NbVisCell)
    {
      NPosCell--;
    }
    if (NPosCell != xobj->value)
    {
      xobj->value = NPosCell;
      DrawList(xobj);
    }
    SendMsg(xobj, SingleClic);
    return ;
  }

  /* Clic fleche haute asc vertical */
  rect.y = 5 + BdWidth;
  rect.x = xobj->width - (6+BdWidth) - SbWidth+3;
  rect.height = 12;
  rect.width = 12;
  if(PtInRect(pt,rect))
  {
    DrawVSbList(xobj, NbCell, NbVisCell, 1);
    do
    {
      XQueryPointer(dpy, *xobj->ParentWin, &Win1, &Win2,
		    &x1, &y1, &x2, &y2, &modif);
      pt.y = y2- xobj->y;
      pt.x = x2- xobj->x;
      if (PtInRect(pt,rect))
      {
	if (In)
	{
	  Wait(8);
	  xobj->value2--;
	  if (xobj->value2 < 1)
	    xobj->value2 = 1;
	  else
	  {
	    DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
	    DrawVSbList(xobj, NbCell, NbVisCell, 1);
	  }
	}
	else
	{
	  In = 1;
	  DrawVSbList(xobj, NbCell, NbVisCell, 1);
	  xobj->value2--;
	  if (xobj->value2 < 1)
	    xobj->value2 = 1;
	  else
	    DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
	}
      }
      else
      {
	if (In)
	{
	  In = 0;
	  DrawVSbList(xobj, NbCell, NbVisCell, 0);
	}
      }
      FD_ZERO(&in_fdset);
      FD_SET(x_fd, &in_fdset);
      select(32, SELECT_FD_SET_CAST &in_fdset, NULL, NULL, NULL);
    }
    while (!XCheckTypedEvent(dpy, ButtonRelease, &event));
    DrawVSbList(xobj, NbCell, NbVisCell, 0);
    return;
  }

  /* Clic flache basse asc vertical */
  rect.y = xobj->height - 2*BdWidth -16;
  if(PtInRect(pt,rect))
  {
    DrawVSbList(xobj, NbCell, NbVisCell, 2);
    do
    {
      XQueryPointer(dpy, *xobj->ParentWin, &Win1, &Win2,
		    &x1, &y1, &x2, &y2, &modif);
      pt.y = y2 - xobj->y;
      pt.x = x2 - xobj->x;
      if (PtInRect(pt, rect))
      {
	if (In)
	{
	  Wait(8);
	  if (xobj->value2 <= NbCell-NbVisCell)
	  {
	    xobj->value2++;
	    DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
	    DrawVSbList(xobj, NbCell, NbVisCell, 2);
	  }
	}
	else
	{
	  In = 1;
	  DrawVSbList(xobj, NbCell, NbVisCell, 2);
	  if (xobj->value2 <= NbCell-NbVisCell)
	  {
	    xobj->value2++;
	    DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
	  }
	}
      }
      else
      {
	if (In)
	{
	  In = 0;
	  DrawVSbList(xobj, NbCell, NbVisCell, 0);
	}
      }
      FD_ZERO(&in_fdset);
      FD_SET(x_fd, &in_fdset);
      select(32, SELECT_FD_SET_CAST &in_fdset, NULL, NULL, NULL);
    }
    while (!XCheckTypedEvent(dpy, ButtonRelease, &event));
    DrawVSbList(xobj, NbCell, NbVisCell, 0);
    return;
  }

  /* clic sur la zone pouce de l'ascenseur de l'ascenseur */
  rect.y = 17 + BdWidth;
  rect.x = xobj->width - (6+BdWidth) - SbWidth + 2;
  rect.height = xobj->height - rect.y - 19 - 2*BdWidth;
  rect.width = SbWidth;
  if(PtInRect(pt,rect))
  {
    /* Clic dans le pouce */
    rectT.x = rect.x;
    rectT.y = rect.y + (xobj->value2 - 1) * (rect.height - 8) / NbCell;
    if (NbVisCell<NbCell)
      rectT.height = NbVisCell * (rect.height-8) / NbCell+8;
    else
      rectT.height = rect.height;
    rectT.width = rect.width;
    if(PtInRect(pt,rectT))
    {
      PosMouse = pt.y - rectT.y - HeightCell / 2 + 2;
      do
      {
	XQueryPointer(dpy, *xobj->ParentWin, &Win1, &Win2,
		      &x1, &y1, &x2, &y2, &modif);
	/* Calcul de l'id de la premiere cellule */
	pt.y = y2-xobj->y - PosMouse;
	NPosCell = (pt.y - rect.y)*NbCell / (rect.height);

	if (NPosCell < 1) NPosCell = 1;
	if (NbCell > NbVisCell)
	{
	  if (NPosCell > (NbCell - NbVisCell + 1))
	    NPosCell = NbCell - NbVisCell + 1;
	}
	else
	  NPosCell = 1;
	if (xobj->value2 != NPosCell)
	{
	  xobj->value2 = NPosCell;
	  DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
	  DrawVSbList(xobj, NbCell, NbVisCell, 0);
	}
	FD_ZERO(&in_fdset);
	FD_SET(x_fd, &in_fdset);
	select(32, SELECT_FD_SET_CAST &in_fdset, NULL, NULL, NULL);
      }
      while (!XCheckTypedEvent(dpy, ButtonRelease, &event));
    }
    else if (pt.y < rectT.y)
    {
      NPosCell = xobj->value2 - NbVisCell;
      if (NPosCell < 1)
	NPosCell = 1;
      if (xobj->value2 != NPosCell)
      {
	xobj->value2 = NPosCell;
	DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
	DrawVSbList(xobj, NbCell, NbVisCell, 0);
      }
    }
    else if (pt.y > rectT.y + rectT.height)
    {
      NPosCell = xobj->value2+NbVisCell;
      if (NbCell>NbVisCell)
      {
	if (NPosCell > (NbCell - NbVisCell + 1))
	  NPosCell = NbCell - NbVisCell + 1;
      }
      else
	NPosCell = 1;
      if (xobj->value2 != NPosCell)
      {
	xobj->value2 = NPosCell;
	DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
	DrawVSbList(xobj, NbCell, NbVisCell, 0);
      }
    }
  }
}

void EvtKeyList(struct XObj *xobj, XKeyEvent *EvtKey)
{ 
  KeySym ks;
  unsigned char buf[10];
  XCharStruct struc;
  int asc,desc,dir;
  int NbVisCell,NbCell,HeightCell,NPosCell;
  Window Win1,Win2;
  int x1,y1,x2,y2;
  unsigned int modif;
  XRectangle rect;
  XPoint pt;

  XTextExtents(xobj->xfont, "lp", strlen("lp"), &dir, &asc, &desc, &struc);
  HeightCell = asc + desc + 3;
  NbVisCell = (xobj->height - 6 - BdWidth) / HeightCell;
  NbCell = CountOption(xobj->title);
  XQueryPointer(dpy, xobj->win, &Win1, &Win2, &x1, &y1, &x2, &y2, &modif);
  pt.x = x2;
  pt.y = y2;
  rect.x = 4 + BdWidth;
  rect.y = rect.x;
  rect.width = xobj->width - rect.x -10 - 2*BdWidth - SbWidth;
  rect.height= xobj->height - rect.y - 4 - 2*BdWidth;

  /* Recherche du charactere */
  XLookupString(EvtKey, (char *)buf, sizeof(buf), &ks, NULL);

  if (ks == XK_Return)
  {
    if(PtInRect(pt,rect))
    {
      /* Determination de la cellule */
      pt.y =  pt.y - rect.y;
      NPosCell = xobj->value2 + (pt.y/(asc+desc+3));
      if (NPosCell > CountOption(xobj->title))
	NPosCell = 0;
      else if (NPosCell >= xobj->value2 + NbVisCell)
      {
	NPosCell--;
      }
      if (NPosCell != xobj->value)
      {
	xobj->value = NPosCell;
	DrawList(xobj);
      }
      SendMsg(xobj, SingleClic);
    }
  }
  else if (ks == XK_Up)
  {
    if (xobj->value2 == 1)
    {
      if(PtInRect(pt,rect))
      {
	/* Determination de la cellule */
	pt.y= pt.y - rect.y;
	NPosCell = xobj->value2+(pt.y/(asc+desc+3));
	if (NPosCell > CountOption(xobj->title))
	  NPosCell = 0;
	if (NPosCell > 1)
	{
	  XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, -HeightCell);
	}
      }
    }
    else
    {
      xobj->value2--;
      DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
      DrawVSbList(xobj, NbCell, NbVisCell, 0);
    }
  }
  else if (ks == XK_Down)
  {
    if (xobj->value2 > NbCell-NbVisCell)
    {
      if(PtInRect(pt,rect))
      {
	/* Determination de la cellule */
	pt.y =  pt.y - rect.y;
	NPosCell =  xobj->value2 + (pt.y/(asc+desc+3));
	if (NPosCell > CountOption(xobj->title))
	  NPosCell = 0;
	if (NPosCell < NbCell && NPosCell > 0)
	{
	  XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, HeightCell);
	}
      }
    }
    else
    {
      xobj->value2++;
      DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
      DrawVSbList(xobj, NbCell, NbVisCell, 0);
    }
  }
  else if (ks == XK_Prior && xobj->value2 > 1)
  {
    xobj->value2 = xobj->value2 - NbVisCell;
    if (xobj->value2 < 1)
      xobj->value2 = 1;
    DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
    DrawVSbList(xobj, NbCell, NbVisCell, 0);
  }
  else if (ks == XK_Next && xobj->value2 <= NbCell-NbVisCell)
  {
    xobj->value2 = xobj->value2 + NbVisCell;
    if (xobj->value2 > NbCell - NbVisCell + 1)
      xobj->value2 = NbCell - NbVisCell + 1;
    DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
    DrawVSbList(xobj, NbCell, NbVisCell, 0);
  }
  else if (ks == XK_Home)
  {
    if (xobj->value2 > 1)
    {
      xobj->value2 = 1;
      DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
      DrawVSbList(xobj, NbCell, NbVisCell, 0);
    }
  }
  else if (ks == XK_End)
  {
    if (xobj->value2 < NbCell - NbVisCell + 1)
    {
      xobj->value2 = NbCell - NbVisCell + 1;
      DrawCellule(xobj, NbCell, NbVisCell, HeightCell, asc);
      DrawVSbList(xobj, NbCell, NbVisCell, 0);
    }
  }
}


void ProcessMsgList(struct XObj *xobj, unsigned long type, unsigned long *body)
{
}

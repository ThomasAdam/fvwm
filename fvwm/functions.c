/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * fvwm built-in functions
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

extern XEvent Event;
extern FvwmWindow *Tmp_win;
extern int menuFromFrameOrWindowOrTitlebar;
Bool desperate;

void setImagePath(F_CMD_ARGS);

/*
 * be sure to keep this list properly ordered for bsearch routine!
 *
 * Note: the 3rd column of this table is no longer in use!
 * Remove the field "code" from the structure "functions" in misc.c.
 * Then remove the values from this table.
 * dje 9/25/98
 *
 * Note: above comment is incorrect. The 3rd column is still used in
 * find_func_type which is called from menus.c. Some work has to be done
 * before it can be removed.
 * Dominik Vogt 21/11/98
 *
 * My   mistake. However,  they  are  no  longer  used for their original
 * purpose, and  the   use menus.c is  putting   them to  seems  somewhat
 * unnecessary.  I'm  going to use  F_NOP for EdgeThickness,  and see
 * how that goes.
 * dje 12/19/98.
 */
static struct functions func_config[] =
{
  {"+",            add_another_item, F_ADDMENU2,            FUNC_NO_WINDOW},
#ifdef MULTISTYLE
  {"AddButtonStyle",AddButtonStyle,  F_ADD_BUTTON_STYLE,    FUNC_NO_WINDOW},
#endif /* MULTISTYLE */
  {"AddModuleConfig",  AddModConfig, F_ADD_MOD,             FUNC_NO_WINDOW},
#ifdef MULTISTYLE
#ifdef EXTENDED_TITLESTYLE
  {"AddTitleStyle",AddTitleStyle,    F_ADD_TITLE_STYLE,     FUNC_NO_WINDOW},
#endif /* EXTENDED_TITLESTYLE */
#endif /* MULTISTYLE */
#ifdef USEDECOR
  {"AddToDecor",   add_item_to_decor,F_ADD_DECOR,	    FUNC_NO_WINDOW},
#endif /* USEDECOR */
  {"AddToFunc",    add_item_to_func, F_ADDFUNC,             FUNC_NO_WINDOW},
  {"AddToMenu",    add_item_to_menu, F_ADDMENU,             FUNC_NO_WINDOW},
  {"AnimatedMove", animated_move_window,F_ANIMATED_MOVE,    FUNC_NEEDS_WINDOW},
  {"Beep",         Bell,             F_BEEP,                FUNC_NO_WINDOW},
#ifdef BORDERSTYLE
  {"BorderStyle",  SetBorderStyle,   F_BORDERSTYLE,         FUNC_NO_WINDOW},
#endif /* BORDERSTYLE */
  {"ButtonStyle",  ButtonStyle,      F_BUTTON_STYLE,        FUNC_NO_WINDOW},
#ifdef USEDECOR
  {"ChangeDecor",  ChangeDecor,      F_CHANGE_DECOR,        FUNC_NEEDS_WINDOW},
#endif /* USEDECOR */
  {"ChangeMenuStyle", ChangeMenuStyle, F_CHANGE_MENUSTYLE,  FUNC_NO_WINDOW},
  {"ClickTime",    SetClick,         F_CLICK,               FUNC_NO_WINDOW},
  {"Close",        close_function,   F_CLOSE,               FUNC_NEEDS_WINDOW},
  {"ColorLimit",   SetColorLimit,    F_COLOR_LIMIT,         FUNC_NO_WINDOW},
  {"ColormapFocus",SetColormapFocus, F_COLORMAP_FOCUS,      FUNC_NO_WINDOW},
  {"Current",      CurrentFunc,      F_CURRENT,             FUNC_NO_WINDOW},
  {"CursorMove",   movecursor,       F_MOVECURSOR,          FUNC_NO_WINDOW},
  {"CursorStyle",  CursorStyle,      F_CURSOR_STYLE,        FUNC_NO_WINDOW},
  {"DefaultColors",SetDefaultColors, F_DFLT_COLORS,         FUNC_NO_WINDOW},
  {"DefaultFont",  LoadDefaultFont,  F_DFLT_FONT,           FUNC_NO_WINDOW},
  {"Delete",       delete_function,  F_DELETE,              FUNC_NEEDS_WINDOW},
  {"Desk",         changeDesks_func, F_DESK,                FUNC_NO_WINDOW},
  {"DesktopSize",  SetDeskSize,      F_SETDESK,             FUNC_NO_WINDOW},
  {"Destroy",      destroy_function, F_DESTROY,             FUNC_NEEDS_WINDOW},
#ifdef USEDECOR
  {"DestroyDecor", DestroyDecor,     F_DESTROY_DECOR,	    FUNC_NO_WINDOW},
#endif /* USEDECOR */
  {"DestroyFunc",  destroy_menu,     F_DESTROY_MENU,        FUNC_NO_WINDOW},
  {"DestroyMenu",  destroy_menu,     F_DESTROY_MENU,        FUNC_NO_WINDOW},
  {"DestroyMenuStyle", DestroyMenuStyle, F_DESTROY_MENUSTYLE,FUNC_NO_WINDOW},
  {"DestroyModuleConfig", DestroyModConfig, F_DESTROY_MOD,  FUNC_NO_WINDOW},
  {"Direction",    DirectionFunc,    F_DIRECTION,           FUNC_NO_WINDOW},
  {"Echo",         echo_func,        F_ECHO,                FUNC_NO_WINDOW},
  {"EdgeResistance",SetEdgeResistance,F_EDGE_RES,           FUNC_NO_WINDOW},
  {"EdgeScroll",   SetEdgeScroll,    F_EDGE_SCROLL,         FUNC_NO_WINDOW},
  {"EdgeThickness",setEdgeThickness, F_NOP,                 FUNC_NO_WINDOW},
  {"Emulate",      Emulate,          F_EMULATE,             FUNC_NO_WINDOW},
  {"Exec",         exec_function,    F_EXEC,                FUNC_NO_WINDOW},
  {"ExecUseSHELL", exec_setup,       F_EXEC_SETUP,          FUNC_NO_WINDOW},
  {"FlipFocus",    flip_focus_func,  F_FLIP_FOCUS,          FUNC_NEEDS_WINDOW},
  {"Focus",        focus_func,       F_FOCUS,               FUNC_NEEDS_WINDOW},
  {"Function",     ComplexFunction,  F_FUNCTION,            FUNC_NO_WINDOW},
  {"GlobalOpts",   SetGlobalOptions, F_GLOBAL_OPTS,         FUNC_NO_WINDOW},
  {"GotoPage",     goto_page_func,   F_GOTO_PAGE,           FUNC_NO_WINDOW},
  {"HilightColor", SetHiColor,       F_HICOLOR,             FUNC_NO_WINDOW},
  {"IconFont",     LoadIconFont,     F_ICONFONT,            FUNC_NO_WINDOW},
  {"Iconify",      iconify_function, F_ICONIFY,             FUNC_NEEDS_WINDOW},
  {"IconPath",     setIconPath,      F_ICON_PATH,           FUNC_NO_WINDOW},
  {"ImagePath",    setImagePath,     F_ICON_PATH,           FUNC_NO_WINDOW},
  {"Key",          ParseKeyEntry,    F_KEY,                 FUNC_NO_WINDOW},
  {"KillModule",   module_zapper,    F_ZAP,                 FUNC_NO_WINDOW},
  {"Lower",        lower_function,   F_LOWER,               FUNC_NEEDS_WINDOW},
  {"Maximize",     Maximize,         F_MAXIMIZE,            FUNC_NEEDS_WINDOW},
  {"Menu",         staysup_func,     F_STAYSUP,             FUNC_NO_WINDOW},
  {"MenuStyle",    SetMenuStyle,     F_MENUSTYLE,           FUNC_NO_WINDOW},
  {"Module",       executeModule,    F_MODULE,              FUNC_NO_WINDOW},
  {"ModulePath",   setModulePath,    F_MODULE_PATH,         FUNC_NO_WINDOW},
  {"Mouse",        ParseMouseEntry,  F_MOUSE,               FUNC_NO_WINDOW},
  {"Move",         move_window,      F_MOVE,                FUNC_NEEDS_WINDOW},
  {"MoveToDesk",   changeWindowsDesk,F_CHANGE_WINDOWS_DESK, FUNC_NEEDS_WINDOW},
  {"MoveToPage",   move_window_to_page,F_MOVE_TO_PAGE,      FUNC_NEEDS_WINDOW},
  {"Next",         NextFunc,         F_NEXT,                FUNC_NO_WINDOW},
  {"None",         NoneFunc,         F_NONE,                FUNC_NO_WINDOW},
  {"Nop",          Nop_func,         F_NOP,                 FUNC_NO_WINDOW},
  {"OpaqueMoveSize", SetOpaque,      F_OPAQUE,              FUNC_NO_WINDOW},
  {"PipeRead",     PipeRead,         F_READ,                FUNC_NO_WINDOW},
  {"PixmapPath",   setPixmapPath,    F_PIXMAP_PATH,         FUNC_NO_WINDOW},
  {"PopUp",        popup_func,       F_POPUP,               FUNC_NO_WINDOW},
  {"Prev",         PrevFunc,         F_PREV,                FUNC_NO_WINDOW},
  {"Quit",         quit_func,        F_QUIT,                FUNC_NO_WINDOW},
  {"QuitScreen",   quit_screen_func, F_QUIT_SCREEN,         FUNC_NO_WINDOW},
  {"Raise",        raise_function,   F_RAISE,               FUNC_NEEDS_WINDOW},
  {"RaiseLower",   raiselower_func,  F_RAISELOWER,          FUNC_NEEDS_WINDOW},
  {"Read",         ReadFile,         F_READ,                FUNC_NO_WINDOW},
  {"Recapture",    Recapture,        F_RECAPTURE,           FUNC_NO_WINDOW},
  {"Refresh",      refresh_function, F_REFRESH,             FUNC_NO_WINDOW},
  {"RefreshWindow",refresh_win_function, F_REFRESH,         FUNC_NEEDS_WINDOW},
  {"Resize",       resize_window,    F_RESIZE,              FUNC_NEEDS_WINDOW},
  {"Restart",      restart_function, F_RESTART,             FUNC_NO_WINDOW},
  {"Scroll",       scroll,           F_SCROLL,              FUNC_NO_WINDOW},
  {"Send_ConfigInfo",SendDataToModule, F_CONFIG_LIST,       FUNC_NO_WINDOW},
  {"Send_WindowList",send_list_func, F_SEND_WINDOW_LIST,    FUNC_NO_WINDOW},
  {"SendToModule", SendStrToModule,  F_SEND_STRING,         FUNC_NO_WINDOW},
  {"set_mask",     set_mask_function,F_SET_MASK,            FUNC_NO_WINDOW},
  {"SetAnimation", set_animation,    F_SET_ANIMATION,	    FUNC_NO_WINDOW},
  {"SetEnv",       SetEnv,           F_SETENV,              FUNC_NO_WINDOW},
  {"SnapAttraction",SetSnapAttraction,F_SNAP_ATT,           FUNC_NO_WINDOW},
  {"SnapGrid",     SetSnapGrid,      F_SNAP_GRID,           FUNC_NO_WINDOW},
  {"Stick",        stick_function,   F_STICK,               FUNC_NEEDS_WINDOW},
  {"Style",        ProcessNewStyle,  F_STYLE,               FUNC_NO_WINDOW},
  {"Title",        Nop_func,         F_TITLE,               FUNC_NO_WINDOW},
  {"TitleStyle",   SetTitleStyle,    F_TITLESTYLE,          FUNC_NO_WINDOW},
  {"UpdateDecor",  UpdateDecor,      F_UPDATE_DECOR,        FUNC_NO_WINDOW},
  {"Wait",         wait_func,        F_WAIT,                FUNC_NO_WINDOW},
  {"WarpToWindow", warp_func,        F_WARP,                FUNC_NEEDS_WINDOW},
  {"WindowFont",   LoadWindowFont,   F_WINDOWFONT,          FUNC_NO_WINDOW},
  {"WindowId",     WindowIdFunc,     F_WINDOWID,            FUNC_NO_WINDOW},
  {"WindowList",   do_windowList,    F_WINDOWLIST,          FUNC_NO_WINDOW},
/*
  {"WindowsDesk",  changeWindowsDesk,F_CHANGE_WINDOWS_DESK, FUNC_NEEDS_WINDOW},
*/
#ifdef WINDOWSHADE
  {"WindowShade",  WindowShade,      F_WINDOW_SHADE,        FUNC_NEEDS_WINDOW},
#endif /* WINDOWSHADE */
  {"XORValue",     SetXOR,           F_XOR,                 FUNC_NO_WINDOW},
  {"",0,0,0}
};


/* migo (02-Oct-1999): add ImagePath not to break 2.3.x configurations */
/* setPath() is from fvwm-2.3.x/libs/System.c */

/**
 * Set a colon-separated path, with environment variable expansions,
 * and expand '+' to be the value of the previous path.
 **/
void setPath( char** p_path, char* newpath, int free_old_path )
{
    char* oldpath = *p_path;
    int oldlen = strlen( oldpath );
    char* stripped_path = stripcpy( newpath );
    int found_plus = strchr( newpath, '+' ) != NULL;

    /** Leave room for the old path, if we find a '+' in newpath **/
    *p_path = envDupExpand( stripped_path, found_plus ? oldlen : 0 );
    free( stripped_path );

    if ( found_plus ) {
        char* p = strchr( *p_path, '+' );
        memmove( p+oldlen, p+1, strlen(p+1) );

        /* copy oldlen+1 bytes to include the trailing NUL */
        strncpy( p, oldpath, oldlen+1 );
    }

    if ( free_old_path )
        free( oldpath );
}
void setImagePath(F_CMD_ARGS)
{
    extern char *PixmapPath, *IconPath;
    char *newPixmapPath = strdup(PixmapPath);
    char *newIconPath   = strdup(IconPath  );
    setPath( &newPixmapPath, action, 1 );
    setPath( &newIconPath,   action, 1 );
    setPixmapPath(eventp, w, tmp_win, context, newPixmapPath, Module);
    setIconPath  (eventp, w, tmp_win, context, newIconPath,   Module);
}


/*
** do binary search on func list
*/
static int func_comp(const void *a, const void *b)
{
  char *key=(char *)a;
  char *f=((struct functions *)b)->keyword;
  return (strcasecmp(key,f));
}

static struct functions *FindBuiltinFunction(char *func)
{
  static int func_config_size=0;

  if (!func)
    return NULL;

  if (!func_config_size)
  {
    /* remove finial NULL entry from size */
    func_config_size=((sizeof(func_config))/(sizeof(struct functions)))-1;
  }

  /* since a lot of lines in a typical rc are probably menu/func continues: */
  if (func[0]=='+')
    return &(func_config[0]);

  return (struct functions *)bsearch(func, func_config, func_config_size,
                                     sizeof(struct functions), func_comp);
}


/***********************************************************************
 *
 *  Procedure:
 *	ExecuteFunction - execute a fvwm built in function
 *
 *  Inputs:
 *	Action	- the menu action to execute
 *	tmp_win	- the fvwm window structure
 *	eventp	- pointer to the event that caused the function
 *	context - the context in which the button was pressed
 *
 ***********************************************************************/
void ExecuteFunction(char *Action, FvwmWindow *tmp_win, XEvent *eventp,
		     unsigned long context, int Module)
{
  Window w;
  int matched,j;
  char *function;
  char *action, *taction;
  char *arguments[10];
  struct functions *bif;

  if (!Action || Action[0] == 0 || Action[1] == 0)
  {
    /* impossibly short command */
    return;                             /* done */
  }
  if (Action[0] == '#') {               /* a comment */
    return;                             /* done */
  }
  /* Note: the module config command, "*" can not be handled by the
     regular command table because there is no required white space after
     the asterisk. */
  if (Action[0] == '*') {               /* a module config command */
    ModuleConfig(NULL,0,0,0,Action,0);  /* process the command */
    return;                             /* done */
  }

  for(j=0;j<10;j++)
    arguments[j] = NULL;

  if(tmp_win == NULL)
    w = Scr.Root;
  else
    w = tmp_win->w;

  if((tmp_win) &&(eventp))
    w = eventp->xany.window;
  if((tmp_win)&&(eventp->xbutton.subwindow != None)&&
     (eventp->xany.window != tmp_win->w))
    w = eventp->xbutton.subwindow;

  taction = expand(Action,arguments,tmp_win);
  action = GetNextToken(taction,&function);
  if (!function)
    return;
  j=0;
  matched = FALSE;

  bif = FindBuiltinFunction(function);
  if (bif)
  {
    matched = TRUE;
    bif->action(eventp,w,tmp_win,context,action,&Module);
  }

  if(!matched)
    {
      desperate = 1;
      ComplexFunction(eventp,w,tmp_win,context,taction, &Module);
      if(desperate)
	executeModule(eventp,w,tmp_win,context,taction, &Module);
      desperate = 0;
    }

  /* Only wait for an all-buttons-up condition after calls from
   * regular built-ins, not from complex-functions or modules. */
  if(Module == -1)
    WaitForButtonsUp();

  if (function)
    free(function);
  if(taction != NULL)
    free(taction);
  return;
}


void find_func_type(char *action, short *func_type, Bool *func_needs_window)
{
  int j, len = 0;
  char *endtok = action;
  Bool matched;
  int mlen;

  if (action)
  {
    while (*endtok&&!isspace(*endtok))++endtok;
    len = endtok - action;
    j=0;
    matched = FALSE;
    while((!matched)&&((mlen = strlen(func_config[j].keyword))>0))
      {
	if((mlen == len) &&
	   (strncasecmp(action,func_config[j].keyword,mlen)==0))
	  {
	    matched=TRUE;
	    /* found key word */
	    *func_type = func_config[j].func_type;
	    *func_needs_window = func_config[j].func_needs_window;
	    return;
	  }
	else
	  j++;
      }
    /* No clue what the function is. Just return "BEEP" */
  }
  *func_type = F_BEEP;
  *func_needs_window = False;
  return;
}

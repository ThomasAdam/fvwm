#include "FvwmIconMan.h"
#include "x.h"
#include "xmanager.h"

#ifdef COMPILE_STANDALONE
#include "fvwm.h"
#include "module.h"
#else
#include "../../fvwm/fvwm.h"
#include "../../fvwm/module.h"
#endif

static char const rcsid[] =
  "$Id: fvwm.c,v 1.1 1998/10/14 00:03:23 tibbs Exp $";

typedef struct {
  Ulong paging_enabled;
} m_toggle_paging_data;

typedef struct {
  Ulong desknum;
} m_new_desk_data;

typedef struct {
  Ulong app_id;
  Ulong frame_id;
  Ulong dbase_entry;
  Ulong xpos;
  Ulong ypos;
  Ulong width;
  Ulong height;
  Ulong desknum;
  Ulong windows_flags;
  Ulong window_title_height;
  Ulong window_border_width;
  Ulong window_base_width;
  Ulong window_base_height;
  Ulong window_resize_width_inc;
  Ulong window_resize_height_inc;
  Ulong window_min_width;
  Ulong window_min_height;
  Ulong window_max_width_inc;
  Ulong window_max_height_inc;
  Ulong icon_label_id;
  Ulong icon_pixmap_id;
  Ulong window_gravity;
} m_add_config_data;
  
typedef struct {
  Ulong x, y, desknum;
} m_new_page_data;

typedef struct {
  Ulong app_id, frame_id, dbase_entry;
} m_minimal_data;

typedef struct {
  Ulong app_id, frame_id, dbase_entry;
  Ulong xpos, ypos, icon_width, icon_height;
} m_icon_data;

typedef struct {
  Ulong app_id, frame_id, dbase_entry;
  union {
    Ulong name_long[1];
    Uchar name[4];
  } name;
} m_name_data;

#ifdef MINI_ICONS

typedef struct {
  Ulong app_id, frame_id, dbase_entry;
  Ulong width, height, depth, picture, mask;
  union {
    Ulong name_long[1];
    Uchar name[4];
  } name;
} m_mini_icon_data;

#endif

typedef struct {
  Ulong start, type, len, time /* in fvwm 2 only */;
} FvwmPacketHeader;

typedef union {
  m_toggle_paging_data toggle_paging_data;
  m_new_desk_data      new_desk_data;
  m_add_config_data    add_config_data;
  m_new_page_data      new_page_data;
  m_minimal_data       minimal_data;
  m_icon_data          icon_data;
  m_name_data          name_data;
#ifdef MINI_ICONS
  m_mini_icon_data     mini_icon_data;
#endif
} FvwmPacketBody;

/* only used by count_nonsticky_in_hashtab */
static WinManager *the_manager;

static int count_nonsticky_in_hashtab (void *arg)
{
  WinData *win = (WinData *)arg;
  WinManager *man = the_manager;

  if (!(win->fvwm_flags & STICKY) && win->complete && win->manager == man)
    return 1;
  return 0;
}

static void set_draw_mode (WinManager *man, int flag)
{
  int num;

  if (!man)
    return;

  if (man->we_are_drawing == 0 && flag) {
    draw_manager (man);
  }
  else if (man->we_are_drawing && !flag) {
    the_manager = man;
    num = accumulate_walk_hashtab (count_nonsticky_in_hashtab);
    ConsoleDebug (FVWM, "SetDrawMode on 0x%x, num = %d\n", man, num);

    if (num == 0)
      return;
    man->configures_expected = num;
  }
  man->we_are_drawing = flag;
}

static int drawing (WinManager *man)
{
  if (!man)
    return 1;

  return man->we_are_drawing;
}

static void got_configure (WinManager *man)
{
  if (man && !man->we_are_drawing) {
    man->configures_expected--;
    ConsoleDebug (FVWM, "got_configure on 0x%x, num_expected now = %d\n",
		  man, man->configures_expected);
    if (man->configures_expected <= 0) 
      set_draw_mode (man, 1);
  }
}

int win_in_viewport (WinData *win)
{
  WinManager *manager = win->manager;
  int flag = 0;
  
  assert (manager);

  switch (manager->res) {
  case SHOW_GLOBAL:
    flag = 1;
    break;

  case SHOW_DESKTOP:
    if ((win->fvwm_flags & STICKY) || win->desknum == globals.desknum)
      flag = 1;
    break;

  case SHOW_PAGE:
    if (win->fvwm_flags & STICKY) {
      flag = 1;
    } else if (win->desknum == globals.desknum) {
      /* win and screen intersect if they are not disjoint in x and y */
      flag = RECTANGLES_INTERSECT (win->x, win->y, win->width, win->height,
				   0, 0, globals.screenx, globals.screeny);
    }
    break;
  }
  return flag;
}


static WinData *id_to_win (Ulong id)
{
  WinData *win;
  win = find_win_hashtab (id);
  if (win == NULL) {
    win = new_windata ();
    win->app_id = id;
    win->app_id_set = 1;
    insert_win_hashtab (win);
  }
  return win;
}

static void set_win_configuration (WinData *win, FvwmPacketBody *body)
{
  win->desknum = body->add_config_data.desknum;
  win->x = body->add_config_data.xpos;
  win->y = body->add_config_data.ypos;
  win->width = body->add_config_data.width;
  win->height = body->add_config_data.height;
  win->geometry_set = 1;
  win->fvwm_flags = body->add_config_data.windows_flags;
}  

static void configure_window (FvwmPacketBody *body)
{
  Ulong app_id = body->add_config_data.app_id;
  WinData *win;
  ConsoleDebug (FVWM, "configure_window: %d\n", app_id);

  win = id_to_win (app_id);

  set_win_configuration (win, body);

  check_win_complete (win);
  check_in_window (win);
  got_configure (win->manager);
}

static void focus_change (FvwmPacketBody *body)
{ 
  Ulong app_id = body->minimal_data.app_id;
  WinData *win = id_to_win (app_id);

  ConsoleDebug (FVWM, "Focus Change\n");
  ConsoleDebug (FVWM, "\tID: %d\n", app_id);

  if (globals.focus_win) {
    del_win_state (globals.focus_win, FOCUS_CONTEXT);
    if (globals.focus_win->manager)
      globals.focus_win->manager->focus_button = NULL;
    globals.focus_win = NULL;
  }


  if (win->complete  &&
      win->button  &&
      win->manager &&
      win->manager->window_up  &&
      win->manager->followFocus) {
    win->manager->focus_button = win->button;
    add_win_state (win, FOCUS_CONTEXT);
  }

  globals.focus_win = win;
  ConsoleDebug (FVWM, "leaving focus_change\n");
}

static void res_name (FvwmPacketBody *body)
{
  Ulong app_id = body->name_data.app_id;
  Uchar *name = body->name_data.name.name;
  WinData *win;

  ConsoleDebug (FVWM, "In res_name\n");

  win = id_to_win (app_id);

  copy_string (&win->resname, (char *)name);
  change_windows_manager (win);
  
  ConsoleDebug (FVWM, "Exiting res_name\n");
}

static void class_name (FvwmPacketBody *body)
{
  Ulong app_id = body->name_data.app_id;
  Uchar *name = body->name_data.name.name;
  WinData *win;

  ConsoleDebug (FVWM, "In class_name\n");

  win = id_to_win (app_id);

  copy_string (&win->classname, (char *)name);
  change_windows_manager (win);
  
  ConsoleDebug (FVWM, "Exiting class_name\n");
}

static void icon_name (FvwmPacketBody *body)
{
  WinData *win;
  Ulong app_id;
  Uchar *name = body->name_data.name.name;

  ConsoleDebug (FVWM, "In icon_name\n");

  app_id = body->name_data.app_id;

  win = id_to_win (app_id);

  if (win->iconname && !strcmp (win->iconname, name)) {
    ConsoleDebug (FVWM, "No icon change: %s %s\n", win->iconname, name);
    return;
  }

  copy_string (&win->iconname, (char *)name);
  ConsoleDebug (FVWM, "new icon name: %s\n", win->iconname);
  if (change_windows_manager (win) == 0 && win->button &&
      (win->manager->format_depend & ICON_NAME)) {
    if (win->manager->sort) {
      resort_windows_button (win);
    }
  }

  ConsoleDebug (FVWM, "Exiting icon_name\n");
}

static void window_name (FvwmPacketBody *body)
{
  WinData *win;
  Ulong app_id;
  Uchar *name = body->name_data.name.name;

  ConsoleDebug (FVWM, "In window_name\n");

  app_id = body->name_data.app_id;

  win = id_to_win (app_id);

  /* This is necessary because bash seems to update the window title on
     every keystroke regardless of whether anything changes */
  if (win->titlename && !strcmp (win->titlename, name)) {
    ConsoleDebug (FVWM, "No name change: %s %s\n", win->titlename, name);
    return;
  }

  copy_string (&win->titlename, (char *)name);
  if (change_windows_manager (win) == 0 && win->button &&
      (win->manager->format_depend & TITLE_NAME)) {
    if (win->manager->sort) {
      resort_windows_button (win);
    }
  }
  ConsoleDebug (FVWM, "Exiting window_name\n");
}

static void new_window (FvwmPacketBody *body)
{
  WinData *win;

  win = new_windata();
  if (!(body->add_config_data.windows_flags & TRANSIENT)) {
    win->app_id = body->add_config_data.app_id;
    win->app_id_set = 1;
    set_win_configuration (win, body);

    insert_win_hashtab (win);
    check_win_complete (win);
    check_in_window (win);
  }
}

static void destroy_window (FvwmPacketBody *body)
{
  WinData *win;
  Ulong app_id;

  app_id = body->minimal_data.app_id;
  win = id_to_win (app_id);
  delete_win_hashtab (win);
  if (win->button) {
    ConsoleDebug (FVWM, "destroy_window: deleting windows_button\n");
    delete_windows_button (win);
  }
  free_windata (win);
}

#ifdef MINI_ICONS
static void mini_icon (FvwmPacketBody *body)
{
  Ulong app_id = body->mini_icon_data.app_id;
  WinData *win;
  
  win = id_to_win (app_id);
  set_win_picture (win, body->mini_icon_data.picture, 
		   body->mini_icon_data.mask, body->mini_icon_data.depth, 
		   body->mini_icon_data.width, body->mini_icon_data.height);
		   

  ConsoleDebug (FVWM, "mini_icon: 0x%x 0x%x %dx%dx%d\n", win->pic.picture,
		win->pic.mask, win->pic.width, win->pic.height,
		win->pic.depth);
}
#endif

static void iconify (FvwmPacketBody *body, int dir)
{
  Ulong app_id = body->minimal_data.app_id;
  WinData *win;
  
  win = id_to_win (app_id);

  set_win_iconified (win, dir);
  
  check_win_complete (win);
  check_in_window (win);
}

/* only used by new_desk */

static void update_win_in_hashtab (void *arg)
{
  WinData *p = (WinData *)arg;
  check_in_window (p);
}

static void new_desk (FvwmPacketBody *body)
{
  globals.desknum = body->new_desk_data.desknum;
  walk_hashtab (update_win_in_hashtab);

  draw_managers ();
}

static void sendtomodule (FvwmPacketBody *body)
{
  extern void execute_function (char *);
  Uchar *string = body->name_data.name.name;

  ConsoleDebug (FVWM, "Got string: %s\n", string);

  execute_function (string);
}

static void ProcessMessage (Ulong type, FvwmPacketBody *body)
{
  int i;

  ConsoleDebug (FVWM, "FVWM Message type: %d\n", type); 

  switch(type) {
  case M_CONFIGURE_WINDOW:
    ConsoleDebug (FVWM, "DEBUG::M_CONFIGURE_WINDOW\n");
    configure_window (body);
    break;

  case M_FOCUS_CHANGE:
    ConsoleDebug (FVWM, "DEBUG::M_FOCUS_CHANGE\n");
    focus_change (body);
    break;

  case M_RES_NAME:
    ConsoleDebug (FVWM, "DEBUG::M_RES_NAME\n");
    res_name (body);
    break;

  case M_RES_CLASS:
    ConsoleDebug (FVWM, "DEBUG::M_RES_CLASS\n");
    class_name (body);
    break;

  case M_MAP:
    ConsoleDebug (FVWM, "DEBUG::M_MAP\n");
    break;

  case M_ADD_WINDOW:
    ConsoleDebug (FVWM, "DEBUG::M_ADD_WINDOW\n");
    new_window (body);
    break;

  case M_DESTROY_WINDOW:
    ConsoleDebug (FVWM, "DEBUG::M_DESTROY_WINDOW\n");
    destroy_window (body);
    break;

#ifdef MINI_ICONS
  case M_MINI_ICON:
    ConsoleDebug (FVWM, "DEBUG::M_MINI_ICON\n");
    mini_icon (body);
    break;
#endif

  case M_WINDOW_NAME:
    ConsoleDebug (FVWM, "DEBUG::M_WINDOW_NAME\n");
    window_name (body);
    break;

  case M_ICON_NAME:
    ConsoleDebug (FVWM, "DEBUG::M_ICON_NAME\n");
    icon_name (body);
    break;

  case M_DEICONIFY:
    ConsoleDebug (FVWM, "DEBUG::M_DEICONIFY\n");
    iconify (body, 0);
    break;

  case M_ICONIFY:
    ConsoleDebug (FVWM, "DEBUG::M_ICONIFY\n");
    iconify (body, 1);
    break;

  case M_END_WINDOWLIST:
    ConsoleDebug (FVWM, "DEBUG::M_END_WINDOWLIST\n");
    ConsoleDebug (FVWM, 
		  ">>>>>>>>>>>>>>>>>>>>>>>End window list<<<<<<<<<<<<<<<\n");
    if (globals.focus_win && globals.focus_win->button) {
	globals.focus_win->manager->focus_button = globals.focus_win->button;
    }
    globals.got_window_list = 1;
    for (i = 0; i < globals.num_managers; i++) {
      create_manager_window (i);
    }
    break;

  case M_NEW_DESK:
    ConsoleDebug (FVWM, "DEBUG::M_NEW_DESK\n");
    new_desk (body);
    break;

  case M_NEW_PAGE:
    ConsoleDebug (FVWM, "DEBUG::M_NEW_PAGE\n");
    if (globals.x == body->new_page_data.x &&
	globals.y == body->new_page_data.y &&
	globals.desknum == body->new_page_data.desknum) {
      ConsoleDebug (FVWM, "Useless NEW_PAGE received\n");
      break;
    }
    globals.x = body->new_page_data.x;
    globals.y = body->new_page_data.y;
    globals.desknum = body->new_page_data.desknum;
    for (i = 0; i < globals.num_managers; i++) {
      set_draw_mode (&globals.managers[i], 0);
    }
    break;

  case M_STRING:
    ConsoleDebug (FVWM, "DEBUG::M_STRING\n");
    sendtomodule (body);
    break;

  default:
    break;
  }
  
  check_managers_consistency();

  for (i = 0; i < globals.num_managers; i++) {
    if (drawing (&globals.managers[i])) 
      draw_manager (&globals.managers[i]);
  }

  check_managers_consistency();
}

void ReadFvwmPipe (void)
{
  int body_length;
  FvwmPacketHeader header;
  FvwmPacketBody *body;

  PrintMemuse();

  ConsoleDebug(FVWM, "DEBUG: entering ReadFvwmPipe\n");
  body_length = ReadFvwmPacket(Fvwm_fd[1], (unsigned long *) &header,
                 (unsigned long **)&body);
  body_length -= HEADER_SIZE;
  if (header.start == START_FLAG) {
    ProcessMessage (header.type, body);
    if (body_length) {
      Free (body);
    }
  }
  else {
    DeadPipe (1);
  }
  ConsoleDebug(FVWM, "DEBUG: leaving ReadFvwmPipe\n");
}


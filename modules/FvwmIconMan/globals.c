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

#include <limits.h>
#include "FvwmIconMan.h"
#include "xmanager.h"
#include "readconfig.h"

#define DEFAULT_MOUSE "0 N sendcommand Iconify"

static char const rcsid[] =
  "$Id: globals.c,v 1.11 1999/11/03 13:49:26 hippo Exp $";

GlobalData globals;
ContextDefaults contextDefaults[] = {
  { "plain", BUTTON_UP, { "black", "black" }, { "white", "gray"} },
  { "focus", BUTTON_UP, { "white", "gray" }, { "black", "black" } },
  { "select", BUTTON_FLAT, { "black", "black" }, { "white", "gray" } },
  { "focusandselect", BUTTON_FLAT, { "white", "gray" }, { "black", "black" } },
  { "title", BUTTON_EDGEUP, { "black", "black"}, {"white", "gray"} },
  { "default", BUTTON_FLAT, { "black", "black"}, {"white", "gray"} }
};

int Fvwm_fd[2];
int x_fd;
/* char *Module = "*FvwmIconMan"; */
char *Module;
/* int ModuleLen = 12; */
int ModuleLen;

/* This is solely so that we can turn a string constant into something
   which can be freed */

static char *alloc_string (char *string)
{
  int len = strlen (string);
  char *ret = (char *)safemalloc ((len + 1) * sizeof (char));
  strcpy (ret, string);
  return ret;
}

static void init_win_manager (int id)
{
  int i;

  globals.managers[id].magic = 0x12344321;
  globals.managers[id].index = id;
#ifdef MINI_ICONS
  globals.managers[id].draw_icons = 0;
#endif
  globals.managers[id].res = SHOW_PAGE;
  globals.managers[id].rev = REVERSE_NONE;
  globals.managers[id].window_up = 0;
  globals.managers[id].can_draw = 0;
  globals.managers[id].window_mapped = 0;
  globals.managers[id].fontname = NULL;
  globals.managers[id].titlename = alloc_string ("FvwmIconMan");
  globals.managers[id].iconname = alloc_string ("FvwmIconMan");
  globals.managers[id].formatstring = alloc_string ("%c: %i");
  globals.managers[id].format_depend = CLASS_NAME | ICON_NAME;
  globals.managers[id].geometry.dir = 0;
  globals.managers[id].geometry.boxwidth = 0;
#ifdef SHAPE
  globals.managers[id].shape.num_rects = 0;
#endif
  globals.managers[id].shaped = 0;
  init_button_array (&globals.managers[id].buttons);

  for ( i = 0; i < NUM_CONTEXTS; i++ ) {
    globals.managers[id].backColorName[i] = NULL;
    globals.managers[id].foreColorName[i] = NULL;
    globals.managers[id].buttonState[i] = contextDefaults[i].state;
    globals.managers[id].colorsets[i] = -1;
  }
  globals.managers[id].geometry_str = NULL;
  globals.managers[id].button_geometry_str = NULL;
  globals.managers[id].show.list = NULL;
  globals.managers[id].show.mask = ALL_NAME;
  globals.managers[id].dontshow.list = NULL;
  globals.managers[id].dontshow.mask = ALL_NAME;
  globals.managers[id].followFocus = 0;
  globals.managers[id].usewinlist = 1;
  globals.managers[id].sort = SortName;
  globals.managers[id].focus_button = NULL;
  globals.managers[id].select_button = NULL;
  globals.managers[id].bindings[MOUSE]    = ParseMouseEntry (DEFAULT_MOUSE);
  globals.managers[id].bindings[KEYPRESS] = NULL;
  globals.managers[id].bindings[SELECT]   = NULL;
  globals.managers[id].we_are_drawing = 1;
  globals.managers[id].configures_expected = 0;
}

void print_managers (void)
{
#ifdef PRINT_DEBUG
  int i;

  for (i = 0; i < globals.num_managers; i++) {
    ConsoleDebug (CORE, "Manager %d:\n", i + 1);
    if (globals.managers[i].res == SHOW_GLOBAL)
      ConsoleDebug (CORE, "ShowGlobal\n");
    else if (globals.managers[i].res == SHOW_DESKTOP)
      ConsoleDebug (CORE, "ShowDesktop\n");
    else if (globals.managers[i].res == SHOW_PAGE)
      ConsoleDebug (CORE, "ShowPage\n");

    ConsoleDebug (CORE, "DontShow:\n");
    print_stringlist (&globals.managers[i].dontshow);
    ConsoleDebug (CORE, "Show:\n");
    print_stringlist (&globals.managers[i].show);

    ConsoleDebug (CORE, "Font: %s\n", (globals.managers[i].fontname)?
      globals.managers[i].fontname : "(NULL)");
    ConsoleDebug (CORE, "Geometry: %s\n", globals.managers[i].geometry_str);
    ConsoleDebug (CORE, "Button geometry: %s\n",
		  (globals.managers[i].button_geometry_str)?
      globals.managers[i].button_geometry_str : "(NULL)");
    ConsoleDebug (CORE, "\n");
  }

#endif

}

int allocate_managers (int num)
{
  int i;

  if (globals.managers) {
    ConsoleMessage ("Already have set the number of managers\n");
    return 0;
  }

  if (num < 1) {
    ConsoleMessage ("Can't have %d managers\n", num);
    return 0;
  }

  globals.num_managers = num;
  globals.managers = (WinManager *)safemalloc (num * sizeof (WinManager));

  for (i = 0; i < num; i++) {
    init_win_manager (i);
  }

  return 1;
}

void init_globals (void)
{
  globals.desknum = ULONG_MAX;
  globals.x = ULONG_MAX;
  globals.y = ULONG_MAX;
  globals.screenx = 0;
  globals.screeny = 0;
  globals.num_managers = 1;
  globals.managers = NULL;
  globals.focus_win = NULL;
  globals.select_win = NULL;
  globals.transient = 0;
  globals.shapes_supported = 0;
  globals.got_window_list = 0;
}

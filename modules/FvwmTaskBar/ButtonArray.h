/* -*-c-*- */
/* FvwmTaskBar Module for Fvwm95, based on the FvwmWinList module
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The functions in this header file that are the original work of Mike Finger.
 *
 * No guarantees or warantees or anything are provided or implied in any way
 * whatsoever. Use this program at your own risk. Permission to use this
 * program for any purpose is given, as long as the copyright is kept intact.
 */

#include "libs/PictureBase.h"

#define DEFAULT_BTN_WIDTH 180

/* Button states */

#define BUTTON_UP     0
#define BUTTON_DOWN   1
#define BUTTON_BRIGHT 2

#define DONT_CARE     -1

/* Struct definitions */

typedef struct button
{
  char   *title;
  int    state, needsupdate, truncate, count, iconified;
  struct button *next;
  FvwmPicture p;
} Button;

typedef struct
{
  int count;
  Button *head, *tail;
  int x, y;
  int w, h, tw;
} ButtonArray;

/* Function Prototypes */

void Draw3dRect(
	Window wn, int x, int y, int w, int h, XRectangle *bounding, int state,
	Bool iconified);
Button *ButtonNew(const char *title, FvwmPicture *p, int state, int count);
int ButtonUpdate(Button *button, const char *title, int state);
char *ButtonName(Button *button);
void InitArray(ButtonArray *array, int x, int y, int w, int h, int tw);
void UpdateArray(ButtonArray *array, int x, int y, int w, int h, int tw);
void AddButton(ButtonArray *array, const char *title, FvwmPicture *p, int state,
	       int count, int iconified);
int UpdateButton(ButtonArray *array, int butnum, const char *title, int state);
int UpdateButtonPicture(ButtonArray *array, int butnum, FvwmPicture *p);
void RemoveButton(ButtonArray *array, int butnum);
Button *find_n(ButtonArray *array, int n);
void FreeButton(Button *ptr);
void FreeAllButtons(ButtonArray *array);
void DoButton(ButtonArray *array, Button *ptr, int x, int y, int w, int h);
void DrawButtonArray(ButtonArray *array, int all, XEvent *evp);
void RadioButton(ButtonArray *array, int butnum, int state);
Bool CheckRootTransparentButtons(ButtonArray *array);
int WhichButton(ButtonArray *array, int x, int y);
int LocateButton(ButtonArray *array, int xp,  int yp,
				     int *xb, int *yb,
				     char **name, int *trunc);
void ArrangeButtonArray(ButtonArray *array);
void ButtonDraw(Button *button, int x, int y, int w, int h, XEvent *evp);
void ButtonCoordinates(ButtonArray *array, int numbut, int *xc, int *yc);
void ButtonDimensions(ButtonArray *array, int *width, int *height);

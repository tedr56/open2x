/*
 * Non-standard SDL header.
 *
 * This header gives access to a few extra features of the GP2X hardware
 * that I added to the video driver.
 */

#ifndef SDL_GP2X__H
#define SDL_GP2X__H

#ifdef __cplusplus
extern "C" {
#endif

#define GP2X_MOUSE_NONE        0
#define GP2X_MOUSE_STD         1
#define GP2X_MOUSE_TOUCHSCREEN 2

  
void SDL_GP2X_SetMonoColours(int background, int foreground);
int  SDL_GP2X_GetPhysicalScreenSize(SDL_Rect *size);
void SDL_GP2X_Display(SDL_Rect *area);
void SDL_GP2X_InitializeCursor();
void SDL_GP2X_SetCursorColour(WMcursor *wcursor,
			      int bred, int bgreen, int bblue, int balpha,
			      int fred, int fgreen, int fblue, int falpha);
void SDL_GP2X_DefineRegion(int region, SDL_Rect *area);
void SDL_GP2X_ActivateRegion(int region, int activate);
void SDL_GP2X_MiniDisplay(int x, int y);
void SDL_GP2X_WaitForBlitter();

int  SDL_GP2X_TV(int state);
int  SDL_GP2X_TVMode(int mode);
void SDL_GP2X_TVAdjust(int direction);

void SDL_GP2X_AllowGfxMemory(char *start, int size);
void SDL_GP2X_DenyGfxMemory(char *start, int size);

void SDL_GP2X_VSync();

// Return a hardware surface's bitmap address for use by the 940
//  Surface has to be SDL_HWSURFACE *and* SDL_LockSurface()ed
//  When you unlock the surface the address you got from here *must*
//  be considered invalid.
void *SDL_GP2X_PhysAddress(SDL_Surface *surface);

// Query GP2X mouse type
int SDL_GP2X_MouseType();

// Get (semi-)raw touchpad position (not SDL position)
//   0 <= x <= 319, 0 <= y <= 239
//   Return value is pressure (any non zero value means pressed)
int SDL_GP2X_Touchpad(int *x, int *y);

// Enable (1) or disable (0) the touchscreen stylus from causing mouse button events.  
// In some games/apps it is desirable to have the stylus only used for positioning of
// the cursor with clicking done by joystick buttons. Default is enabled.
void SDL_GP2X_TouchpadMouseButtonEvents(int enabled);

// Enable (1) or disable (0) the touchscreen stylus from causing mouse motion events or
// registering changes in cursor state.  Sometimes a developer may wish to easily
// disable the touchscreen and not have to modify large sections of code in a porting
// project. Default is enabled.
void SDL_GP2X_TouchpadMouseMotionEvents(int enabled);

#ifdef __cplusplus
}
#endif

#endif

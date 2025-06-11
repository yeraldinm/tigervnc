#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "KeyboardGrab.h"

#include <FL/Fl.H>
#include <FL/x.H>

#ifdef WIN32
#include "../win32.h"
#elif defined(__APPLE__)
#include "../cocoa.h"
#endif

fltk_keyboard_grab_result fltk_grab_keyboard(Fl_Window *win)
{
#if defined(WIN32)
  int ret = win32_enable_lowlevel_keyboard(fl_xid(win));
  return ret == 0 ? FLTK_KEYBOARD_GRAB_SUCCESS : FLTK_KEYBOARD_GRAB_FAIL;
#elif defined(__APPLE__)
  bool ret = cocoa_tap_keyboard();
  return ret ? FLTK_KEYBOARD_GRAB_SUCCESS : FLTK_KEYBOARD_GRAB_FAIL;
#else
  int ret = XGrabKeyboard(fl_display, fl_xid(win), True,
                          GrabModeAsync, GrabModeAsync, CurrentTime);
  if (ret == GrabSuccess)
    return FLTK_KEYBOARD_GRAB_SUCCESS;
  if (ret == AlreadyGrabbed)
    return FLTK_KEYBOARD_GRAB_ALREADY;
  return FLTK_KEYBOARD_GRAB_FAIL;
#endif
}

void fltk_ungrab_keyboard(Fl_Window *win)
{
#if defined(WIN32)
  win32_disable_lowlevel_keyboard(fl_xid(win));
#elif defined(__APPLE__)
  cocoa_untap_keyboard();
#else
  if (Fl::grab())
    return;

  XUngrabKeyboard(fl_display, CurrentTime);
#endif
}

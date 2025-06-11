#ifndef __FLTK_KEYBOARD_GRAB_H__
#define __FLTK_KEYBOARD_GRAB_H__

class Fl_Window;

enum fltk_keyboard_grab_result {
  FLTK_KEYBOARD_GRAB_SUCCESS,
  FLTK_KEYBOARD_GRAB_ALREADY,
  FLTK_KEYBOARD_GRAB_FAIL
};

fltk_keyboard_grab_result fltk_grab_keyboard(Fl_Window *win);
void fltk_ungrab_keyboard(Fl_Window *win);

#endif

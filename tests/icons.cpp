#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>

Window findClient(Display *d, Window win)
{
  Window r, *children;
  unsigned int n;
  XQueryTree(d, win, &r, &r, &children, &n);
  for (unsigned int i = 0; i < n; ++i) {
    Window w = findClient(d, children[i]);
    if (w) return w;
  }

  // try me
  Atom state = XInternAtom(d, "WM_STATE", true);
  Atom ret_type;
  int ret_format;
  unsigned long ret_items, ret_bytesleft;
  unsigned long *prop_return;
  XGetWindowProperty(d, win, state, 0, 1,
		     false, state, &ret_type, &ret_format,
		     &ret_items, &ret_bytesleft,
		     (unsigned char**) &prop_return); 
  if (ret_type == None || ret_items < 1)
    return None;
  return win; // found it!
}

int main(int argc, char **argv)
{
  Display *d = XOpenDisplay(NULL);
  int s = DefaultScreen(d);
  Atom net_wm_icon = XInternAtom(d, "_NET_WM_ICON", true);
  Atom ret_type;
  unsigned int winw = 0, winh = 0;
  int ret_format;
  unsigned long ret_items, ret_bytesleft;
  const int MAX_IMAGES = 10;
  unsigned long *prop_return[MAX_IMAGES];
  XImage *i[MAX_IMAGES];

  printf("Click on a window with an icon...\n");
  
  //int id = strtol(argv[1], NULL, 16);
  XUngrabPointer(d, CurrentTime);
  Window id;
  Cursor cur = XCreateFontCursor(d, XC_crosshair);
  XGrabPointer(d, RootWindow(d, s), false, ButtonPressMask, GrabModeAsync,
	       GrabModeAsync, None, cur, CurrentTime);
  XEvent ev;
  while (true) {
    XNextEvent(d, &ev);
    if (ev.type == ButtonPress) {
      XUngrabPointer(d, CurrentTime);
      id = ev.xbutton.subwindow;
      id = findClient(d, id);
      break;
    }
  }

  printf("Using window 0x%lx\n", id);
  
  long offset = 0;
  int image = 0;
  
  do {
    XGetWindowProperty(d, id, net_wm_icon, offset++, 1,
		       false, XA_CARDINAL, &ret_type, &ret_format,
		       &ret_items, &ret_bytesleft,
		       (unsigned char**) &prop_return[image]);
    if (ret_type == None || ret_items < 1) {
      printf("No icon found\n");
      return 1;
    }
    unsigned int w = prop_return[image][0];
    XFree(prop_return[image]);

    XGetWindowProperty(d, id, net_wm_icon, offset++, 1,
		       false, XA_CARDINAL, &ret_type, &ret_format,
		       &ret_items, &ret_bytesleft,
		       (unsigned char**) &prop_return[image]);
    if (ret_type == None || ret_items < 1) {
      printf("Failed to get height\n");
      return 1;
    }
    unsigned int h = prop_return[image][0];
    XFree(prop_return[image]);

    XGetWindowProperty(d, id, net_wm_icon, offset, w*h,
		       false, XA_CARDINAL, &ret_type, &ret_format,
		       &ret_items, &ret_bytesleft,
		       (unsigned char**) &prop_return[image]);
    if (ret_type == None || ret_items < w*h) {
      printf("Failed to get image data\n");
      return 1;
    }
    offset += w*h;

    printf("Image dimentions: %d, %d\n", w, h);
  
    i[image] = XCreateImage(d, DefaultVisual(d, s), DefaultDepth(d, s),
			    ZPixmap, 0, NULL, w, h, 32, 0);
    assert(i[image]);
    i[image]->byte_order = LSBFirst;
    i[image]->data = (char*)prop_return[image];
    for (unsigned int j = 0; j < w*h; j++) {
      unsigned char alpha = (unsigned char)i[image]->data[j*4+3];
      unsigned char r = (unsigned char) i[image]->data[j*4+0];
      unsigned char g = (unsigned char) i[image]->data[j*4+1];
      unsigned char b = (unsigned char) i[image]->data[j*4+2];

      // background color
      unsigned char bgr = 40;
      unsigned char bgg = 0x8f;
      unsigned char bgb = 40;
      
      r = bgr + (r - bgr) * alpha / 256;
      g = bgg + (g - bgg) * alpha / 256;
      b = bgb + (b - bgb) * alpha / 256;

      i[image]->data[j*4+0] = (char) r;
      i[image]->data[j*4+1] = (char) g;
      i[image]->data[j*4+2] = (char) b;
    }

    winw += w;
    if (h > winh) winh = h;

    ++image;
  } while (ret_bytesleft > 0 && image < MAX_IMAGES);
  
  Window win = XCreateSimpleWindow(d, RootWindow(d, s), 0, 0, winw, winh,
				   0, 0, 0);
  assert(win);
  XMapWindow(d, win);

  Pixmap p = XCreatePixmap(d, win, winw, winh, DefaultDepth(d, s));
  XFillRectangle(d, p, DefaultGC(d, s), 0, 0, winw, winh);

  unsigned int x = 0;
  for (int j = 0; j < image; ++j) {
    XPutImage(d, p, DefaultGC(d, s), i[j], 0, 0, x, 0,
	      i[j]->width, i[j]->height);
    x += i[j]->width;
    XFree(prop_return[j]);
    XDestroyImage(i[j]);
  }
    
  XSetWindowBackgroundPixmap(d, win, p);
  XClearWindow(d, win);

  XFlush(d);

  getchar();

  XFreePixmap(d, p);
  XCloseDisplay(d);
}

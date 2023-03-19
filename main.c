// xcursorview
// Copyright (C) 2023 Justin Collier <m@jpcx.dev>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ASSERT(_expr_, _why_) \
	do { \
		if (!(_expr_)) { \
			fprintf(stderr, "Error: " _why_ "\n"); \
			exit(1); \
		} \
	} while (0)

#define PRINT_HELP(_argv0_) \
	printf("Usage: %s [-w width] [-c color] [-Fh] device\n" \
	       "Tracks an xinput cursor and draws a crosshair at its position.\n" \
	       "\n" \
	       "Options:\n" \
	       "  -w, --width width     Set the total crosshair width (default: " \
	       "11)\n" \
	       "  -c, --color color     Set the crosshair color as a hex value " \
	       "(default: 0x800080)\n" \
	       "  -F, --foreground      Do not daemonize.\n" \
	       "  -h, --help            Display this help text and exit\n" \
	       "\n" \
	       "Arguments:\n" \
	       "  device                The xinput device ID\n", \
	       _argv0_)

int
main(int argc, char** argv) {
	int          width      = 11;
	unsigned int color      = 0x800080;
	bool         foreground = false;
	int          device;

	{ // parse args
		struct option longopt[] = {{"width", required_argument, NULL, 'w'},
		                           {"color", required_argument, NULL, 'c'},
		                           {"foreground", no_argument, NULL, 'F'},
		                           {"help", no_argument, NULL, 'h'},
		                           {NULL, 0, NULL, 0}};

		int   longind = 0;
		int   c;
		char* end;

		while ((c = getopt_long(argc, argv, "w:c:Fh", longopt, &longind)) !=
		       -1) {
			switch (c) {
				case 'w':
					width = strtol(optarg, &end, 10);
					ASSERT(!errno && !*end, "Invalid width input.");
					break;
				case 'c':
					color = strtol(optarg, &end, 16);
					ASSERT(!errno && !*end && color < (1 << 24),
					       "Invalid color input. Must be hex up to 6 digits.");
					break;
				case 'F':
					foreground = true;
					break;
				case 'h':
					PRINT_HELP(argv[0]);
					return 0;
				default:
					PRINT_HELP(argv[0]);
					return 1;
			}
		}

		if (optind < argc) {
			device = strtol(argv[optind], &end, 10);
			ASSERT(!errno && !*end, "Invalid device input.");
		} else {
			PRINT_HELP(argv[0]);
			return 1;
		}
	}

	// post-args initializations
	Display* dpy = XOpenDisplay(NULL);
	ASSERT(dpy, "Could not open display.");
	int    scr    = DefaultScreen(dpy);
	Window root   = DefaultRootWindow(dpy);
	int    hwidth = width / 2;

	// opcode for XInput
	int opcode;
	{ // load extensions
		int ev, err;
		ASSERT(XShapeQueryExtension(dpy, &ev, &err),
		       "XShape extension not available.");
		ASSERT(XFixesQueryExtension(dpy, &ev, &err),
		       "XFixes extension not available.");
		ASSERT(XQueryExtension(dpy, "XInputExtension", &opcode, &ev, &err),
		       "XInput extension not available.");
	}

	Window win;
	GC     gc;
	{ // initalize crosshairs window and graphics
		Visual* visual;
		{ // find ARGB
			XVisualInfo info;
			ASSERT(XMatchVisualInfo(dpy, scr, 32, TrueColor, &info),
			       "No ARGB visual found.");
			visual = info.visual;
		}

		// create window
		XSetWindowAttributes attrs = {
			.override_redirect = true,
			.border_pixel      = 0,
			.background_pixel  = 0,
			.colormap          = XCreateColormap(dpy, root, visual, AllocNone)};
		win = XCreateWindow(
			dpy, root, 0, 0, width, width, 0, 32, InputOutput, visual,
			CWOverrideRedirect | CWBorderPixel | CWBackPixel | CWColormap,
			&attrs);
		gc = XCreateGC(dpy, win, 0, NULL);
		ASSERT(gc, "Could not create graphics context.");
		XMapRaised(dpy, win);
		XSetForeground(dpy, gc, 0xFF000000 | color);
		XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
		XSetWindowBackground(dpy, win, 0x00000000);

		XClearWindow(dpy, win);
		XDrawLine(dpy, win, gc, 0, hwidth, width, hwidth);
		XDrawLine(dpy, win, gc, hwidth, 0, hwidth, width);
	}

	{ // make window intangible
		XserverRegion region = XFixesCreateRegion(dpy, NULL, 0);
		XFixesSetWindowShapeRegion(dpy, win, ShapeInput, 0, 0, region);
		XFixesDestroyRegion(dpy, region);
	}

	// flush to finalize window init
	XFlush(dpy);

	{ // subscribe to motion events
		unsigned char mask[(XI_LASTEVENT + 7) / 8] = {0};

		XIEventMask evmask = {
			.deviceid = device,
			.mask_len = sizeof(mask),
			.mask     = mask,
		};

		ASSERT(XISelectEvents(dpy, root, &evmask, 1) != BadRequest,
		       "XInput2 not supported");
		XISetMask(mask, XI_Motion);
		XISelectEvents(dpy, root, &evmask, 1);
	}

	if (foreground || !fork()) {
		while (1) {
			XEvent               ev;
			XGenericEventCookie* cookie = &ev.xcookie;

			XNextEvent(dpy, &ev);

			if (XGetEventData(dpy, cookie) && cookie->type == GenericEvent &&
			    cookie->extension == opcode && cookie->evtype == XI_Motion) {
				// move window to new cursor position
				XIDeviceEvent* ev = (XIDeviceEvent*)cookie->data;
				XMoveWindow(dpy, win, ev->event_x - hwidth,
				            ev->event_y - hwidth);
			}

			XFreeEventData(dpy, cookie);
		}

		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}

	return 0;
}

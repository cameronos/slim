/*
 * SLIM - Simple Lightweight Image Manager
 * author: cameronos
 * copyright (c) Orange Computers 2025
 * version: 0.4 (12/18/2025)
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Imlib2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

typedef struct {
	Display		*display;
	Window		win;
	Pixmap		backbuffer;
	int		screen;
	Imlib_Image	img;
	Imlib_Image	cached_scaled;
	int		img_width;
	int		img_height;
	int		win_width;
	int		win_height;
	double		zoom;
	double		initial_zoom;
	int		offset_x;
	int		offset_y;
	int		dragging;
	int		last_drag_x;
	int		last_drag_y;
	int		cached_scaled_width;
	int		cached_scaled_height;
	int		needs_rescale;
} viewer_state_t;

/*
 * create_backbuffer(state)
 *
 * (re)Creates the backbuffer pixmap for double buffer
 */
static void
create_backbuffer(viewer_state_t *state)
{
	if (state->backbuffer != 0) {
		XFreePixmap(state->display, state->backbuffer);
	}
	state->backbuffer = XCreatePixmap(state->display, state->win,
	    state->win_width, state->win_height,
	    DefaultDepth(state->display, state->screen));
}

/*
 * clamp_offsets(state)
 *
 * limits panning, keeps 20% of img in view
 */
static void
clamp_offsets(viewer_state_t *state)
{
	int scaled_width;
	int scaled_height;
	int max_offset_x;
	int max_offset_y;

	scaled_width = (int)(state->img_width * state->zoom);
	scaled_height = (int)(state->img_height * state->zoom);

	max_offset_x = (int)(scaled_width * 0.8);
	max_offset_y = (int)(scaled_height * 0.8);

	if (state->offset_x > max_offset_x)
		state->offset_x = max_offset_x;
	if (state->offset_x < -max_offset_x)
		state->offset_x = -max_offset_x;
	if (state->offset_y > max_offset_y)
		state->offset_y = max_offset_y;
	if (state->offset_y < -max_offset_y)
		state->offset_y = -max_offset_y;
}

/*
 * render_to_backbuffer(state)
 *
 * renders visible image to backbuffer
 * scales portion that is visible to minimize CPU usage, reduce overdraw
 */
static void
render_to_backbuffer(viewer_state_t *state)
{
	int scaled_width;
	int scaled_height;
	int draw_x;
	int draw_y;
	int src_x;
	int src_y;
	int src_w;
	int src_h;
	int dest_x;
	int dest_y;
	int dest_w;
	int dest_h;
	int clip_amount;

	imlib_context_set_drawable(state->backbuffer);
	XSetForeground(state->display,
	    DefaultGC(state->display, state->screen),
	    BlackPixel(state->display, state->screen));
	XFillRectangle(state->display, state->backbuffer,
	    DefaultGC(state->display, state->screen),
	    0, 0, state->win_width, state->win_height);

	scaled_width = (int)(state->img_width * state->zoom);
	scaled_height = (int)(state->img_height * state->zoom);

	draw_x = (state->win_width - scaled_width) / 2 + state->offset_x;
	draw_y = (state->win_height - scaled_height) / 2 + state->offset_y;

	src_x = 0;
	src_y = 0;
	src_w = state->img_width;
	src_h = state->img_height;
	dest_x = draw_x;
	dest_y = draw_y;
	dest_w = scaled_width;
	dest_h = scaled_height;

	if (draw_x < 0) {
		src_x = (int)((-draw_x) / state->zoom);
		dest_x = 0;
	}
	if (draw_y < 0) {
		src_y = (int)((-draw_y) / state->zoom);
		dest_y = 0;
	}
	if (draw_x + scaled_width > state->win_width) {
		clip_amount = (draw_x + scaled_width) - state->win_width;
		src_w = state->img_width - src_x -
		    (int)(clip_amount / state->zoom);
	} else {
		src_w = state->img_width - src_x;
	}
	if (draw_y + scaled_height > state->win_height) {
		clip_amount = (draw_y + scaled_height) - state->win_height;
		src_h = state->img_height - src_y -
		    (int)(clip_amount / state->zoom);
	} else {
		src_h = state->img_height - src_y;
	}


	if (src_x < 0)
		src_x = 0;
	if (src_y < 0)
		src_y = 0;
	if (src_x >= state->img_width)
		return;
	if (src_y >= state->img_height)
		return;
	if (src_w <= 0 || src_h <= 0)
		return;
	if (src_x + src_w > state->img_width)
		src_w = state->img_width - src_x;
	if (src_y + src_h > state->img_height)
		src_h = state->img_height - src_y;

	dest_w = (int)(src_w * state->zoom);
	dest_h = (int)(src_h * state->zoom);

	/* frees old cache */
	if (state->cached_scaled != NULL) {
		imlib_context_set_image(state->cached_scaled);
		imlib_free_image();
	}

	imlib_context_set_image(state->img);
	state->cached_scaled = imlib_create_cropped_scaled_image(
	    src_x, src_y, src_w, src_h, dest_w, dest_h);
	state->cached_scaled_width = dest_w;
	state->cached_scaled_height = dest_h;
	state->needs_rescale = 0;

	imlib_context_set_image(state->cached_scaled);
	imlib_render_image_on_drawable(dest_x, dest_y);
}

/*
 * present_backbuffer(state)
 *
 * xcopyarea's backbuffer to window, flicker-free?
 */
static void
present_backbuffer(viewer_state_t *state)
{
	XCopyArea(state->display, state->backbuffer, state->win,
	    DefaultGC(state->display, state->screen),
	    0, 0, state->win_width, state->win_height, 0, 0);
	XFlush(state->display);
}

/*
 * reset_view(state)
 *
 * resets zoom and pan to fit image in window size
 */
static void
reset_view(viewer_state_t *state)
{
	double scale_w;
	double scale_h;

	scale_w = (double)state->win_width / state->img_width;
	scale_h = (double)state->win_height / state->img_height;
	state->zoom = (scale_w < scale_h) ? scale_w : scale_h;

	state->offset_x = 0;
	state->offset_y = 0;
	state->needs_rescale = 1;
}

int
main(int argc, char **argv)
{
	viewer_state_t state;
	const char *image_path;
	char *path_copy;
	char *filename;
	char title[512];
	Window root;
	int screen_width;
	int screen_height;
	int max_width;
	int max_height;
	double scale_w;
	double scale_h;
	double scale;
	XEvent event;
	KeySym key;
	int running;
	int dx;
	int dy;

	if (argc < 2) {
		(void) fprintf(stderr, "Usage: %s <image>\n", argv[0]);
		return (1);
	}

	image_path = argv[1];

	path_copy = strdup(image_path);
	if (path_copy == NULL) {
		(void) fprintf(stderr, "Error: Out of memory\n");
		return (1);
	}
	filename = basename(path_copy);
	(void) snprintf(title, sizeof (title), "%s - SLIM", filename);

	(void) memset(&state, 0, sizeof (state));
	state.needs_rescale = 1;

	state.display = XOpenDisplay(NULL);
	if (state.display == NULL) {
		(void) fprintf(stderr, "Error: Cannot open/find X display\n");
		free(path_copy);
		return (1);
	}

	state.screen = DefaultScreen(state.display);
	root = RootWindow(state.display, state.screen);

	state.img = imlib_load_image(image_path);
	if (state.img == NULL) {
		(void) fprintf(stderr, "Error: Failed to load image '%s'\n",
		    image_path);
		XCloseDisplay(state.display);
		free(path_copy);
		return (1);
	}

	imlib_context_set_image(state.img);
	state.img_width = imlib_image_get_width();
	state.img_height = imlib_image_get_height();
	state.zoom = 1.0;

	screen_width = DisplayWidth(state.display, state.screen);
	screen_height = DisplayHeight(state.display, state.screen);

	max_width = screen_width - 100;
	max_height = screen_height - 100;

	state.win_width = state.img_width;
	state.win_height = state.img_height;

	if (state.win_width > max_width || state.win_height > max_height) {
		scale_w = (double)max_width / state.img_width;
		scale_h = (double)max_height / state.img_height;
		scale = (scale_w < scale_h) ? scale_w : scale_h;

		state.win_width = (int)(state.img_width * scale);
		state.win_height = (int)(state.img_height * scale);
		state.zoom = scale;
	}

	state.initial_zoom = state.zoom;

	state.win = XCreateSimpleWindow(state.display, root,
	    100, 100, state.win_width, state.win_height, 1,
	    BlackPixel(state.display, state.screen),
	    BlackPixel(state.display, state.screen));

	XStoreName(state.display, state.win, title);

	XSelectInput(state.display, state.win,
	    ExposureMask | KeyPressMask | StructureNotifyMask |
	    ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
	XMapWindow(state.display, state.win);

	create_backbuffer(&state);

	imlib_context_set_display(state.display);
	imlib_context_set_visual(DefaultVisual(state.display, state.screen));
	imlib_context_set_colormap(DefaultColormap(state.display,
	    state.screen));
	imlib_context_set_image(state.img);

	(void) printf("SLIM Controls:\n");
	(void) printf("  Mouse wheel / +/- : Zoom in/out\n");
	(void) printf("  Left mouse drag   : Pan image\n");
	(void) printf("  R                 : Reset view\n");
	(void) printf("  Q / ESC           : Quit\n");

	render_to_backbuffer(&state);
	present_backbuffer(&state);

	running = 1;
	while (running != 0) {
		XNextEvent(state.display, &event);

		switch (event.type) {
		case Expose:
			if (event.xexpose.count == 0) {
				present_backbuffer(&state);
			}
			break;

		case ConfigureNotify:
			if (event.xconfigure.width != state.win_width ||
			    event.xconfigure.height != state.win_height) {
				state.win_width = event.xconfigure.width;
				state.win_height = event.xconfigure.height;
				create_backbuffer(&state);
				render_to_backbuffer(&state);
				present_backbuffer(&state);
			}
			break;

		case ButtonPress:
			if (event.xbutton.button == Button1) {
				state.dragging = 1;
				state.last_drag_x = event.xbutton.x;
				state.last_drag_y = event.xbutton.y;
			} else if (event.xbutton.button == Button4) {
				state.zoom *= 1.1;
				if (state.zoom > 5.0)
					state.zoom = 5.0;
				state.needs_rescale = 1;
				clamp_offsets(&state);
				render_to_backbuffer(&state);
				present_backbuffer(&state);
			} else if (event.xbutton.button == Button5) {
				state.zoom *= 0.9;
				if (state.zoom < 0.1)
					state.zoom = 0.1;
				state.needs_rescale = 1;
				clamp_offsets(&state);
				render_to_backbuffer(&state);
				present_backbuffer(&state);
			}
			break;

		case ButtonRelease:
			if (event.xbutton.button == Button1) {
				state.dragging = 0;
			}
			break;

		case MotionNotify:
			if (state.dragging != 0) {
				while (XCheckTypedWindowEvent(state.display,
				    state.win, MotionNotify, &event)) {
				}

				dx = event.xmotion.x - state.last_drag_x;
				dy = event.xmotion.y - state.last_drag_y;

				state.offset_x += dx;
				state.offset_y += dy;

				clamp_offsets(&state);
				render_to_backbuffer(&state);
				present_backbuffer(&state);

				state.last_drag_x = event.xmotion.x;
				state.last_drag_y = event.xmotion.y;
			}
			break;

		case KeyPress:
			key = XLookupKeysym(&event.xkey, 0);

			if (key == XK_q || key == XK_Q || key == XK_Escape) {
				running = 0;
			} else if (key == XK_r || key == XK_R) {
				reset_view(&state);
				render_to_backbuffer(&state);
				present_backbuffer(&state);
			} else if (key == XK_plus || key == XK_equal) {
				state.zoom *= 1.2;
				if (state.zoom > 5.0)
					state.zoom = 5.0;
				state.needs_rescale = 1;
				clamp_offsets(&state);
				render_to_backbuffer(&state);
				present_backbuffer(&state);
			} else if (key == XK_minus || key == XK_underscore) {
				state.zoom *= 0.8;
				if (state.zoom < 0.1)
					state.zoom = 0.1;
				state.needs_rescale = 1;
				clamp_offsets(&state);
				render_to_backbuffer(&state);
				present_backbuffer(&state);
			}
			break;

		default:
			break;
		}
	}

	/* cleanup cache, free and destroy memory */
	imlib_context_set_image(state.img);
	imlib_free_image();
	if (state.cached_scaled != NULL) {
		imlib_context_set_image(state.cached_scaled);
		imlib_free_image();
	}
	if (state.backbuffer != 0) {
		XFreePixmap(state.display, state.backbuffer);
	}
	XDestroyWindow(state.display, state.win);
	XCloseDisplay(state.display);
	free(path_copy);

	return (0);
}

/*
 * Released with love
 * listen to Jah Music: wRqMiwhVPXY !
 */

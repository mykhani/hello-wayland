#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GL/gl.h>
#include "xdg-shell-unstable-v6-client-protocol.h"


static int running = 1;
// forward declaration
struct window;

struct display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shell *shell;
	struct {
		EGLDisplay dpy;
		EGLContext ctx;
		EGLConfig conf;
	} egl;
	struct window *window;
};

struct geometry {
	int width; int height;
};

struct window {
	struct display *display;
	struct geometry geometry, window_size;
	struct wl_egl_window *native;
	struct wl_surface *surface;
	struct wl_shell_surface *shell_surface;
	EGLSurface egl_surface;
	bool wait_for_configure;
};


static void shell_surface_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial) {
	wl_shell_surface_pong(shell_surface, serial);
}
static void shell_surface_configure(void *data, struct wl_shell_surface *shell_surface,
                                    uint32_t edges, int32_t width, int32_t height) 
{
	struct window *window = data;
	wl_egl_window_resize(window->native, width, height, 0, 0);
}
static void shell_surface_popup_done (void *data, struct wl_shell_surface *shell_surface) {
	
}

static struct wl_shell_surface_listener shell_surface_listener = {
	&shell_surface_ping, 
	&shell_surface_configure, 
	&shell_surface_popup_done
};

/* Callbacks for global objects add/remove */
void global_add(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
	struct display *d = (struct display *)data;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
		printf("DEBUG: bound to wl_compositor interface\n");
	}
	#if 0
	else if (strcmp(interface, "zxdg_shell_v6") == 0) {
		xdg_shell = wl_registry_bind(registry, id, &zxdg_shell_v6_interface, 1);
		zxdg_shell_v6_add_listener(xdg_shell, &xdg_shell_listener, NULL);
		printf("DEBUG: bound to zxdg_shell_v6 interface\n");
	}
	#endif
	else if (strcmp(interface, "wl_shell") == 0) {
		d->shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
		printf("DEBUG: bound to wl_shell interface\n");
	}
}

void global_remove(void *data, struct wl_registry *registry, uint32_t id)
{
	printf("In global remove cb\n");
}

static void init_egl(struct display *display, struct window *window)
{
	EGLint major, minor, count, n, size;
	EGLConfig *configs;
	int ret;
	int i;

	EGLint config_attribs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_NONE
	};

	display->egl.dpy = eglGetDisplay(display->display);
	assert(display->egl.dpy);

	ret = eglInitialize(display->egl.dpy, &major, &minor);
	//assert(ret == EGL_TRUE);
	
	printf("EGL major: %d, minor %d\n", major, minor);

	ret = eglBindAPI(EGL_OPENGL_ES_API);
	assert(ret == EGL_TRUE);

	if (!eglGetConfigs(display->egl.dpy, NULL, 0, &count) || count < 1)
                assert(0);

 	printf("EGL has %d configs\n", count);

	configs = calloc(count, sizeof *configs);
	assert(configs);

 	ret = eglChooseConfig(display->egl.dpy, config_attribs,
			      configs, count, &n);
	assert(ret && n >= 1);

	for (i = 0; i < n; i++) {
		eglGetConfigAttrib(display->egl.dpy, configs[i],
				   EGL_BUFFER_SIZE, &size);

	    printf("Buffer size for config %d is %d\n", i, size);

	    // just choose the first one
	    display->egl.conf = configs[i];
	    break;
	}

	free(configs);

	display->egl.ctx = eglCreateContext(display->egl.dpy, display->egl.conf,
				       EGL_NO_CONTEXT, NULL);
	assert(display->egl.ctx);
}

static void create_surface(struct window *window)
{
	struct display *display = window->display;
	int ret;

	window->surface = wl_compositor_create_surface(display->compositor);

	window->shell_surface = wl_shell_get_shell_surface(display->shell,
							   window->surface);

	wl_shell_surface_add_listener(window->shell_surface,
				      &shell_surface_listener, window);

	wl_shell_surface_set_toplevel(window->shell_surface);

	window->native = wl_egl_window_create(window->surface,
					      window->geometry.width,
					      window->geometry.height);

	window->egl_surface = eglCreateWindowSurface(display->egl.dpy,
						     display->egl.conf,
						     window->native, NULL);

	ret = eglMakeCurrent(display->egl.dpy, window->egl_surface,
		             window->egl_surface, display->egl.ctx);
	assert(ret == EGL_TRUE);
}

static void draw_window(struct window *window)
{
	struct display *display = window->display;
	int ret;

	glClearColor (1.0, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	ret = eglSwapBuffers(display->egl.dpy, window->egl_surface);
}

static const struct wl_registry_listener registry_listener = {
	.global = global_add,
	.global_remove = global_remove
};

int main(int argc, char *argv[])
{
	struct display display = {0};
	struct window window = {0};

	display.window = &window;
	window.display = &display;

	window.geometry.width = 250;
	window.geometry.height = 250;

	window.window_size = window.geometry;

	display.display = wl_display_connect(NULL);
	assert(display.display);

	display.registry = wl_display_get_registry(display.display);
	wl_registry_add_listener(display.registry,
				 &registry_listener, &display);

	/* blocking wait for the response from the server */
	wl_display_roundtrip(display.display);

	assert(display.compositor);
	assert(display.shell);

	/* Initialize EGL */
	init_egl(&display, &window);

	create_surface(&window);

	while (running) {
		wl_display_dispatch_pending(display.display);
		draw_window(&window);
	}

	exit(0);
}

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <wayland-client.h>

/* Callbacks for global objects add/remove */
void global_add(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    printf("In global add cb: interface: %s, version: %s\n", interface, version);
}

void global_remove(void *data, struct wl_registry *registry, uint32_t id)
{
    printf("In global remove cb\n");
}

int main(int argc, char *argv[])
{
    /* Get the default wayland server display */
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        printf("Failed to connect to wayland display\n");
        exit(EXIT_FAILURE);
    }
    /* Get wayland registry */
    struct wl_registry *registry = wl_display_get_registry(display);

    struct wl_registry_listener registry_listener = {
        .global = global_add,
        .global_remove = global_remove
    };

    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);

    while(1) {
        ;
    }

    return 0;
}

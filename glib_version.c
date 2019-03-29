#include <stdio.h>
#include <glib.h>

int main(void) {
    printf("GLib version = %d.%d.%d \n", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);

    const gchar *version_check = glib_check_version(GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
    printf("version_check = %s\n", version_check);
    return 0;
}

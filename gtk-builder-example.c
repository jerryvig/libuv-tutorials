#include <stdlib.h>
#include <gtk/gtk.h>

static void decir_hola(GtkWidget *widget, gpointer data) {
    g_print("Hola!\n");
}

static void print_hello(GtkWidget *widget, gpointer data) {
    g_print("Hello!\n");
}

int main(int argc, char **argv) {
    GError *error = NULL;

    gtk_init(&argc, &argv);

    GtkBuilder *builder = gtk_builder_new();
    if (gtk_builder_add_from_file(builder, "builder.ui", &error) == 0) {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
        return EXIT_FAILURE;
    }

    GtkWidget *window = gtk_builder_get_object(builder, "window");
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *button1 = gtk_builder_get_object(builder, "button1");
    g_signal_connect(button1, "clicked", G_CALLBACK(print_hello), NULL);

    GtkWidget *button2 = gtk_builder_get_object(builder, "button2");
    g_signal_connect(button2, "clicked", G_CALLBACK(decir_hola), NULL);

    GtkWidget *quit_button = gtk_builder_get_object(builder, "quit");
    g_signal_connect(quit_button, "clicked", G_CALLBACK(gtk_main_quit), NULL);

    gtk_main();

    return EXIT_SUCCESS;
}

#include <gtk/gtk.h>

static void print_hello(GtkWidget *widget, gpointer data) {
    g_print("Hello world!\n");
}

static void decir_hola(GtkWidget *widget, gpointer data) {
    g_print("Hola mundo!\n");
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Root Window");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    GtkWidget *grid = gtk_grid_new();

    gtk_container_add(GTK_CONTAINER(window), grid);

    GtkWidget *button = gtk_button_new_with_label("Say Hello");
    g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);

    gtk_grid_attach(GTK_GRID(grid), button, 0, 0, 1, 1);

    GtkWidget *button2 = gtk_button_new_with_label("Decir Hola");
    g_signal_connect(button2, "clicked", G_CALLBACK(decir_hola), NULL);

    gtk_grid_attach(GTK_GRID(grid), button2, 1, 0, 1, 1);

    GtkWidget *button3 = gtk_button_new_with_label("Quit");
    g_signal_connect_swapped(button3, "clicked", G_CALLBACK(gtk_widget_destroy), window);

    gtk_grid_attach(GTK_GRID(grid), button3, 0, 1, 2, 1);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

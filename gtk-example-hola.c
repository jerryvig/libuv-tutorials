#include <stdio.h>
#include <gtk/gtk.h>

static int8_t click_counter;

static void print_hola(GtkWidget *widget, gpointer data) {
    click_counter++;
    g_print("Hola mundo! %d veces!\n", click_counter);
}

static void destroy_window(GtkWidget *window) {
    g_print("ABOUT TO DESTROY THE WINDOW\n");
    gtk_widget_destroy(window);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Hola Mundo Ventana");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

    GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_container_add(GTK_CONTAINER(window), button_box);

    GtkWidget *button = gtk_button_new_with_label("Hola Mundo!");
    g_signal_connect(button, "clicked", G_CALLBACK(print_hola), NULL);
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(destroy_window), window);
    gtk_container_add(GTK_CONTAINER(button_box), button);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    click_counter = 0;
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

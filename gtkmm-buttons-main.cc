#include "gtkmm-buttons.h"
#include <gtkmm/application.h>

int main(int argc, char *argv[]) {
    Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

    Buttons buttons;

    return app->run(buttons);
}

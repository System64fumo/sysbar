#include <gtkmm/window.h>

class css_loader : public Glib::RefPtr<Gtk::StyleProvider> {
	public:
		css_loader(const std::string &path, Gtk::Window *window);
};

#include "../module.hpp"

class module_clock : public module {
	public:
		module_clock(const bool &icon_on_start = false, const bool &clickable = false);
		Gtk::Button button_btn;
		Gtk::Label label_date;

	private:
		bool update_info();
};

#pragma once
#include "../module.hpp"
#ifdef MODULE_CELLULAR

#include <giomm/dbusproxy.h>

// Pulled from modem manager
enum MMModemAccessTechnology {
	MM_MODEM_ACCESS_TECHNOLOGY_UNKNOWN		= 0,
	MM_MODEM_ACCESS_TECHNOLOGY_POTS			= 1 << 0,
	MM_MODEM_ACCESS_TECHNOLOGY_GSM			= 1 << 1,
	MM_MODEM_ACCESS_TECHNOLOGY_GSM_COMPACT	= 1 << 2,
	MM_MODEM_ACCESS_TECHNOLOGY_GPRS			= 1 << 3,
	MM_MODEM_ACCESS_TECHNOLOGY_EDGE			= 1 << 4,
	MM_MODEM_ACCESS_TECHNOLOGY_UMTS			= 1 << 5,
	MM_MODEM_ACCESS_TECHNOLOGY_HSDPA		= 1 << 6,
	MM_MODEM_ACCESS_TECHNOLOGY_HSUPA		= 1 << 7,
	MM_MODEM_ACCESS_TECHNOLOGY_HSPA			= 1 << 8,
	MM_MODEM_ACCESS_TECHNOLOGY_HSPA_PLUS	= 1 << 9,
	MM_MODEM_ACCESS_TECHNOLOGY_1XRTT		= 1 << 10,
	MM_MODEM_ACCESS_TECHNOLOGY_EVDO0		= 1 << 11,
	MM_MODEM_ACCESS_TECHNOLOGY_EVDOA		= 1 << 12,
	MM_MODEM_ACCESS_TECHNOLOGY_EVDOB		= 1 << 13,
	MM_MODEM_ACCESS_TECHNOLOGY_LTE			= 1 << 14,
	MM_MODEM_ACCESS_TECHNOLOGY_5GNR			= 1 << 15,
	MM_MODEM_ACCESS_TECHNOLOGY_LTE_CAT_M	= 1 << 16,
	MM_MODEM_ACCESS_TECHNOLOGY_LTE_NB_IOT	= 1 << 17,
	MM_MODEM_ACCESS_TECHNOLOGY_ANY			= 0xFFFFFFFF,
};

class module_cellular : public module {
	public:
		module_cellular(sysbar*, const bool&);

	private:
		Glib::RefPtr<Gio::DBus::Proxy> proxy;

		uint32_t signal = 0;

		void setup();
		void on_properties_changed(
			const Glib::ustring&,
			const Glib::ustring&,
			const Glib::VariantContainerBase&);
		void update_info();
		void extract_data(const Glib::VariantBase&);
		std::string tech_to_icon(const int&);
};

#endif

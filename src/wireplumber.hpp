#pragma once
#include <glibmm/dispatcher.h>
#include <wp/wp.h>

class sys_wireplumber {
	public:
		int volume;
		bool muted;

		sys_wireplumber(Glib::Dispatcher* callback);
		virtual ~sys_wireplumber();

	private:
		Glib::Dispatcher* callback;

		GPtrArray *apis;
		WpCore *core;
		WpObjectManager *om;
		int pending_plugins;

		uint32_t node_id = 0;
		const gchar* node_name;
		WpPlugin *mixer_api;
		WpPlugin *def_nodes_api;

		void activatePlugins();
		static bool isValidNodeId(uint32_t id);
		static void updateVolume(uint32_t id, sys_wireplumber* self);
		static void onMixerChanged(sys_wireplumber* self);
		static void onDefaultNodesApiChanged(sys_wireplumber* self);
		static void onPluginActivated(WpObject* p, GAsyncResult* res, sys_wireplumber* self);
		static void onMixerApiLoaded(WpObject* p, GAsyncResult* res, sys_wireplumber* self);
		static void onDefaultNodesApiLoaded(WpObject* p, GAsyncResult* res, sys_wireplumber* self);
		static void onObjectManagerInstalled(sys_wireplumber* self);
};

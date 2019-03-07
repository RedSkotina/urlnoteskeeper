
/**
 * URL Notes Keeper plugin - keep your notes for detected url in Geany. Adds a menu item to the
 * Tools menu.
 *
 * Note: This is not installed by default, but (on *nix) you can build it as follows:
 * sudo apt install meson
 * sudo apt install libsqlite3-dev
 * sudo apt install geany (1.34.1+)
 * cd plugins/urlnoteskeeper
 * mkdir build
 * meson build
 * ninja -C build
 *
 * Then copy or symlink the plugins/urnoteskeeper.so file to ~/.config/geany/plugins
 * - it will be loaded at next startup.
 */


#include "geanyplugin.h"	/* plugin API, always comes first */
#include "Scintilla.h"	/* for the SCNotification struct */

#include "unkplugin.h"
#include "unkgui.h"
#include "unksidebar.h"
#include "unkdb.h"
#include "unkkeys.h"

PLUGIN_VERSION_CHECK(GEANY_API_VERSION)

GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;

UnkInfo *unk_info = NULL;

static PluginCallback unk_plugin_callbacks[] =
{
	/* Set 'after' (third field) to TRUE to run the callback @a after the default handler.
	 * If 'after' is FALSE, the callback is run @a before the default handler, so the plugin
	 * can prevent Geany from processing the notification. Use this with care. */
	{ "document-open", (GCallback) &on_document_open, TRUE, NULL },
	{ "editor-notify", (GCallback) &unk_gui_editor_notify, FALSE, NULL },
	{ NULL, NULL, FALSE, NULL }
};



static void config_init(void)
{
    GKeyFile *config = g_key_file_new();
	
    unk_info->config_file = g_strconcat(geany->app->configdir, G_DIR_SEPARATOR_S,
		"plugins", G_DIR_SEPARATOR_S, "unk", G_DIR_SEPARATOR_S, "general.conf", NULL);

	g_key_file_load_from_file(config, unk_info->config_file, G_KEY_FILE_NONE, NULL);

    unk_info->enable_urls_detect_on_open_document = utils_get_setting_boolean(
		config, "general", "enable_urls_detect_on_open_document", FALSE);

    unk_info->enable_db_detect_on_open_document = utils_get_setting_boolean(
		config, "general", "enable_db_detect_on_open_document", TRUE);

	unk_info->db_path = utils_get_setting_string(
		config, "general", "db_path", "~/unk.sqlite3");
	
	unk_info->positive_rating_color = g_malloc0(sizeof(GdkRGBA));
	gchar* positive_rating_color_s = utils_get_setting_string(
		config, "colors", "positive_rating_color", "rgba(0,255,0,0.6)");
	
	if (!gdk_rgba_parse (unk_info->positive_rating_color, positive_rating_color_s))
	{
		g_print("cant parse positive_rating_color string '%s'",positive_rating_color_s);
		gdk_rgba_parse (unk_info->positive_rating_color, "rgba(0,255,0,0.6)");
	};
	g_free(positive_rating_color_s);
	
	
	unk_info->neutral_rating_color = g_malloc0(sizeof(GdkRGBA));
	gchar* neutral_rating_color_s = utils_get_setting_string(
		config, "colors", "neutral_rating_color", "rgba(0,0,255,0.6)");
	if (!gdk_rgba_parse (unk_info->neutral_rating_color, neutral_rating_color_s))
	{
		g_print("cant parse neutral_rating_color string '%s'",neutral_rating_color_s);
		gdk_rgba_parse (unk_info->neutral_rating_color, "rgba(0,0,255,0.6)");
	};
	g_free(neutral_rating_color_s);
	
	unk_info->negative_rating_color = g_malloc0(sizeof(GdkRGBA));
	gchar* negative_rating_color_s = utils_get_setting_string(
		config, "colors", "negative_rating_color", "rgba(255,0,0,0.6)");
	if (!gdk_rgba_parse (unk_info->negative_rating_color, negative_rating_color_s))
	{
		g_print("cant parse negative_rating_color string '%s'",negative_rating_color_s);
		gdk_rgba_parse (unk_info->negative_rating_color, "rgba(255,0,0,0.6)");
	};
	g_free(negative_rating_color_s);
	
	g_key_file_free(config);
}

/* Called by Geany to initialize the plugin */
static gboolean unk_plugin_init(GeanyPlugin *plugin, gpointer data)
{
	geany_plugin = plugin;
	geany_data = plugin->geany_data;
	
	config_widgets = g_malloc (sizeof (ConfigWidgets));
	unk_info = g_malloc (sizeof (UnkInfo));
	
	config_init();
	
	keys_init();
	
	menu_init();
	
	/* This might seem strange but is a method to get the GeanyPlugin pointer passed to
	 * on_editor_notify(). PluginCallback functions get the same data that was set via
	 * GEANY_PLUING_REGISTER_FULL() or geany_plugin_set_data() by default (unless the data pointer
	 * was set to non-NULL at compile time).
	 * This is really only done for demoing PluginCallback. Actual plugins will use real custom
	 * data and perhaps embed the GeanyPlugin or GeanyData pointer their if they also use
	 * PluginCallback. */
	geany_plugin_set_data(plugin, plugin, NULL);
	
	unk_db_init(unk_info->db_path);
	sidebar_init(plugin);
	
	return TRUE;
}

static GtkWidget *unk_plugin_configure(GeanyPlugin *plugin, GtkDialog *dialog, gpointer data)
{
	return create_configure_widget(plugin, dialog, data);
}


/* Called by Geany before unloading the plugin.
 * Here any UI changes should be removed, memory freed and any other finalization done.
 * Be sure to leave Geany as it was before plugin_init(). */
static void unk_plugin_cleanup(GeanyPlugin *plugin, gpointer data)
{
	unk_db_cleanup();
	sidebar_cleanup();
	menu_cleanup();
	g_free(unk_info->positive_rating_color);
	g_free(unk_info->neutral_rating_color);
	g_free(unk_info->negative_rating_color);
	g_free(unk_info);
	g_free(config_widgets);
}

void geany_load_module(GeanyPlugin *plugin)
{
	/* main_locale_init() must be called for your package before any localization can be done */
	main_locale_init(LOCALEDIR, GETTEXT_PACKAGE);
	plugin->info->name = _("URLNotesKeeper");
	plugin->info->description = _(" Edit and keep your notes for any url in text.");
	plugin->info->version = "0.2";
	plugin->info->author =  _("RedSkotina");

	plugin->funcs->init = unk_plugin_init;
	plugin->funcs->configure = unk_plugin_configure;
	plugin->funcs->help = NULL; /* This plugin has no help but it is an option */
	plugin->funcs->cleanup = unk_plugin_cleanup;
	plugin->funcs->callbacks = unk_plugin_callbacks;

	GEANY_PLUGIN_REGISTER(plugin, 225);
}

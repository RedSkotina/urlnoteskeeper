
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

PLUGIN_VERSION_CHECK(GEANY_API_VERSION)

GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;

static GtkWidget *main_menu_item = NULL;
UnkInfo *unk_info = NULL;
ConfigWidgets* config_widgets = NULL;

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

	//g_print("%s", unk_info->config_file);
    
    g_key_file_load_from_file(config, unk_info->config_file, G_KEY_FILE_NONE, NULL);

    unk_info->enable_parse_on_open_document = utils_get_setting_boolean(
		config, "general", "enable_parse_on_open_document", TRUE);
	
	unk_info->db_path = utils_get_setting_string(
		config, "general", "db_path", "~/unk.db");

	 g_key_file_free(config);
}

/* Called by Geany to initialize the plugin */
static gboolean unk_plugin_init(GeanyPlugin *plugin, gpointer data)
{
	GtkWidget *unk_menu_item;
	geany_plugin = plugin;
	geany_data = plugin->geany_data;
	
	config_widgets = g_malloc (sizeof (config_widgets));
	unk_info = g_malloc (sizeof (unk_info));
	
	config_init();
	
	/* Add an item to the Tools menu */
	unk_menu_item = gtk_menu_item_new_with_mnemonic(_("_Url Notes Keeper"));
	gtk_widget_show(unk_menu_item);
	gtk_container_add(GTK_CONTAINER(geany_data->main_widgets->tools_menu), unk_menu_item);
	g_signal_connect(unk_menu_item, "activate", G_CALLBACK(item_activate), plugin);

	/* make the menu item sensitive only when documents are open */
	ui_add_document_sensitive(unk_menu_item);
	/* keep a pointer to the menu item, so we can remove it when the plugin is unloaded */
	main_menu_item = unk_menu_item;

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
	GtkWidget *label_db_path, *entry_db_path, *vbox, *checkbox_enable_parse_on_open_document;

	vbox = gtk_vbox_new(FALSE, 6);

	{
		label_db_path = gtk_label_new(_("Database:"));
		entry_db_path = gtk_entry_new();
		if (unk_info->db_path != NULL)
			gtk_entry_set_text(GTK_ENTRY(entry_db_path), unk_info->db_path);
		config_widgets->entry_db_path = entry_db_path;
		
		gtk_container_add(GTK_CONTAINER(vbox), label_db_path);
		gtk_container_add(GTK_CONTAINER(vbox), entry_db_path);
	}
	
	{
		checkbox_enable_parse_on_open_document = gtk_check_button_new_with_mnemonic(_("_Enable parse on open document"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_enable_parse_on_open_document), unk_info->enable_parse_on_open_document);
		gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_enable_parse_on_open_document), FALSE);
		config_widgets->checkbox_enable_parse_on_open_document = checkbox_enable_parse_on_open_document;
		gtk_box_pack_start(GTK_BOX(vbox), checkbox_enable_parse_on_open_document, FALSE, FALSE, 6);
	}
	
	gtk_widget_show_all(vbox);

	g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), NULL);
	return vbox;
}


/* Called by Geany before unloading the plugin.
 * Here any UI changes should be removed, memory freed and any other finalization done.
 * Be sure to leave Geany as it was before plugin_init(). */
static void unk_plugin_cleanup(GeanyPlugin *plugin, gpointer data)
{
	/* remove the menu item added in demo_init() */
	gtk_widget_destroy(main_menu_item);
	
	sidebar_cleanup();
	unk_db_cleanup();
	g_free(unk_info);
	g_free(config_widgets);
}

void geany_load_module(GeanyPlugin *plugin)
{
	/* main_locale_init() must be called for your package before any localization can be done */
	main_locale_init(LOCALEDIR, GETTEXT_PACKAGE);
	plugin->info->name = _("URLNotesKeeper");
	plugin->info->description = _(" Edit and keep your notes for any url in text.");
	plugin->info->version = "0.1";
	plugin->info->author =  _("Dmitry Naumov");

	plugin->funcs->init = unk_plugin_init;
	plugin->funcs->configure = unk_plugin_configure;
	plugin->funcs->help = NULL; /* This plugin has no help but it is an option */
	plugin->funcs->cleanup = unk_plugin_cleanup;
	plugin->funcs->callbacks = unk_plugin_callbacks;

	GEANY_PLUGIN_REGISTER(plugin, 225);
}


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
#include "unkdb.h"

PLUGIN_VERSION_CHECK(GEANY_API_VERSION)

static GtkWidget *main_menu_item = NULL;
UnkInfo *unk_info = NULL;

static PluginCallback unk_plugin_callbacks[] =
{
	/* Set 'after' (third field) to TRUE to run the callback @a after the default handler.
	 * If 'after' is FALSE, the callback is run @a before the default handler, so the plugin
	 * can prevent Geany from processing the notification. Use this with care. */
	{ "editor-notify", (GCallback) &unk_gui_editor_notify, FALSE, NULL },
	{ NULL, NULL, FALSE, NULL }
};




/* Called by Geany to initialize the plugin */
static gboolean unk_plugin_init(GeanyPlugin *plugin, gpointer data)
{
	GtkWidget *unk_menu_item;
	GeanyData *geany_data = plugin->geany_data;

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
	
	unk_db_init("/home/red/unk.db");
	sidebar_init(plugin);
	
	return TRUE;
}


/* Called by Geany to show the plugin's configure dialog. This function is always called after
 * plugin_init() was called.
 * You can omit this function if the plugin doesn't need to be configured.
 * Note: parent is the parent window which can be used as the transient window for the created
 *       dialog. */
static GtkWidget *unk_plugin_configure(GeanyPlugin *plugin, GtkDialog *dialog, gpointer data)
{
	GtkWidget *label, *entry, *vbox;

	/* example configuration dialog */
	vbox = gtk_vbox_new(FALSE, 6);

	/* add a label and a text entry to the dialog */
	label = gtk_label_new(_("Welcome text to show:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	entry = gtk_entry_new();
	//if (welcome_text != NULL)
	//	gtk_entry_set_text(GTK_ENTRY(entry), welcome_text);

	gtk_container_add(GTK_CONTAINER(vbox), label);
	gtk_container_add(GTK_CONTAINER(vbox), entry);

	gtk_widget_show_all(vbox);

	/* Connect a callback for when the user clicks a dialog button */
	g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), entry);
	return vbox;
}


/* Called by Geany before unloading the plugin.
 * Here any UI changes should be removed, memory freed and any other finalization done.
 * Be sure to leave Geany as it was before plugin_init(). */
static void unk_plugin_cleanup(GeanyPlugin *plugin, gpointer data)
{
	/* remove the menu item added in demo_init() */
	gtk_widget_destroy(main_menu_item);
	/* release other allocated strings and objects */
	//g_free(welcome_text);
	
	sidebar_cleanup();
	unk_db_cleanup();
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

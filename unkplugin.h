#ifndef UNK_PLUGIN_H
#define UNK_PLUGIN_H 1
typedef struct
{
	gchar *config_file;
	gchar *default_language;
	gchar *dictionary_dir;
	gboolean use_msgwin;
	gboolean check_while_typing;
	gboolean check_on_document_open;
	gboolean show_toolbar_item;
	gboolean show_editor_menu_item;
	gboolean show_editor_menu_item_sub_menu;
	GPtrArray *dicts;
	GtkWidget *main_menu;
	GtkWidget *menu_item;
	GtkWidget *submenu_item_default;
	GtkWidget *edit_menu;
	GtkWidget *edit_menu_sep;
	GtkWidget *edit_menu_sub;
	GtkToolItem *toolbar_button;
	GSList *edit_menu_items;
} UnkInfo;


extern UnkInfo		*unk_info;
extern GeanyPlugin		*geany_plugin;
extern GeanyData		*geany_data;
#endif

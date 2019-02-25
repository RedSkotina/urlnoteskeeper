#ifndef UNK_GUI_H
#define UNK_GUI_H 1

typedef struct {
 GtkWidget* checkbox_enable_urls_detect_on_open_document;
 GtkWidget* checkbox_enable_db_detect_on_open_document;
 GtkWidget* entry_db_path;
} ConfigWidgets;

extern ConfigWidgets* config_widgets;	

void set_mark(GeanyEditor *editor, gint range_start_pos, gint range_end_pos);

void set_url_marks(GeanyEditor *editor, gint range_start_pos, gint range_end_pos);

void set_db_marks(GeanyEditor *editor, gint range_start_pos, gint range_end_pos);

void set_marks(GeanyEditor *editor, gint range_start_pos, gint range_end_pos);

void on_document_open(GObject *obj, GeanyDocument *doc, gpointer user_data);

gboolean unk_gui_editor_notify(GObject *object, GeanyEditor *editor,
							  SCNotification *nt, gpointer data);

void item_activate(GtkMenuItem *menuitem, gpointer gdata);	

void on_configure_response(GtkDialog *dialog, gint response, gpointer user_data);

GtkWidget *create_configure_widget(GeanyPlugin *plugin, GtkDialog *dialog, gpointer data);

typedef struct
{
	GtkWidget* main_menu;
	GtkWidget* submenu_item_urls_detect;
	GtkWidget* submenu_item_db_detect;
	
} MenuWidgets;


extern MenuWidgets	*menu_widgets;

void menu_init(void);

void menu_cleanup(void);

					  
#endif

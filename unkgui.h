#ifndef UNK_GUI_H
#define UNK_GUI_H 1

extern gint GEANY_INDICATOR_UNK_AUTO_DETECTED;
extern gint GEANY_INDICATOR_UNK_POSITIVE_DB;
extern gint GEANY_INDICATOR_UNK_NEUTRAL_DB;
extern gint GEANY_INDICATOR_UNK_NEGATIVE_DB;

typedef struct MarkInfo{
 gint start;
 gint end;
 gint rating;
} MarkInfo;

typedef struct ConfigWidgets{
 GtkWidget* checkbox_enable_urls_detect_on_open_document;
 GtkWidget* checkbox_enable_db_detect_on_open_document;
 GtkWidget* entry_db_path;
 GtkColorButton * button_positive_rating_color;
 GtkColorButton * button_neutral_rating_color;
 GtkColorButton * button_negative_rating_color;
  
} ConfigWidgets;

extern ConfigWidgets* config_widgets;	

void set_mark(GeanyEditor *editor, gint range_start_pos, gint range_end_pos, gint rating) ;

void set_url_marks(GeanyEditor *editor, gint range_start_pos, gint range_end_pos);

void set_db_marks(GeanyEditor *editor, gint range_start_pos, gint range_end_pos);

void set_marks(GeanyEditor *editor, gint range_start_pos, gint range_end_pos);

void clear_all_db_marks(GeanyEditor *editor);

void on_document_open(GObject *obj, GeanyDocument *doc, gpointer user_data);

gboolean unk_gui_editor_notify(GObject *object, GeanyEditor *editor,
							  SCNotification *nt, gpointer data);

void item_activate(GtkMenuItem *menuitem, gpointer gdata);	

void on_configure_response(GtkDialog *dialog, gint response, gpointer user_data);

GtkWidget *create_configure_widget(GeanyPlugin *plugin, GtkDialog *dialog, gpointer data);

typedef struct MenuWidgets
{
	GtkWidget* main_menu;
	GtkWidget* submenu_item_urls_detect;
	GtkWidget* submenu_item_db_detect;
	
} MenuWidgets;


extern MenuWidgets	*menu_widgets;

void menu_init(void);

void menu_cleanup(void);

					  
#endif

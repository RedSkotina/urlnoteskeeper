#ifndef UNK_GUI_H
#define UNK_GUI_H 1

typedef struct {
 GtkWidget* checkbox_enable_parse_on_open_document;
 GtkWidget* entry_db_path;
} ConfigWidgets;

void set_mark(GeanyEditor *editor, gint range_start_pos, gint range_end_pos);

void set_marks(GeanyEditor *editor, gint range_start_pos, gint range_end_pos);

void on_document_open(GObject *obj, GeanyDocument *doc, gpointer user_data);

gboolean unk_gui_editor_notify(GObject *object, GeanyEditor *editor,
							  SCNotification *nt, gpointer data);


void item_activate(GtkMenuItem *menuitem, gpointer gdata);	
void on_configure_response(GtkDialog *dialog, gint response, gpointer user_data);

extern ConfigWidgets* config_widgets;						  
#endif

#ifndef UNK_GUI_H
#define UNK_GUI_H 1

gboolean unk_gui_editor_notify(GObject *object, GeanyEditor *editor,
							  SCNotification *nt, gpointer data);


void item_activate(GtkMenuItem *menuitem, gpointer gdata);	
void on_configure_response(GtkDialog *dialog, gint response, gpointer user_data);
						  
#endif

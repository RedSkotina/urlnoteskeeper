#include <geanyplugin.h>

#include <ctype.h>
#include <string.h>

#include "unkgui.h"
#include "unksidebar.h"
#include "unkdb.h"

//static gchar* unk_sidebar_group_name = "urlnotes";	

typedef struct SIDEBAR
{
    GtkWidget *unk_view_vbox;
    GtkWidget *unk_view_label;
    GtkWidget *unk_text_view;
    GtkWidget *radio1, *radio2, *radio3, *rating_box;
	 
}SIDEBAR;

static SIDEBAR sidebar = {NULL, NULL, NULL};


gboolean sidebar_unk_text_view_on_focus_out (GtkWidget *widget, GdkEvent  *event, G_GNUC_UNUSED gpointer user_data)
{
	GtkTextIter start;
    GtkTextIter end;
    
    GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (sidebar.unk_text_view));
	
	if (gtk_text_buffer_get_modified (buffer) ) 
    {
		const gchar* const_key = gtk_label_get_text(GTK_LABEL(sidebar.unk_view_label));
		gchar* key = g_strdup(const_key);
		
		gtk_widget_set_sensitive (sidebar.unk_text_view, FALSE);
		gtk_text_buffer_get_start_iter (buffer, &start);
		gtk_text_buffer_get_end_iter (buffer, &end);
		gchar* value = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		
		gint rating = 0;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio1)))
			rating = -1;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio2)))
			rating = 0;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio3)))
			rating = 1;
		
		unk_db_set(key, value, rating);
		
		gtk_text_buffer_set_modified (buffer, FALSE);
		gtk_widget_set_sensitive (sidebar.unk_text_view, TRUE);
		g_free(key);
		g_free(value);
		
		GeanyDocument *doc = document_get_current();
		if (doc)
		{
			clear_all_db_marks(doc->editor);
			
			gint doc_length = sci_get_length(doc->editor->sci);
			if ( doc_length > 0)
			{
				set_db_marks(doc->editor, 0, doc_length); //TODO: more precise at update
			}
		}
		
	}
	return FALSE;
}

void sidebar_set_url(gpointer text)
{
	gtk_label_set_label(GTK_LABEL(sidebar.unk_view_label), text);
}

void sidebar_set_note(gpointer text)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (sidebar.unk_text_view));
	gtk_text_buffer_set_text(buffer, text, strlen(text));	
}

void sidebar_set_rating(gint rating)
{
	switch (rating) {
		case -1:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio1), TRUE);
			break;
		case 0:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio2), TRUE);
   			break;
		case 1:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio3), TRUE);
			break;
		default:
			g_print("wrong rating %d", rating);
			
	}
}

void sidebar_show(GeanyPlugin* geany_plugin)
{
	gtk_widget_show_all(sidebar.unk_view_vbox);
	
	gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), sidebar.unk_view_vbox);
	if (page_num >= 0) 
		gtk_notebook_set_current_page(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), page_num);
	else
		ui_set_statusbar(TRUE, "error: cant find url notes page in sidebar notebook");
	
}

static void radio_button_on_toggle (GtkToggleButton *source, gpointer user_data) {
  //printf ("Active: %d\n", gtk_toggle_button_get_active (source));
	gint rating = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio1)))
		rating = -1;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio2)))
		rating = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio3)))
		rating = 1;

	if (gtk_toggle_button_get_active (source)) {
		
		GtkTextIter start;
		GtkTextIter end;
    
		GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (sidebar.unk_text_view));
	
		const gchar* const_key = gtk_label_get_text(GTK_LABEL(sidebar.unk_view_label));
		gchar* key = g_strdup(const_key);
		
		gtk_text_buffer_get_start_iter (buffer, &start);
		gtk_text_buffer_get_end_iter (buffer, &end);
		gchar* value = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		
		unk_db_set(key, value, rating);
		
		g_free(key);
		g_free(value);
		
		GeanyDocument *doc = document_get_current();
		if (doc)
		{
			clear_all_db_marks(doc->editor);
			
			gint doc_length = sci_get_length(doc->editor->sci);
			if ( doc_length > 0)
			{
				set_db_marks(doc->editor, 0, doc_length); //TODO: more precise at update
			}
		}
		
	}
}

void sidebar_init(GeanyPlugin* geany_plugin)
{
	GtkWidget *scrollwin;
	GtkTextBuffer *buffer;

	sidebar.unk_view_vbox = gtk_vbox_new(FALSE, 0);

	/**** label ****/
	sidebar.unk_view_label = gtk_label_new (_("No url selected."));
	gtk_box_pack_start(GTK_BOX(sidebar.unk_view_vbox), sidebar.unk_view_label, FALSE, FALSE, 0);

	/**** radio buttons ****/
	//GtkWidget *radio1, *radio2, *radio3, *rating_box;
	 
	sidebar.rating_box = gtk_hbox_new (GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_box_set_homogeneous (GTK_BOX (sidebar.rating_box), TRUE);

	sidebar.radio1 = gtk_radio_button_new_with_mnemonic  ( NULL, "-");
	sidebar.radio2 = gtk_radio_button_new_with_label_from_widget   ( GTK_RADIO_BUTTON (sidebar.radio1), " ");
	sidebar.radio3 = gtk_radio_button_new_with_label_from_widget   ( GTK_RADIO_BUTTON (sidebar.radio1), "+");
	
	gtk_widget_set_size_request(sidebar.radio1, 80, 20);
	gtk_widget_set_size_request(sidebar.radio2, 80, 20);
	gtk_widget_set_size_request(sidebar.radio3, 80, 20);

	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(sidebar.radio1), FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(sidebar.radio2), FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(sidebar.radio3), FALSE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio2), TRUE);
   
	g_signal_connect (GTK_TOGGLE_BUTTON (sidebar.radio1), "toggled", G_CALLBACK (radio_button_on_toggle), NULL);
	g_signal_connect (GTK_TOGGLE_BUTTON (sidebar.radio2), "toggled", G_CALLBACK (radio_button_on_toggle), NULL);
	g_signal_connect (GTK_TOGGLE_BUTTON (sidebar.radio3), "toggled", G_CALLBACK (radio_button_on_toggle), NULL);

	gtk_box_pack_start (GTK_BOX (sidebar.rating_box), sidebar.radio1, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (sidebar.rating_box), sidebar.radio2, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (sidebar.rating_box), sidebar.radio3, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(sidebar.unk_view_vbox), sidebar.rating_box, FALSE, FALSE, 0);

	/**** text ****/
	sidebar.unk_text_view = gtk_text_view_new ();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(sidebar.unk_text_view),GTK_WRAP_WORD);
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (sidebar.unk_text_view));
	gtk_text_buffer_set_text (buffer, "", -1);
	gtk_widget_add_events(sidebar.unk_text_view, GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(G_OBJECT(sidebar.unk_text_view), "focus-out-event",
			G_CALLBACK(sidebar_unk_text_view_on_focus_out), NULL);
	/**** the rest ****/
	//focus_chain = g_list_prepend(focus_chain, sidebar.unk_text_view);
	//gtk_container_set_focus_chain(GTK_CONTAINER(sidebar.unk_view_vbox), focus_chain);
	//g_list_free(focus_chain);
	
	scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
					   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollwin), sidebar.unk_text_view);
	gtk_box_pack_start(GTK_BOX(sidebar.unk_view_vbox), scrollwin, TRUE, TRUE, 0);

	gtk_widget_show_all(sidebar.unk_view_vbox);
	gtk_notebook_append_page(
		GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook),
		sidebar.unk_view_vbox,
		gtk_label_new(_("Url Notes")));
	
	//gtk_notebook_set_group_name (GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), unk_sidebar_group_name);
	
	sidebar_deactivate();			 

}

void sidebar_activate(void)
{
	gtk_widget_set_sensitive(sidebar.unk_view_vbox, TRUE);
}


void sidebar_deactivate(void)
{
	gtk_widget_set_sensitive(sidebar.unk_view_vbox, FALSE);
}

void sidebar_cleanup(void)
{
	gtk_widget_destroy(sidebar.unk_view_vbox);
}


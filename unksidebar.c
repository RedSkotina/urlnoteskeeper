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
}SIDEBAR;

static SIDEBAR sidebar = {NULL, NULL, NULL};

gboolean sidebar_unk_text_view_on_focus_out (GtkWidget *widget, GdkEvent  *event, G_GNUC_UNUSED gpointer user_data)
{
	GtkTextIter start;
    GtkTextIter end;
    
    GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (sidebar.unk_text_view));
	
	if (gtk_text_buffer_get_modified (buffer)) 
    {
		const gchar* const_key = gtk_label_get_text(GTK_LABEL(sidebar.unk_view_label));
		gchar* key = g_strdup(const_key);
		
		gtk_widget_set_sensitive (sidebar.unk_text_view, FALSE);
		gtk_text_buffer_get_start_iter (buffer, &start);
		gtk_text_buffer_get_end_iter (buffer, &end);
		gchar* value = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		unk_db_set(key, value);
		gtk_text_buffer_set_modified (buffer, FALSE);
		gtk_widget_set_sensitive (sidebar.unk_text_view, TRUE);
		g_free(key);
		g_free(value);
		
		GeanyDocument *doc = document_get_current();
		if (doc)
		{
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

void sidebar_show(GeanyPlugin* geany_plugin)
{
	gtk_widget_show_all(sidebar.unk_view_vbox);
	
	gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), sidebar.unk_view_vbox);
	if (page_num >= 0) 
		gtk_notebook_set_current_page(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), page_num);
	else
		ui_set_statusbar(TRUE, "error: cant find url notes page in sidebar notebook");
	
}

void sidebar_init(GeanyPlugin* geany_plugin)
{
	GtkWidget *scrollwin;
	GtkTextBuffer *buffer;

	sidebar.unk_view_vbox = gtk_vbox_new(FALSE, 0);

	/**** label ****/
	sidebar.unk_view_label = gtk_label_new (_("No url selected."));
	gtk_box_pack_start(GTK_BOX(sidebar.unk_view_vbox), sidebar.unk_view_label, FALSE, FALSE, 0);

	/**** text ****/
	sidebar.unk_text_view = gtk_text_view_new ();
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


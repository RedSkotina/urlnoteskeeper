#include <geanyplugin.h>

#include <ctype.h>
#include <string.h>
#include "gtk3compat.h"
#include "unkplugin.h"
#include "unkgui.h"
#include "unksidebar.h"
#include "unkdb.h"

//static gchar* unk_sidebar_group_name = "urlnotes";	

static PangoFontDescription *thight_font_desc = NULL;

typedef struct SIDEBAR
{
    GtkWidget *unk_view_vbox;
    GtkWidget *unk_view_label;
    GtkWidget *unk_text_view;
    GtkWidget *rating_eventbox;
    GtkWidget *radio1, *radio2, *radio3, *rating_box_1;
    GtkWidget *radio4, *radio5, *radio6, *rating_box_2;
    GtkWidget *rating_vbox;
    GtkWidget *frame, *vbox;
	 
}SIDEBAR;

static SIDEBAR sidebar = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

typedef struct TextEntry
{
    GtkWidget *main_box;
    GtkWidget *rating_eventbox;
    GtkWidget *rating_label;
    GtkWidget *text_view;
    GtkWidget *frame;
}TextEntry;

gboolean sidebar_unk_text_view_on_focus_out (GtkWidget *widget, GdkEvent  *event, G_GNUC_UNUSED gpointer user_data)
{
    g_debug("sidebar_unk_text_view_on_focus_out");
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
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio4)))
			rating = 2;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio5)))
			rating = 3;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio6)))
			rating = 4;
		
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
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    switch (rating) 
    {
		case -1:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio1), TRUE);
			
            gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->negative_rating_color);
			break;
		case 0:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio2), TRUE);
   			
            gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->neutral_rating_color);
			break;
		case 1:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio3), TRUE);
			
            gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->positive_rating_color);
			break;
		case 2:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio4), TRUE);
			
            gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->rating_color_2);
			break;
		case 3:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio5), TRUE);
			
            gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->rating_color_3);
			break;
		case 4:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio6), TRUE);
			
            gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->rating_color_4);
			break;
		default:
			g_warning("wrong rating %d", rating);
			
	}
	G_GNUC_END_IGNORE_DEPRECATIONS
}

void sidebar_show(GeanyPlugin* geany_plugin)
{
	gtk_widget_show_all(sidebar.unk_view_vbox);
	
	gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), sidebar.unk_view_vbox);
	if (page_num >= 0) 
		gtk_notebook_set_current_page(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), page_num);
	else 
    {   
		g_warning("error: cant find url notes page in sidebar notebook");
    	msgwin_status_add_string("error: cant find 'url notes' page in sidebar notebook");
    }
}

static void radio_button_on_toggle (GtkToggleButton *source, gpointer user_data) {
 	g_debug("radio_button_on_toggle");
    gint rating = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio1)))
		rating = -1;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio2)))
		rating = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio3)))
		rating = 1;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio4)))
		rating = 2;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio5)))
		rating = 3;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidebar.radio6)))
		rating = 4;

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    switch (rating) 
    {
		case -1:
			gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->negative_rating_color);
			break;
		case 0:
			gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->neutral_rating_color);
			break;
		case 1:
			gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->positive_rating_color);
			break;
		case 2:
			gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->rating_color_2);
			break;
		case 3:
			gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->rating_color_3);
			break;
		case 4:
			gtk_widget_override_background_color(sidebar.rating_eventbox, GTK_STATE_NORMAL, unk_info->rating_color_4);
			break;
		default:
			g_warning("wrong rating %d", rating);
	}
	G_GNUC_END_IGNORE_DEPRECATIONS
	
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
	g_debug("sidebar_init");
        
    GtkWidget *scrollwin;
	GtkTextBuffer *buffer;

	sidebar.unk_view_vbox = gtk_vbox_new(FALSE, 0);

    /**** frame ****/
	sidebar.frame = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(sidebar.frame), 2);
    
	/**** label ****/
	GtkWidget* url_frame = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(url_frame), 2);
    
    sidebar.unk_view_label = gtk_label_new (_("No url selected."));
	//gtk_box_pack_start(GTK_BOX(sidebar.unk_view_vbox), sidebar.unk_view_label, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(url_frame), sidebar.unk_view_label);
    gtk_box_pack_start(GTK_BOX(sidebar.unk_view_vbox), url_frame, FALSE, FALSE, 0);

    sidebar.vbox = gtk_vbox_new(FALSE, 0);
	
    /**** color label ****/
	
    sidebar.rating_eventbox = gtk_event_box_new(); 
    gtk_event_box_set_above_child(GTK_EVENT_BOX(sidebar.rating_eventbox), FALSE); 
    
    GtkWidget *rating_label = gtk_label_new ("");
	
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gtk_widget_override_font(rating_label, thight_font_desc);
	G_GNUC_END_IGNORE_DEPRECATIONS
		
	gtk_widget_set_size_request(rating_label, -1, 5);
	
    gtk_container_add(GTK_CONTAINER(sidebar.rating_eventbox),rating_label);
    gtk_box_pack_start(GTK_BOX(sidebar.vbox), sidebar.rating_eventbox, FALSE, FALSE, 1);
    
	/**** radio buttons ****/
	sidebar.rating_vbox = gtk_vbox_new (GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_set_homogeneous (GTK_BOX (sidebar.rating_vbox), TRUE);

	sidebar.rating_box_1 = gtk_hbox_new (GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_box_set_homogeneous (GTK_BOX (sidebar.rating_box_1), TRUE);

	sidebar.rating_box_2 = gtk_hbox_new (GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_box_set_homogeneous (GTK_BOX (sidebar.rating_box_2), TRUE);

	sidebar.radio1 = gtk_radio_button_new_with_mnemonic  ( NULL, "-");
	sidebar.radio2 = gtk_radio_button_new_with_label_from_widget   ( GTK_RADIO_BUTTON (sidebar.radio1), " ");
	sidebar.radio3 = gtk_radio_button_new_with_label_from_widget   ( GTK_RADIO_BUTTON (sidebar.radio1), "+");
	sidebar.radio4 = gtk_radio_button_new_with_label_from_widget   ( GTK_RADIO_BUTTON (sidebar.radio1), "2");
	sidebar.radio5 = gtk_radio_button_new_with_label_from_widget   ( GTK_RADIO_BUTTON (sidebar.radio1), "3");
	sidebar.radio6 = gtk_radio_button_new_with_label_from_widget   ( GTK_RADIO_BUTTON (sidebar.radio1), "4");
	
	gtk_widget_set_size_request(sidebar.radio1, 80, 20);
	gtk_widget_set_size_request(sidebar.radio2, 80, 20);
	gtk_widget_set_size_request(sidebar.radio3, 80, 20);
	gtk_widget_set_size_request(sidebar.radio4, 80, 20);
	gtk_widget_set_size_request(sidebar.radio5, 80, 20);
	gtk_widget_set_size_request(sidebar.radio6, 80, 20);

	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(sidebar.radio1), FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(sidebar.radio2), FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(sidebar.radio3), FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(sidebar.radio4), FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(sidebar.radio5), FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(sidebar.radio6), FALSE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio2), TRUE);
   
	g_signal_connect (GTK_TOGGLE_BUTTON (sidebar.radio1), "toggled", G_CALLBACK (radio_button_on_toggle), NULL);
	g_signal_connect (GTK_TOGGLE_BUTTON (sidebar.radio2), "toggled", G_CALLBACK (radio_button_on_toggle), NULL);
	g_signal_connect (GTK_TOGGLE_BUTTON (sidebar.radio3), "toggled", G_CALLBACK (radio_button_on_toggle), NULL);
	g_signal_connect (GTK_TOGGLE_BUTTON (sidebar.radio4), "toggled", G_CALLBACK (radio_button_on_toggle), NULL);
	g_signal_connect (GTK_TOGGLE_BUTTON (sidebar.radio5), "toggled", G_CALLBACK (radio_button_on_toggle), NULL);
	g_signal_connect (GTK_TOGGLE_BUTTON (sidebar.radio6), "toggled", G_CALLBACK (radio_button_on_toggle), NULL);

	gtk_box_pack_start (GTK_BOX (sidebar.rating_box_1), sidebar.radio1, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (sidebar.rating_box_1), sidebar.radio2, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (sidebar.rating_box_1), sidebar.radio3, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (sidebar.rating_box_2), sidebar.radio4, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (sidebar.rating_box_2), sidebar.radio5, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (sidebar.rating_box_2), sidebar.radio6, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(sidebar.rating_vbox), sidebar.rating_box_1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(sidebar.rating_vbox), sidebar.rating_box_2, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(sidebar.vbox), sidebar.rating_vbox, FALSE, FALSE, 0);
    
	/**** text ****/
	sidebar.unk_text_view = gtk_text_view_new ();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(sidebar.unk_text_view),GTK_WRAP_WORD);
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (sidebar.unk_text_view));
	gtk_text_buffer_set_text (buffer, "", -1);
	gtk_widget_add_events(sidebar.unk_text_view, GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(G_OBJECT(sidebar.unk_text_view), "focus-out-event",
			G_CALLBACK(sidebar_unk_text_view_on_focus_out), NULL);
	/**** the rest ****/
	
	scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
					   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollwin), sidebar.unk_text_view);
	gtk_box_pack_start(GTK_BOX(sidebar.vbox), scrollwin, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(sidebar.frame), sidebar.vbox);
    
    gtk_box_pack_start(GTK_BOX(sidebar.unk_view_vbox), sidebar.frame, TRUE, TRUE, 0);
    
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
	g_debug("sidebar_cleanup");
    gtk_widget_destroy(sidebar.unk_view_vbox);
}


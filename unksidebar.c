#include <geanyplugin.h>

#include <ctype.h>
#include <string.h>
#include "gtk3compat.h"
#include "unkplugin.h"
#include "unkgui.h"
#include "unksidebar.h"
#include "unkdb.h"

//static gchar* unk_sidebar_group_name = "urlnotes";	

typedef struct SIDEBAR
{
    GtkWidget *unk_view_vbox;
    GtkWidget *unk_view_label;
    GtkWidget *unk_text_view;
    GtkWidget *rating_eventbox;
    GtkWidget *radio1, *radio2, *radio3, *rating_box;
    GtkWidget *frame, *vbox;
	 
}SIDEBAR;

static SIDEBAR sidebar = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

typedef struct TextEntry
{
    GtkWidget *main_box;
    GtkWidget *rating_eventbox;
    GtkWidget *rating_label;
    GtkWidget *text_view;
    GtkWidget *frame;
}TextEntry;

static GHashTable*  sec_entries_ht = NULL;

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
    GdkColor color;
    guint16 alpha;
    
	switch (rating) {
		case -1:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio1), TRUE);
			
            gdk_rgba_to_color(unk_info->negative_rating_color, &color, &alpha);
            gtk_widget_modify_bg(sidebar.rating_eventbox,GTK_STATE_NORMAL,&color);
            break;
		case 0:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio2), TRUE);
   			
            gdk_rgba_to_color(unk_info->neutral_rating_color, &color, &alpha);
            gtk_widget_modify_bg(sidebar.rating_eventbox,GTK_STATE_NORMAL,&color);
            break;
		case 1:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sidebar.radio3), TRUE);
			
            gdk_rgba_to_color(unk_info->positive_rating_color, &color, &alpha);
            gtk_widget_modify_bg(sidebar.rating_eventbox,GTK_STATE_NORMAL,&color);
            break;
		default:
			g_warning("wrong rating %d", rating);
			
	}
}

void sidebar_set_secondary_note(gchar* frame_name, gpointer text)
{
    TextEntry* text_entry = (TextEntry*)g_hash_table_lookup(sec_entries_ht, frame_name);
    if (!text_entry)
    {
        g_warning("cant find secondary frame %s", frame_name);
        return;
    }
    GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_entry->text_view));
    gtk_text_buffer_set_text(buffer, text, strlen(text));
     
};

void sidebar_set_secondary_rating(gchar* frame_name, gint rating)
{
    TextEntry* text_entry = (TextEntry*)g_hash_table_lookup(sec_entries_ht, frame_name);
    if (!text_entry)
    {
        g_warning("cant find secondary frame %s", frame_name);
        return;
    }
    
    GdkColor color;
    guint16 alpha;
            
    switch (rating) {
		case -1:
			gdk_rgba_to_color(unk_info->negative_rating_color, &color, &alpha);
            gtk_widget_modify_bg(text_entry->rating_eventbox,GTK_STATE_NORMAL,&color);
            break;
		case 0:
			gdk_rgba_to_color(unk_info->neutral_rating_color, &color, &alpha);
            gtk_widget_modify_bg(text_entry->rating_eventbox,GTK_STATE_NORMAL,&color);
   			break;
		case 1:
			gdk_rgba_to_color(unk_info->positive_rating_color, &color, &alpha);
            gtk_widget_modify_bg(text_entry->rating_eventbox,GTK_STATE_NORMAL,&color);
			break;
		default:
			g_warning("wrong rating %d", rating);
	}
};

void sidebar_show(GeanyPlugin* geany_plugin)
{
	gtk_widget_show_all(sidebar.unk_view_vbox);
	
	gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), sidebar.unk_view_vbox);
	if (page_num >= 0) 
		gtk_notebook_set_current_page(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), page_num);
	else 
    {   
		g_error("error: cant find url notes page in sidebar notebook");
    	msgwin_status_add_string("error: cant find 'url notes' page in sidebar notebook");
    }
}

void sidebar_hide_all_secondary_frames()
{
    GHashTableIter it;
    gpointer key, value;
				
    g_hash_table_iter_init (&it, sec_entries_ht);
    while (g_hash_table_iter_next (&it, &key, &value))
    {
        gtk_widget_hide_all(((TextEntry*)value)->frame);
    }
}

void side_show_secondary_frame(gchar* frame_name)
{
    TextEntry* text_entry = (TextEntry*)g_hash_table_lookup(sec_entries_ht, frame_name);
    if (!text_entry)
    {
        g_warning("cant find secondary frame %s", frame_name);
        return;
    }
    gtk_widget_show_all(text_entry->frame);
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

    GdkColor color;
    guint16 alpha;
    
	switch (rating) {
		case -1:
			gdk_rgba_to_color(unk_info->negative_rating_color, &color, &alpha);
            gtk_widget_modify_bg(sidebar.rating_eventbox,GTK_STATE_NORMAL,&color);
            break;
		case 0:
			gdk_rgba_to_color(unk_info->neutral_rating_color, &color, &alpha);
            gtk_widget_modify_bg(sidebar.rating_eventbox,GTK_STATE_NORMAL,&color);
            break;
		case 1:
			gdk_rgba_to_color(unk_info->positive_rating_color, &color, &alpha);
            gtk_widget_modify_bg(sidebar.rating_eventbox,GTK_STATE_NORMAL,&color);
            break;
		default:
			g_warning("wrong rating %d", rating);
	}
	
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

GtkWidget* create_secondary_frame(gchar* name)
{
    g_debug("create_secondary_frame");
    GtkWidget *scrollwin;
	GtkTextBuffer *buffer;
    GtkWidget *frame;
	
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    
    GtkWidget* rating_eventbox = gtk_event_box_new(); 
    gtk_event_box_set_above_child(GTK_EVENT_BOX(rating_eventbox), FALSE); 
    
    GtkWidget *rating_label = gtk_label_new ("");
	gtk_widget_set_size_request(rating_label, -1, 5);
	
    gtk_container_add(GTK_CONTAINER(rating_eventbox),rating_label);
    gtk_box_pack_start(GTK_BOX(vbox), rating_eventbox, FALSE, FALSE, 1);
    
    GtkWidget *text_view = gtk_text_view_new ();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view),GTK_WRAP_WORD);
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
	gtk_text_buffer_set_text (buffer, "", -1);
	
    scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
					   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollwin), text_view);
	
    gtk_box_pack_start(GTK_BOX(vbox), scrollwin, TRUE, TRUE, 0);

    frame = gtk_frame_new(name);
    
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 2);
    
    TextEntry* text_entry = g_malloc (sizeof (TextEntry));
	text_entry->main_box = vbox;
    text_entry->rating_label = rating_label;
    text_entry->text_view = text_view;
    text_entry->rating_eventbox = rating_eventbox;
    text_entry->frame = frame;
    
    g_hash_table_insert(sec_entries_ht, g_strdup(name), text_entry);
        
    return frame;
}

void remove_secondary_frame(gchar* name)
{
    g_hash_table_remove(sec_entries_ht, name);
    
}
 
static void text_hashtable_key_destroyed(gpointer key) {
	g_free((gchar*)key);
}

static void text_destroyed(gpointer value) {
	g_free((DBRow*)value);
}

void sidebar_init(GeanyPlugin* geany_plugin)
{
	g_debug("sidebar_init");
    
    sec_entries_ht = g_hash_table_new_full (g_str_hash, g_str_equal, text_hashtable_key_destroyed, text_destroyed);
    
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
	gtk_widget_set_size_request(rating_label, -1, 5);
	
    gtk_container_add(GTK_CONTAINER(sidebar.rating_eventbox),rating_label);
    gtk_box_pack_start(GTK_BOX(sidebar.vbox), sidebar.rating_eventbox, FALSE, FALSE, 1);
    
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

	gtk_box_pack_start (GTK_BOX (sidebar.rating_box), sidebar.radio1, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (sidebar.rating_box), sidebar.radio2, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (sidebar.rating_box), sidebar.radio3, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(sidebar.vbox), sidebar.rating_box, FALSE, FALSE, 0);
    
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

    GList* secondary_dbc_list = unk_db_get_all_db_names();
    for (GList* it = secondary_dbc_list; it; it = it->next) 
	{
        GtkWidget* secondary_frame = create_secondary_frame(((DBInfo*)(it->data))->name);
        gtk_box_pack_start(GTK_BOX(sidebar.unk_view_vbox), secondary_frame, FALSE, FALSE, 0);
        
    }
    
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


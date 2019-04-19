#include <geanyplugin.h>

#include <ctype.h>
#include <string.h>
#include "gtk3compat.h"
#include "unkplugin.h"
#include "unkgui.h"
#include "unkdb.h"
#include "unkfeedbar.h"


typedef struct Feedbar
{
    GtkWidget *main_box;
    GtkWidget *text_view;
}Feedbar;

static Feedbar feedbar = {NULL, NULL};

static GList*  text_marks_list = NULL;


GtkTextTag* get_tag_by_rating(gint rating) 
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (feedbar.text_view));
    GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table (buffer);
    GtkTextTag* res = NULL;
	switch(rating)
	{
		case -1:
			res = gtk_text_tag_table_lookup (tag_table, "negative_rating");
			break;
		case 0:
			res = gtk_text_tag_table_lookup (tag_table, "neutral_rating");
			break;
		case 1:
			res = gtk_text_tag_table_lookup (tag_table, "positive_rating");
			break;
		default:
			g_warning("unknown rating value %d", rating);
	}
	return res;
}

static void text_marks_list_destroyed(gpointer value) {
	GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (feedbar.text_view));
    gtk_text_buffer_delete_mark (buffer, (GtkTextMark*)value);
}


void feedbar_show(GeanyPlugin* geany_plugin)
{
	gtk_widget_show_all(feedbar.main_box);
	
	gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook), feedbar.main_box);
	if (page_num >= 0) 
		gtk_notebook_set_current_page(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook), page_num);
	else 
    {   
		g_warning("error: cant find 'feedbar' page in message window notebook");
    	msgwin_status_add_string("error: cant find 'feedbar' page in message window notebook");
    }
}

void feedbar_set_notes(GHashTable* rows)
{
    g_debug("feedbar_set_notes");
    GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (feedbar.text_view));
    
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    gtk_text_buffer_delete(buffer, &start_iter, &end_iter); 
    
    g_list_free_full(text_marks_list, text_marks_list_destroyed);
    text_marks_list = NULL;
    
    GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table (buffer);
    
    GHashTableIter it;
    gpointer key, value;
    
    g_hash_table_iter_init (&it, rows);
    while (g_hash_table_iter_next (&it, &key, &value))
    {
        gtk_text_buffer_get_end_iter(buffer, &end_iter);
        GtkTextMark * mark = gtk_text_buffer_create_mark(buffer, g_strdup((gchar*)key), &end_iter, TRUE);
        text_marks_list = g_list_append(text_marks_list, mark);
        //gtk_text_buffer_insert (buffer, &end_iter, g_strdup((gchar*)key), -1);
        gtk_text_buffer_insert_with_tags(buffer, &end_iter, g_strdup((gchar*)key), -1,
            get_tag_by_rating(((DBRow*)value)->rating), 
            gtk_text_tag_table_lookup (tag_table, "key_justification"),
            NULL);
        gtk_text_buffer_get_end_iter(buffer, &end_iter);
        gtk_text_buffer_insert (buffer, &end_iter, "\n", -1);
        gtk_text_buffer_get_end_iter(buffer, &end_iter);
        gtk_text_buffer_insert_with_tags(buffer, &end_iter, g_strdup(((DBRow*)value)->note), -1,
            get_tag_by_rating(((DBRow*)value)->rating), 
            gtk_text_tag_table_lookup (tag_table, "value_justification"),
            NULL);
        gtk_text_buffer_get_end_iter(buffer, &end_iter);
        gtk_text_buffer_insert (buffer, &end_iter, "\n", -1);
    }
              
}

void feedbar_reset()
{
    g_debug("feedbar_reset");
    GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (feedbar.text_view));
    
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    gtk_text_buffer_delete(buffer, &start_iter, &end_iter); 
    
    gtk_text_buffer_remove_all_tags(buffer, &start_iter, &end_iter);
    
    g_list_free_full(text_marks_list, text_marks_list_destroyed);
    text_marks_list = NULL;
    
    GList* secondary_dbc_list = unk_db_get_all_db_names();
    for (GList* it = secondary_dbc_list; it; it = it->next) 
	{
        gtk_text_buffer_get_end_iter(buffer, &end_iter);
        GtkTextMark * mark = gtk_text_buffer_create_mark(buffer, ((DBInfo*)(it->data))->name, &end_iter, TRUE);
        text_marks_list = g_list_append(text_marks_list, mark);
        gtk_text_buffer_insert (buffer, &end_iter, ((DBInfo*)(it->data))->name, -1);
        gtk_text_buffer_get_end_iter(buffer, &end_iter);
        gtk_text_buffer_insert (buffer, &end_iter, "\n\n", -1);
    }
    
    GdkRGBA inv_color; 
    gchar buf[100];
    gchar buf2[100];
    gtk_text_buffer_create_tag( buffer, "rating_font", "font", "Serif 12", NULL);
    gtk_text_buffer_create_tag( buffer, "key_justification", "justification", GTK_JUSTIFY_CENTER, NULL);
    gtk_text_buffer_create_tag( buffer, "value_justification", "justification", GTK_JUSTIFY_FILL, NULL);
    gtk_text_buffer_create_tag( buffer, "positive_rating", 
        "background", gdk_rgba_to_hex_string (unk_info->positive_rating_color, buf),
        "foreground", gdk_rgba_to_hex_string (gdk_rgba_contrast(unk_info->positive_rating_color, &inv_color), buf2),
        NULL);
    gtk_text_buffer_create_tag( buffer, "neutral_rating", 
        "background", gdk_rgba_to_hex_string (unk_info->neutral_rating_color, buf),
        "foreground", gdk_rgba_to_hex_string (gdk_rgba_contrast(unk_info->neutral_rating_color, &inv_color), buf2),
            NULL);
    gtk_text_buffer_create_tag( buffer, "negative_rating",
        "background", gdk_rgba_to_hex_string (unk_info->negative_rating_color, buf),
        "foreground", gdk_rgba_to_hex_string (gdk_rgba_contrast(unk_info->negative_rating_color, &inv_color), buf2),
        NULL);
     
}

void feedbar_init(GeanyPlugin* geany_plugin)
{
	g_debug("feedbar_init");
       
    GtkWidget *scrollwin;
	GtkTextBuffer *buffer;

	feedbar.main_box = gtk_vbox_new(FALSE, 1);
    
	/**** text ****/
	feedbar.text_view = gtk_text_view_new ();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(feedbar.text_view),GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(feedbar.text_view), FALSE);
    
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (feedbar.text_view));
	gtk_text_buffer_set_text (buffer, "", -1);
	
    scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
					   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollwin), feedbar.text_view);
	
    gtk_box_pack_start(GTK_BOX(feedbar.main_box), scrollwin, TRUE, TRUE, 0);
    /**** the rest ****/
	
   
    
	gtk_widget_show_all(feedbar.main_box);
	gtk_notebook_append_page(
		GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook),
		feedbar.main_box,
		gtk_label_new(_("Feedbar")));
	
    feedbar_reset();
	feedbar_deactivate();			 
}

void feedbar_activate(void)
{
	gtk_widget_set_sensitive(feedbar.main_box, TRUE);
}

void feedbar_deactivate(void)
{
	gtk_widget_set_sensitive(feedbar.main_box, FALSE);
}

void feedbar_cleanup(void)
{
	g_debug("feedbar_cleanup");
    gtk_widget_destroy(feedbar.main_box);
}
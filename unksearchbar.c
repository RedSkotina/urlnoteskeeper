#include <geanyplugin.h>

#include <ctype.h>
#include <string.h>
#include "gtk3compat.h"
#include "unkplugin.h"
#include "unkgui.h"
#include "unkdb.h"
#include "unksidebar.h"
#include "unksearchbar.h"
#include "unkfeedbar.h"

typedef struct Searchbar
{
    GtkWidget *main_box;
    GtkWidget *pattern_entry;
    GtkTreeView *results_view;
    GtkListStore       *store;
    GtkTreeModelFilter *filtered;
    
}Searchbar;

static Searchbar searchbar = {NULL, NULL, NULL, NULL, NULL};

enum
{
    COLUMN_KEY = 0,
    COLUMN_VALUE,
    COLUMN_BACKGROUND,
    COLUMN_FOREGROUND,
    N_COLUMNS
};

gchar* get_hex_color_by_rating(gint rating) 
{
    gchar buf[100];
    
	switch(rating)
	{
		case -1:
			gdk_rgba_to_hex_string (unk_info->negative_rating_color, buf);
			break;
		case 0:
			gdk_rgba_to_hex_string (unk_info->neutral_rating_color, buf);
			break;
		case 1:
			gdk_rgba_to_hex_string (unk_info->positive_rating_color, buf);
			break;
		default:
			g_warning("unknown rating value %d", rating);
	}
	return g_strdup(buf);
}

gchar* get_hex_inv_color_by_rating(gint rating) 
{
    gchar buf[100];
    GdkRGBA inv_color; 
    
	switch(rating)
	{
		case -1:
			gdk_rgba_to_hex_string (gdk_rgba_contrast(unk_info->negative_rating_color, &inv_color), buf);
			break;
		case 0:
			gdk_rgba_to_hex_string (gdk_rgba_contrast(unk_info->neutral_rating_color, &inv_color), buf);
			break;
		case 1:
			gdk_rgba_to_hex_string (gdk_rgba_contrast(unk_info->positive_rating_color, &inv_color), buf);
			break;
		default:
			g_warning("unknown rating value %d", rating);
	}
	return g_strdup(buf);
}


gboolean row_visible(GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
    gboolean is_visible = FALSE;
    const gchar* pattern_text = gtk_entry_get_text(GTK_ENTRY(searchbar.pattern_entry));
    
    gchar* value;
    gtk_tree_model_get (model, iter, COLUMN_VALUE, &value, -1);
    
    gchar* found_seq = strstr(value, pattern_text);
    if (found_seq)
            is_visible = TRUE;
    
    return is_visible; 
};

static void on_row_activated (GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreePath *child_path;

    child_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (searchbar.filtered),
                                                                   path);

    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (searchbar.store), &iter, child_path)) {
        gchar *key;
        gchar *value;

        gtk_tree_model_get (GTK_TREE_MODEL (searchbar.store), &iter,
                            COLUMN_KEY, &key,
                            COLUMN_VALUE, &value,
                            -1);

        sidebar_set_url(key);
		
        DBRow* row = unk_db_get_primary(key, "");
		sidebar_set_note(row->note);
		sidebar_set_rating(row->rating);
				
        sidebar_show(geany_plugin);			
				
        GHashTable* secondary_rows = unk_db_get_secondary(key, "none");
				
        feedbar_set_notes(secondary_rows);
                
        sidebar_activate();
		feedbar_activate();
				
        g_hash_table_destroy (secondary_rows);
        g_free(row->note);
        g_free(row);
        
        GList *list_marks = NULL;
        GeanyDocument *doc = document_get_current();
        if (doc)
        {
            list_marks = search_marks(doc->editor, key);
            if (list_marks != NULL) 
            {
                scintilla_send_message(doc->editor->sci, SCI_SCROLLRANGE, ((MarkInfo*)(list_marks->data))->start , ((MarkInfo*)(list_marks->data))->end);
            }
            g_list_free_full (list_marks, g_free);
        }
        g_free (key);
        g_free (value);
       
    }
}

static void pattern_entry_changed(GtkWidget *pattern_entry, gpointer user_data)
{
    gtk_tree_model_filter_refilter (searchbar.filtered);
}
    
void searchbar_init(GeanyPlugin* geany_plugin)
{
	g_debug("searchbar_init");
       
    GtkWidget *scrollwin;
	
	searchbar.main_box = gtk_vbox_new(FALSE, 1);
    
    /**** search pattern ****/
	searchbar.pattern_entry = gtk_entry_new();
    g_signal_connect (searchbar.pattern_entry, "changed", G_CALLBACK (pattern_entry_changed), NULL);
    
    gtk_box_pack_start(GTK_BOX(searchbar.main_box), searchbar.pattern_entry, FALSE, FALSE, 1);
    
	/**** text ****/
	searchbar.store = gtk_list_store_new (N_COLUMNS,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING);
    
    
    // searchbar.results_list = gtk_list_box_new();
	// gtk_list_box_set_sort_func (GTK_LIST_BOX (searchbar.results_list), (GtkListBoxSortFunc)gtk_message_row_sort, searchbar.results_list, NULL);
    // gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (searchbar.results_list), TRUE);
    // //g_signal_connect (searchbar.results_list, "row-activated", G_CALLBACK (row_activated), NULL);
    searchbar.filtered = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (searchbar.store), NULL));
    gtk_tree_model_filter_set_visible_func (searchbar.filtered, (GtkTreeModelFilterVisibleFunc) row_visible, NULL, NULL);
    
    searchbar.results_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL(searchbar.filtered)));
    
    g_signal_connect (searchbar.results_view, "row-activated", G_CALLBACK (on_row_activated), searchbar.store);
                      
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn   *key_column = gtk_tree_view_column_new_with_attributes (
            "URL", renderer,
            "text", COLUMN_KEY,
            "background", COLUMN_BACKGROUND,
            "foreground", COLUMN_FOREGROUND,
            NULL);
    gtk_tree_view_append_column (searchbar.results_view, key_column);

    GtkTreeViewColumn   *value_column = gtk_tree_view_column_new_with_attributes (
            "Note", renderer,
            "text", COLUMN_VALUE,
            "background", COLUMN_BACKGROUND,
            "foreground", COLUMN_FOREGROUND,
            NULL);
    gtk_tree_view_append_column (searchbar.results_view, value_column);
    
    GtkTreeIter iter;
    
    GList* iterator;
    GList* list_keys = unk_db_get_all();
	for (iterator = list_keys; iterator; iterator = iterator->next) 
	{
        gtk_list_store_append (searchbar.store, &iter);
        gtk_list_store_set (searchbar.store, &iter, 
            COLUMN_KEY, ((DBRow*)iterator->data)->url, 
            COLUMN_VALUE, ((DBRow*)iterator->data)->note,
            COLUMN_BACKGROUND, get_hex_color_by_rating(((DBRow*)iterator->data)->rating),
            COLUMN_FOREGROUND, get_hex_inv_color_by_rating(((DBRow*)iterator->data)->rating),
            -1);
    };
    g_list_free_full (list_keys, row_destroyed );
    
    scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
					   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollwin), GTK_WIDGET(searchbar.results_view));

    gtk_box_pack_start(GTK_BOX(searchbar.main_box), scrollwin, TRUE, TRUE, 0);

    /**** the rest ****/
	gtk_widget_show_all(searchbar.main_box);
	gtk_notebook_append_page(
		GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook),
		searchbar.main_box,
		gtk_label_new(_("Searchbar")));

}

void searchbar_activate(void)
{
	gtk_widget_set_sensitive(searchbar.main_box, TRUE);
}

void searchbar_deactivate(void)
{
	gtk_widget_set_sensitive(searchbar.main_box, FALSE);
}

void searchbar_cleanup(void)
{
	g_debug("searchbar_cleanup");
    gtk_widget_destroy(searchbar.main_box);
}
#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
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
    GtkWidget *checkbutton;
    GtkWidget *pattern_entry;
    GtkTreeView *results_view;
    GtkListStore       *store;
    GtkTreeModelSort   *sorted;
    GtkTreeModelFilter *filtered;
    
}Searchbar;

static Searchbar searchbar = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

enum
{
    COLUMN_KEY = 0,
    COLUMN_VALUE,
    COLUMN_RATING,
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

gint searchbar_get_mode()
{
    gint mode = SEARCH_ONLY_CURRENT_DOCUMENT;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(searchbar.checkbutton)))
        mode = SEARCH_ONLY_CURRENT_DOCUMENT;
    else
        mode = SEARCH_FULL_BASE;
    return mode;
}

gboolean row_visible(GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
    gboolean is_visible = FALSE;
    const gchar* pattern_text = gtk_entry_get_text(GTK_ENTRY(searchbar.pattern_entry));
    if (!pattern_text)
		return FALSE;
    gchar* value;
    gtk_tree_model_get (model, iter, COLUMN_VALUE, &value, -1);
    if (!value)
		return FALSE;
    gchar* found_seq = strstr(value, pattern_text);
    if (found_seq)
            is_visible = TRUE;
    g_free(value);
    return is_visible; 
};

static void on_row_activated (GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreePath *child_path, *filtered_path;

    filtered_path = gtk_tree_model_sort_convert_path_to_child_path (GTK_TREE_MODEL_SORT (searchbar.sorted), path);
    child_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (searchbar.filtered), filtered_path);

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
				
        GHashTable* secondary_rows = unk_db_get_secondary (key, "none");
				
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
 
static void pattern_entry_activate(GtkWidget *pattern_entry, gpointer user_data)
{
    gtk_widget_grab_focus(GTK_WIDGET(searchbar.results_view));
}

gboolean tree_on_keypress (GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->keyval == GDK_KEY_Escape){
        gtk_widget_grab_focus(GTK_WIDGET(searchbar.pattern_entry));
        return TRUE;
    }
    return FALSE;
}

static void toggled_cb (GtkToggleButton *toggle_button, gpointer user_data)
{
  // if (gtk_toggle_button_get_active (toggle_button))
      // // only doc;
  // else
     // all base;
}

static void refresh_button_clicked( GtkWidget *widget,  gpointer user_data )
{
    searchbar_store_reset(searchbar_get_mode(), document_get_current());
}


gint row_compare_func(gconstpointer a, gconstpointer b)
{
    gint res;
    res = ( ((DBRow*)a)->rating > ((DBRow*)b)->rating )? 1 : (( ((DBRow*)a)->rating == ((DBRow*)b)->rating )? 0 : -1);
    if (res == 0)
        res = g_strcmp0( ((DBRow*)a)->url, ((DBRow*)b)->url );
    return res;
}

// not transfer ownage of list elements
static GList* filter_current_document_keys(GeanyDocument * doc, GList* list)
{
    if (!doc)
        return NULL;
    GList *res_list = g_list_copy (list);
	gint flags = 0;
	struct Sci_TextToFind ttf;
	
    ScintillaObject *sci = doc->editor->sci;
	
	gint len = scintilla_send_message(sci, SCI_GETLENGTH, 0, 0);
	
    GList* iterator;
    for (iterator = list; iterator; iterator = iterator->next) 
	{ 
        ttf.chrg.cpMin = 0;
        ttf.chrg.cpMax = len;
        ttf.lpstrText = ((DBRow*)iterator->data)->url;
		
        if (scintilla_send_message(sci, SCI_FINDTEXT, flags, (sptr_t)&ttf) == -1)
        {
            res_list = g_list_remove(res_list, (DBRow*)iterator->data);
        }
    }   
	
    return res_list;
}

void searchbar_store_reset(gint mode, gpointer userdata)
{
    g_debug("searchbar_store_reset");
    // cleanup current store
    GtkTreeIter       iter;
    gboolean          iter_valid;
    
    iter_valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(searchbar.store), &iter);
    while (iter_valid)
    {
      // Maybe additional cleanup of related data
      // gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, .... -1);
      iter_valid = gtk_list_store_remove(searchbar.store, &iter);
    }
    // Maybe update filtering and sorting afterwards
    gtk_tree_model_filter_refilter(searchbar.filtered);
    
    // populate new data in store
    GList* iterator;
    GList* list_keys;
    GList* list;
    list_keys = unk_db_get_all_local();
    switch (mode)
    {
        case SEARCH_FULL_BASE: 
            list = list_keys;
            break;
        case SEARCH_ONLY_CURRENT_DOCUMENT: 
            list = filter_current_document_keys( (GeanyDocument*) userdata, list_keys);
            break;
        default:
            list = list_keys;
            break;
    }
    list = g_list_sort(list, row_compare_func);
    list = g_list_reverse(list);
	for (iterator = list; iterator; iterator = iterator->next) 
	{
        gtk_list_store_append (searchbar.store, &iter);
        gtk_list_store_set (searchbar.store, &iter, 
            COLUMN_KEY, ((DBRow*)iterator->data)->url, 
            COLUMN_VALUE, ((DBRow*)iterator->data)->note,
            COLUMN_RATING, ((DBRow*)iterator->data)->rating,
            COLUMN_BACKGROUND, get_hex_color_by_rating(((DBRow*)iterator->data)->rating),
            COLUMN_FOREGROUND, get_hex_inv_color_by_rating(((DBRow*)iterator->data)->rating),
            -1);
     };
     
    g_list_free_full (list_keys, row_destroyed);
}
    
void searchbar_init(GeanyPlugin* geany_plugin)
{
	g_debug("searchbar_init");
       
    GtkWidget *scrollwin;
	
	searchbar.main_box = gtk_vbox_new(FALSE, 1);
    
    /**** search pattern ****/
	GtkWidget *hbox = gtk_hbox_new(FALSE, 1);
    
    searchbar.pattern_entry = gtk_entry_new();
    g_signal_connect (searchbar.pattern_entry, "changed", G_CALLBACK (pattern_entry_changed), NULL);
    g_signal_connect (searchbar.pattern_entry, "activate", G_CALLBACK (pattern_entry_activate), NULL);
    
    gtk_box_pack_start(GTK_BOX(hbox), searchbar.pattern_entry, TRUE, TRUE, 1);
    
    searchbar.checkbutton = gtk_check_button_new_with_label ("Current document");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (searchbar.checkbutton), TRUE);
    g_signal_connect (GTK_TOGGLE_BUTTON (searchbar.checkbutton), "toggled", G_CALLBACK (toggled_cb), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), searchbar.checkbutton, FALSE, FALSE, 1);
    
    GtkWidget * refresh_button = gtk_button_new_with_label ("Refresh");
    g_signal_connect (refresh_button, "clicked", G_CALLBACK (refresh_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), refresh_button, FALSE, FALSE, 1);
    
    gtk_box_pack_start(GTK_BOX(searchbar.main_box), hbox, FALSE, FALSE, 1);
    
	/**** text ****/
	searchbar.store = gtk_list_store_new (N_COLUMNS,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_INT,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING);
    
    searchbar.filtered = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (searchbar.store), NULL));
    gtk_tree_model_filter_set_visible_func (searchbar.filtered, (GtkTreeModelFilterVisibleFunc) row_visible, NULL, NULL);
    
    searchbar.sorted = GTK_TREE_MODEL_SORT (gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (searchbar.filtered)));
    
    searchbar.results_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL(searchbar.sorted)));
    g_signal_connect (G_OBJECT (searchbar.results_view), "key_press_event", G_CALLBACK (tree_on_keypress), NULL);
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
    
    gtk_tree_view_column_set_sort_column_id (key_column, COLUMN_KEY);
    
    searchbar_store_reset(searchbar_get_mode(), document_get_current());
    
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

void searchbar_show(GeanyPlugin* geany_plugin)
{
	gtk_widget_show_all(searchbar.main_box);
	
	gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook), searchbar.main_box);
	if (page_num >= 0) 
		gtk_notebook_set_current_page(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook), page_num);
	else 
    {   
		g_warning("error: cant find url notes page in message notebook");
    	msgwin_status_add_string("error: cant find 'searchbar' page in message notebook");
    }
    gtk_widget_grab_focus(GTK_WIDGET(searchbar.pattern_entry));
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

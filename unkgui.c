#include <geanyplugin.h>

#include <ctype.h>
#include <string.h>

#include "gtk3compat.h"
#include "unkgui.h"
#include "unkmatcher.h"
#include "unkplugin.h"
#include "unksidebar.h"
#include "unkfeedbar.h"
#include "unkdb.h"
#include <math.h>

gint GEANY_INDICATOR_UNK_AUTO_DETECTED = 23;

gint GEANY_INDICATOR_UNK_POSITIVE_DB = 24;
gint GEANY_INDICATOR_UNK_NEUTRAL_DB = 25;
gint GEANY_INDICATOR_UNK_NEGATIVE_DB = 26;

static GtkWidget *main_menu_item = NULL;
ConfigWidgets* config_widgets = NULL;
MenuWidgets* menu_widgets = NULL;

void gdk_rgba_copy_value(GdkRGBA* dst, GdkRGBA* src)
{
	dst->red = src->red;
	dst->green = src->green;
	dst->blue = src->blue;
	dst->alpha = src->alpha;
}

gdouble scale_round(gdouble val, gdouble factor)
{
	val = floor(val * factor + 0.5);
	val = MAX(val, 0);
	val = MIN(val, factor);

	return val;
}

guint gdk_rgba_rgb_to_integer(GdkRGBA* color)
{
	return (guint) (scale_round(color->red, 255))*0x000001 + (guint) (scale_round(color->green , 255))*0x000100 + (guint) (scale_round(color->blue , 255))*0x010000;
		
} 

guint gdk_rgba_alpha_to_integer(GdkRGBA* color)
{
	return (guint) (scale_round(color->alpha, 255));
}


gint get_indicator_id_by_rating(gint rating) 
{
	gint res = 0;
	switch(rating)
	{
		case -1:
			res = GEANY_INDICATOR_UNK_NEGATIVE_DB;
			break;
		case 0:
			res = GEANY_INDICATOR_UNK_NEUTRAL_DB;
			break;
		case 1:
			res = GEANY_INDICATOR_UNK_POSITIVE_DB;
			break;
		default:
			g_warning("unknown rating value %d", rating);
	}
	return res;
}
gchar* get_backward_word(GeanyDocument *doc, gint cur_pos)
{
	g_debug("get_backward_word");
    gint wstart, wend;
	gchar* word;
	
	gint wordchars_len;
	gchar *wordchars;
	gboolean wordchars_modified = FALSE;

	wordchars_len = scintilla_send_message(doc->editor->sci, SCI_GETWORDCHARS, 0, 0);
	wordchars = g_malloc0(wordchars_len + 5); /* 2 = temporarily added "'" and "\0" */
	scintilla_send_message(doc->editor->sci, SCI_GETWORDCHARS, 0, (sptr_t)wordchars);
	
	if (! strchr(wordchars, '.'))
	{
		/* temporarily add "'" to the wordchars */
		wordchars[wordchars_len] = '.';
		wordchars[wordchars_len+1] = '/';
		wordchars[wordchars_len+2] = '\\';
		wordchars[wordchars_len+3] = ':';
		wordchars_modified = TRUE;
	}
	if (wordchars_modified)
	{
		/* apply previously changed WORDCHARS setting */
		scintilla_send_message(doc->editor->sci, SCI_SETWORDCHARS, 0, (sptr_t)wordchars);
	}
	
	wstart = scintilla_send_message(doc->editor->sci, SCI_WORDSTARTPOSITION, cur_pos, TRUE);
	wend = scintilla_send_message(doc->editor->sci, SCI_WORDENDPOSITION, wstart, TRUE);
	if (wstart != wend)
	{
		word = sci_get_contents_range(doc->editor->sci, wstart, wend);
		return word;
	}
	
	if (wordchars_modified)
	{
		wordchars[wordchars_len] = '\0';
		scintilla_send_message(doc->editor->sci, SCI_SETWORDCHARS, 0, (sptr_t)wordchars);
	}
	g_free(wordchars);
	return NULL;
}

GList* search_marks(GeanyEditor *editor, const gchar *search_text)
{
	g_debug("search_marks");
    GList *list = NULL;
	gint start_pos, flags = 0;
	struct Sci_TextToFind ttf;
	
	ScintillaObject *sci = editor->sci;
	
	gint len = scintilla_send_message(sci, SCI_GETLENGTH, 0, 0);
		
	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = len;
	ttf.lpstrText = search_text;
		
	
	while ((start_pos = scintilla_send_message(sci, SCI_FINDTEXT, flags, (sptr_t)&ttf)) != -1)
	{
		MarkInfo* pos = (MarkInfo*)g_malloc0(sizeof(MarkInfo));
		pos->start = ttf.chrgText.cpMin;
		pos->end = ttf.chrgText.cpMax;
		list = g_list_append(list, pos);
		
		ttf.chrg.cpMin = start_pos + 1;
		ttf.chrg.cpMax = len;
	}
	
	return list;
}

void set_mark(GeanyEditor *editor, gint range_start_pos, gint range_end_pos, gint rating) 
{
	editor_indicator_set_on_range(editor, get_indicator_id_by_rating( rating ) , range_start_pos, range_end_pos);
}

void set_url_marks(GeanyEditor *editor, gint range_start_pos, gint range_end_pos)
{
    g_debug("set_url_marks");
    GList* list_url = NULL, *iterator = NULL;
	
	struct Sci_TextRange tr;
	tr.chrg.cpMin = range_start_pos;
	tr.chrg.cpMax = range_end_pos;
	tr.lpstrText = g_malloc(range_end_pos - range_start_pos + 1);
	scintilla_send_message(editor->sci, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
				
	list_url = unk_match_url_text(tr.lpstrText);
	
	for (iterator = list_url; iterator; iterator = iterator->next) 
	{
		gint cur_start_pos = range_start_pos + ((MarkInfo*)(iterator->data))->start;
		gint cur_end_pos = range_start_pos + ((MarkInfo*)(iterator->data))->end;
		
		editor_indicator_set_on_range(editor, GEANY_INDICATOR_UNK_AUTO_DETECTED , cur_start_pos, cur_end_pos);
	}
	
	g_list_free_full (list_url, g_free);
	g_free(tr.lpstrText); 
}

void set_db_marks(GeanyEditor *editor, gint range_start_pos, gint range_end_pos)
{
	g_debug("set_db_marks");
    GList *list_db = NULL, *list_db2 = NULL, *list_keys = NULL, *iterator = NULL, *it2 = NULL ;
	struct Sci_TextRange tr;
	tr.chrg.cpMin = range_start_pos;
	tr.chrg.cpMax = range_end_pos;
	tr.lpstrText = g_malloc(range_end_pos - range_start_pos + 1);
	scintilla_send_message(editor->sci, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
	
	list_keys = unk_db_get_all();
	for (iterator = list_keys; iterator; iterator = iterator->next) 
	{
		list_db2 = search_marks(editor, ((DBRow*)iterator->data)->url);
		for (it2 = list_db2; it2; it2 = it2->next) 
		{
			((MarkInfo*)(it2->data))->rating = ((DBRow*)iterator->data)->rating;
		}
		
		list_db = g_list_concat(list_db, list_db2);
	};
	g_list_free_full (list_keys, row_destroyed );
	
	
	for (iterator = list_db; iterator; iterator = iterator->next) 
	{
		gint cur_start_pos = range_start_pos + ((MarkInfo*)(iterator->data))->start;
		gint cur_end_pos = range_start_pos + ((MarkInfo*)(iterator->data))->end;
		
		gint indicator_id = get_indicator_id_by_rating(((MarkInfo*)(iterator->data))->rating); 
		editor_indicator_set_on_range(editor, indicator_id, cur_start_pos, cur_end_pos);
	}
	
	g_list_free_full (list_db, g_free);
	g_free(tr.lpstrText); 
}

void clear_all_db_marks(GeanyEditor *editor)
{
	g_debug("clear_all_db_marks");
    gint len = sci_get_length(editor->sci);
	scintilla_send_message(editor->sci, SCI_SETINDICATORCURRENT, GEANY_INDICATOR_UNK_POSITIVE_DB, 0);
	scintilla_send_message(editor->sci, SCI_INDICATORCLEARRANGE, 0, len);
	scintilla_send_message(editor->sci, SCI_SETINDICATORCURRENT, GEANY_INDICATOR_UNK_NEUTRAL_DB, 0);
	scintilla_send_message(editor->sci, SCI_INDICATORCLEARRANGE, 0, len);
	scintilla_send_message(editor->sci, SCI_SETINDICATORCURRENT, GEANY_INDICATOR_UNK_NEGATIVE_DB, 0);
	scintilla_send_message(editor->sci, SCI_INDICATORCLEARRANGE, 0, len);
	
}

/*
 * 	Occures on document opening.
 */
void on_document_open(GObject *obj, GeanyDocument *doc, gpointer user_data)
{
	g_debug("on_document_open");
    /*set dwell interval*/
	scintilla_send_message(doc->editor->sci, SCI_SETMOUSEDWELLTIME, 500, 0);

	/* set tab size for calltips */
	scintilla_send_message(doc->editor->sci, SCI_CALLTIPUSESTYLE, 20, (long)NULL);
	
	scintilla_send_message(doc->editor->sci, SCI_INDICSETSTYLE, GEANY_INDICATOR_UNK_AUTO_DETECTED, INDIC_BOX);
	scintilla_send_message(doc->editor->sci, SCI_INDICSETALPHA, GEANY_INDICATOR_UNK_AUTO_DETECTED, 50);
	scintilla_send_message(doc->editor->sci, SCI_INDICSETFORE, GEANY_INDICATOR_UNK_AUTO_DETECTED, 0x007f00);
	
	scintilla_send_message(doc->editor->sci, SCI_INDICSETSTYLE, GEANY_INDICATOR_UNK_POSITIVE_DB, INDIC_STRAIGHTBOX);
	scintilla_send_message(doc->editor->sci, SCI_INDICSETALPHA, GEANY_INDICATOR_UNK_POSITIVE_DB, gdk_rgba_alpha_to_integer(unk_info->positive_rating_color));
	scintilla_send_message(doc->editor->sci, SCI_INDICSETFORE,  GEANY_INDICATOR_UNK_POSITIVE_DB, gdk_rgba_rgb_to_integer(unk_info->positive_rating_color));	
	
	scintilla_send_message(doc->editor->sci, SCI_INDICSETSTYLE, GEANY_INDICATOR_UNK_NEUTRAL_DB, INDIC_STRAIGHTBOX);
	scintilla_send_message(doc->editor->sci, SCI_INDICSETALPHA, GEANY_INDICATOR_UNK_NEUTRAL_DB, gdk_rgba_alpha_to_integer(unk_info->neutral_rating_color));
	scintilla_send_message(doc->editor->sci, SCI_INDICSETFORE,  GEANY_INDICATOR_UNK_NEUTRAL_DB, gdk_rgba_rgb_to_integer(unk_info->neutral_rating_color));	
	
	scintilla_send_message(doc->editor->sci, SCI_INDICSETSTYLE, GEANY_INDICATOR_UNK_NEGATIVE_DB, INDIC_STRAIGHTBOX);
	scintilla_send_message(doc->editor->sci, SCI_INDICSETALPHA, GEANY_INDICATOR_UNK_NEGATIVE_DB, gdk_rgba_alpha_to_integer(unk_info->negative_rating_color));
	scintilla_send_message(doc->editor->sci, SCI_INDICSETFORE,  GEANY_INDICATOR_UNK_NEGATIVE_DB, gdk_rgba_rgb_to_integer(unk_info->negative_rating_color));	
	
	
	if (unk_info->enable_urls_detect_on_open_document)
	{
		gint nLen = sci_get_length(doc->editor->sci);
		set_url_marks(doc->editor, 0, nLen);
	}
	if (unk_info->enable_db_detect_on_open_document)
	{
		gint nLen = sci_get_length(doc->editor->sci);
		set_db_marks(doc->editor, 0, nLen);
	}
}

gboolean unk_gui_editor_notify(GObject *object, GeanyEditor *editor,
								 SCNotification *nt, gpointer data)
{
	/* data == GeanyPlugin because the data member of PluginCallback was set to NULL
	 * and this plugin has called geany_plugin_set_data() with the GeanyPlugin pointer as
	 * data */
	gchar* word;
	gchar prev_char;
	gint cur_pos;
	gboolean indicator_exist = FALSE;
	gint indicator_id = 0;
	gint indicator_bitmap = 0;
			
	/* For detailed documentation about the SCNotification struct, please see
	 * http://www.scintilla.org/ScintillaDoc.html#Notifications. */
	switch (nt->nmhdr.code)
	{
		case SCN_UPDATEUI:
			/* This notification is sent very often, you should not do time-consuming tasks here */
			break;
		case SCN_CHARADDED:
			g_debug("unk_gui_editor_notify(SCN_CHARADDED)");
            if (nt->ch == ' ' || nt->ch == '\n') 
			{
				cur_pos = scintilla_send_message(editor->sci, SCI_GETCURRENTPOS, 0, 0);
				if (cur_pos > 0)
				{ 
					prev_char = scintilla_send_message(editor->sci, SCI_GETCHARAT, cur_pos-2, 0);
					if (prev_char > 0 && (prev_char != ' ' && prev_char != '\n')) 
					{
						word = get_backward_word(editor->document, cur_pos-1);
						if (word) 
						{ 
							gint g_start_pos = cur_pos - strlen(word) - 1;
							
							if (unk_info->enable_urls_detect_on_open_document)
							{
								set_url_marks(editor, g_start_pos, cur_pos-1);
							}
							if (unk_info->enable_db_detect_on_open_document)
							{
								set_db_marks(editor, g_start_pos, cur_pos-1);
							}
						}
					}
				}
			}
			break;
		case SCN_INDICATORCLICK:
			g_debug("unk_gui_editor_notify(SCN_INDICATORCLICK)");
            indicator_exist = FALSE;
			indicator_id = 0;
			indicator_bitmap = scintilla_send_message(editor->sci, SCI_INDICATORALLONFOR, nt->position, 0);
			
			if (indicator_bitmap & (1<<(GEANY_INDICATOR_UNK_POSITIVE_DB))) 
			{
				indicator_exist = TRUE;
				indicator_id = GEANY_INDICATOR_UNK_POSITIVE_DB;
			};
			if (indicator_bitmap & (1<<(GEANY_INDICATOR_UNK_NEUTRAL_DB)))
			{
				indicator_exist = TRUE;
				indicator_id = GEANY_INDICATOR_UNK_NEUTRAL_DB;
			};
			if (indicator_bitmap & (1<<(GEANY_INDICATOR_UNK_NEGATIVE_DB))) 
			{
				indicator_exist = TRUE;
				indicator_id = GEANY_INDICATOR_UNK_NEGATIVE_DB;
			};
					
			if (indicator_exist)
			{
				gint start = scintilla_send_message(editor->sci, SCI_INDICATORSTART, indicator_id, nt->position);
				gint end = scintilla_send_message(editor->sci, SCI_INDICATOREND, indicator_id, nt->position);
				
				struct Sci_TextRange tr;
				tr.chrg.cpMin = start;
				tr.chrg.cpMax = end;
				tr.lpstrText = g_malloc(end - start + 1);
				
				scintilla_send_message(editor->sci, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
				sidebar_set_url(tr.lpstrText);
				
				DBRow* row = unk_db_get_primary(tr.lpstrText, "");
				sidebar_set_note(row->note);
				sidebar_set_rating(row->rating);
				
                sidebar_show(geany_plugin);			
				feedbar_show(geany_plugin);			
				
                GHashTable* secondary_rows = unk_db_get_secondary(tr.lpstrText, "none");
				
                feedbar_set_notes(secondary_rows);
                
                sidebar_activate();
				feedbar_activate();
				
                g_hash_table_destroy (secondary_rows);
				g_free(tr.lpstrText);
				g_free(row->note);
				g_free(row);
                
			}
			else if (scintilla_send_message(editor->sci, SCI_INDICATORVALUEAT, GEANY_INDICATOR_UNK_AUTO_DETECTED, nt->position))
			{
				gint start = scintilla_send_message(editor->sci, SCI_INDICATORSTART, GEANY_INDICATOR_UNK_AUTO_DETECTED, nt->position);
				gint end = scintilla_send_message(editor->sci, SCI_INDICATOREND, GEANY_INDICATOR_UNK_AUTO_DETECTED, nt->position);
				
				struct Sci_TextRange tr;
				tr.chrg.cpMin = start;
				tr.chrg.cpMax = end;
				tr.lpstrText = g_malloc(end - start + 1);
				
				scintilla_send_message(editor->sci, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
				sidebar_set_url(tr.lpstrText);
				
				gchar* note = g_strdup("");
				sidebar_set_note(note);
				
				sidebar_set_rating(0);
				
				sidebar_show(geany_plugin);			
				
                feedbar_reset();
                feedbar_show(geany_plugin);			
				
                sidebar_activate();
				feedbar_activate();
				
                
				g_free(tr.lpstrText);
				g_free(note);
				
			};
			
								
			break;
		case SCN_DWELLSTART:
			g_debug("unk_gui_editor_notify(SCN_DWELLSTART)");
            indicator_exist = FALSE;
			indicator_id = 0;
			indicator_bitmap = scintilla_send_message(editor->sci, SCI_INDICATORALLONFOR, nt->position, 0);
			
			if (indicator_bitmap & (1<<(GEANY_INDICATOR_UNK_POSITIVE_DB))) 
			{
				indicator_exist = TRUE;
				indicator_id = GEANY_INDICATOR_UNK_POSITIVE_DB;
			};
			if (indicator_bitmap & (1<<(GEANY_INDICATOR_UNK_NEUTRAL_DB)))
			{
				indicator_exist = TRUE;
				indicator_id = GEANY_INDICATOR_UNK_NEUTRAL_DB;
			};
			if (indicator_bitmap & (1<<(GEANY_INDICATOR_UNK_NEGATIVE_DB))) 
			{
				indicator_exist = TRUE;
				indicator_id = GEANY_INDICATOR_UNK_NEGATIVE_DB;
			};
				
			if (indicator_exist)
			{
				gint start = scintilla_send_message(editor->sci, SCI_INDICATORSTART, indicator_id, nt->position);
				gint end = scintilla_send_message(editor->sci, SCI_INDICATOREND, indicator_id, nt->position);
				struct Sci_TextRange tr;
				tr.chrg.cpMin = start;
				tr.chrg.cpMax = end;
				tr.lpstrText = g_malloc(end - start + 1);
				scintilla_send_message(editor->sci, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
				
				GString *text = g_string_new(NULL);
				gchar* notes_delimiter = "------------------\n";
				DBRow* row = unk_db_get_primary(tr.lpstrText, "none");
				text = g_string_append(text, row->note);
				text = g_string_append_c(text, '\n');
				
				GHashTable* secondary_rows = unk_db_get_secondary(tr.lpstrText, "none");
				
				GHashTableIter it;
				gpointer key, value;
				
				g_hash_table_iter_init (&it, secondary_rows);
				while (g_hash_table_iter_next (&it, &key, &value))
				{
					text = g_string_append(text, notes_delimiter);
					text = g_string_append(text, (gchar*)key);
					text = g_string_append_c(text, '\n');
					text = g_string_append(text, ((DBRow*)value)->note);
					text = g_string_append_c(text, '\n');
				}
				
				scintilla_send_message(editor->sci, SCI_CALLTIPSHOW, nt->position, (sptr_t)(text->str));
				
				g_string_free (text, TRUE);
				g_hash_table_destroy (secondary_rows);
				g_free(tr.lpstrText);
				g_free(row->note);
				g_free(row);
			}
			break;
		case SCN_DWELLEND:
			g_debug("unk_gui_editor_notify(SCN_DWELLEND)");
            if (scintilla_send_message (editor->sci, SCI_CALLTIPACTIVE, 0, 0))
			{
				scintilla_send_message (editor->sci, SCI_CALLTIPCANCEL, 0, 0);
			}
			break;
		
	};

	return FALSE;
}

/* Callback when the menu item is clicked. */
void
item_activate(GtkMenuItem *menuitem, gpointer gdata)
{
	GeanyPlugin *plugin = gdata;

	sidebar_show(plugin);
    feedbar_show(plugin);			
				
}

void
on_configure_response(G_GNUC_UNUSED GtkDialog *dialog, gint response,
                      G_GNUC_UNUSED gpointer user_data)
{
    g_debug("on_configure_response");
    if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
    {
        GKeyFile *config = g_key_file_new();
        gchar *data;
        gchar *config_dir = g_path_get_dirname(unk_info->config_file);

        unk_info->enable_urls_detect_on_open_document = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(config_widgets->checkbox_enable_urls_detect_on_open_document));
		
		unk_info->enable_db_detect_on_open_document = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(config_widgets->checkbox_enable_db_detect_on_open_document));
		
		SETPTR(unk_info->db_path, g_strdup(gtk_entry_get_text(GTK_ENTRY(config_widgets->entry_db_path))));
		
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    
		gtk_color_button_get_rgba( config_widgets->button_positive_rating_color, unk_info->positive_rating_color);
		
		gtk_color_button_get_rgba( config_widgets->button_neutral_rating_color, unk_info->neutral_rating_color);
		
		gtk_color_button_get_rgba( config_widgets->button_negative_rating_color, unk_info->negative_rating_color);
		
		G_GNUC_END_IGNORE_DEPRECATIONS
			
		guint i;
		foreach_document(i)
		{
			GeanyDocument *doc = documents[i];
			g_assert(doc->is_valid);
			
			scintilla_send_message(doc->editor->sci, SCI_INDICSETSTYLE, GEANY_INDICATOR_UNK_POSITIVE_DB, INDIC_STRAIGHTBOX);
			scintilla_send_message(doc->editor->sci, SCI_INDICSETALPHA, GEANY_INDICATOR_UNK_POSITIVE_DB, gdk_rgba_alpha_to_integer(unk_info->positive_rating_color));
			scintilla_send_message(doc->editor->sci, SCI_INDICSETFORE,  GEANY_INDICATOR_UNK_POSITIVE_DB, gdk_rgba_rgb_to_integer(unk_info->positive_rating_color));	
			
			scintilla_send_message(doc->editor->sci, SCI_INDICSETSTYLE, GEANY_INDICATOR_UNK_NEUTRAL_DB, INDIC_STRAIGHTBOX);
			scintilla_send_message(doc->editor->sci, SCI_INDICSETALPHA, GEANY_INDICATOR_UNK_NEUTRAL_DB, gdk_rgba_alpha_to_integer(unk_info->neutral_rating_color));
			scintilla_send_message(doc->editor->sci, SCI_INDICSETFORE,  GEANY_INDICATOR_UNK_NEUTRAL_DB, gdk_rgba_rgb_to_integer(unk_info->neutral_rating_color));	
			
			scintilla_send_message(doc->editor->sci, SCI_INDICSETSTYLE, GEANY_INDICATOR_UNK_NEGATIVE_DB, INDIC_STRAIGHTBOX);
			scintilla_send_message(doc->editor->sci, SCI_INDICSETALPHA, GEANY_INDICATOR_UNK_NEGATIVE_DB, gdk_rgba_alpha_to_integer(unk_info->negative_rating_color));
			scintilla_send_message(doc->editor->sci, SCI_INDICSETFORE,  GEANY_INDICATOR_UNK_NEGATIVE_DB, gdk_rgba_rgb_to_integer(unk_info->negative_rating_color));	
			
		}
		
		
		/* write config to file */
        g_key_file_load_from_file(config, unk_info->config_file, G_KEY_FILE_NONE, NULL);

        g_key_file_set_boolean(config, "general", "enable_urls_detect_on_open_document",
            unk_info->enable_urls_detect_on_open_document);
		g_key_file_set_boolean(config, "general", "enable_db_detect_on_open_document",
            unk_info->enable_db_detect_on_open_document);

		g_key_file_set_string(config, "general", "db_path", unk_info->db_path);
		
		gchar* positive_rating_color_s = gdk_rgba_to_string (unk_info->positive_rating_color);
		g_key_file_set_string(config, "colors", "positive_rating_color", positive_rating_color_s);
		g_free(positive_rating_color_s);
		
		gchar* neutral_rating_color_s = gdk_rgba_to_string (unk_info->neutral_rating_color);
		g_key_file_set_string(config, "colors", "neutral_rating_color", neutral_rating_color_s);
		g_free(neutral_rating_color_s);
		
		gchar* negative_rating_color_s = gdk_rgba_to_string (unk_info->negative_rating_color);
		g_key_file_set_string(config, "colors", "negative_rating_color", negative_rating_color_s);
		g_free(negative_rating_color_s);
		
		
		if (!g_file_test(config_dir, G_FILE_TEST_IS_DIR)
            && utils_mkdir(config_dir, TRUE) != 0)
        {
            dialogs_show_msgbox(GTK_MESSAGE_ERROR,
                _("Plugin configuration directory could not be created."));
        }
        else
        {
            data = g_key_file_to_data(config, NULL, NULL);
            utils_write_file(unk_info->config_file, data);
            g_free(data);
        }

        g_free(config_dir);
        g_key_file_free(config);
    }
}

GtkWidget *create_configure_widget(GeanyPlugin *plugin, GtkDialog *dialog, gpointer data)
{
	g_debug("create_configure_widget");
    GtkWidget *label_db_path, *entry_db_path, *vbox, *checkbox_enable_urls_detect_on_open_document, *checkbox_enable_db_detect_on_open_document;

	GtkWidget *label_positive_rating_color, *label_neutral_rating_color, *label_negative_rating_color;
	GtkColorButton *button_positive_rating_color, *button_neutral_rating_color, *button_negative_rating_color;
	GtkWidget *positive_rating_color_box, *neutral_rating_color_box, *negative_rating_color_box; 
	
	vbox = gtk_vbox_new(FALSE, 6);

	{
		label_db_path = gtk_label_new(_("Database path:"));
		entry_db_path = gtk_entry_new();
		if (unk_info->db_path != NULL)
			gtk_entry_set_text(GTK_ENTRY(entry_db_path), unk_info->db_path);
		config_widgets->entry_db_path = entry_db_path;
		
		gtk_box_pack_start(GTK_BOX(vbox), label_db_path, FALSE, FALSE, 5);
		gtk_box_pack_start(GTK_BOX(vbox), entry_db_path, FALSE, FALSE, 2);
		
	}
	
	{
		positive_rating_color_box = gtk_hbox_new(FALSE, 2);
		label_positive_rating_color = gtk_label_new(_("Positive rating"));
		
		button_positive_rating_color = (GtkColorButton*)gtk_color_button_new_with_rgba (unk_info->positive_rating_color);
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS
		gtk_color_button_set_use_alpha(button_positive_rating_color, TRUE); 
		G_GNUC_END_IGNORE_DEPRECATIONS
	
		gtk_color_button_set_title (button_positive_rating_color, _("Positive rating"));
		
		gtk_box_pack_start(GTK_BOX(positive_rating_color_box), label_positive_rating_color, FALSE, FALSE, 5);
		gtk_box_pack_start(GTK_BOX(positive_rating_color_box), GTK_WIDGET(button_positive_rating_color), TRUE, TRUE, 5);
		gtk_box_pack_start(GTK_BOX(vbox), positive_rating_color_box, FALSE, FALSE, 2);
		config_widgets->button_positive_rating_color = button_positive_rating_color;
		
		
		neutral_rating_color_box = gtk_hbox_new(FALSE, 2);
		label_neutral_rating_color = gtk_label_new(_("Neutral rating"));
		button_neutral_rating_color = (GtkColorButton*)gtk_color_button_new_with_rgba (unk_info->neutral_rating_color);
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS
		gtk_color_button_set_use_alpha(button_neutral_rating_color, TRUE); 
		G_GNUC_END_IGNORE_DEPRECATIONS
		gtk_color_button_set_title (button_neutral_rating_color, _("Neutral rating"));
		
		gtk_box_pack_start(GTK_BOX(neutral_rating_color_box), label_neutral_rating_color, FALSE, FALSE, 5);
		gtk_box_pack_start(GTK_BOX(neutral_rating_color_box), GTK_WIDGET(button_neutral_rating_color), TRUE, TRUE, 5);
		gtk_box_pack_start(GTK_BOX(vbox), neutral_rating_color_box, FALSE, FALSE, 5);
		config_widgets->button_neutral_rating_color = button_neutral_rating_color;
		
	
		negative_rating_color_box = gtk_hbox_new(FALSE, 2);
		label_negative_rating_color = gtk_label_new(_("Negative rating"));
		button_negative_rating_color = (GtkColorButton*)gtk_color_button_new_with_rgba (unk_info->negative_rating_color);
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS
		gtk_color_button_set_use_alpha(button_negative_rating_color, TRUE); 
		G_GNUC_END_IGNORE_DEPRECATIONS
		gtk_color_button_set_title (button_negative_rating_color, _("Negative rating"));
		
		gtk_box_pack_start(GTK_BOX(negative_rating_color_box), label_negative_rating_color, FALSE, FALSE, 5);
		gtk_box_pack_start(GTK_BOX(negative_rating_color_box), GTK_WIDGET(button_negative_rating_color), TRUE, TRUE, 5);
		gtk_box_pack_start(GTK_BOX(vbox), negative_rating_color_box, FALSE, FALSE, 5);
		config_widgets->button_negative_rating_color = button_negative_rating_color;
		
	}
	
	{
		checkbox_enable_urls_detect_on_open_document = gtk_check_button_new_with_mnemonic(_("_Enable url detect on open document"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_enable_urls_detect_on_open_document), unk_info->enable_urls_detect_on_open_document);
		//gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_enable_urls_detect_on_open_document), FALSE);
		config_widgets->checkbox_enable_urls_detect_on_open_document = checkbox_enable_urls_detect_on_open_document;
		gtk_box_pack_start(GTK_BOX(vbox), checkbox_enable_urls_detect_on_open_document, FALSE, FALSE, 6);
	}
	
	{
		checkbox_enable_db_detect_on_open_document = gtk_check_button_new_with_mnemonic(_("_Enable detect of words from database on open document"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox_enable_db_detect_on_open_document), unk_info->enable_db_detect_on_open_document);
		//gtk_button_set_focus_on_click(GTK_BUTTON(checkbox_enable_db_detect_on_open_document), FALSE);
		config_widgets->checkbox_enable_db_detect_on_open_document = checkbox_enable_db_detect_on_open_document;
		gtk_box_pack_start(GTK_BOX(vbox), checkbox_enable_db_detect_on_open_document, FALSE, FALSE, 6);
	}
	
	gtk_widget_show_all(vbox);

	g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), NULL);
	return vbox;
}

static void on_submenu_item_urls_detect_activate(G_GNUC_UNUSED GtkMenuItem *menuitem, G_GNUC_UNUSED gpointer user_data)
{
	GeanyDocument *doc = document_get_current();
	gint doc_length = sci_get_length(doc->editor->sci);
	set_url_marks(doc->editor, 0, doc_length);
}

static void on_submenu_item_db_detect_activate(G_GNUC_UNUSED GtkMenuItem *menuitem, G_GNUC_UNUSED gpointer user_data)
{
	GeanyDocument *doc = document_get_current();
	gint doc_length = sci_get_length(doc->editor->sci);
	set_db_marks(doc->editor, 0, doc_length);
}

void menu_init(void)
{
	g_debug("menu_init");
    menu_widgets = g_malloc (sizeof (MenuWidgets));
	
	menu_widgets->main_menu = gtk_menu_new();
	
	main_menu_item = gtk_menu_item_new_with_mnemonic(_("_Url Notes Keeper"));
	
	gtk_container_add(GTK_CONTAINER(geany_data->main_widgets->tools_menu), main_menu_item);
	
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(main_menu_item), menu_widgets->main_menu);

	{
		menu_widgets->submenu_item_urls_detect = gtk_menu_item_new_with_label(_("Detect urls in document"));
		gtk_widget_show(menu_widgets->submenu_item_urls_detect);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_widgets->main_menu), menu_widgets->submenu_item_urls_detect);
		g_signal_connect(menu_widgets->submenu_item_urls_detect, "activate",
			G_CALLBACK(on_submenu_item_urls_detect_activate), NULL);
	}
	
	{
		menu_widgets->submenu_item_db_detect = gtk_menu_item_new_with_label(_("Detect words from database in document"));
		gtk_widget_show(menu_widgets->submenu_item_db_detect);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_widgets->main_menu), menu_widgets->submenu_item_db_detect);
		g_signal_connect(menu_widgets->submenu_item_db_detect, "activate",
			G_CALLBACK(on_submenu_item_db_detect_activate), NULL);
	}
	
	
	gtk_widget_show_all(main_menu_item);
	
	/* make the menu item sensitive only when documents are open */
	ui_add_document_sensitive(main_menu_item);

}

void menu_cleanup(void)
{
	g_debug("menu_cleanup");
    gtk_widget_destroy(main_menu_item);
}

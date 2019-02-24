#include <geanyplugin.h>

#include <ctype.h>
#include <string.h>

#include "unkgui.h"
#include "unkmatcher.h"
#include "unkplugin.h"
#include "unksidebar.h"
#include "unkdb.h"

static gint GEANY_INDICATOR_UNK = 23;

gchar* get_backward_word(GeanyDocument *doc, gint cur_pos)
{
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
		URLPosition* pos = (URLPosition*)g_malloc0(sizeof(URLPosition));
		pos->start = ttf.chrgText.cpMin;
		pos->end = ttf.chrgText.cpMax;
		list = g_list_append(list, pos);
		
		ttf.chrg.cpMin = start_pos + 1;
		ttf.chrg.cpMax = len;
	}
	
	return list;
}

void set_mark(GeanyEditor *editor, gint range_start_pos, gint range_end_pos) 
{
	editor_indicator_set_on_range(editor, GEANY_INDICATOR_UNK , range_start_pos, range_end_pos);
	scintilla_send_message(editor->sci, SCI_INDICSETSTYLE, GEANY_INDICATOR_UNK, INDIC_DOTBOX);
	scintilla_send_message(editor->sci, SCI_INDICSETALPHA, GEANY_INDICATOR_UNK, 60);
}

void set_marks(GeanyEditor *editor, gint range_start_pos, gint range_end_pos)
{
	GList* list_url = NULL, *list_db = NULL, *list_db2 = NULL, *list_keys = NULL, *iterator = NULL;
	
	struct Sci_TextRange tr;
	tr.chrg.cpMin = range_start_pos;
	tr.chrg.cpMax = range_end_pos;
	tr.lpstrText = g_malloc(range_end_pos - range_start_pos + 1);
	scintilla_send_message(editor->sci, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
				
	
	list_url = unk_match_url_text(tr.lpstrText);
	
	list_keys = unk_db_get_keys();
	for (iterator = list_keys; iterator; iterator = iterator->next) 
	{
		list_db2 = search_marks(editor, (gchar*)iterator->data);
		list_db = g_list_concat(list_db, list_db2);
	};
	g_list_free_full (list_keys, g_free);
	
	list_url = g_list_concat(list_url, list_db);
	
	for (iterator = list_url; iterator; iterator = iterator->next) 
	{
		gint cur_start_pos = range_start_pos + ((URLPosition*)(iterator->data))->start;
		gint cur_end_pos = range_start_pos + ((URLPosition*)(iterator->data))->end;
		
		editor_indicator_set_on_range(editor, GEANY_INDICATOR_UNK , cur_start_pos, cur_end_pos);
		scintilla_send_message(editor->sci, SCI_INDICSETSTYLE, GEANY_INDICATOR_UNK, INDIC_DOTBOX);
		scintilla_send_message(editor->sci, SCI_INDICSETALPHA, GEANY_INDICATOR_UNK, 60);
	}
	
	
	g_list_free_full (list_url, g_free);
	g_free(tr.lpstrText);
							
}

/*
 * 	Occures on document opening.
 */
void on_document_open(GObject *obj, GeanyDocument *doc, gpointer user_data)
{
	if (unk_info->enable_parse_on_open_document)
	{
		/*set dwell interval*/
		scintilla_send_message(doc->editor->sci, SCI_SETMOUSEDWELLTIME, 500, 0);

		/* set tab size for calltips */
		scintilla_send_message(doc->editor->sci, SCI_CALLTIPUSESTYLE, 20, (long)NULL);
		 
		gint nLen = scintilla_send_message(doc->editor->sci, SCI_GETLENGTH, 0, 0);
		
		set_marks(doc->editor, 0, nLen);
	}
}

gboolean unk_gui_editor_notify(GObject *object, GeanyEditor *editor,
								 SCNotification *nt, gpointer data)
{
	/* data == GeanyPlugin because the data member of PluginCallback was set to NULL
	 * and this plugin has called geany_plugin_set_data() with the GeanyPlugin pointer as
	 * data */
	//GeanyPlugin *plugin = data;
	//GeanyData *geany_data = plugin->geany_data;
	gchar* word;
	gchar prev_char;
	gint cur_pos;
	/* For detailed documentation about the SCNotification struct, please see
	 * http://www.scintilla.org/ScintillaDoc.html#Notifications. */
	switch (nt->nmhdr.code)
	{
		case SCN_UPDATEUI:
			/* This notification is sent very often, you should not do time-consuming tasks here */
			break;
		case SCN_CHARADDED:
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
							
							set_marks(editor, g_start_pos, cur_pos-1);
						}
					}
				}
			}
			break;
		case SCN_INDICATORCLICK:
			
			if (scintilla_send_message(editor->sci, SCI_INDICATORVALUEAT, GEANY_INDICATOR_UNK, nt->position))
			{
				gint start = scintilla_send_message(editor->sci, SCI_INDICATORSTART, GEANY_INDICATOR_UNK, nt->position);
				gint end = scintilla_send_message(editor->sci, SCI_INDICATOREND, GEANY_INDICATOR_UNK, nt->position);
				
				struct Sci_TextRange tr;
				tr.chrg.cpMin = start;
				tr.chrg.cpMax = end;
				tr.lpstrText = g_malloc(end - start + 1);
				
				scintilla_send_message(editor->sci, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
				sidebar_set_url(tr.lpstrText);
				
				gchar* note = unk_db_get(tr.lpstrText, "");
				sidebar_set_note(note);
				
				sidebar_activate();
				sidebar_show(geany_plugin);			
				
				g_free(tr.lpstrText);
				g_free(note);
				
			};
								
			break;
		case SCN_DWELLSTART:
			if (scintilla_send_message(editor->sci, SCI_INDICATORVALUEAT, GEANY_INDICATOR_UNK, nt->position))
			{
				gint start = scintilla_send_message(editor->sci, SCI_INDICATORSTART, GEANY_INDICATOR_UNK, nt->position);
				gint end = scintilla_send_message(editor->sci, SCI_INDICATOREND, GEANY_INDICATOR_UNK, nt->position);
				struct Sci_TextRange tr;
				tr.chrg.cpMin = start;
				tr.chrg.cpMax = end;
				tr.lpstrText = g_malloc(end - start + 1);
				scintilla_send_message(editor->sci, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
				
				gchar* note = unk_db_get(tr.lpstrText, "none");
				scintilla_send_message(editor->sci, SCI_CALLTIPSHOW, nt->position, (sptr_t)note);
				
				g_free(tr.lpstrText);
				g_free(note);
			}
			break;
		case SCN_DWELLEND:
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
}

void
on_configure_response(G_GNUC_UNUSED GtkDialog *dialog, gint response,
                      G_GNUC_UNUSED gpointer user_data)
{
    if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
    {
        GKeyFile *config = g_key_file_new();
        gchar *data;
        gchar *config_dir = g_path_get_dirname(unk_info->config_file);

        //~ config_file = g_strconcat(geany->app->configdir,
            //~ G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
            //~ "unk", G_DIR_SEPARATOR_S, "general.conf", NULL);

        unk_info->enable_parse_on_open_document = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(config_widgets->checkbox_enable_parse_on_open_document));
		
		SETPTR(unk_info->db_path, g_strdup(gtk_entry_get_text(GTK_ENTRY(config_widgets->entry_db_path))));
		
		/* write stuff to file */
        g_key_file_load_from_file(config, unk_info->config_file, G_KEY_FILE_NONE, NULL);

        g_key_file_set_boolean(config, "general", "enable_parse_on_open_document",
            unk_info->enable_parse_on_open_document);

		g_key_file_set_string(config, "general", "db_path", unk_info->db_path);
		
		        if (!g_file_test(config_dir, G_FILE_TEST_IS_DIR)
            && utils_mkdir(config_dir, TRUE) != 0)
        {
            dialogs_show_msgbox(GTK_MESSAGE_ERROR,
                _("Plugin configuration directory could not be created."));
        }
        else
        {
            /* write config to file */
            data = g_key_file_to_data(config, NULL, NULL);
            utils_write_file(unk_info->config_file, data);
            g_free(data);
        }

        g_free(config_dir);
        g_key_file_free(config);
    }
}

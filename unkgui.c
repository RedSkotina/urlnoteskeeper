#include <geanyplugin.h>

#include <ctype.h>
#include <string.h>

#include "unkgui.h"
#include "unkmatcher.h"
#include "unkplugin.h"
#include "unksidebar.h"
#include "unkdb.h"


gchar* get_backward_word(GeanyDocument *doc, gint cur_pos)
{
	gint wstart, wend;
	gchar* word;
	
	gint wordchars_len;
	gchar *wordchars;
	gboolean wordchars_modified = FALSE;

	wordchars_len = scintilla_send_message(doc->editor->sci, SCI_GETWORDCHARS, 0, 0);
	wordchars = g_malloc0(wordchars_len + 2); /* 2 = temporarily added "'" and "\0" */
	scintilla_send_message(doc->editor->sci, SCI_GETWORDCHARS, 0, (sptr_t)wordchars);
	
	if (! strchr(wordchars, '.'))
	{
		/* temporarily add "'" to the wordchars */
		wordchars[wordchars_len] = '.';
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
			//ui_set_statusbar(TRUE, "type position:%ld", nt->position);
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
							ui_set_statusbar(TRUE, "word: %s position:%d", word, cur_pos);
							gint g_start_pos = cur_pos - strlen(word) - 1;
							
							GList* list = NULL, *iterator = NULL;
							list = unk_match_url_word(word);
							
							for (iterator = list; iterator; iterator = iterator->next) {
								//printf("Current item is '%s'\n", iterator->data);
								gint start_pos = g_start_pos + ((URLPosition*)(iterator->data))->start;
								gint end_pos = g_start_pos + ((URLPosition*)(iterator->data))->end;
								ui_set_statusbar(TRUE, "start: %d end:%d", start_pos, end_pos);
								//g_print("start: %d end:%d", start_pos, end_pos);
							
								editor_indicator_set_on_range(editor, GEANY_INDICATOR_SNIPPET , start_pos, end_pos);
								scintilla_send_message(editor->sci, SCI_INDICSETSTYLE, GEANY_INDICATOR_SNIPPET, INDIC_DOTBOX);
								scintilla_send_message(editor->sci, SCI_INDICSETALPHA, GEANY_INDICATOR_SNIPPET, 60);
  						
						}
							
							g_list_free_full (list, g_free);
													
							g_free(word);
						}
					}
				}
			}
			break;
		case SCN_INDICATORCLICK:
			
			if (scintilla_send_message(editor->sci, SCI_INDICATORVALUEAT, GEANY_INDICATOR_SNIPPET, nt->position))
			{
				gint start = scintilla_send_message(editor->sci, SCI_INDICATORSTART, GEANY_INDICATOR_SNIPPET, nt->position);
				gint end = scintilla_send_message(editor->sci, SCI_INDICATOREND, GEANY_INDICATOR_SNIPPET, nt->position);
				ui_set_statusbar(TRUE, "indicator click start: %d end:%d", start, end);
				struct Sci_TextRange tr;
				tr.chrg.cpMin = start;
				tr.chrg.cpMax = end;
				tr.lpstrText = g_malloc(end - start + 1);
				scintilla_send_message(editor->sci, SCI_GETTEXTRANGE, NULL, (sptr_t)&tr);
				sidebar_set_url(tr.lpstrText);
				gchar* note = unk_db_get(tr.lpstrText, "");
				sidebar_set_note(note);
							
				g_free(tr.lpstrText);
				g_free(note);
				
			};
								
			break;
		case SCN_DWELLSTART:
			if (scintilla_send_message(editor->sci, SCI_INDICATORVALUEAT, GEANY_INDICATOR_SNIPPET, nt->position))
			{
				gint start = scintilla_send_message(editor->sci, SCI_INDICATORSTART, GEANY_INDICATOR_SNIPPET, nt->position);
				gint end = scintilla_send_message(editor->sci, SCI_INDICATOREND, GEANY_INDICATOR_SNIPPET, nt->position);
				struct Sci_TextRange tr;
				tr.chrg.cpMin = start;
				tr.chrg.cpMax = end;
				tr.lpstrText = g_malloc(end - start + 1);
				scintilla_send_message(editor->sci, SCI_GETTEXTRANGE, NULL, (sptr_t)&tr);
				
				//gchar* d = g_strdup("none");
				gchar* note = unk_db_get(tr.lpstrText, "none");
				
				scintilla_send_message(editor->sci, SCI_CALLTIPSHOW, nt->position, note);
				//g_print("qq2");
				//g_print("db_gety: %s %s", tr.lpstrText, note);
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


/* Callback connected in plugin_configure(). */
void
on_configure_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	/* catch OK or Apply clicked */
	if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
	{
		/* We only have one pref here, but for more you would use a struct for user_data */
		//GtkWidget *entry = GTK_WIDGET(user_data);

		//g_free(welcome_text);
		//welcome_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		/* maybe the plugin should write here the settings into a file
		 * (e.g. using GLib's GKeyFile API)
		 * all plugin specific files should be created in:
		 * geany->app->configdir G_DIR_SEPARATOR_S plugins G_DIR_SEPARATOR_S pluginname G_DIR_SEPARATOR_S
		 * e.g. this could be: ~/.config/geany/plugins/Demo/, please use geany->app->configdir */
	}
}

#include <geanyplugin.h>

#include "unkkeys.h"
#include "unkdb.h"
#include "unksidebar.h"
#include "unkfeedbar.h"
#include "unkgui.h"

extern GeanyPlugin		*geany_plugin;

/* geany debugger key group */
struct GeanyKeyGroup* key_group;

/* type to hold information about a hotkey */
typedef struct _keyinfo {
	const char* key_name;
	const char* key_label;
	enum KEYS key_id;
} keyinfo; 

/* hotkeys list */
keyinfo keys[] = {
	{ "key_add", _("Add word to database"), KEY_ADD},
	{ "key_delete", _("Delete word from database"), KEY_DELETE},
	{ "key_detect_urls", _("Detect urls in text"), KEY_DETECT_URLS},
	{ "key_detect_db", _("Detect words from database in text"), KEY_DETECT_DB},
	{ NULL, NULL, 0}
};

/* 
 * init hotkeys
 */
gboolean keys_init(void)
{
	g_debug("keys_init");
    int _index, count;

	/* keys count */
	count = 0;
	while (keys[count++].key_name)
		;
	
	/* set keygroup */
	key_group = plugin_set_key_group(
		geany_plugin,
		_("UNK"),
		count - 1,
		keys_callback);

	/* add keys */
	_index = 0;
	while (keys[_index].key_name)
	{
		keybindings_set_item(
			key_group,
			keys[_index].key_id,
			NULL,
			0,
			0,
			keys[_index].key_name,
			_(keys[_index].key_label),
			NULL);
		_index++;
	}
	 	
	return 1;
}


gboolean keys_callback(guint key_id)
{
	switch (key_id)
	{
		case KEY_ADD:
		{
			GeanyDocument *doc = document_get_current();
			if (doc)
			{
				if (sci_get_selected_text_length(doc->editor->sci) > 0)
				{
					gchar* text = sci_get_selection_contents(doc->editor->sci);
					
					struct DBRow* row = unk_db_get_primary(text, "");
					if ( strlen(row->note) == 0 )
					{
						unk_db_set(text, "", 0);
					
						gint sel_start_pos = sci_get_selection_start(doc->editor->sci);
						gint sel_end_pos = sci_get_selection_end(doc->editor->sci);
		
						set_mark(doc->editor, sel_start_pos, sel_end_pos, 0);
						
						sidebar_set_url(text);
						sidebar_set_note("");
						sidebar_set_rating(0);
				
                        
						sidebar_show(geany_plugin);			
                        //sidebar_hide_all_secondary_frames();
                        feedbar_reset();
                        feedbar_show(geany_plugin);			
				
                        sidebar_activate();
						feedbar_activate();
						
					}
					g_free(row->note);
					g_free(row);
					g_free(text);
				}
			}
			break;
		}
		case KEY_DELETE:
		{
			GeanyDocument *doc = document_get_current();
			if (doc)
			{
				if (sci_get_selected_text_length(doc->editor->sci) > 0)
				{
					gint current_position = scintilla_send_message(doc->editor->sci, SCI_GETCURRENTPOS, 0, 0 );
					
					gboolean indicator_exist = FALSE;
					gint indicator_id = 0;
					gint indicator_bitmap = scintilla_send_message(doc->editor->sci, SCI_INDICATORALLONFOR, current_position, 0);
					
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
						gint start = scintilla_send_message(doc->editor->sci, SCI_INDICATORSTART, indicator_id, current_position);
						gint end = scintilla_send_message(doc->editor->sci, SCI_INDICATOREND, indicator_id, current_position);
						
						struct Sci_TextRange tr;
						tr.chrg.cpMin = start;
						tr.chrg.cpMax = end;
						tr.lpstrText = g_malloc(end - start + 1);
					
						scintilla_send_message(doc->editor->sci, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
					
						gboolean ok = unk_db_delete(tr.lpstrText);
						if ( ok )
						{
						
							sidebar_set_url(_("No url selected."));
							sidebar_set_note("");
							sidebar_set_rating(0);
				
							sidebar_deactivate();
							
                            //sidebar_hide_all_secondary_frames();
                            feedbar_reset();
                            feedbar_deactivate();
							
							clear_all_db_marks(doc->editor);
			
							gint doc_length = sci_get_length(doc->editor->sci);
							if ( doc_length > 0)
							{
								set_db_marks(doc->editor, 0, doc_length);
								
							}
                            
						}
						g_free(tr.lpstrText);
					}
				}
			}
			break;
		}
		case KEY_DETECT_URLS:
		{
			GeanyDocument *doc = document_get_current();
			if (doc)
			{
				gint doc_length = sci_get_length(doc->editor->sci);
				if ( doc_length > 0)
				{
					set_url_marks(doc->editor, 0, doc_length);
					
				}
			}
			break;
		}
		case KEY_DETECT_DB:
		{
			GeanyDocument *doc = document_get_current();
			if (doc)
			{
				clear_all_db_marks(doc->editor);
			
				gint doc_length = sci_get_length(doc->editor->sci);
				if ( doc_length > 0)
				{
					set_db_marks(doc->editor, 0, doc_length);
					
				}
			}
			break;
		}
	}
	
	return TRUE;
} 

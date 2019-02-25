#include <geanyplugin.h>

#include "unkkeys.h"
#include "unkdb.h"
#include "unksidebar.h"
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
	{ "key_add", N_("Add word to database"), KEY_ADD},
	{ "key_detect_urls", N_("Detect urls in text"), KEY_DETECT_URLS},
	{ "key_detect_db", N_("Detect words from database in text"), KEY_DETECT_DB},
	{ NULL, NULL, 0}
};

/* 
 * init hotkeys
 */
gboolean keys_init(void)
{
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
					gchar* ret = unk_db_get(text, "");
					if ( strlen(ret) == 0 )
					{
						unk_db_set(text, "");
					
						gint sel_start_pos = sci_get_selection_start(doc->editor->sci);
						gint sel_end_pos = sci_get_selection_end(doc->editor->sci);
		
						set_mark(doc->editor, sel_start_pos, sel_end_pos);
						
						sidebar_set_url(text);
						sidebar_set_note("");
				
						sidebar_activate();
						sidebar_show(geany_plugin);			
				
					}
					g_free(ret);
					g_free(text);
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
				if ( doc_length > 2)
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
				gint doc_length = sci_get_length(doc->editor->sci);
				if ( doc_length > 2)
				{
					set_db_marks(doc->editor, 0, doc_length);
					
				}
			}
			break;
		}
	}
	
	return TRUE;
} 

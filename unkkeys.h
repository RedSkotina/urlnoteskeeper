#ifndef UNK_KEYS_H
#define UNK_KEYS_H

//#include <glib.h>

/* hotkeys enumeration */
enum KEYS
{
	KEY_ADD
};

gboolean keys_init(void);

gboolean keys_callback(guint key_id);

#endif 

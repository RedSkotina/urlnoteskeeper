#ifndef UNK_DB_H
#define UNK_DB_H 1

gint unk_db_init(const gchar* filename);

gchar* unk_db_get(const gchar* key, const gchar* default_value);

gboolean unk_db_set(const gchar* key, const gchar* value);

gint unk_db_cleanup(void);
		
GList* unk_db_get_keys();				  
#endif

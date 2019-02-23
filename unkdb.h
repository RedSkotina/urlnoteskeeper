#ifndef UNK_DB_H
#define UNK_DB_H 1

gint unk_db_init(const gchar* filename);

gchar* unk_db_get(gchar* key, gchar* default_value);

gboolean unk_db_set(gpointer key, gpointer value);

void unk_db_cleanup(void);
						  
#endif

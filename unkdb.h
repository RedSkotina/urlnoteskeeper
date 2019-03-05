#ifndef UNK_DB_H
#define UNK_DB_H 1


#include <sqlite3.h>

struct DBRow
{
    gchar* url;
    gchar* note;
    gint rating;
    gchar* error;
};

struct DBInfo
{
    gchar* name;
    sqlite3* dbc;
};

gint unk_db_init(const gchar* filepath);

struct DBRow* unk_db_get_primary(const gchar* key, const gchar* default_value);

GHashTable* unk_db_get_secondary(const gchar* key, const gchar* default_value);

gboolean unk_db_set(const gchar* key, const gchar* value, gint rating);

gboolean unk_db_delete(const gchar* key);

gint unk_db_cleanup(void);
		
GList* unk_db_get_keys();				  

GList* unk_db_get_all();

void row_destroyed(gpointer value);				  

#endif

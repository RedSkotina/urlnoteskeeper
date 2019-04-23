#ifndef UNK_DB_H
#define UNK_DB_H 1


#include <sqlite3.h>

typedef struct DBRow
{
    gchar* url;
    gchar* note;
    gint rating;
    gchar* error;
} DBRow;

typedef struct DBInfo
{
    gchar* name;
    sqlite3* dbc;
} DBInfo;

gint unk_db_init(const gchar* filepath);

DBRow* unk_db_get_primary(const gchar* key, const gchar* default_value);

GHashTable* unk_db_get_secondary(const gchar* key, const gchar* default_value);

gboolean unk_db_set(const gchar* key, const gchar* value, gint rating);

gboolean unk_db_delete(const gchar* key);

gint unk_db_cleanup(void);
		
GList* unk_db_get_keys();				  

GList* unk_db_get_all_local();

GList* unk_db_get_all_global();

GList* unk_db_get_all_db_names();

void row_destroyed(gpointer value);				  

#endif

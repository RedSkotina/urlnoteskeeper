#include <geanyplugin.h>

#include <sqlite3.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
//#include <glib/gi18n.h>
#include "unkdb.h"

#define SQLITE_DB_FILE_SUFFIX ".sqlite3"
#define SQLITE_DB_USER_VERSION 2

static sqlite3* primary_dbc = NULL;
static GList* secondary_dbc_list = NULL;

static gchar* primary_db_filename = NULL;

#define HANDLE_ERROR(r, f, h) \
	 if (r != SQLITE_OK && r != SQLITE_DONE) \
	 { \
		extract_error(r, f, h, __LINE__,__func__); \
		return -1; \
	 }

#define SHOW_ERROR(r, f, h) \
	if (r != SQLITE_OK && r != SQLITE_DONE) \
	{ \
		gchar *err_msg = extract_error(r, f, h, __LINE__,__func__); \
		g_warning("%s",err_msg); \
        msgwin_status_add_string(err_msg); \
	}

#define GET_ERROR(r, f, h) \
	((r != SQLITE_OK && r != SQLITE_DONE)? extract_error(r, f, h, __LINE__,__func__) : NULL)
	

static void row_hashtable_key_destroyed(gpointer key) {
	g_free((gchar*)key);
}

void row_destroyed(gpointer value) {
	g_free(((DBRow*)value)->url);
	g_free(((DBRow*)value)->note);
	g_free((DBRow*)value);
}
 
gchar*
extract_error (int retcode, gchar *fn, sqlite3 *dbc, int line, const char *func)
{
	gchar* buffer = g_malloc(255);
	gchar *temp = g_malloc(255);
	const char *err = NULL;
	
	g_sprintf(buffer, "Sqlite error. In '%s' (line no. %d) reported for %s : Error-code = %d\n", func, line-1, fn, retcode);
	err = sqlite3_errmsg (dbc);
	if (err)
	{
		g_sprintf(temp, "Error message : %s\n", err);
		g_strlcat(buffer, temp, 255);
	}
	g_free(temp);
	return buffer; 
	 
}

gchar* expand_shell_replacers(const gchar* filename)
{
	//separator unix  - > platform-specific 
    gchar **split0 = g_strsplit(filename, "/", -1);
	gchar* text0 = g_strjoinv(G_DIR_SEPARATOR_S, split0);
	g_strfreev(split0);
	
    gchar **split = g_strsplit(text0, G_DIR_SEPARATOR_S, -1);
	for (int i =0; i < g_strv_length (split); i++)
		if (*split[i] == '~')
			SETPTR(split[i], g_strdup(g_get_home_dir()));
	
	gchar* text = g_strjoinv(G_DIR_SEPARATOR_S, split);
	g_strfreev(split);
    
    g_free(text0);
	return text;
}
 
gint get_user_version(sqlite3* dbc)
 {
    g_debug("get_user_version");
    static sqlite3_stmt *stmt;
    int db_version;

    if(sqlite3_prepare_v2(dbc, "PRAGMA user_version;", -1, &stmt, NULL) == SQLITE_OK) {
        while(sqlite3_step(stmt) == SQLITE_ROW) {
            db_version = sqlite3_column_int(stmt, 0);
        }
    } else {
		g_error("sqlite3_prepare_v2 return error: %s", sqlite3_errmsg(dbc));
    }
    sqlite3_finalize(stmt);

    return db_version;
}

static gint unk_update_db_schema(sqlite3 *dbc, gint start_version, gint end_version)
{
	g_debug("unk_update_db_schema");
    sqlite3_stmt *stmt = NULL;
	gint ret;
	gchar sql[255];
				
	gint current_version = start_version;
	while (current_version < end_version)
	{
		g_message("update database from user_version %d to %d", current_version, current_version + 1);
				
		switch (current_version)
		{
			case 1:
				ret = sqlite3_exec(primary_dbc, "BEGIN TRANSACTION;", NULL, NULL, NULL);
				if (ret != SQLITE_OK)
				{
					SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
					return current_version; 
				}
				
				ret = sqlite3_exec(primary_dbc, "ALTER TABLE urlnotes ADD RATING INTEGER NOT NULL DEFAULT 0;", NULL, NULL, NULL);
				if (ret != SQLITE_OK)
				{
					SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
					return current_version; 
				}
				
				g_sprintf(sql, "PRAGMA user_version = %d;", current_version + 1);
				ret = sqlite3_prepare_v2 (primary_dbc, sql, -1, &stmt, NULL);
				if (ret != SQLITE_OK)
				{
					SHOW_ERROR(ret, "sqlite3_prepare_v2", primary_dbc)
					return current_version; 
				}
				
				ret = sqlite3_step (stmt);
				if (ret != SQLITE_OK && ret != SQLITE_DONE)
				{
					SHOW_ERROR(ret, "sqlite3_step", primary_dbc)
					return current_version; 
				}
	
				ret = sqlite3_exec(primary_dbc, "COMMIT TRANSACTION;", NULL, NULL, NULL);
				if (ret != SQLITE_OK)
				{
					SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
					return current_version; 
				}
				sqlite3_reset(stmt);
	
				break;
		}
		
		g_message("success");
				
		current_version++;
	}
	
	sqlite3_finalize(stmt);
				
	return current_version;
}

gboolean unk_db_lock_create(gchar* filename)
{
	g_debug("unk_db_lock_create");
	gchar* uuid = NULL;
	gssize uuid_size;
	
	gchar* lock_filename;
	
	lock_filename = g_strconcat (filename, ".lock", NULL);
	
	if (!g_file_test (lock_filename, G_FILE_TEST_EXISTS)) 
	{
		uuid = g_uuid_string_random();
		uuid_size = strlen(uuid);	
		g_file_set_contents(lock_filename, uuid, uuid_size, NULL);
	}
	else
	{
		g_free(lock_filename);
		g_free(uuid);
		return FALSE;
	}	
	g_free(lock_filename);
	g_free(uuid);
	return TRUE;
}

gboolean unk_db_lock_remove(gchar* filename)
{
	g_debug("unk_db_lock_remove");
	gchar* lock_filename = g_strconcat (filename, ".lock", NULL);
	
	if (g_file_test (lock_filename, G_FILE_TEST_EXISTS)) 
	{
		if( g_remove (lock_filename) != 0) 
		{
			g_free(lock_filename);
			return FALSE;
		}
	}
	g_free(lock_filename);
	return TRUE;
}

static gboolean unk_db_check_duplicates()
{
    g_debug("unk_db_check_duplicates");
	gboolean result = TRUE;
    GList* iterator;
    GList* list = unk_db_get_all_local();
    
    GHashTable* ht = g_hash_table_new_full (g_str_hash, g_str_equal, row_hashtable_key_destroyed, g_free);
    
    for (iterator = list; iterator; iterator = iterator->next) 
    {
        gchar *lkey = g_utf8_strdown(((DBRow*)iterator->data)->url, -1);
        g_print(lkey);
        gint* value = g_hash_table_lookup(ht, lkey);
        gint* value1 = g_malloc(sizeof (gint));
        *value1 = 1;        
        if (value)
        {
            *value1 = *value1 + 1;
            g_hash_table_replace(ht, lkey, value1);
        }
        else
            g_hash_table_insert(ht, lkey, value1);
    }
    g_list_free_full (list, row_destroyed);
    
    gchar* buffer = g_malloc(255);
	
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, ht);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        if (*((gint*)value) > 1)
        {
            g_sprintf(buffer, "Warning: detected duplicated key '%s' in database '%s'", (gchar*)key, primary_db_filename);
	        g_warning(buffer);
            msgwin_status_add_string(buffer);
            result = FALSE;           
        }
    }
    g_hash_table_remove_all(ht);
    return result;
}

// GList* filter_duplicated_keys(GList* list)
// {
    
// }

gint unk_db_init(const gchar* filepath)
{
	g_debug("unk_db_init");
    sqlite3* secondary_dbc = NULL;
	gint ret = 0;
	gchar *native_filepath, *dirname, *db_filename;
	const gchar *filename;
	GDir *db_dir;
    GError *dir_error;
    gint user_version;
    
    native_filepath = expand_shell_replacers(filepath);
	
	primary_db_filename = g_strdup(native_filepath);
	
	if (!unk_db_lock_create(primary_db_filename))
	{
		g_warning("cant lock database. probably another instance of plugin is running.");
		msgwin_status_add_string(_("error: cant lock database. another instance of plugin probably running.")); 
        return -1;
	}
	dirname = g_path_get_dirname(native_filepath);
	
	db_dir = g_dir_open(dirname, 0, &dir_error);
	if (db_dir == NULL)
	{
		g_warning("cant open primary database directory '%s'", dirname);
		if (dir_error != NULL)
		{
			g_warning("error: %s", dir_error->message);
			g_clear_error (&dir_error);
		}
		g_free(dirname);
		g_free(native_filepath);
		return -1;
	}
	
	// primary database
	ret = sqlite3_open_v2(native_filepath , &primary_dbc, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if (ret != SQLITE_OK)
	{
		g_warning("sqlite3_open_v2 primary database '%s' return error: %s", native_filepath, sqlite3_errmsg(primary_dbc));
		primary_dbc = NULL;
		g_free(dirname);
		g_free(native_filepath);
		return -1;
	}
	
	if ((user_version = get_user_version(primary_dbc)) == 0)
	{
        ret = sqlite3_exec(primary_dbc, "BEGIN TRANSACTION;", NULL, NULL, NULL);
        if (ret != SQLITE_OK)
        {
            SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
            return -1; 
        }
        
        gchar* sql = 	"CREATE TABLE IF NOT EXISTS urlnotes("  \
					"URL TEXT PRIMARY KEY     NOT NULL," \
					"NOTE           TEXT    NOT NULL," \
					"RATING INTEGER NOT NULL DEFAULT 0);";
		
        ret = sqlite3_exec(primary_dbc, sql, NULL, NULL, NULL);
        if (ret != SQLITE_OK)
        {
            SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
            return -1; 
        }
        
		gchar sql2[255];
        g_sprintf(sql2, "PRAGMA user_version = %d;", SQLITE_DB_USER_VERSION);
        ret = sqlite3_exec(primary_dbc, sql2, NULL, NULL, NULL);
        if (ret != SQLITE_OK)
        {
            SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
            return -1; 
        }
       
        ret = sqlite3_exec(primary_dbc, "COMMIT TRANSACTION;", NULL, NULL, NULL);
        if (ret != SQLITE_OK)
        {
            SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
            return -1; 
        }
    }
	else
	{
		if (user_version > 0 && user_version < SQLITE_DB_USER_VERSION)
		{
			user_version = unk_update_db_schema(primary_dbc, user_version, SQLITE_DB_USER_VERSION);
		}
		else if (user_version == SQLITE_DB_USER_VERSION) 
		{
			//ok
    	}
		else 
        {
			g_warning("primary database '%s' has wrong user_version %d . Current user_version %d", native_filepath, user_version, SQLITE_DB_USER_VERSION);
            unk_db_cleanup();
            return -1;
        }
    };

	unk_db_check_duplicates();
    
	//secondary database
	while ((filename = g_dir_read_name(db_dir)))
	{
		if (g_str_has_suffix(filename, SQLITE_DB_FILE_SUFFIX)) 
		{
			db_filename = g_build_filename (dirname, filename, NULL);
			
			if (g_strcmp0(db_filename, native_filepath) != 0 )
			{
				g_message("open secondary database: %s",db_filename);
                gint ret = sqlite3_open_v2(db_filename , &secondary_dbc, SQLITE_OPEN_READONLY, NULL);
				
                if (ret != SQLITE_OK)
				{ 
					g_warning("sqlite3_open_v2 secondary database '%s' return error: %s", db_filename, sqlite3_errmsg(secondary_dbc));
					secondary_dbc = NULL;
					g_free(db_filename);
					continue;
				}
				//TODO: verify schema: PRAGMA schema_version; or PRAGMA user_version;
				// SELECT * FROM sqlite_master;
				if ((user_version = get_user_version(secondary_dbc)) == SQLITE_DB_USER_VERSION)
				{
					struct DBInfo* db = g_malloc(sizeof (struct DBInfo));
					db->name = g_strdup(filename);
					db->dbc = secondary_dbc;
					secondary_dbc_list = g_list_append(secondary_dbc_list, db);
				}
				else
                { 
					g_warning("secondary database '%s' has wrong user_version %d instead %d", db_filename, user_version, SQLITE_DB_USER_VERSION);
                    sqlite3_close_v2(secondary_dbc);
                }
					
			}
			g_free(db_filename);
		}
		
    }
	
	g_dir_close(db_dir);
	g_free(dirname);
	g_free(native_filepath);
	return ret;
}

DBRow* unk_db_get_primary(const gchar* key, const gchar* default_value)
{	
	g_debug("unk_db_get_primary");
    gchar *lkey = g_utf8_strdown(key, -1);
    
    DBRow* row = g_malloc(sizeof(DBRow));
    row->error = NULL;
    row->url = g_strdup(lkey);
	row->note = NULL;
    row->rating = 0;
    
    if (!primary_dbc) 
    {
        g_warning(_("primary database is not opened"));
        msgwin_status_add_string(_("error: primary database is not opened"));
        g_free(lkey);
        return row;
    }
      
	sqlite3_stmt *stmt = NULL;
	gint ret;
	
	gchar* sql = "SELECT note, rating FROM urlnotes WHERE url=?1 LIMIT 1;";
	ret = sqlite3_prepare_v2(primary_dbc, sql, strlen(sql)+1, &stmt, 0);    
    if (ret != SQLITE_OK) {
		SHOW_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		g_free(lkey);
        return row;
    }
    
    ret = sqlite3_bind_text(stmt, 1, lkey, strlen(lkey), SQLITE_TRANSIENT );
    if (ret != SQLITE_OK) {
		SHOW_ERROR(ret, "sqlite3_bind_text", primary_dbc);
		g_free(lkey);
        return row;
    }
    
    //g_debug("%s", sqlite3_expanded_sql(stmt));
    
    ret = sqlite3_step(stmt);		
    if ( ret == SQLITE_ROW )
    {
		row->note = g_strdup((const gchar*)sqlite3_column_text(stmt,0));
		row->rating = (gint)sqlite3_column_int(stmt,1);
	}
    else if (ret == SQLITE_DONE)
    {
		//key not found 
		row->note = g_strdup(default_value);
	}
    else
    {
		SHOW_ERROR(ret, "sqlite3_step", primary_dbc);
		const char *err = NULL;
		err = sqlite3_errmsg (primary_dbc);
		if (err)
			g_warning ("\nError message : %s\n\n",err);
		row->error = g_strdup(err);
	}
         
	sqlite3_finalize(stmt);
    g_free(lkey);
    
    return row;
}


GHashTable* unk_db_get_secondary(const gchar* key, const gchar* default_value)
{	
	g_debug("unk_db_get_secondary");
    gchar *lkey = g_utf8_strdown(key, -1);
    GHashTable* ht = g_hash_table_new_full (g_str_hash, g_str_equal, row_hashtable_key_destroyed, row_destroyed);
    
    sqlite3_stmt *stmt = NULL;
	gint ret;
	gchar* sql = "SELECT note, rating FROM urlnotes WHERE url=?1 LIMIT 1;";
	    
	for (GList* it = secondary_dbc_list; it; it = it->next) 
	{
        DBRow* row = g_malloc(sizeof(DBRow));
        DBInfo* db = (DBInfo*)(it->data);
        
        row->url = g_strdup(lkey);
        row->note = NULL;
        row->rating = 0;
        
        ret = sqlite3_prepare_v2(db->dbc, sql, strlen(sql)+1, &stmt, 0);    
        if (ret != SQLITE_OK) {
            row->error = GET_ERROR(ret, "sqlite3_prepare_v2", db->dbc);
            goto select_continue;
        }
        
        ret = sqlite3_bind_text(stmt, 1, lkey, strlen(lkey), SQLITE_TRANSIENT );
        if (ret != SQLITE_OK) {
            row->error = GET_ERROR(ret, "sqlite3_bind_text", db->dbc);
            goto select_continue;
        }
        
        ret = sqlite3_step(stmt);		
        if ( ret == SQLITE_ROW )
        {
            row->note = g_strdup((const gchar*)sqlite3_column_text(stmt,0));
            row->rating = (gint)sqlite3_column_int(stmt,1);
            row->error = NULL;
        }
        else if (ret == SQLITE_DONE)
        {
			//not found
            //row->note = g_strdup(default_value);
            g_free(row->url);
            g_free(row);
            sqlite3_reset(stmt);
            continue;
        }
        else
        {
            row->error = GET_ERROR(ret, "sqlite3_step", db->dbc);
        }
        
    select_continue:
        sqlite3_reset(stmt);
    
        g_hash_table_insert(ht, g_strdup(db->name), row);
        
    }
             
	sqlite3_finalize(stmt);
	g_free(lkey);
	return ht;
}

gboolean unk_db_set(const gchar* key, const gchar* value, gint rating)
{
	g_debug("unk_db_set");
    gchar *lkey = g_utf8_strdown(key, -1);
    sqlite3_stmt *stmt;
	gint ret;
	
    if (!primary_dbc) 
    {
        g_warning(_("primary database is not opened"));
        msgwin_status_add_string(_("error: primary database is not opened"));
        g_free(lkey);
        return FALSE;
    }
	if (lkey == NULL )
	{
		g_warning("error unk_db_set: lkey == NULL");
		g_free(lkey);
        return FALSE;
	}
	if (!strlen(lkey) )
	{
		g_warning("error unk_db_set: strlen(lkey) == 0");
		g_free(lkey);
        return FALSE;
	}
	
	ret = sqlite3_prepare_v2 (primary_dbc, "REPLACE INTO urlnotes (url, note, rating) VALUES (?1,?2,?3);", -1, &stmt, NULL);
	HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);

	ret = sqlite3_bind_text(stmt, 1, lkey,  strlen(lkey), SQLITE_TRANSIENT );
    HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
	
	ret = sqlite3_bind_text(stmt, 2, value, strlen(value), NULL);
    HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
	
	ret = sqlite3_bind_int(stmt, 3, rating);
    HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		
	ret = sqlite3_step (stmt);
	HANDLE_ERROR(ret, "sqlite3_step", primary_dbc);
	 
	sqlite3_finalize(stmt);
	
	g_free(lkey);
	return TRUE;
}

gboolean unk_db_delete(const gchar* key)
{
	g_debug("unk_db_delete");
    gchar *lkey = g_utf8_strdown(key, -1);
    sqlite3_stmt *stmt;
	gint ret;
	
    if (!primary_dbc) 
    {
        g_warning(_("primary database is not opened"));
        msgwin_status_add_string(_("error: primary database is not opened"));
        g_free(lkey);
        return FALSE;
    }
	if (lkey == NULL )
	{
		g_warning("error unk_db_delete: lkey == NULL");
		g_free(lkey);
        return FALSE;
	}
	if (!strlen(lkey) )
	{
		g_warning("error unk_db_delete: strlen(lkey) == 0");
		g_free(lkey);
        return FALSE;
	}
	
	ret = sqlite3_prepare_v2 (primary_dbc, "DELETE FROM urlnotes WHERE url =  ?1;", -1, &stmt, NULL);
	HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);

	ret = sqlite3_bind_text(stmt, 1, lkey,  strlen(lkey), SQLITE_TRANSIENT );
    HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		
	ret = sqlite3_step (stmt);
	HANDLE_ERROR(ret, "sqlite3_step", primary_dbc);
	 
	sqlite3_finalize(stmt);
	g_free(lkey);
	return TRUE;
}

GList* unk_db_get_keys()
{
	g_debug("unk_db_get_keys");
    GList* list = NULL;
	//gchar* value;
	sqlite3_stmt *stmt;
	gint ret;
	
    if (!primary_dbc) 
    {
        g_warning(_("primary database is not opened"));
        msgwin_status_add_string(_("error: primary database is not opened"));
        return list;
    }
    
	gchar* sql = "SELECT url FROM urlnotes;";
	ret = sqlite3_prepare_v2(primary_dbc, sql, strlen(sql)+1, &stmt, 0);    
    if (ret != SQLITE_OK) {
		SHOW_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		return list;
    }
    while ( (ret = sqlite3_step(stmt)) == SQLITE_ROW )
    {
		gchar * lkey = g_utf8_strdown((const gchar*)sqlite3_column_text(stmt,0), -1);
        list = g_list_append(list, lkey);
    }
    
    if (ret != SQLITE_DONE)
    {
		SHOW_ERROR(ret, "sqlite3_step", primary_dbc);
		const char *err = NULL;
		err = sqlite3_errmsg (primary_dbc);
		if (err)
			g_warning ("\nError message : %s\n\n",err);
	}
         
	sqlite3_finalize(stmt);
	
	return list;
}

GList* unk_db_get_all_local()
{
	
	g_debug("unk_db_get_all_local");
    GList* list = NULL;
	sqlite3_stmt *stmt;
	gint ret;
	if (!primary_dbc) 
    {
        g_warning(_("primary database is not opened"));
        msgwin_status_add_string(_("error: primary database is not opened"));
        return list;
    }
	gchar* sql = "SELECT url,note,rating FROM urlnotes;";
	ret = sqlite3_prepare_v2(primary_dbc, sql, strlen(sql)+1, &stmt, 0);    
    if (ret != SQLITE_OK) {
		SHOW_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		return list;
    }
    while ( (ret = sqlite3_step(stmt)) == SQLITE_ROW )
    {
		DBRow* row = g_malloc(sizeof(DBRow));
		row->error = NULL;
		row->url = NULL;
		row->note = NULL;
		row->rating = 0;
    
		row->url = g_utf8_strdown((const gchar*)sqlite3_column_text(stmt,0), -1);
		row->note = g_strdup((const gchar*)sqlite3_column_text(stmt,1));
		row->rating = (gint)sqlite3_column_int(stmt,2);
		list = g_list_append(list, row);
	}
    
    if (ret != SQLITE_DONE)
    {
		SHOW_ERROR(ret, "sqlite3_step", primary_dbc);
		const char *err = NULL;
		err = sqlite3_errmsg (primary_dbc);
		if (err)
			g_warning("\nError message : %s\n\n",err);
	}
         
	sqlite3_finalize(stmt);
	
	return list;
}

GList* unk_db_get_all_global()
{
	
	g_debug("unk_db_get_all_global");
    GList* list = NULL;
	sqlite3_stmt *stmt;
	gint ret;
	if (!primary_dbc) 
    {
        g_warning(_("primary database is not opened"));
        msgwin_status_add_string(_("error: primary database is not opened"));
        return list;
    }
	gchar* sql = "SELECT url,note,rating FROM urlnotes;";
	ret = sqlite3_prepare_v2(primary_dbc, sql, strlen(sql)+1, &stmt, 0);    
    if (ret != SQLITE_OK) {
		SHOW_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		return list;
    }
    while ( (ret = sqlite3_step(stmt)) == SQLITE_ROW )
    {
		DBRow* row = g_malloc(sizeof(DBRow));
		row->error = NULL;
		row->url = NULL;
		row->note = NULL;
		row->rating = 0;
    
		row->url = g_utf8_strdown((const gchar*)sqlite3_column_text(stmt,0), -1);
		row->note = g_strdup((const gchar*)sqlite3_column_text(stmt,1));
		row->rating = (gint)sqlite3_column_int(stmt,2);
		list = g_list_append(list, row);
	}
    
    if (ret != SQLITE_DONE)
    {
		SHOW_ERROR(ret, "sqlite3_step", primary_dbc);
		const char *err = NULL;
		err = sqlite3_errmsg (primary_dbc);
		if (err)
			g_warning("\nError message : %s\n\n",err);
	}
         
	sqlite3_finalize(stmt);
	
	return list;
}

GList* unk_db_get_all_db_names()
{
    return secondary_dbc_list;
};

static void dbc_list_destroyed(gpointer data) {
	
	gint ret = sqlite3_close_v2 ( ((DBInfo*)data)->dbc);
	SHOW_ERROR(ret, "sqlite3_step", ((DBInfo*)data)->dbc);
	g_free( ((DBInfo*)data)->name);
}

gint unk_db_cleanup(void)
{
	g_debug("unk_db_cleanup");
    gint ret;
	
	if (primary_dbc)
	{
        if (!unk_db_lock_remove(primary_db_filename))
        {
            g_warning("cant remove database lock '%s'. remove it manually.", primary_db_filename);
            msgwin_status_add("cant remove database lock '%s'. remove it manually.", primary_db_filename);
        }
        
        ret = sqlite3_close_v2 (primary_dbc);
		SHOW_ERROR(ret, "sqlite3_step", primary_dbc);
	}
	
	g_list_free_full (secondary_dbc_list, dbc_list_destroyed);
    g_free(primary_db_filename);
	
	return ret;
}


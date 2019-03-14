#include <geanyplugin.h>

#include <sqlite3.h>

#include <glib.h>
#include <glib/gprintf.h>
#include "unkdb.h"

#define SQLITE_DB_FILE_SUFFIX ".sqlite3"
#define SQLITE_DB_USER_VERSION 2

static sqlite3* primary_dbc = NULL;
static GList* secondary_dbc_list = NULL;



#define HANDLE_ERROR(r, f, h) \
	 if (r != SQLITE_OK && r != SQLITE_DONE) \
	 { \
		extract_error(r, f, h, __LINE__,__func__); \
		return FALSE; \
	 }

#define SHOW_ERROR(r, f, h) \
	if (r != SQLITE_OK && r != SQLITE_DONE) \
	{ \
		gchar *err_msg = extract_error(r, f, h, __LINE__,__func__); \
		g_print("%s",err_msg); \
	}

#define GET_ERROR(r, f, h) \
	((r != SQLITE_OK && r != SQLITE_DONE)? extract_error(r, f, h, __LINE__,__func__) : NULL)
	
 
gchar*
extract_error (int retcode, gchar *fn, sqlite3 *dbc, int line, const char *func)
{
	gchar* buffer = g_malloc(255);
	gchar *temp = g_malloc(255);
	const char *err = NULL;
	
	g_sprintf(buffer, "Error!!! In '%s' (line no. %d)\nSQLite reported following error for %s : Error-code = %d\n", func, line-1, fn, retcode);
	err = sqlite3_errmsg (dbc);
	if (err)
	{
		g_sprintf(temp, "Error-Message : %s\n", err);
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
    static sqlite3_stmt *stmt;
    int db_version;

    if(sqlite3_prepare_v2(dbc, "PRAGMA user_version;", -1, &stmt, NULL) == SQLITE_OK) {
        while(sqlite3_step(stmt) == SQLITE_ROW) {
            db_version = sqlite3_column_int(stmt, 0);
        }
    } else {
		g_print("sqlite3_prepare_v2 return error: %s", sqlite3_errmsg(dbc));
    }
    sqlite3_finalize(stmt);

    return db_version;
}

static gint unk_update_db_schema(sqlite3 *dbc, gint start_version, gint end_version)
{
	sqlite3_stmt *stmt = NULL;
	gint ret;
	gchar sql[255];
				
	gint current_version = start_version;
	while (current_version < end_version)
	{
		g_print("update database from user_version %d to %d", current_version, current_version + 1);
				
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
					//~ ret = sqlite3_exec(primary_dbc, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
					return current_version; 
				}
				
				//sqlite3_reset(stmt);
				//HANDLE_ERROR(ret, "sqlite3_reset", primary_dbc);
				
				g_sprintf(sql, "PRAGMA user_version = %d;", current_version + 1);
				ret = sqlite3_prepare_v2 (primary_dbc, sql, -1, &stmt, NULL);
				if (ret != SQLITE_OK)
				{
					SHOW_ERROR(ret, "sqlite3_prepare_v2", primary_dbc)
					//~ ret = sqlite3_exec(primary_dbc, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
					return current_version; 
				}
				
				//~ ret = sqlite3_bind_int(stmt, 1, current_version + 1 );
				//~ if (ret != SQLITE_OK)
				//~ {
					//~ SHOW_ERROR(ret, "sqlite3_bind_int", primary_dbc)
					//~ ret = sqlite3_exec(primary_dbc, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
					//~ return current_version; 
				//~ }
				
				ret = sqlite3_step (stmt);
				if (ret != SQLITE_OK && ret != SQLITE_DONE)
				{
					SHOW_ERROR(ret, "sqlite3_step", primary_dbc)
					//~ ret = sqlite3_exec(primary_dbc, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
					return current_version; 
				}
	
				ret = sqlite3_exec(primary_dbc, "COMMIT TRANSACTION;", NULL, NULL, NULL);
				if (ret != SQLITE_OK)
				{
					SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
					//~ ret = sqlite3_exec(primary_dbc, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
					return current_version; 
				}
				sqlite3_reset(stmt);
	
				break;
		}
		
		g_print("success");
				
		current_version++;
	}
	
	sqlite3_finalize(stmt);
				
	return current_version;
}

gint unk_db_init(const gchar* filepath)
{
	sqlite3* secondary_dbc = NULL;
	//sqlite3_stmt *stmt;
	gint ret = 0;
	gchar *native_filepath, *dirname, *db_filename;
	const gchar *filename;
	GDir *db_dir;
    GError *dir_error;
    gint user_version;
    
    native_filepath = expand_shell_replacers(filepath);
	
	dirname = g_path_get_dirname(native_filepath);
	
	db_dir = g_dir_open(dirname, 0, &dir_error);
	if (db_dir == NULL)
	{
		g_print("cant open primary database directory '%s'", dirname);
		if (dir_error != NULL)
		{
			g_print("error: %s", dir_error->message);
			g_clear_error (&dir_error);
		}
		g_free(dirname);
		g_free(native_filepath);
		return 1;
	}
	
	// primary database
	ret = sqlite3_open_v2(native_filepath , &primary_dbc, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if (ret != SQLITE_OK)
	{
		g_print("sqlite3_open_v2 primary database '%s' return error: %s", native_filepath, sqlite3_errmsg(primary_dbc));
		primary_dbc = NULL;
		g_free(dirname);
		g_free(native_filepath);
		return 1;
	}
	
	if ((user_version = get_user_version(primary_dbc)) == 0)
	{
        ret = sqlite3_exec(primary_dbc, "BEGIN TRANSACTION;", NULL, NULL, NULL);
        if (ret != SQLITE_OK)
        {
            SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
            return 1; 
        }
        
        gchar* sql = 	"CREATE TABLE IF NOT EXISTS urlnotes("  \
					"URL TEXT PRIMARY KEY     NOT NULL," \
					"NOTE           TEXT    NOT NULL," \
					"RATING INTEGER NOT NULL DEFAULT 0);";

		//ret = sqlite3_prepare_v2 (primary_dbc, sql, -1, &stmt, NULL);
		//SHOW_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		
		//ret = sqlite3_step (stmt);
		//SHOW_ERROR(ret, "sqlite3_step", primary_dbc);

        ret = sqlite3_exec(primary_dbc, sql, NULL, NULL, NULL);
        if (ret != SQLITE_OK)
        {
            SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
            //~ ret = sqlite3_exec(primary_dbc, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
            return 1; 
        }
        
		//sqlite3_reset(stmt);
		//SHOW_ERROR(ret, "sqlite3_reset", primary_dbc);
        gchar sql2[255];
        g_sprintf(sql2, "PRAGMA user_version = %d;", SQLITE_DB_USER_VERSION);
        ret = sqlite3_exec(primary_dbc, sql2, NULL, NULL, NULL);
        if (ret != SQLITE_OK)
        {
            SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
            //~ ret = sqlite3_exec(primary_dbc, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
            return 1; 
        }
       
        // ret = sqlite3_prepare_v2 (primary_dbc, sql, -1, &stmt, NULL);
        // if (ret != SQLITE_OK)
        // {
            // SHOW_ERROR(ret, "sqlite3_prepare_v2", primary_dbc)
            //~ ret = sqlite3_exec(primary_dbc, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
            // return 1; 
        // }
		
		// if (ret != SQLITE_OK && ret != SQLITE_DONE)
        // {
            // SHOW_ERROR(ret, "sqlite3_step", primary_dbc)
            //~ ret = sqlite3_exec(primary_dbc, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
            // return 1; 
        // }
	
        ret = sqlite3_exec(primary_dbc, "COMMIT TRANSACTION;", NULL, NULL, NULL);
        if (ret != SQLITE_OK)
        {
            SHOW_ERROR(ret, "sqlite3_exec", primary_dbc)
            //~ ret = sqlite3_exec(primary_dbc, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
            return 1; 
        }
        //sqlite3_finalize(stmt);
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
			g_print("primary database '%s' has wrong user_version %d . Current user_version %d", native_filepath, user_version, SQLITE_DB_USER_VERSION);
    };

	    		
	//secondary database
	while ((filename = g_dir_read_name(db_dir)))
	{
		if (g_str_has_suffix(filename, SQLITE_DB_FILE_SUFFIX)) 
		{
			db_filename = g_build_filename (dirname, filename, NULL);
			
			if (g_strcmp0(db_filename, native_filepath) != 0 )
			{
				g_print("open secondary database: %s",db_filename);
                gint ret = sqlite3_open_v2(db_filename , &secondary_dbc, SQLITE_OPEN_READONLY, NULL);
				
                if (ret != SQLITE_OK)
				{ 
					g_print("sqlite3_open_v2 secondary database '%s' return error: %s", db_filename, sqlite3_errmsg(secondary_dbc));
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
					g_print("secondary database '%s' has wrong user_version %d instead %d", db_filename, user_version, SQLITE_DB_USER_VERSION);
				
					
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
	DBRow* row = g_malloc(sizeof(DBRow));
    row->error = NULL;
    row->url = g_strdup(key);
	row->note = NULL;
    row->rating = 0;
       
	sqlite3_stmt *stmt = NULL;
	gint ret;
	
	gchar* sql = "SELECT note, rating FROM urlnotes WHERE url=?1 LIMIT 1;";
	ret = sqlite3_prepare_v2(primary_dbc, sql, strlen(sql)+1, &stmt, 0);    
    if (ret != SQLITE_OK) {
		SHOW_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		return row;
    }
    
    ret = sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_TRANSIENT );
    if (ret != SQLITE_OK) {
		SHOW_ERROR(ret, "sqlite3_bind_text", primary_dbc);
		return row;
    }
    
    //g_print("%s", sqlite3_expanded_sql(stmt));
    
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
			g_print ("\nError-Message : %s\n\n",err);
		row->error = g_strdup(err);
	}
         
	sqlite3_finalize(stmt);
	return row;
}

static void row_hashtable_key_destroyed(gpointer key) {
	g_free((gchar*)key);
}

void row_destroyed(gpointer value) {
	g_free(((DBRow*)value)->url);
	g_free(((DBRow*)value)->note);
	g_free((DBRow*)value);
}

GHashTable* unk_db_get_secondary(const gchar* key, const gchar* default_value)
{	
	GHashTable* ht = g_hash_table_new_full (NULL, NULL, row_hashtable_key_destroyed, row_destroyed);
    
    sqlite3_stmt *stmt = NULL;
	gint ret;
	gchar* sql = "SELECT note, rating FROM urlnotes WHERE url=?1 LIMIT 1;";
	    
	for (GList* it = secondary_dbc_list; it; it = it->next) 
	{
        DBRow* row = g_malloc(sizeof(DBRow));
        DBInfo* db = (DBInfo*)(it->data);
        
        row->url = g_strdup(key);
        row->note = NULL;
        row->rating = 0;
        
        ret = sqlite3_prepare_v2(db->dbc, sql, strlen(sql)+1, &stmt, 0);    
        if (ret != SQLITE_OK) {
            row->error = GET_ERROR(ret, "sqlite3_prepare_v2", db->dbc);
            goto select_continue;
        }
        
        ret = sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_TRANSIENT );
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
            row->note = g_strdup(default_value);
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
	
	return ht;
}

gboolean unk_db_set(const gchar* key, const gchar* value, gint rating)
{
	sqlite3_stmt *stmt;
	gint ret;
	
	if (key == NULL )
	{
		g_print("error unk_db_set: key == NULL");
		return FALSE;
	}
	if (!strlen(key) )
	{
		g_print("error unk_db_set: strlen(key) == 0");
		return FALSE;
	}
	
	ret = sqlite3_prepare_v2 (primary_dbc, "REPLACE INTO urlnotes (url, note, rating) VALUES (?1,?2,?3);", -1, &stmt, NULL);
	HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);

	ret = sqlite3_bind_text(stmt, 1, key,  strlen(key), SQLITE_TRANSIENT );
    HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
	
	ret = sqlite3_bind_text(stmt, 2, value, strlen(value), NULL);
    HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
	
	ret = sqlite3_bind_int(stmt, 3, rating);
    HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		
	ret = sqlite3_step (stmt);
	HANDLE_ERROR(ret, "sqlite3_step", primary_dbc);
	 
	sqlite3_finalize(stmt);
	
	return TRUE;
}

gboolean unk_db_delete(const gchar* key)
{
	sqlite3_stmt *stmt;
	gint ret;
	
	if (key == NULL )
	{
		g_print("error unk_db_delete: key == NULL");
		return FALSE;
	}
	if (!strlen(key) )
	{
		g_print("error unk_db_delete: strlen(key) == 0");
		return FALSE;
	}
	
	ret = sqlite3_prepare_v2 (primary_dbc, "DELETE FROM urlnotes WHERE url =  ?1;", -1, &stmt, NULL);
	HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);

	ret = sqlite3_bind_text(stmt, 1, key,  strlen(key), SQLITE_TRANSIENT );
    HANDLE_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		
	ret = sqlite3_step (stmt);
	HANDLE_ERROR(ret, "sqlite3_step", primary_dbc);
	 
	sqlite3_finalize(stmt);
	
	return TRUE;
}

GList* unk_db_get_keys()
{
	GList* list = NULL;
	gchar* value;
	sqlite3_stmt *stmt;
	gint ret;
	
	gchar* sql = "SELECT url FROM urlnotes;";
	ret = sqlite3_prepare_v2(primary_dbc, sql, strlen(sql)+1, &stmt, 0);    
    if (ret != SQLITE_OK) {
		SHOW_ERROR(ret, "sqlite3_prepare_v2", primary_dbc);
		return list;
    }
    while ( (ret = sqlite3_step(stmt)) == SQLITE_ROW )
    {
		value = g_strdup((const gchar*)sqlite3_column_text(stmt,0));
		list = g_list_append(list, value);
	}
    
    if (ret != SQLITE_DONE)
    {
		SHOW_ERROR(ret, "sqlite3_step", primary_dbc);
		const char *err = NULL;
		err = sqlite3_errmsg (primary_dbc);
		if (err)
			g_print ("\nError-Message : %s\n\n",err);
	}
         
	sqlite3_finalize(stmt);
	
	return list;
}

GList* unk_db_get_all()
{
	
	GList* list = NULL;
	sqlite3_stmt *stmt;
	gint ret;
	
	gchar* sql = "SELECT url,rating FROM urlnotes;";
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
    
		row->url = g_strdup((const gchar*)sqlite3_column_text(stmt,0));
		row->rating = (gint)sqlite3_column_int(stmt,1);
		list = g_list_append(list, row);
	}
    
    if (ret != SQLITE_DONE)
    {
		SHOW_ERROR(ret, "sqlite3_step", primary_dbc);
		const char *err = NULL;
		err = sqlite3_errmsg (primary_dbc);
		if (err)
			g_print ("\nError-Message : %s\n\n",err);
	}
         
	sqlite3_finalize(stmt);
	
	return list;
}
static void dbc_list_destroyed(gpointer data) {
	
	gint ret = sqlite3_close_v2 ( ((DBInfo*)data)->dbc);
	SHOW_ERROR(ret, "sqlite3_step", ((DBInfo*)data)->dbc);
	g_free( ((DBInfo*)data)->name);
}

gint unk_db_cleanup(void)
{
	gint ret;
	
	if (primary_dbc)
	{
		ret = sqlite3_close_v2 (primary_dbc);
		SHOW_ERROR(ret, "sqlite3_step", primary_dbc);
	}
	
	g_list_free_full (secondary_dbc_list, dbc_list_destroyed);
	return ret;
}


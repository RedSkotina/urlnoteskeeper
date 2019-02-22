#include <geanyplugin.h>

#include <sqlite3.h>

#include "unkdb.h"
static sqlite3* dbc = NULL;

#define HANDLE_ERROR(r, f, h) \
 if (r != SQLITE_OK && r != SQLITE_DONE) \
 { \
 extract_error(r, f, h, __LINE__,__func__); \
 return FALSE; \
 }

#define SHOW_ERROR(r, f, h) \
 if (r != SQLITE_OK && r != SQLITE_DONE) \
 { \
 extract_error(r, f, h, __LINE__,__func__); \
 }
 
void
extract_error (int retcode, char *fn, sqlite3 *dbc, int line, const char *func)
{
	 const char *err = NULL;

	 g_print ("\nError!!! In '%s' (line no. %d) ", func, line-1);
	 g_print ("\nSQLite reported following error for %s : Error-code = %d", fn, retcode);

	 err = sqlite3_errmsg (dbc);
	 if (err)
		g_print ("\nError-Message : %s\n\n",err);
}
 
gint unk_db_init(gchar* filename)
{
	sqlite3_stmt *stmt;
	gint ret;
	
	int rc = sqlite3_open(filename, &dbc);
	//HANDLE_ERROR(ret, "sqlite3_open", dbc);
	if (rc)
	{
		g_print(stderr, "sqlite3_open error: %s\n", sqlite3_errmsg(dbc));
		return rc;
	}	
	gchar* sql = "CREATE TABLE IF NOT EXISTS urlnotes("  \
      "URL TEXT PRIMARY KEY     NOT NULL," \
      "NOTE           TEXT    NOT NULL);";

	ret = sqlite3_prepare_v2 (dbc, sql, -1, &stmt, NULL);
	HANDLE_ERROR(ret, "sqlite3_prepare_v2", dbc);
	
	ret = sqlite3_step (stmt);
	HANDLE_ERROR(ret, "sqlite3_step", dbc);

	sqlite3_finalize(stmt);
	
	return rc;
}

gchar* unk_db_get(gchar* key, gchar* default_value)
{	
	gchar* value = g_strdup(default_value);
	sqlite3_stmt *stmt;
	gint rc;
	gchar* sql = "SELECT note FROM urlnotes WHERE url=?1 LIMIT 1;";
	rc = sqlite3_prepare_v2(dbc, sql, strlen(sql)+1, &stmt, 0);    
    if (rc != SQLITE_OK) {
		SHOW_ERROR(rc, "sqlite3_prepare_v2", dbc);
		return default_value;
    }
    //g_print("%d",sqlite3_column_count(stmt));
	rc = sqlite3_bind_text(stmt, 1, key, strlen(key)+1, 0);
    if (rc != SQLITE_OK) {
		SHOW_ERROR(rc, "sqlite3_bind_text", dbc);
		return default_value;
    }
    	
    rc = sqlite3_step(stmt);		
    if ( rc == SQLITE_ROW )
    {
		value = g_strdup(sqlite3_column_text(stmt,0));
		g_print("db_get1v: %s ", value);
	}
    else if (rc == SQLITE_DONE)
    {
		g_print("db_get: not found %s ", key);
	}
    else
    {
		SHOW_ERROR(rc, "sqlite3_step", dbc);
		const char *err = NULL;
		err = sqlite3_errmsg (dbc);
		if (err)
			g_print ("\nError-Message : %s\n\n",err);
		g_print("%d", rc);
	}
         
	//gchar* value = g_strdup("description");  
	sqlite3_finalize(stmt);
	g_print("db_get3: %s ", value);
	
	return value;
}

gboolean unk_db_set(gpointer key, gpointer value)
{
	sqlite3_stmt *stmt;
	gint ret;
	
	ui_set_statusbar(TRUE, "db set: %s %s", (gchar*)key, (gchar*)value);
	
	ret = sqlite3_prepare_v2 (dbc, "REPLACE INTO urlnotes (url, note) VALUES (?1,?2)", -1, &stmt, NULL);
	HANDLE_ERROR(ret, "sqlite3_prepare_v2", dbc);

	ret = sqlite3_bind_text(stmt, 1, key,  strlen(key)+1, NULL);
    HANDLE_ERROR(ret, "sqlite3_prepare_v2", dbc);
	
	ret = sqlite3_bind_text(stmt, 2, value, strlen(value)+1, NULL);
    HANDLE_ERROR(ret, "sqlite3_prepare_v2", dbc);

		
	ret = sqlite3_step (stmt);
	HANDLE_ERROR(ret, "sqlite3_step", dbc);

	//ret = sqlite3_reset (stmt);
	//HANDLE_ERROR(ret, "sqlite3_reset", dbc);
	 
	sqlite3_finalize(stmt);
	
	return TRUE;
}

void unk_db_cleanup(void)
{
	gint ret;
	
	if (dbc)
	{
		ret = sqlite3_close_v2 (dbc);
		HANDLE_ERROR(ret, "sqlite3_close_v2", dbc);
	}
}

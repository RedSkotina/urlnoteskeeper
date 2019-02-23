#include <geanyplugin.h>

#include <sqlite3.h>
//#include <wordexp.h>
#include <glib.h>
#include <glib/gprintf.h>
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
gchar* expand_shell_replacers(const gchar* filename)
{
	gchar **split = g_strsplit(filename, G_DIR_SEPARATOR_S, -1);
	for (int i =0; i < g_strv_length (split); i++)
		if (*split[i] == '~')
			SETPTR(split[i], g_get_home_dir());
	
	gchar* text = g_strjoinv(G_DIR_SEPARATOR_S, split);
	g_strfreev(split);
	return text;
}
 
gint unk_db_init(const gchar* filename)
{
	sqlite3_stmt *stmt;
	gint ret;
	gchar* p;
	p = expand_shell_replacers(filename);
	
	gint rc = sqlite3_open(p , &dbc);
	//HANDLE_ERROR(ret, "sqlite3_open", dbc);
	if (rc != SQLITE_OK)
	{
		g_print("sqlite3_open error: %s", sqlite3_errmsg(dbc));
		g_print("database path: %s", p);
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
	
	g_free(p);

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
    
    rc = sqlite3_bind_text(stmt, 1, key, strlen(key)+1, 0);
    if (rc != SQLITE_OK) {
		SHOW_ERROR(rc, "sqlite3_bind_text", dbc);
		return default_value;
    }
    	
    rc = sqlite3_step(stmt);		
    if ( rc == SQLITE_ROW )
    {
		value = g_strdup((const gchar*)sqlite3_column_text(stmt,0));
	}
    else if (rc == SQLITE_DONE)
    {
		//g_print("db_get: not found %s ", key);
	}
    else
    {
		SHOW_ERROR(rc, "sqlite3_step", dbc);
		const char *err = NULL;
		err = sqlite3_errmsg (dbc);
		if (err)
			g_print ("\nError-Message : %s\n\n",err);
	}
         
	sqlite3_finalize(stmt);
	
	return value;
}

gboolean unk_db_set(gpointer key, gpointer value)
{
	sqlite3_stmt *stmt;
	gint ret;
	
	//ui_set_statusbar(TRUE, "db set: %s %s", (gchar*)key, (gchar*)value);
	
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
	gint rc;
	
	if (dbc)
	{
		rc = sqlite3_close_v2 (dbc);
		if (rc != SQLITE_OK)
		{
			g_print("sqlite3_close error: %s\n", sqlite3_errmsg(dbc));
		}	
	}
}

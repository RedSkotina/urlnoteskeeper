#ifndef UNK_PLUGIN_H
#define UNK_PLUGIN_H 1
typedef struct UnkInfo
{
	gchar *config_file;
	gchar* db_path;
	gboolean enable_urls_detect_on_open_document;
	gboolean enable_db_detect_on_open_document;
    gboolean enable_search_results_fill_on_open_document;
    gboolean enable_tooltips;
    GdkRGBA * positive_rating_color;
	GdkRGBA * neutral_rating_color;
	GdkRGBA * negative_rating_color;
	
} UnkInfo;


extern UnkInfo		*unk_info;
extern GeanyPlugin		*geany_plugin;
extern GeanyData		*geany_data;
#endif

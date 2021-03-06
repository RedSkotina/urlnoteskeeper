#ifndef UNK_SIDEBAR_H
#define UNK_SIDEBAR_H 1

#include <geanyplugin.h>

//static gchar* unk_sidebar_group_name; 

void sidebar_set_url(gpointer text);

void sidebar_set_note(gpointer text);

void sidebar_set_rating(gint rating);

void sidebar_show(GeanyPlugin* geany_plugin);
	
void sidebar_init(GeanyPlugin* geany_plugin);

void sidebar_activate(void);

void sidebar_deactivate(void);

void sidebar_cleanup(void);
						  
#endif

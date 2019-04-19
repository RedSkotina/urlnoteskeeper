#ifndef UNK_SEARCHBAR_H
#define UNK_SEARCHBAR_H 1

enum
{
    SEARCH_FULL_BASE,
    SEARCH_ONLY_CURRENT_DOCUMENT
};

gint searchbar_get_mode();

void searchbar_show(GeanyPlugin* geany_plugin);

void searchbar_store_reset(gint mode, gpointer userdata);

void searchbar_init(GeanyPlugin* geany_plugin);

void searchbar_activate(void);

void searchbar_deactivate(void);

void searchbar_cleanup(void);

#endif
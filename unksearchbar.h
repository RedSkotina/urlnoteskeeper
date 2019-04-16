#ifndef UNK_SEARCHBAR_H
#define UNK_SEARCHBAR_H 1

void searchbar_show(GeanyPlugin* geany_plugin);

//void searchbar_set_notes(GHashTable* rows);

//void searchbar_reset(void);

void searchbar_init(GeanyPlugin* geany_plugin);

void searchbar_activate(void);

void searchbar_deactivate(void);

void searchbar_cleanup(void);

#endif
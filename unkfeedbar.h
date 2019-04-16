#ifndef UNK_FEEDBAR_H
#define UNK_FEEDBAR_H 1

void feedbar_show(GeanyPlugin* geany_plugin);

void feedbar_set_notes(GHashTable* rows);

void feedbar_reset(void);

void feedbar_init(GeanyPlugin* geany_plugin);

void feedbar_activate(void);

void feedbar_deactivate(void);

void feedbar_cleanup(void);
#endif
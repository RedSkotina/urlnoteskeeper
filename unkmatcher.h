#ifndef UNK_MATCHER_H
#define UNK_MATCHER_H 1

typedef struct {
 gint start;
 gint end;
} URLPosition;

gboolean unk_match_url_text(gpointer text);

GList* unk_match_url_word(gpointer word);


						  
#endif

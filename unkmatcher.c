#include <geanyplugin.h>

#include "unkmatcher.h"

#include <string.h>
#include <regex.h>

gboolean unk_match_url_text(gpointer text)
{
	return TRUE;
};

GList* unk_match_url_word(gpointer word)
{
	GList *list = NULL;
	GError *err = NULL;
	GMatchInfo *matchInfo;
	GRegex *regex;
	
	regex = g_regex_new ("(http:\\/\\/www\\.|https:\\/\\/www\\.|http:\\/\\/|https:\\/\\/)?[a-z0-9]+([\\-\\.]{1}[a-z0-9]+)*\\.[a-z]{2,5}(:[0-9]{1,5})?(\\/.*)?", 0, 0, &err);
	g_regex_match (regex, word, 0, &matchInfo);

	while (g_match_info_matches (matchInfo)) {
		URLPosition* pos = (URLPosition*)g_malloc0(sizeof(URLPosition));
		gboolean result = g_match_info_fetch_pos(matchInfo, 0, &pos->start, &pos->end);
		if (result)
		{
			list = g_list_append(list, pos);
			//g_print ("mykey=%d %d %d\n", pos->start, pos->end, result);
		}
		g_match_info_next (matchInfo, &err);
	}
	g_regex_unref(regex);						
	return list;
};

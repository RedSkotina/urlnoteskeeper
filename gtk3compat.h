#ifndef _COMPAT_H_
#define _COMPAT_H_
#include <gtk/gtk.h>

#if !GTK_CHECK_VERSION(3,0,0)

typedef struct {
	gdouble red;
	gdouble green;
	gdouble blue;
	gdouble alpha;
} GdkRGBA;

#endif

#if ! GTK_CHECK_VERSION(3,4,0)

G_BEGIN_DECLS

G_GNUC_INTERNAL gboolean
gdk_rgba_parse (GdkRGBA* rgba, const gchar* spec);


G_GNUC_INTERNAL gchar *
gdk_rgba_to_string(GdkRGBA *rgba);

G_GNUC_INTERNAL void
gtk_color_button_get_rgba (GtkColorButton *chooser, GdkRGBA *color);

G_GNUC_INTERNAL GtkWidget *
gtk_color_button_new_with_rgba (const GdkRGBA *rgba);

G_END_DECLS

#endif

void
gdk_color_to_rgba(const GdkColor *color, guint16 alpha, GdkRGBA *rgba);

void
gdk_rgba_to_color(const GdkRGBA *rgba, GdkColor *color, guint16 *alpha);

#endif
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

typedef enum
{
  GTK_STATE_FLAG_NORMAL       = 0,
  GTK_STATE_FLAG_ACTIVE       = 1 << 0,
  GTK_STATE_FLAG_PRELIGHT     = 1 << 1,
  GTK_STATE_FLAG_SELECTED     = 1 << 2,
  GTK_STATE_FLAG_INSENSITIVE  = 1 << 3,
  GTK_STATE_FLAG_INCONSISTENT = 1 << 4,
  GTK_STATE_FLAG_FOCUSED      = 1 << 5,
  GTK_STATE_FLAG_BACKDROP     = 1 << 6,
  GTK_STATE_FLAG_DIR_LTR      = 1 << 7,
  GTK_STATE_FLAG_DIR_RTL      = 1 << 8,
  GTK_STATE_FLAG_LINK         = 1 << 9,
  GTK_STATE_FLAG_VISITED      = 1 << 10,
  GTK_STATE_FLAG_CHECKED      = 1 << 11,
  GTK_STATE_FLAG_DROP_ACTIVE  = 1 << 12
} GtkStateFlags;

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

#if ! GTK_CHECK_VERSION(3,0,0)
G_BEGIN_DECLS

G_GNUC_INTERNAL void
gtk_widget_override_background_color (GtkWidget *widget, GtkStateFlags state, const GdkRGBA *rgba);

G_GNUC_INTERNAL void
gtk_widget_override_font (GtkWidget *widget, PangoFontDescription *font_desc);

G_GNUC_INTERNAL void
gtk_list_box_row_new(void);

G_END_DECLS
#endif

#if GTK_CHECK_VERSION(3,0,0)
G_GNUC_INTERNAL void
gtk_widget_hide_all (GtkWidget *widget);
#endif

void
gdk_color_to_rgba(const GdkColor *color, guint16 alpha, GdkRGBA *rgba);

void
gdk_rgba_to_color(const GdkRGBA *rgba, GdkColor *color, guint16 *alpha);

GdkRGBA *
gdk_rgba_contrast(const GdkRGBA *rgba, GdkRGBA *inv_rgba);

gchar* gdk_rgba_to_hex_string(const GdkRGBA *rgba, gchar* spec);

#endif


#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <gtk/gtk.h>

#include "gtk3compat.h"


#if !GTK_CHECK_VERSION(3,4,0)

//#define        ERANGE                34        /* Math result not representable */
#define SKIP_WHITESPACES(s) while (*(s) == ' ') (s)++;

static gboolean
parse_rgb_value (const gchar  *str,
                 gchar       **endp,
                 gdouble      *number)
{
  const char *p;
  *number = g_ascii_strtod (str, endp);
  if (errno == ERANGE || *endp == str ||
      isinf (*number) || isnan (*number))
    return FALSE;
  p = *endp;
  SKIP_WHITESPACES (p);
  if (*p == '%')
    {
      *endp = (char *)(p + 1);
      *number = CLAMP(*number / 100., 0., 1.);
    }
  else
    {
      *number = CLAMP(*number / 255., 0., 1.);
    }
  return TRUE;
}

gboolean
gdk_rgba_parse (GdkRGBA* rgba, const gchar* spec) {
 gboolean has_alpha;
  gdouble r, g, b, a;
  gchar *str = (gchar *) spec;
  gchar *p;
  g_return_val_if_fail (spec != NULL, FALSE);
  if (strncmp (str, "rgba", 4) == 0)
    {
      has_alpha = TRUE;
      str += 4;
    }
  else if (strncmp (str, "rgb", 3) == 0)
    {
      has_alpha = FALSE;
      a = 1;
      str += 3;
    }
  else
    {
      PangoColor pango_color;
      /* Resort on PangoColor for rgb.txt color
       * map and '#' prefixed colors
       */
      if (pango_color_parse (&pango_color, str))
        {
          if (rgba)
            {
              rgba->red = pango_color.red / 65535.;
              rgba->green = pango_color.green / 65535.;
              rgba->blue = pango_color.blue / 65535.;
              rgba->alpha = 1;
            }
          return TRUE;
        }
      else
        return FALSE;
    }
  SKIP_WHITESPACES (str);
  if (*str != '(')
    return FALSE;
  str++;
  /* Parse red */
  SKIP_WHITESPACES (str);
  if (!parse_rgb_value (str, &str, &r))
    return FALSE;
  SKIP_WHITESPACES (str);
  if (*str != ',')
    return FALSE;
  str++;
  /* Parse green */
  SKIP_WHITESPACES (str);
  if (!parse_rgb_value (str, &str, &g))
    return FALSE;
  SKIP_WHITESPACES (str);
  if (*str != ',')
    return FALSE;
  str++;
  /* Parse blue */
  SKIP_WHITESPACES (str);
  if (!parse_rgb_value (str, &str, &b))
    return FALSE;
  SKIP_WHITESPACES (str);
  if (has_alpha)
    {
      if (*str != ',')
        return FALSE;
      str++;
      SKIP_WHITESPACES (str);
      a = g_ascii_strtod (str, &p);
      if (errno == ERANGE || p == str ||
          isinf (a) || isnan (a))
        return FALSE;
      str = p;
      SKIP_WHITESPACES (str);
    }
  if (*str != ')')
    return FALSE;
  str++;
  SKIP_WHITESPACES (str);
  if (*str != '\0')
    return FALSE;
  if (rgba)
    {
      rgba->red = CLAMP (r, 0, 1);
      rgba->green = CLAMP (g, 0, 1);
      rgba->blue = CLAMP (b, 0, 1);
      rgba->alpha = CLAMP (a, 0, 1);
    }
  return TRUE;
}
#endif

#if !GTK_CHECK_VERSION(3,4,0)

gchar *
gdk_rgba_to_string(GdkRGBA *rgba) {

    if (rgba->alpha > 0.999)
    {
      return g_strdup_printf ("rgb(%d,%d,%d)",
                              (int)(0.5 + CLAMP (rgba->red, 0., 1.) * 255.),
                              (int)(0.5 + CLAMP (rgba->green, 0., 1.) * 255.),
                              (int)(0.5 + CLAMP (rgba->blue, 0., 1.) * 255.));
    }
    else 
    {
      gchar alpha[G_ASCII_DTOSTR_BUF_SIZE];
      g_ascii_formatd (alpha, G_ASCII_DTOSTR_BUF_SIZE, "%g", CLAMP (rgba->alpha, 0, 1));
      return g_strdup_printf ("rgba(%d,%d,%d,%s)",
                              (int)(0.5 + CLAMP (rgba->red, 0., 1.) * 255.),
                              (int)(0.5 + CLAMP (rgba->green, 0., 1.) * 255.),
                              (int)(0.5 + CLAMP (rgba->blue, 0., 1.) * 255.),
                              alpha);
    }
}

#endif

#if !GTK_CHECK_VERSION(3,4,0)
GtkWidget *
gtk_color_button_new_with_rgba (const GdkRGBA *rgba)
{
	GdkColor c;
	guint16 alpha;
	GtkWidget *ret;
	gdk_rgba_to_color(rgba, &c, &alpha);

	ret = gtk_color_button_new_with_color(&c);
	gtk_color_button_set_alpha (GTK_COLOR_BUTTON(ret), alpha);
	return ret;
}
#endif

#if !GTK_CHECK_VERSION(3,4,0)
void
gtk_color_button_get_rgba (GtkColorButton *chooser, GdkRGBA *color)
{
	GdkColor c;
	gtk_color_button_get_color(chooser, &c);
	gdk_color_to_rgba(&c, gtk_color_button_get_alpha(chooser), color);
}
#endif

#if !GTK_CHECK_VERSION(3,0,0)
void
gtk_widget_override_background_color (GtkWidget *widget, GtkStateFlags state, const GdkRGBA *rgba)
{
	GdkColor c;
	guint16 alpha;
	
	gdk_rgba_to_color(rgba, &c, &alpha);

	gtk_widget_modify_bg(widget,state, &c);
}
#endif

#if !GTK_CHECK_VERSION(3,0,0)
void
gtk_widget_override_font (GtkWidget *widget, PangoFontDescription *font_desc)
{
	gtk_widget_modify_font(widget, font_desc);
}
#endif

#if !GTK_CHECK_VERSION(3,0,0)
void
gtk_list_box_row_new(void)
{
    
}
#endif


#if GTK_CHECK_VERSION(3,0,0)
void
gtk_widget_hide_all (GtkWidget *widget)
{
	gtk_widget_hide(widget);
}
#endif


void
gdk_color_to_rgba(const GdkColor *color, guint16 alpha, GdkRGBA *rgba)
{
	rgba->red = color->red / 65535.0;
	rgba->green = color->green / 65535.0;
	rgba->blue = color->blue / 65535.0;
	rgba->alpha = alpha / 65535.0;
}

void
gdk_rgba_to_color(const GdkRGBA *rgba, GdkColor *color, guint16 *alpha)
{
	color->red = 65535 * rgba->red;
	color->green = 65535 * rgba->green;
	color->blue = 65535 * rgba->blue;
	if (alpha != NULL)
		*alpha = 65535 * rgba->alpha;
}

gchar* gdk_rgba_to_hex_string(const GdkRGBA *rgba, gchar* spec)
{
    sprintf(spec, "#%02X%02X%02X", (int)(rgba->red*255), (int)(rgba->green*255), (int)(rgba->blue*255));
    //g_print(spec);
    return spec;
}

GdkRGBA *
gdk_rgba_contrast(const GdkRGBA *rgba, GdkRGBA *inv_rgba)
{
	gdouble y = (299 * 255 * rgba->red + 587 * 255 * rgba->green + 114 * 255 * rgba->blue) / 1000;
    //g_print("%f %f %f %f", rgba->red, rgba->green, rgba->blue, y);
    if (y >= 128)
    {   //white
        inv_rgba->red = 0;
        inv_rgba->green = 0;
        inv_rgba->blue = 0;
        inv_rgba->alpha = 1;
    }
    else
    {   //black
        inv_rgba->red = 1;
        inv_rgba->green = 1;
        inv_rgba->blue = 1;
        inv_rgba->alpha = 1;
    }
    return inv_rgba;
}


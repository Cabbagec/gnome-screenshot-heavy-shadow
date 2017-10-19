/* screenshot-shadow.c - part of GNOME Screenshot
 *
 * Copyright (C) 2001-2006  Jonathan Blandford <jrb@alum.mit.edu>
 * Copyright (C) 2013  Nils Dagsson Moskopp <nils@dieweltistgarnichtso.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 */

/* Shadow code from anders */
/* Enhanced shadow code by brandon <root@everdream.xyz> */

#include "config.h"

#include "screenshot-shadow.h"
#include <math.h>

#define BLUR_RADIUS    5
#define SHADOW_OFFSET  (BLUR_RADIUS * 4 / 5)
#define SHADOW_OPACITY 0.5

// Constants I've fully tested with.
// Don't change them if you don't know what they mean 
// 下列常量我已完整测试过
// 如不理解他们的含义，请勿修改
#define SHADE_RADIUS    80
#define ALPHA_HIGH      139
#define SIGMA           27
#define SHADE_COMPENSATE  -3
#define BASE_SHRINK     23
#define UP_OFFSET       22 

#define OUTLINE_RADIUS  1
#define OUTLINE_OFFSET  0
#define OUTLINE_OPACITY 1.0

#define VINTAGE_SATURATION 0.8
#define VINTAGE_OVERLAY_COLOR 0xFFFBF2A3
#define VINTAGE_SOURCE_ALPHA 192
#define VINTAGE_OUTLINE_COLOR 0xFFEEEEEE
#define VINTAGE_OUTLINE_RADIUS 24

#define dist(x0, y0, x1, y1) sqrt(((x0) - (x1))*((x0) - (x1)) + ((y0) - (y1))*((y0) - (y1)))

typedef struct {
  int size;
  double *data;
} ConvFilter;

// static double
// gaussian (double x, double y, double r)
// {
//     return ((1 / (2 * M_PI * r)) *
// 	    exp ((- (x * x + y * y)) / (2 * r * r)));
// }

// The length of gauss_real and gauss_odds are radius.
static void
gaussian_distribution (int* gauss_real, double* gauss_odds, int radius, int sigma) 
{
  double con, fact;
  int i;
  con = 1/(sqrt( 2*M_PI ) * sigma);

  for(i = 0; i < radius; i++) {
    gauss_odds[i] = i - radius + 1;
    gauss_odds[i] = con * exp( -gauss_odds[i]*gauss_odds[i] / (2*sigma*sigma) );
  }

  fact = (double)(ALPHA_HIGH / gauss_odds[radius - 1]);
  for( i = 0; i < radius; i++)
    gauss_real[i] = (int)(gauss_odds[i] * fact);
}

// static ConvFilter *
// create_blur_filter (int radius)
// {
//   ConvFilter *filter;
//   int x, y;
//   double sum;
  
//   filter = g_new0 (ConvFilter, 1);
//   filter->size = radius * 2 + 1;
//   filter->data = g_new (double, filter->size * filter->size);

//   sum = 0.0;
  
//   for (y = 0 ; y < filter->size; y++)
//     {
//       for (x = 0 ; x < filter->size; x++)
// 	{
// 	  sum += filter->data[y * filter->size + x] = gaussian (x - (filter->size >> 1),
// 								y - (filter->size >> 1),
// 								radius);
// 	}
//     }

//   for (y = 0; y < filter->size; y++)
//     {
//       for (x = 0; x < filter->size; x++)
// 	{
// 	  filter->data[y * filter->size + x] /= sum;
// 	}
//     }

//   return filter;
  
// }

static ConvFilter *
create_outline_filter (int radius)
{
  ConvFilter *filter;
  double *iter;
  
  filter = g_new0 (ConvFilter, 1);
  filter->size = radius * 2 + 1;
  filter->data = g_new (double, filter->size * filter->size);

  for (iter = filter->data;
       iter < filter->data + (filter->size * filter->size);
       iter++)
    {
      *iter = 1.0;
    }

  return filter;
}

static GdkPixbuf *
create_effect (GdkPixbuf *src,
               ConvFilter const *filter,
               int radius,
               int offset,
               double opacity)
{
  GdkPixbuf *dest;
  int x, y, i, j;
  int src_x, src_y;
  int suma;
  int dest_width, dest_height;
  int src_width, src_height;
  int src_rowstride, dest_rowstride;
  gboolean src_has_alpha;
  
  guchar *src_pixels, *dest_pixels;

  src_has_alpha =  gdk_pixbuf_get_has_alpha (src);

  src_width = gdk_pixbuf_get_width (src);
  src_height = gdk_pixbuf_get_height (src);
  dest_width = src_width + 2 * radius + offset;
  dest_height = src_height + 2 * radius + offset;

  dest = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
			 TRUE,
			 gdk_pixbuf_get_bits_per_sample (src),
			 dest_width, dest_height);
  
  gdk_pixbuf_fill (dest, 0);  
  
  src_pixels = gdk_pixbuf_get_pixels (src);
  src_rowstride = gdk_pixbuf_get_rowstride (src);
  
  dest_pixels = gdk_pixbuf_get_pixels (dest);
  dest_rowstride = gdk_pixbuf_get_rowstride (dest);
  
  for (y = 0; y < dest_height; y++)
    {
      for (x = 0; x < dest_width; x++)
	{
	  suma = 0;

	  src_x = x - radius;
	  src_y = y - radius;

	  /* We don't need to compute effect here, since this pixel will be 
	   * discarded when compositing */
	  if (src_x >= 0 && src_x < src_width &&
	      src_y >= 0 && src_y < src_height && 
	      (!src_has_alpha ||
	       src_pixels [src_y * src_rowstride + src_x * 4 + 3] == 0xFF))
	    continue;

	  for (i = 0; i < filter->size; i++)
	    {
	      for (j = 0; j < filter->size; j++)
		{
		  src_y = -(radius + offset) + y - (filter->size >> 1) + i;
		  src_x = -(radius + offset) + x - (filter->size >> 1) + j;

		  if (src_y < 0 || src_y >= src_height ||
		      src_x < 0 || src_x >= src_width)
		    continue;

		  suma += ( src_has_alpha ? 
			        src_pixels [src_y * src_rowstride + src_x * 4 + 3] :
			        0xFF ) * filter->data [i * filter->size + j];
		}
      }
      dest_pixels [y * dest_rowstride + x * 4 + 3] = CLAMP (suma * opacity, 0x00, 0xFF);
	}
    }
  
  return dest;
}

static GdkPixbuf* 
create_window_shadow(GdkPixbuf *src,
                     int radius,
                     int offset,
                     int shrink,
                     int  gauss_compensate,
                     int* gauss_real,
                     double* gauss_odds)
{
  GdkPixbuf* dest;
  int src_height, dest_height,
      src_width, dest_width,
  //    src_rowstride, 
      dest_rowstride,
      target_corner_i1, target_corner_j1,
  //    target_corner_i2, target_corner_j2,
      corner_template[radius][radius];
  double multiplier;
  //gboolean src_has_alpha;
  guchar *dest_pixels;
  //guchar *src_pixels;

  //src_has_alpha = gdk_pixbuf_get_has_alpha(src);
  src_width = gdk_pixbuf_get_width(src);
  src_height = gdk_pixbuf_get_height(src);
  
  dest_width = src_width + 2 * radius - 2 * shrink;
  dest_height = src_height + 2 * radius - 2 * shrink;
  target_corner_j1 = radius - shrink - offset;                        // Up bound
  //target_corner_j2 = dest_height - radius + shrink - offset - 1;      // Low bound
  target_corner_i1 = radius - shrink;                                 // Left bound
  //target_corner_i2 = dest_width - radius + shrink - 1;                // Right bound

  // 创建 dest 新区域，并填 0;
  dest = gdk_pixbuf_new(gdk_pixbuf_get_colorspace(src),
                        TRUE,
                        gdk_pixbuf_get_bits_per_sample(src),
                        dest_width, dest_height);
  gdk_pixbuf_fill(dest, 0);
  
  //src_pixels = gdk_pixbuf_get_pixels(src);
  dest_pixels = gdk_pixbuf_get_pixels(dest);
  //src_rowstride = gdk_pixbuf_get_rowstride(src);
  dest_rowstride = gdk_pixbuf_get_rowstride(dest);


  // Deal with deltas in gauss_real.
  for(int i = 0; i < radius; ++i) {
    gauss_real[i] += gauss_compensate;
    if(gauss_real[i] < 0)
      gauss_real[i] = 0;
  }

  // And generate corner template,
  for(int i = 0; i < radius; ++i) {
    multiplier = gauss_real[radius - i - 1]/gauss_odds[radius - 1];
    for (int j = 0; j < radius; ++j)
      corner_template[j][i] = gauss_odds[radius - j - 1] * multiplier;
  }
  
  // Make it symmetry.
  for(int i = 0; i < radius; ++i)
    for(int j = 0; j < radius; ++j)
      if( j > i )
        corner_template[j][i] = corner_template[i][j];

  // Fullfill shadow(alpha channel) in the dest area.
  // i for y-axis from top to bottom, j for x-axis from left to right.
  // Top left corner
  for(int i = 0; i < radius; ++i)
    for(int j = 0; j < radius; ++j)
      dest_pixels[i*dest_rowstride + j*4 + 3] = 
        corner_template[radius - i - 1][radius - j - 1];

  // Top bar
  for(int i = 0; i < radius; ++i)
    for(int j = radius; j < dest_width - radius; ++j)
      dest_pixels[i*dest_rowstride + j*4 + 3] =
        gauss_real[i];

  // Top right corner
  for(int i = 0; i < radius; ++i)
    for(int j = dest_width - radius; j < dest_width; ++j)
      dest_pixels[i*dest_rowstride + j*4 + 3] =
        corner_template[radius - i - 1][j - dest_width + radius];

  // Left bar
  for(int i = radius; i < dest_height - radius; ++i)
    for(int j = 0; j < radius; ++j)
      dest_pixels[i*dest_rowstride + j*4 + 3] =
        gauss_real[j];

  // Middle part
  for(int i = radius; i < dest_height - radius; ++i)
    for(int j = radius; j < dest_width - radius; ++j)
      dest_pixels[i*dest_rowstride + j*4 + 3] = ALPHA_HIGH;

  // Right bar
  for(int i = radius; i < dest_height - radius; ++i)
    for(int j = dest_width - radius; j < dest_width; ++j)
      dest_pixels[i*dest_rowstride + j*4 + 3] =
        gauss_real[dest_width - j - 1];

  // Bottom left corner
  for(int i = dest_height - radius; i < dest_height; ++i)
    for(int j = 0; j < radius; ++j)
      dest_pixels[i*dest_rowstride + j*4 + 3] = 
        corner_template[i - dest_height + radius][radius - j - 1];

  // Bottom bar
  for(int i = dest_height - radius; i < dest_height; ++i)
    for(int j = radius; j < dest_width - radius; ++j)
      dest_pixels[i*dest_rowstride + j*4 + 3] = 
        gauss_real[dest_height - i - 1];

  // Bottom right corner
  for(int i = dest_height - radius; i < dest_height; ++i)
    for(int j = dest_width - radius; j < dest_width; ++j)
      dest_pixels[i*dest_rowstride + j*4 + 3] = 
        corner_template[i - dest_height + radius][j - dest_width + radius];
  
  // Drop the origin image onto the canvas
  gdk_pixbuf_composite(src, dest, 
                      target_corner_i1, target_corner_j1,
                      src_width, src_height, 
                      target_corner_i1, target_corner_j1, 1.0, 1.0,
                      GDK_INTERP_BILINEAR, 255);

  return dest; 
}

void
screenshot_add_shadow (GdkPixbuf **src)
{
  GdkPixbuf *dest;
  double gauss_odds[SHADE_RADIUS];
  int gauss_real[SHADE_RADIUS];

  gaussian_distribution(gauss_real, gauss_odds, 
                        SHADE_RADIUS, SIGMA);
  dest = create_window_shadow(*src, SHADE_RADIUS, 
                              UP_OFFSET, BASE_SHRINK, 
                              SHADE_COMPENSATE, 
                              gauss_real, gauss_odds);
  if (dest == NULL)
    return;
    
  g_object_unref (*src);
  *src = dest;
}

void
screenshot_add_border (GdkPixbuf **src)
{
  GdkPixbuf *dest;
  static ConvFilter *filter = NULL;
  
  if (!filter)
    filter = create_outline_filter (OUTLINE_RADIUS);
  
  dest = create_effect (*src, filter, 
			OUTLINE_RADIUS,
                        OUTLINE_OFFSET, OUTLINE_OPACITY);

  if (dest == NULL)
	  return;

  gdk_pixbuf_composite (*src, dest,
			OUTLINE_RADIUS, OUTLINE_RADIUS,
			gdk_pixbuf_get_width (*src),
			gdk_pixbuf_get_height (*src),
			OUTLINE_RADIUS, OUTLINE_RADIUS, 1.0, 1.0,
			GDK_INTERP_BILINEAR, 255);
  g_object_unref (*src);
  *src = dest;
}

void
screenshot_add_vintage (GdkPixbuf **src)
{
  GdkPixbuf *dest;
  static ConvFilter *filter = NULL;
  
  if (!filter)
    filter = create_outline_filter (VINTAGE_OUTLINE_RADIUS);
  
  dest = create_effect (*src, filter, 
			VINTAGE_OUTLINE_RADIUS,
                        OUTLINE_OFFSET, OUTLINE_OPACITY);

  if (dest == NULL)
	  return;

  gdk_pixbuf_fill(dest, VINTAGE_OUTLINE_COLOR);
  gdk_pixbuf_composite (*src, dest,
			VINTAGE_OUTLINE_RADIUS, VINTAGE_OUTLINE_RADIUS,
			gdk_pixbuf_get_width (*src),
			gdk_pixbuf_get_height (*src),
			VINTAGE_OUTLINE_RADIUS, VINTAGE_OUTLINE_RADIUS, 1.0, 1.0,
			GDK_INTERP_HYPER, 255);
  g_object_unref (*src);
  *src = dest;

  gdk_pixbuf_saturate_and_pixelate(*src, *src,
    VINTAGE_SATURATION, FALSE);
  dest = gdk_pixbuf_composite_color_simple(*src,
			gdk_pixbuf_get_width(*src),
			gdk_pixbuf_get_height(*src),
			GDK_INTERP_BILINEAR,
			VINTAGE_SOURCE_ALPHA, 64,
			VINTAGE_OVERLAY_COLOR, VINTAGE_OVERLAY_COLOR);

  if (dest == NULL)
	  return;

  g_object_unref (*src);
  *src = dest;
}

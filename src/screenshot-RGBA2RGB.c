#include "screenshot-RGBA2RGB.h"
#include "config.h"

void
screenshot_RGBA2RGB(GdkPixbuf** input_screenshot)
{
    gboolean has_alpha = 
        gdk_pixbuf_get_has_alpha((const GdkPixbuf*)(*input_screenshot));
    if(!has_alpha)
        return;

    gint height = 
        gdk_pixbuf_get_height((const GdkPixbuf*)(*input_screenshot));
    gint width  = 
        gdk_pixbuf_get_width((const GdkPixbuf*)(*input_screenshot));
    GdkColorspace colorspace = 
        GDK_COLORSPACE_RGB;
    gint bits_per_sample = 
        gdk_pixbuf_get_bits_per_sample((const GdkPixbuf*)(*input_screenshot));

    GdkPixbuf* tmp = gdk_pixbuf_new(colorspace,
                                    FALSE,
                                    bits_per_sample,
                                    width,
                                    height);
    
    gint input_rowstride = 
        gdk_pixbuf_get_rowstride((const GdkPixbuf*)(*input_screenshot));
    gint output_rowstride = 
        gdk_pixbuf_get_rowstride((const GdkPixbuf*)tmp);
    gint input_channels = 
        gdk_pixbuf_get_n_channels((const GdkPixbuf*)(*input_screenshot));
    gint output_channels = 
        gdk_pixbuf_get_n_channels((const GdkPixbuf*)tmp);
    guchar* input_pixel_entry_ptr = 
        gdk_pixbuf_get_pixels((const GdkPixbuf*)(*input_screenshot));
    guchar* output_pixel_entry_ptr = 
        gdk_pixbuf_get_pixels((const GdkPixbuf*)tmp);

    /* Convert RGBA image into white background RGB image in pixel wise 
     * output_color = (input_color * alpha + background * (1 - alpha)) * rate 
     */
    double rate = 1.003921569;  /* 256/255 */
    guchar* input_row_head_ptr = input_pixel_entry_ptr;
    guchar* output_row_head_ptr = output_pixel_entry_ptr;
    guchar* input_ptr;
    guchar* output_ptr;
    for(int y = 0; y < height;
        input_row_head_ptr += input_rowstride, 
        output_row_head_ptr += output_rowstride, y++) 
    {
        input_ptr = input_row_head_ptr;
        output_ptr = output_row_head_ptr;
        for(int x = 0; x < width; 
            input_ptr += input_channels,
            output_ptr += output_channels, x++) 
        {
            // int tmp1 = (255 - input_ptr[3]) * 255;
            // output_ptr[0] = (tmp1 + input_ptr[3] * input_ptr[0]) * rate + 0.5;
            // output_ptr[1] = (tmp1 + input_ptr[3] * input_ptr[1]) * rate + 0.5;
            // output_ptr[2] = (tmp1 + input_ptr[3] * input_ptr[2]) * rate + 0.5;
            output_ptr[0] = (((255 - input_ptr[3]) * 255 + input_ptr[3] * input_ptr[0]) >> 8) * rate + 0.5;
            output_ptr[1] = (((255 - input_ptr[3]) * 255 + input_ptr[3] * input_ptr[1]) >> 8) * rate + 0.5;
            output_ptr[2] = (((255 - input_ptr[3]) * 255 + input_ptr[3] * input_ptr[2]) >> 8) * rate + 0.5;
        }
    }

    GdkPixbuf* tmp2 = *input_screenshot;
    *input_screenshot = tmp;
    gdk_pixbuf_unref(tmp2);
    return;
}

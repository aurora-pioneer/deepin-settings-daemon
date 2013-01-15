#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gaussianiir2d.h"

/*
 *	./a.out <sigma> <numsteps> <picture_path>
 */
int
main (int argc, char** argv)
{
    gtk_init (&argc, &argv);
    //0. parse arguments.
    const char* picture_path = NULL;
    double sigma = 0;
    long numsteps = 0; //numsteps should be >=4 

    if ((argc == 2 && !g_strcmp0 (argv[2], "--help")) || 
	argc != 4)
    {
	g_print ("background_helper:  a program to generate a gaussian-blurred\n"
		 "		      image of the current background.\n"
		 "Usage:\n"
	         "	background_helper <sigma> <num of steps> <picture_path>");
	return 1;
    }

    sigma = g_strtod (argv[1], NULL);
    numsteps = (long) g_strtod (argv[2], NULL);
    picture_path = g_strdup (argv[3]);
#if 1
    g_print ("sigma: %f\n", sigma);
    g_print ("numsteps: %ld\n", numsteps);
    g_print ("picture_path: %s\n", picture_path);
#endif 

    //1. get screen size
    int root_width = 0;
    int root_height = 0;
    GdkScreen* gdkscreen = NULL;
    gdkscreen = gdk_screen_get_default ();
    root_width = gdk_screen_get_width (gdkscreen);
    root_height = gdk_screen_get_height (gdkscreen);

    //2. create a cairo surface and draw background onto it.
    cairo_surface_t* surface = NULL;
    cairo_t* cr = NULL;
    GdkPixbuf* pixbuf = NULL;
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					  root_width, root_height);
    cr = cairo_create (surface);
    pixbuf = gdk_pixbuf_new_from_file_at_scale (picture_path, root_width, 
	                                        root_height, FALSE, NULL);
    gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
    cairo_paint (cr);
    cairo_surface_flush (surface);

    //3. IIR gaussian blur previous created surface.
    cairo_format_t format = CAIRO_FORMAT_INVALID;
    unsigned char* image_data = NULL;
    format = cairo_image_surface_get_format (surface);
    if ((format == CAIRO_FORMAT_ARGB32) || (format == CAIRO_FORMAT_RGB24)) 
	image_data = cairo_image_surface_get_data (surface);

    cairo_surface_write_to_png (surface, "gaussian1.png");

    clock_t start = clock ();
    gaussianiir2d_c(image_data, root_width, root_height, sigma, numsteps);
    clock_t end = clock ();
    g_print ("time : %f\n", (end-start)/(float)CLOCKS_PER_SEC);

    //4. write out the picture.
    cairo_surface_write_to_png (surface, "gaussian2.png");

    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    return 0;
}

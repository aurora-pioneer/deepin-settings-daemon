/*
 *	we put generated gaussian-blurred background pictures
 *	in $XDG_CACHE/"gaussian-background".
 *	and use methods similar to those specified in freedesktop
 *	thumbnail spec.
 *	PNG namespace used here:
 */
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gtk/gtk.h>

#include "background_util.h"
#include "gaussianiir2d.h"


#define	BG_BLUR_PICT_CACHE_DIR "gaussian-background"
#define BG_EXT_URI	 "tEXt::Blur::URI"
#define BG_EXT_MTIME	 "tEXt::Blur::MTime"


static char* bg_blur_pict_get_dest_uri (const char* src_uri);
static char* bg_blur_pict_factory_lookup (const char* uri, time_t mtime);
static char* bg_blur_pict_is_valid (GdkPixbuf* pixbuf, const char* uri, time_t mtime);
static char* bg_blur_pict_generate (const char* src_uri, const char* dest_uri, time_t mtime);
static gboolean bg_blur_pict_make_cache_dir (void);
/*
 *	
 */
static char*
bg_blur_pict_get_dest_uri (const char* src_uri)
{
    g_return_val_if_fail (uri != NULL, NULL);

    //1. calculate original picture md5
    GChecksum* checksum;
    checksum = g_checksum_new (G_CHECKSUM_MD5);
    g_checksum_update (checksum, (const guchar *) src_uri, strlen (src_uri));

    guint8 digest[16];
    gsize digest_len = sizeof (digest);
    g_checksum_get_digest (checksum, digest, &digest_len);
    g_assert (digest_len == 16);

    //2. build blurred picture path
    char* file;
    file = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);
    g_checksum_free (checksum);
    char* path;
    path = g_build_filename (g_get_user_cache_dir (),
			     BG_BLUR_PICT_CACHE_DIR,
			     file,
			     NULL);
    g_free (file);

    return path;
}
/*
 *	@uri: the original background file path
 *	@mtime: the original background file modification time
 */
static char *
bg_blur_pict_factory_lookup (const char *dest_uri, time_t src_mtime)
{
    g_return_val_if_fail (dest_uri != NULL, NULL);

    GdkPixbuf *pixbuf;
    pixbuf = gdk_pixbuf_new_from_file (dest_uri, NULL);

    gboolean is_valid = FALSE;
    if (pixbuf != NULL)
    {
	is_valid = bg_blur_pict_is_valid (pixbuf, dest_uri, src_mtime);
	g_object_unref (pixbuf);
    }
    if (is_valid)
	return path;

    return NULL;
}
/*
 *	
 */
static char* 
bg_blur_pict_is_valid (GdkPixbuf* pixbuf, const char* dest_uri, time_t src_mtime)
{
  
    //1. check if the original uri matches the provided @uri
    const char *blur_uri;
    blur_uri = gdk_pixbuf_get_option (pixbuf, BG_EXT_URI);
    if (!blur_uri)
	return FALSE;
    if (strcmp (uri, blur_uri) != 0)
	return FALSE;
  
    //2. check if the modification time matches
    const char *blur_mtime_str;
    time_t blur_mtime;
    blur_mtime_str = gdk_pixbuf_get_option (pixbuf, BG_EXT_MTIME);
    if (!blur_mtime_str)
	return FALSE;
    blur_time = atol (blur_mtime_str);
    if (mtime != blur_mtime)
	return FALSE;
  
    return TRUE;
}
/*
 *	create cache directory for blurred images.	
 */
static gboolean
bg_blur_pict_make_cache_dir ()
{
  char *cache_dir;
  cache_dir = g_build_filename (g_get_user_cache_dir (),
				BG_BLUR_PICT_CACHE_DIR,
				NULL);

  gboolean retval = FALSE;

  if (!g_file_test (cache_dir, G_FILE_TEST_IS_DIR))
  {
      g_mkdir (cache_dir, 0700);
      retval = TRUE;
  }
  g_free (cache_dir);

  return retval;
}
static char* 
bg_blur_pict_generate (const char* src_uri, const char* dest_uri, time_t src_mtime)
{
    GError* error = NULL;

    GdkPixbuf* pixbuf = NULL;
    int width = 0;
    int height = 0;

    cairo_surface_t* surface = NULL;
    cairo_t* cr = NULL;

    pixbuf = gdk_pixbuf_new_from_file (src_uri, &error);
    if (error != NULL)
    {
	g_debug ("background_helper: %s", error->message);
	g_error_free (error);
    }
    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create (surface);
    //2. otherwise generate the picture
#if 0
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    //2. create a cairo surface and draw background onto it.
    cairo_surface_t* surface = NULL;
    cairo_t* cr = NULL;
    GdkPixbuf* pixbuf = NULL;
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					  root_width, root_height);
    cr = cairo_create (surface);
    pixbuf = gdk_pixbuf_new_from_file_at_scale (src_uri, root_width, 
	                                        root_height, FALSE, NULL);
    gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
    cairo_paint (cr);
    cairo_surface_flush (surface);
    //original pictures.
    
    unlink (dest_picture_path);
    cairo_surface_write_to_png (surface, dest_picture_path);

    //3. IIR gaussian blur previous created surface.
    cairo_format_t format = CAIRO_FORMAT_INVALID;
    unsigned char* image_data = NULL;

    format = cairo_image_surface_get_format (surface);
    if ((format == CAIRO_FORMAT_ARGB32) || (format == CAIRO_FORMAT_RGB24)) 
	image_data = cairo_image_surface_get_data (surface);

    clock_t start = clock ();
    gaussianiir2d_c(image_data, root_width, root_height, sigma, numsteps);
    clock_t end = clock ();
    g_debug ("time : %f", (end-start)/(float)CLOCKS_PER_SEC);

    //4. write out the picture.
#if 1
    //if we use symlink, uncomment this
    unlink (dest_picture_path);
#endif
    cairo_surface_mark_dirty (surface);
    status = cairo_surface_write_to_png (surface, dest_picture_path);
    if (status)
    {
	g_debug ("gsd-background-helper: cairo status: %d", status);
    }

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
    g_free (dest_picture_path);
#endif

    return 0;
}
/*
 *	./a.out <sigma> <numsteps> <picture_uri>
 */
int
main (int argc, char** argv)
{
    if ((argc == 2 && !g_strcmp0 (argv[2], "--help")) || 
	argc != 4)
    {
	g_print ("background_helper:  a program to generate a gaussian-blurred\n"
		 "		      image of the current background.\n"
		 "Usage:\n"
	         "	background_helper <sigma> <num of steps> <picture_uri>");
	return 1;
    }

    g_type_init ();
    
    double sigma = 0;
    long numsteps = 0; //numsteps should be >=4 

    const char* src_uri = NULL;
    const char* dest_uri = NULL;
    time_t src_mtime = 0;

    //1. init parameters
    sigma = g_strtod (argv[1], NULL);
    numsteps = (long) g_strtod (argv[2], NULL);

    src_uri = g_strdup (argv[3]);
    if (!g_file_test (src_uri, G_FILE_TEST_EXISTS))
	return 1
    dest_uri = bg_blur_pict_get_dest_uri (src_uri);
    struct stat _stat_buffer;
    memset (&_stat_buffer, 0, sizeof (struct stat));
    if (stat (src_uri, &_stat_buffer) == 0)// success
    {
	src_mtime = _stat_buffer.st_mtime;
    }
    //2. check if cache directory is existing.
    bg_blur_pict_make_cache_dir ();

    //3. lookup cached pngs
    char* blur_uri = NULL;
    blur_uri = bg_blur_pict_factory_lookup (dest_uri, src_mtime);
    
    //create the picture.
    if (G_UNLIKELY (blur_uri == NULL))
    {
	blur_uri = bg_blur_pict_generate (src_uri, dest_uri, src_mtime);
	if (blur_uri == NULL)
	    return EXIT_FAILURE;
    }
    g_free (blur_uri);

    return EXIT_SUCCESS;
}

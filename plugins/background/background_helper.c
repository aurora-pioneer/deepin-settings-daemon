/*
 *	we put generated gaussian-blurred background pictures
 *	in $XDG_CACHE/"gaussian-background".
 *	and use methods similar to those specified in freedesktop
 *	thumbnail spec.
 *	PNG namespace used here:
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
//remove dependency on gtk
#include <glib.h>
#include <glib/gstdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gaussianiir2d.h"

//gaussian blur
#define BG_GAUSSIAN_PICT_PATH	"/tmp/.deepin_background_gaussian.png"

#define	BG_BLUR_PICT_CACHE_DIR "gaussian-background"
#define BG_EXT_URI	 "tEXt::Blur::URI"
#define BG_EXT_MTIME	 "tEXt::Blur::MTime"


static char* bg_blur_pict_get_dest_path (const char* src_uri);
static char * bg_blur_pict_factory_lookup (const char* src_uri, const char *dest_path, time_t src_mtime);
static gboolean bg_blur_pict_is_valid (GdkPixbuf* pixbuf, const char* src_uri, const char* dest_path, time_t src_mtime);
static char* bg_blur_pict_generate (const char* src_uri, const char* dest_path, time_t src_mtime, double sigma, long numsteps);
static gboolean bg_blur_pict_make_cache_dir ();
/*
 *	@src_uri: source background image URI	
 *
 *	return  : generated destination path (not URI).
 */
static char*
bg_blur_pict_get_dest_path (const char* src_uri)
{
    g_debug ("bg_blur_pict_get_dest_path: src_uri=%s", src_uri);
    g_return_val_if_fail (src_uri != NULL, NULL);

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
 *	@dest_path:  possible destination file path
 *	@mtime: the original background file modification time
 *
 *	return : valid blurred picture file path or NULL.
 */
static char *
bg_blur_pict_factory_lookup (const char* src_uri, 
			     const char *dest_path, 
			     time_t src_mtime)
{
    g_debug ("bg_blur_pict_factory_lookup: src_uri=%s", src_uri);
    g_debug ("                             dest_path=%s", dest_path);
    g_debug ("                             src_mtime=%ld", src_mtime);
    g_return_val_if_fail (dest_path != NULL, NULL);

    GdkPixbuf *pixbuf;
    pixbuf = gdk_pixbuf_new_from_file (dest_path, NULL);

    gboolean is_valid = FALSE;
    if (pixbuf != NULL)
    {
	is_valid = bg_blur_pict_is_valid (pixbuf, src_uri, dest_path, src_mtime);
	g_object_unref (pixbuf);
    }
    if (is_valid)
	return g_strdup (dest_path);

    return NULL;
}
/*
 *	@pixbuf: 
 *	@dest_path: 	
 */
static gboolean 
bg_blur_pict_is_valid (GdkPixbuf* pixbuf, 
		       const char* src_uri, 
		       const char* dest_path, 
		       time_t src_mtime)
{
    g_debug ("bg_blur_pict_is_valid:");
    //1. check if the original uri matches the provided @uri
    const char *blur_uri;
    blur_uri = gdk_pixbuf_get_option (pixbuf, BG_EXT_URI);
    if (!blur_uri)
	return FALSE;
    if (strcmp (src_uri, blur_uri) != 0)
	return FALSE;
  
    //2. check if the modification time matches
    const char *blur_mtime_str;
    time_t blur_mtime;
    blur_mtime_str = gdk_pixbuf_get_option (pixbuf, BG_EXT_MTIME);
    if (!blur_mtime_str)
	return FALSE;
    blur_mtime = atol (blur_mtime_str);
    if (src_mtime != blur_mtime)
	return FALSE;
  
    return TRUE;
}
/*
 *	create cache directory for blurred images.	
 */
static gboolean
bg_blur_pict_make_cache_dir ()
{
    g_debug ("bg_blur_pict_make_cache_dir:");
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
bg_blur_pict_generate (const char* src_uri, 
		       const char* dest_path, 
		       time_t src_mtime,
		       double sigma,
		       long numsteps)
{
    g_debug ("bg_blur_pict_generate: src_uri=%s", src_uri);
    g_debug ("                       dest_path=%s", dest_path);
    g_debug ("                       src_mtime=%ld", src_mtime);
    g_debug ("                       sigma=%lf", sigma);
    g_debug ("                       numsteps=%ld", numsteps);
    GError* error = NULL;

    GdkPixbuf* pixbuf = NULL;
    guchar* image_data = NULL;
    int width = 0;
    int height = 0;
    int rowstride = 0;
    int n_channels = 0;

    pixbuf = gdk_pixbuf_new_from_file (src_uri, &error);
    if (error != NULL)
    {
	g_debug ("background_helper: %s", error->message);
	g_error_free (error);
	return NULL;
    }
    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);
    rowstride = gdk_pixbuf_get_rowstride (pixbuf);
    n_channels = gdk_pixbuf_get_n_channels (pixbuf);
    image_data = gdk_pixbuf_get_pixels (pixbuf);

    //TODO: should add support for various formats
    //      no alpha, n_channels
    clock_t start = clock ();
    gaussianiir2d_pixbuf_c(image_data, width, height, rowstride, n_channels, sigma, numsteps);
    clock_t end = clock ();
    g_debug ("time : %f", (end-start)/(float)CLOCKS_PER_SEC);

    // create tmp file
    char* tmp_path;
    int tmp_fd;
    tmp_path = g_strconcat (dest_path, ".XXXXXX", NULL);
    tmp_fd = g_mkstemp (tmp_path);
    if (tmp_fd == -1)
    {
      g_free (tmp_path);
      return NULL;
    }
    close (tmp_fd);

    // save pixbuf
    char mtime_str[21];
    gboolean saved_ok;
    g_snprintf (mtime_str, 21, "%ld",  src_mtime);
    saved_ok = gdk_pixbuf_save (pixbuf,
				tmp_path,
				"png", NULL,
				BG_EXT_URI, src_uri,
				BG_EXT_MTIME, mtime_str,
				NULL);

    if (saved_ok)
    {
	g_chmod (tmp_path, 0600);
	g_rename (tmp_path, dest_path);
    }
    g_free (tmp_path);
    return 0;
}

static gboolean 
is_app_running (void)
{
    char* path = "/tmp/gsd/background/helper";
    int server_sockfd;
    socklen_t server_len;
    struct sockaddr_un server_addr;

    server_addr.sun_path[0] = '\0'; //abstract namespace socket.
    strcpy(server_addr.sun_path+1, path);
    server_addr.sun_family = AF_UNIX;
    server_len = 1 + strlen(path) + offsetof(struct sockaddr_un, sun_path);

    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (0 == bind(server_sockfd, (struct sockaddr *)&server_addr, server_len)) 
        return FALSE;
    else 
        return TRUE;
    
}
/*
 *	./a.out <sigma> <numsteps> <picture_uri>
 */
int
main (int argc, char** argv)
{
    //exit if a gsd-background-helper is already running
    if (is_app_running ())
	return EXIT_SUCCESS;

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
    g_setenv ("G_MESSAGES_DEBUG", "all", FALSE);
    
    double sigma = 0;
    long numsteps = 0; //numsteps should be >=4 

    const char* src_uri = NULL;
    const char* dest_path = NULL;
    time_t src_mtime = 0;

    //1. init parameters
    sigma = g_strtod (argv[1], NULL);
    numsteps = (long) g_strtod (argv[2], NULL);
    g_debug ("sigma: %f", sigma);
    g_debug ("numsteps: %ld", numsteps);

    src_uri = g_strdup (argv[3]);
    if (!g_file_test (src_uri, G_FILE_TEST_EXISTS))
	return EXIT_FAILURE;
    dest_path = bg_blur_pict_get_dest_path (src_uri);
    struct stat _stat_buffer;
    memset (&_stat_buffer, 0, sizeof (struct stat));
    if (stat (src_uri, &_stat_buffer) == 0)// success
    {
	src_mtime = _stat_buffer.st_mtime;
    }
    g_debug ("src_uri: %s", src_uri);
    g_debug ("dest_path: %s", dest_path);
    //2. check if cache directory is existing.
    bg_blur_pict_make_cache_dir ();

    //3. lookup cached pngs
    char* blur_path = NULL;
    blur_path = bg_blur_pict_factory_lookup (src_uri, dest_path, src_mtime);
    
    if (G_LIKELY (blur_path !=  NULL))
    {
	g_debug ("blurred picture exists");
	//symlink to this file
	unlink (BG_GAUSSIAN_PICT_PATH);
	(void)symlink (blur_path, BG_GAUSSIAN_PICT_PATH);
	g_free (blur_path);
    }
    else
    {
	g_debug ("blurred picture does not exist");
        //create the picture. 
	//we should not re-symlink BG_GAUSSIAN_PICT_PATH even after we
	//have created it. because at the time of the re-symlink the current
	//picture is changed.
	blur_path = bg_blur_pict_generate (src_uri, dest_path, src_mtime,
					   sigma, numsteps);
	if (blur_path == NULL)
	    return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

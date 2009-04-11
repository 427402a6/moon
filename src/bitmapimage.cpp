/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bitmapimage.cpp
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007-2008 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "application.h"
#include "bitmapimage.h"
#include "deployment.h"
#include "runtime.h"
#include "uri.h"
#include "debug.h"

#ifdef WORDS_BIGENDIAN
#define set_pixel_bgra(pixel,index,b,g,r,a) \
	G_STMT_START { \
		((unsigned char *)(pixel))[index]   = a; \
		((unsigned char *)(pixel))[index+1] = r; \
		((unsigned char *)(pixel))[index+2] = g; \
		((unsigned char *)(pixel))[index+3] = b; \
	} G_STMT_END
#define get_pixel_bgr_p(p,b,g,r) \
	G_STMT_START { \
		r = *(p);   \
		g = *(p+1); \
		b = *(p+2); \
	} G_STMT_END
#else
#define set_pixel_bgra(pixel,index,b,g,r,a) \
	G_STMT_START { \
		((unsigned char *)(pixel))[index]   = b; \
		((unsigned char *)(pixel))[index+1] = g; \
		((unsigned char *)(pixel))[index+2] = r; \
		((unsigned char *)(pixel))[index+3] = a; \
	} G_STMT_END
#define get_pixel_bgr_p(p,b,g,r) \
	G_STMT_START { \
		b = *(p);   \
		g = *(p+1); \
		r = *(p+2); \
	} G_STMT_END
#endif
#define get_pixel_bgra(color, b, g, r, a) \
	G_STMT_START { \
		a = *(p+3);	\
		r = *(p+2);	\
		g = *(p+1);	\
		b = *(p+0);	\
	} G_STMT_END
#include "alpha-premul-table.inc"

//
// Expands RGB to ARGB allocating new buffer for it.
//
static gpointer
expand_rgb_to_argb (GdkPixbuf *pixbuf)
{
	guchar *pb_pixels = gdk_pixbuf_get_pixels (pixbuf);
	guchar *p;
	int w = gdk_pixbuf_get_width (pixbuf);
	int h = gdk_pixbuf_get_height (pixbuf);
	int stride = w * 4;
	guchar *data = (guchar *) g_malloc (stride * h);
	guchar *out;

	for (int y = 0; y < h; y ++) {
		p = pb_pixels + y * gdk_pixbuf_get_rowstride (pixbuf);
		out = data + y * (stride);
		for (int x = 0; x < w; x ++) {
			guchar r, g, b;

			get_pixel_bgr_p (p, b, g, r);
			set_pixel_bgra (out, 0, r, g, b, 255);

			p += 3;
			out += 4;
		}
	}

	return (gpointer) data;
}

//
// Converts RGBA unmultiplied alpha to ARGB pre-multiplied alpha.
//
static gpointer
premultiply_rgba (GdkPixbuf *pixbuf)
{
	guchar *pb_pixels = gdk_pixbuf_get_pixels (pixbuf);
	guchar *p;
	int w = gdk_pixbuf_get_width (pixbuf);
	int h = gdk_pixbuf_get_height (pixbuf);
	int stride = w * 4;
	guchar *data = (guchar *) g_malloc (stride * h);
	guchar *out;

	for (int y = 0; y < h; y ++) {
		p = pb_pixels + y * gdk_pixbuf_get_rowstride (pixbuf);
		out = data + y * (stride);
		for (int x = 0; x < w; x ++) {
			guchar r, g, b, a;

			get_pixel_bgra (p, b, g, r, a);

			/* pre-multipled alpha */
			if (a == 0) {
				r = g = b = 0;
			}
			else if (a < 255) {
				r = pre_multiplied_table [r][a];
				g = pre_multiplied_table [g][a];
				b = pre_multiplied_table [b][a];
			}

			/* store it back, swapping red and blue */
			set_pixel_bgra (out, 0, r, g, b, a);

			p += 4;
			out += 4;
		}
	}

	return (gpointer) data;
}

BitmapImage::BitmapImage ()
{
	SetObjectType (Type::BITMAPIMAGE);
	downloader = NULL;
	loader = NULL;
	error = NULL;
	part_name = NULL;
}

BitmapImage::~BitmapImage ()
{
	if (downloader)
		downloader->unref ();

	if (part_name)
		g_free (part_name);

	CleanupLoader ();
}

void
BitmapImage::Dispose ()
{
	if (downloader) {
		CleanupDownloader ();
		downloader->Abort ();
	}
	EventObject::Dispose ();
}

void
BitmapImage::uri_source_changed_callback (EventObject *user_data)
{
	BitmapImage *image = (BitmapImage *) user_data;
	image->UriSourceChanged ();
}

void
BitmapImage::UriSourceChanged ()
{
	Surface *surface = Deployment::GetCurrent ()->GetSurface ();
	Application *current = Application::GetCurrent ();
	Uri *uri = GetUriSource ();
	
	if (current && uri) {
		int size = 0;
		unsigned char *buffer = (unsigned char *) current->GetResource (uri, &size);
		if (size > 0) {
			PixbufWrite (buffer, 0, size);
			PixmapComplete ();

			g_free (buffer);
			return;
		}
	}
	
	if (surface == NULL) {
		SetBitmapData (NULL);
		return;
	}

	if (!(downloader = surface->CreateDownloader ()))
		return;

	SetDownloader (downloader, uri, NULL);
	downloader->unref ();
}


void
BitmapImage::OnPropertyChanged (PropertyChangedEventArgs *args, MoonError *error)
{
	if (args->GetId () == BitmapImage::UriSourceProperty) {
		Uri *uri = args->GetNewValue () ? args->GetNewValue ()->AsUri () : NULL;

		if (downloader) {
			CleanupDownloader ();
			downloader->Abort ();
			downloader->unref ();
			downloader = NULL;
		}

		if (Uri::IsNullOrEmpty (uri)) {
			SetBitmapData (NULL);
		} else {
			AddTickCall (uri_source_changed_callback);
		}
	} else if (args->GetId () == BitmapImage::ProgressProperty) {
		Emit (DownloadProgressEvent, new DownloadProgressEventArgs (GetProgress ()));
	}

	NotifyListenersOfPropertyChange (args);
}

void
BitmapImage::SetDownloader (Downloader *downloader, Uri *uri, const char *part_name)
{
	this->downloader = downloader;
	this->part_name = g_strdup (part_name);

	downloader->ref();

	downloader->AddHandler (Downloader::DownloadProgressChangedEvent, downloader_progress_changed, this);
	downloader->AddHandler (Downloader::DownloadFailedEvent, downloader_failed, this);
	downloader->AddHandler (Downloader::CompletedEvent, downloader_complete, this);

	if (downloader->Completed ()) {
		DownloaderComplete ();
	} else {
		if (!downloader->Started ()) {
			if (uri) {
				char *str = uri->ToString ();
				downloader->Open ("GET", str, MediaPolicy);
				g_free (str);
			}
			downloader->SetStreamFunctions (pixbuf_write, NULL, this);
			downloader->Send ();
		}
	}
}

void
BitmapImage::CleanupDownloader ()
{
	downloader->RemoveHandler (Downloader::DownloadProgressChangedEvent, downloader_progress_changed, this);
	downloader->RemoveHandler (Downloader::DownloadFailedEvent, downloader_failed, this);
	downloader->RemoveHandler (Downloader::CompletedEvent, downloader_complete, this);
}

void
BitmapImage::downloader_progress_changed (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	BitmapImage *media = (BitmapImage *) closure;

	media->DownloaderProgressChanged ();
}

void
BitmapImage::downloader_complete (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	BitmapImage *media = (BitmapImage *) closure;

	media->DownloaderComplete ();
}

void
BitmapImage::downloader_failed (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	BitmapImage *media = (BitmapImage *) closure;

	media->DownloaderFailed ();
}

void
BitmapImage::DownloaderProgressChanged ()
{
	SetProgress (downloader->GetDownloadProgress ());
}

void
BitmapImage::DownloaderComplete ()
{
	CleanupDownloader ();

	SetProgress (1.0);

	if (loader == NULL) {
		char *filename = downloader->GetDownloadedFilename (part_name);

		if (filename == NULL) {
			guchar *buffer = (guchar *)downloader->GetBuffer ();

			if (buffer == NULL) goto failed;

			PixbufWrite (buffer, 0, downloader->GetSize ());
		} else {
			guchar b[4096];
			int offset = 0;
			ssize_t n;
			int fd;

			if ((fd = open (filename, O_RDONLY)) == -1)
				goto failed;
	
			do {
				do {
					n = read (fd, b, sizeof (b));
				} while (n == -1 && errno == EINTR);

				if (n == -1) break;

				PixbufWrite (b, offset, n);

				offset += n;
			} while (n > 0 && !error);

			close (fd);

			if (error) goto failed;
		}
	}

	downloader->unref ();
	downloader = NULL;

	PixmapComplete ();

	return;
failed:
	downloader->unref ();
	downloader = NULL;

	gdk_pixbuf_loader_close (loader, NULL);
	CleanupLoader ();

	Emit (ImageFailedEvent, new ExceptionRoutedEventArgs ());
}


void
BitmapImage::PixmapComplete ()
{
	SetProgress (1.0);

	gdk_pixbuf_loader_close (loader, error == NULL ? &error : NULL);

	if (error) goto failed;

	{
		GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
		
		if (pixbuf == NULL) goto failed;

		SetPixelWidth (gdk_pixbuf_get_width (pixbuf));
		SetPixelHeight (gdk_pixbuf_get_height (pixbuf));

		if (gdk_pixbuf_get_n_channels (pixbuf) == 4) {
			SetPixelFormat (PixelFormatPbgra32);
			SetBitmapData (premultiply_rgba (pixbuf));
		} else {
			SetPixelFormat (PixelFormatBgr32);
			SetBitmapData (expand_rgb_to_argb (pixbuf));
		}

		Invalidate ();

		g_object_unref (loader);
		loader = NULL;
	}

	Emit (ImageOpenedEvent, new RoutedEventArgs ());

	return;

failed:
	CleanupLoader ();

	Emit (ImageFailedEvent, new ExceptionRoutedEventArgs ());
}

void
BitmapImage::DownloaderFailed ()
{
	CleanupDownloader ();

	downloader->unref ();
	downloader = NULL;

	Emit (ImageFailedEvent, new ExceptionRoutedEventArgs ());
}

void
BitmapImage::CleanupLoader ()
{
	if (loader) {
		g_object_unref (loader);
		loader = NULL;
	}
	if (error) {
		g_error_free (error);
		error = NULL;
	}
}

void
BitmapImage::CreateLoader (unsigned char *buffer)
{
	if (!(moonlight_flags & RUNTIME_INIT_ALL_IMAGE_FORMATS)) {
		// 89 50 4E 47 == png magic
		if (buffer[0] == 0x89)
			loader = gdk_pixbuf_loader_new_with_type ("png", NULL);
		// ff d8 ff e0 == jfif magic
		if (buffer[0] == 0xff)
			loader = gdk_pixbuf_loader_new_with_type ("jpeg", NULL);
	} else {
		loader = gdk_pixbuf_loader_new ();
	}
}

void
BitmapImage::PixbufWrite (gpointer buffer, gint32 offset, gint32 n)
{
	if (loader == NULL && offset == 0)
		CreateLoader ((unsigned char *)buffer);

	if (loader != NULL && error == NULL) {
		gdk_pixbuf_loader_write (GDK_PIXBUF_LOADER (loader), (const guchar *)buffer, n, &error);
	}
}

void
BitmapImage::pixbuf_write (void *buffer, gint32 offset, gint32 n, gpointer data)
{
	BitmapImage *source = (BitmapImage *) data;

	source->PixbufWrite ((unsigned char *)buffer, offset, n);
}

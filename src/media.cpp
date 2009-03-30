/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * media.cpp: 
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "writeablebitmap.h"
#include "bitmapimage.h"
#include "uri.h"
#include "runtime.h"
#include "media.h"
#include "error.h"
#include "downloader.h"
#include "geometry.h"
#include "timeline.h"
#include "debug.h"

/*
 * MediaBase
 */

MediaBase::MediaBase ()
{
	SetObjectType (Type::MEDIABASE);

	source.downloader = NULL;
	source.part_name = NULL;
	source.queued = false;
	downloader = NULL;
	part_name = NULL;
	updating_size_from_media = false;
	allow_downloads = false;
	use_media_height = true;
	use_media_width = true;
	source_changed = false;
}

MediaBase::~MediaBase ()
{
	DownloaderAbort ();
}

void
MediaBase::downloader_complete (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	MediaBase *media = (MediaBase *) closure;
	
	media->DownloaderComplete ();
}

void
MediaBase::downloader_failed (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	MediaBase *media = (MediaBase *) closure;
	
	media->DownloaderFailed (calldata);
}

void
MediaBase::DownloaderComplete ()
{
	// Nothing for MediaBase to do...
}

void
MediaBase::DownloaderFailed (EventArgs *args)
{
	// Nothing for MediaBase to do...
}

void
MediaBase::DownloaderAbort ()
{
	if (downloader) {
		downloader->RemoveHandler (Downloader::DownloadFailedEvent, downloader_failed, this);
		downloader->RemoveHandler (Downloader::CompletedEvent, downloader_complete, this);
		downloader->SetWriteFunc (NULL, NULL, NULL);
		downloader->Abort ();
		downloader->unref ();
		g_free (part_name);
		downloader = NULL;
		part_name = NULL;
	}
}

void
MediaBase::SetAllowDownloads (bool allow)
{
	Surface *surface = GetSurface ();
	const char *uri;
	Downloader *dl;
	
	if ((allow_downloads && allow) || (!allow_downloads && !allow))
		return;
	
	if (allow && surface && source_changed) {
		source_changed = false;
		
		if ((uri = GetSource ()) && *uri) {
			if (!(dl = surface->CreateDownloader ())) {
				// we're shutting down
				return;
			}
			
			dl->Open ("GET", uri, GetDownloaderPolicy (uri));
			SetSource (dl, "");
			dl->unref ();
		}
	}
	
	allow_downloads = allow;
}

void
MediaBase::OnLoaded ()
{
	FrameworkElement::OnLoaded ();
	SetAllowDownloads (true);
}

void
MediaBase::SetSourceAsyncCallback ()
{
	Downloader *downloader;
	char *part_name;

	DownloaderAbort ();

	downloader = source.downloader;
	part_name = source.part_name;

	source.queued = false;
	source.downloader = NULL;
	source.part_name = NULL;
	
	if (GetSurface () == NULL)
		return;
	
	SetSourceInternal (downloader, part_name);
	
	if (downloader)
		downloader->unref ();
}

void
MediaBase::SetSourceInternal (Downloader *downloader, char *PartName)
{
	this->downloader = downloader;
	part_name = PartName;
	
	if (downloader)
		downloader->ref ();
}

void
MediaBase::set_source_async (EventObject *user_data)
{
	MediaBase *media = (MediaBase *) user_data;
	
	media->SetSourceAsyncCallback ();
}

void
MediaBase::SetSource (Downloader *downloader, const char *PartName)
{
	source_changed = false;
	
	if (source.queued) {
		if (source.downloader)
			source.downloader->unref ();

		g_free (source.part_name);
		source.downloader = NULL;
		source.part_name = NULL;
	}
	
	source.part_name = g_strdup (PartName);
	source.downloader = downloader;
	
	if (downloader)
		downloader->ref ();

	if (source.downloader && source.downloader->Completed ()) {
		SetSourceInternal (source.downloader, source.part_name);
		source.downloader->unref ();
	} else if (!source.queued) {
		AddTickCall (MediaBase::set_source_async);
		source.queued = true;
	}
}

void
MediaBase::OnPropertyChanged (PropertyChangedEventArgs *args, MoonError *error)
{
	if (args->GetId () == MediaBase::SourceProperty) {
		const char *uri = args->GetNewValue() ? args->GetNewValue()->AsString () : NULL;
		Surface *surface = GetSurface ();
					
		if (surface && AllowDownloads ()) {
			if (uri && *uri) {
				Downloader *dl;
				if ((dl = surface->CreateDownloader ())) {
					dl->Open ("GET", uri, GetDownloaderPolicy (uri));
					SetSource (dl, "");
					dl->unref ();
				} else {
					// we're shutting down
				}
			} else {
				SetSource (NULL, NULL);
			}
		} else {
			source_changed = true;
		}
	}
	
	if (args->GetProperty ()->GetOwnerType() != Type::MEDIABASE) {
		FrameworkElement::OnPropertyChanged (args, error);
		return;
	}
	
	NotifyListenersOfPropertyChange (args);
}

//
// Image
//
Image::Image ()
{
	SetObjectType (Type::IMAGE);
}

Image::~Image ()
{
}

void
Image::download_progress (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	Image *media = (Image *) closure;

	media->DownloadProgress ();
}

void
Image::image_opened (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	Image *media = (Image *) closure;

	media->ImageOpened ();
}

void
Image::image_failed (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	Image *media = (Image *) closure;

	media->ImageFailed ();
}

void
Image::DownloadProgress ()
{
	BitmapImage *source = (BitmapImage *) GetSource ();

	SetDownloadProgress (source->GetProgress ());
	Emit (DownloadProgressChangedEvent);
}

void
Image::ImageOpened ()
{
	BitmapImage *source = (BitmapImage *) GetSource ();

	source->RemoveHandler (BitmapImage::DownloadProgressEvent, download_progress, this);
	source->RemoveHandler (BitmapImage::ImageOpenedEvent, image_opened, this);
	source->RemoveHandler (BitmapImage::ImageFailedEvent, image_failed, this);

	UpdateSize ();
}

void
Image::ImageFailed ()
{
	BitmapImage *source = (BitmapImage *) GetSource ();

	source->RemoveHandler (BitmapImage::DownloadProgressEvent, download_progress, this);
	source->RemoveHandler (BitmapImage::ImageOpenedEvent, image_opened, this);
	source->RemoveHandler (BitmapImage::ImageFailedEvent, image_failed, this);

	Emit (ImageFailedEvent, new ImageErrorEventArgs (NULL));
}

void
Image::SetSourceInternal (Downloader *downloader, char *PartName)
{
	BitmapImage *source = (BitmapImage *) GetSource ();

	MediaBase::SetSourceInternal (downloader, PartName);

	source->AddHandler (BitmapImage::DownloadProgressEvent, download_progress, this);
	source->AddHandler (BitmapImage::ImageOpenedEvent, image_opened, this);
	source->AddHandler (BitmapImage::ImageFailedEvent, image_failed, this);

	source->SetDownloader (downloader, NULL, PartName);
}

void
Image::SetSource (Downloader *downloader, const char *PartName)
{
	MediaBase::SetSource (downloader, PartName);
}

void
Image::UpdateSize ()
{
	/* XXX FIXME horrible hack to keep old world charm until canvas logic is updated */
	if (GetVisualParent () && GetVisualParent ()->Is (Type::CANVAS)) {
		updating_size_from_media = true;
		
		if (use_media_width) {
			Value *height = GetValueNoDefault (FrameworkElement::HeightProperty);
			
			if (!use_media_height)
				SetWidth ((double) GetSource ()->GetPixelWidth () * height->AsDouble () / (double) GetSource ()->GetPixelHeight ());
			else
				SetWidth ((double) GetSource ()->GetPixelWidth ());
		}
		
		if (use_media_height) {
			Value *width = GetValueNoDefault (FrameworkElement::WidthProperty);
			
			if (!use_media_width)
				SetHeight ((double) GetSource ()->GetPixelHeight () * width->AsDouble () / (double) GetSource ()->GetPixelWidth ());
			else
				SetHeight ((double) GetSource ()->GetPixelHeight ());
		}
		
		updating_size_from_media = false;
	}
	
	InvalidateMeasure ();
	Invalidate ();
}

void
Image::Render (cairo_t *cr, Region *region, bool path_only)
{
	ImageSource *source = GetSource ();
	cairo_surface_t *cairo_surface;
	cairo_pattern_t *pattern;
	cairo_matrix_t matrix;
	Rect image;
	Rect paint;
	Geometry *clip;

	if (!source)
		return;

	source->Lock ();

	cairo_surface = source->GetSurface (cr);

	if (GetActualWidth () == 0.0 && GetActualHeight () == 0.0)
		return;
	if (source->GetPixelWidth () == 0.0 && source->GetPixelWidth () == 0.0)
		return;

	cairo_save (cr);

	image = Rect (0, 0, source->GetPixelWidth (), source->GetPixelHeight ());
	paint = Rect (0, 0, GetActualWidth (), GetActualHeight ());
	pattern = cairo_pattern_create_for_surface (cairo_surface);
	
	image_brush_compute_pattern_matrix (&matrix, paint.width, paint.height, image.width, image.height, GetStretch (), 
					    AlignmentXCenter, AlignmentYCenter, NULL, NULL);
	
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_set_source (cr, pattern);

	cairo_set_matrix (cr, &absolute_xform);
	
	clip = LayoutInformation::GetLayoutClip (this);
	if (clip) {
		clip->Draw (cr);
		cairo_clip (cr);
	}	

	paint.Draw (cr);
	cairo_fill (cr);

	cairo_restore (cr);
	cairo_pattern_destroy (pattern);

	source->Unlock ();
}

Size
Image::MeasureOverride (Size availableSize)
{
	Size desired = availableSize;
	Rect shape_bounds = Rect ();
	ImageSource *source = GetSource ();
	double sx = 0.0;
	double sy = 0.0;

	if (source)
		shape_bounds = Rect (0,0,source->GetPixelWidth (),source->GetPixelHeight ());

	if (GetStretch () == StretchNone)
		return desired.Min (shape_bounds.width, shape_bounds.height);

	/* don't stretch to infinite size */
	if (isinf (desired.width))
		desired.width = shape_bounds.width;
	if (isinf (desired.height))
		desired.height = shape_bounds.height;
	
	/* compute the scaling */
	if (shape_bounds.width > 0)
		sx = desired.width / shape_bounds.width;
	if (shape_bounds.height > 0)
		sy = desired.height / shape_bounds.height;

	switch (GetStretch ()) {
	case StretchUniform:
		sx = sy = MIN (sx, sy);
		break;
	case StretchUniformToFill:
		sx = sy = MAX (sx, sy);
		break;
	default:
		break;
	}

	desired = desired.Min (shape_bounds.width * sx, shape_bounds.height * sy);

	return desired;
}

Size
Image::ArrangeOverride (Size finalSize)
{
	Size arranged = finalSize;
	Rect shape_bounds = Rect ();
	ImageSource *source = GetSource ();
	double sx = 1.0;
	double sy = 1.0;


	if (source)
		shape_bounds = Rect (0, 0, source->GetPixelWidth (), source->GetPixelHeight ());

	if (GetStretch () == StretchNone) {
		arranged = Size (shape_bounds.x + shape_bounds.width,
				 shape_bounds.y + shape_bounds.height);

		if (GetHorizontalAlignment () == HorizontalAlignmentStretch)
			arranged.width = MAX (arranged.width, finalSize.width);

		if (GetVerticalAlignment () == VerticalAlignmentStretch)
			arranged.height = MAX (arranged.height, finalSize.height);

		return arranged;
	}

	/* compute the scaling */
	if (shape_bounds.width == 0)
		shape_bounds.width = arranged.width;
	if (shape_bounds.height == 0)
		shape_bounds.height = arranged.height;

	if (shape_bounds.width != arranged.width)
		sx = arranged.width / shape_bounds.width;
	if (shape_bounds.height != arranged.height)
		sy = arranged.height / shape_bounds.height;

	switch (GetStretch ()) {
	case StretchUniform:
		sx = sy = MIN (sx, sy);
		break;
	case StretchUniformToFill:
		sx = sy = MAX (sx, sy);
		break;
	default:
		break;
	}

	arranged = Size (shape_bounds.width * sx, shape_bounds.height * sy);

	return arranged;
}

Rect
Image::GetCoverageBounds ()
{
	Stretch stretch = GetStretch ();
	ImageSource *source = GetSource ();

	if (!source || source->GetPixelFormat () == PixelFormatPbgra32)
		return Rect ();

	if (stretch == StretchFill || stretch == StretchUniformToFill)
		return bounds;

	cairo_matrix_t matrix;
	Rect image = Rect (0, 0, source->GetPixelWidth (), source->GetPixelHeight ());
	Rect paint = Rect (0, 0, GetActualWidth (), GetActualHeight ());

	image_brush_compute_pattern_matrix (&matrix, 
					    paint.width, paint.height,
					    image.width, image.height, stretch, 
					    AlignmentXCenter, AlignmentYCenter, NULL, NULL);

	cairo_matrix_invert (&matrix);
	cairo_matrix_multiply (&matrix, &matrix, &absolute_xform);

	image = image.Transform (&matrix);
	image = image.Intersection (bounds);
	
	return image;
}

void
Image::OnPropertyChanged (PropertyChangedEventArgs *args, MoonError *error)
{
	if (args->GetId () == FrameworkElement::HeightProperty) {
		if (!updating_size_from_media)
			use_media_height = args->GetNewValue() == NULL;
	} else if (args->GetId () == FrameworkElement::WidthProperty) {
		if (!updating_size_from_media)
			use_media_width = args->GetNewValue() == NULL;
	} else if (args->GetId () == Image::SourceProperty) {
		ImageSource *source = args->GetNewValue () ? args->GetNewValue ()->AsImageSource () : NULL; 
		ImageSource *old = args->GetOldValue () ? args->GetOldValue ()->AsImageSource () : NULL;

		if (old && old->Is(Type::BITMAPIMAGE)) {
			((BitmapImage *)old)->RemoveHandler (BitmapImage::DownloadProgressEvent, download_progress, this);
			((BitmapImage *)old)->RemoveHandler (BitmapImage::ImageOpenedEvent, image_opened, this);
			((BitmapImage *)old)->RemoveHandler (BitmapImage::ImageFailedEvent, image_failed, this);
		}
		if (source && source->Is(Type::BITMAPIMAGE)) {
			((BitmapImage *)source)->AddHandler (BitmapImage::DownloadProgressEvent, download_progress, this);
			((BitmapImage *)source)->AddHandler (BitmapImage::ImageOpenedEvent, image_opened, this);
			((BitmapImage *)source)->AddHandler (BitmapImage::ImageFailedEvent, image_failed, this);
		}
	}

	if (args->GetProperty ()->GetOwnerType() != Type::IMAGE) {
		MediaBase::OnPropertyChanged (args, error);
		return;
	}
	
	// we need to notify attachees if our DownloadProgress changed.
	NotifyListenersOfPropertyChange (args);
}

bool
Image::InsideObject (cairo_t *cr, double x, double y)
{
	if (!GetSource ())
		return false;

	return FrameworkElement::InsideObject (cr, x, y);
}

Value *
Image::CreateDefaultImageSource (DependencyObject *instance, DependencyProperty *property)
{
	return Value::CreateUnrefPtr (new BitmapImage ());
}

//
// MediaAttributeCollection
//

MediaAttribute *
MediaAttributeCollection::GetItemByName (const char *name)
{
	MediaAttribute *attr;
	const char *value;
	
	for (guint i = 0; i < array->len; i++) {
		attr = ((Value *) array->pdata[i])->AsMediaAttribute ();
		if (!(value = attr->GetName ()))
			continue;
		
		if (!strcmp (value, name))
			return attr;
	}
	
	return NULL;
}


//
// TimelineMarkerCollection
//

int
TimelineMarkerCollection::AddWithError (Value *value, MoonError *error)
{
	TimelineMarker *marker, *cur;
	
	marker = value->AsTimelineMarker ();
	
	for (guint i = 0; i < array->len; i++) {
		cur = ((Value *) array->pdata[i])->AsTimelineMarker ();
		if (cur->GetTime () >= marker->GetTime ()) {
			DependencyObjectCollection::InsertWithError (i, value, error);
			return i;
		}
	}
	
	return DependencyObjectCollection::InsertWithError (array->len, value, error) ? array->len - 1 : -1;
}

bool
TimelineMarkerCollection::InsertWithError (int index, Value *value, MoonError *error)
{
	return AddWithError (value, error) != -1;
}


//
// MarkerReachedEventArgs
//

MarkerReachedEventArgs::MarkerReachedEventArgs (TimelineMarker *marker)
{
	SetObjectType (Type::MARKERREACHEDEVENTARGS);
	this->marker = marker;
	marker->ref ();
}

MarkerReachedEventArgs::~MarkerReachedEventArgs ()
{
	marker->unref ();
}

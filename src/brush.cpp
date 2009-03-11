/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * brush.cpp: Brushes
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007, 2008 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#include <config.h>
#include <cairo.h>
#include <glib.h>
#include "brush.h"
#include "media.h"
#include "mediaelement.h"
#include "color.h"
#include "transform.h"
#include "mediaplayer.h"

//
// SL-Cairo convertion and helper routines
//

static cairo_extend_t
convert_gradient_spread_method (GradientSpreadMethod method)
{
	switch (method) {
	case GradientSpreadMethodPad:
		return CAIRO_EXTEND_PAD;
	case GradientSpreadMethodReflect:
		return CAIRO_EXTEND_REFLECT;
	// unknown (e.g. bad) values are considered to be Repeat by Silverlight
	// even if the default, i.e. *no* value) is Pad
	case GradientSpreadMethodRepeat:
	default:
		return CAIRO_EXTEND_REPEAT;
	}
}

//
// Brush
//


Brush::Brush()
{
	SetObjectType (Type::BRUSH);
}

void
Brush::SetupBrush (cairo_t *cr, const Rect &area)
{
	g_warning ("Brush:SetupBrush has been called. The derived class should have overridden it.");
}

void
Brush::Fill (cairo_t *cr, bool preserve)
{
	if (preserve)
		cairo_fill_preserve (cr);
	else 
		cairo_fill (cr);
}

void
Brush::Stroke (cairo_t *cr, bool preserve)
{
	if (preserve)
		cairo_stroke_preserve (cr);
	else
		cairo_stroke (cr);
}

void
Brush::OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args)
{
	// if our transforms change in some fashion, we need to redraw
	// the element.
	NotifyListenersOfPropertyChange (Brush::ChangedProperty);
	
	DependencyObject::OnSubPropertyChanged (prop, obj, subobj_args);
}

bool
Brush::IsOpaque ()
{
	return !IS_TRANSLUCENT (GetOpacity ());
}

bool
Brush::IsAnimating ()
{
	return FALSE;
}

static void
transform_get_absolute_transform (Transform *relative_transform, double width, double height, cairo_matrix_t *result)
{
	cairo_matrix_t tm;
	
	cairo_matrix_init_scale (result, width, height);
	relative_transform->GetTransform (&tm);
	cairo_matrix_multiply (result, &tm, result);
	cairo_matrix_scale (result, 1.0/width, 1.0/height);
}



//
// SolidColorBrush
//

SolidColorBrush::SolidColorBrush ()
{
	SetObjectType (Type::SOLIDCOLORBRUSH);
}

SolidColorBrush::SolidColorBrush (const char *color)
{
	SetObjectType (Type::SOLIDCOLORBRUSH);
	Color *c = color_from_str (color);
	SetColor (c);
	delete c;
}

void
SolidColorBrush::SetupBrush (cairo_t *cr, const Rect &area)
{
	double opacity = GetOpacity ();
	Color *color = GetColor ();

	cairo_set_source_rgba (cr, color->r, color->g, color->b, opacity * color->a);
}

bool
SolidColorBrush::IsOpaque ()
{
	return Brush::IsOpaque () && !IS_TRANSLUCENT (GetColor ()->a);
}


//
// GradientBrush
//

GradientBrush::GradientBrush ()
{
	SetObjectType (Type::GRADIENTBRUSH);
}

GradientBrush::~GradientBrush ()
{
}

void
GradientBrush::OnCollectionChanged (Collection *col, CollectionChangedEventArgs *args)
{
	if (col != GetValue (GradientBrush::GradientStopsProperty)->AsCollection ()) {
		Brush::OnCollectionChanged (col, args);
		return;
	}
	
	NotifyListenersOfPropertyChange (GradientBrush::GradientStopsProperty);
}

void
GradientBrush::OnCollectionItemChanged (Collection *col, DependencyObject *obj, PropertyChangedEventArgs *args)
{
	if (col != GetValue (GradientBrush::GradientStopsProperty)->AsCollection ()) {
		Brush::OnCollectionItemChanged (col, obj, args);
		return;
	}
	
	NotifyListenersOfPropertyChange (GradientBrush::GradientStopsProperty);
}

void
GradientBrush::SetupGradient (cairo_pattern_t *pattern, const Rect &area, bool single)
{
	GradientStopCollection *children = GetGradientStops ();
	GradientSpreadMethod gsm = GetSpreadMethod ();
	double opacity = GetOpacity ();
	GradientStop *stop;
	double offset;
	int index;
	
	cairo_pattern_set_extend (pattern, convert_gradient_spread_method (gsm));
	
	// TODO - ColorInterpolationModeProperty is ignored (map to ?)
	if (single) {
		// if a single color is shown (e.g. start == end point) Cairo will,
		// by default, use the start color while SL use the end color
		index = children->GetCount () - 1;
	} else {
		index = 0;
	}
	
	GradientStop *negative_stop = NULL;	//the biggest negative stop
	double neg_offset = 0.0;		//the cached associated offset
	GradientStop *first_stop = NULL;	//the smallest positive stop
	double first_offset = 0.0;		//idem
	GradientStop *last_stop = NULL;		//the biggest stop <= 1
	double last_offset = 0.0;		//idem
	GradientStop *outofbounds_stop = NULL;	//the smallest stop > 1
	double out_offset = 0.0;		//idem
	
	for ( ; index < children->GetCount (); index++) {
		stop = children->GetValueAt (index)->AsGradientStop ();
		offset = stop->GetOffset ();
		
		if (offset >= 0.0 && offset <= 1.0) {
			Color *color = stop->GetColor ();
			
			cairo_pattern_add_color_stop_rgba (pattern, offset, color->r, color->g, color->b, color->a * opacity);
			
			if (!first_stop || (first_offset != 0.0 && offset < first_offset)) {
				first_offset = offset;
				first_stop = stop;
			}
			
			if (!last_stop || (last_offset != 1.0 && offset > last_offset)) {
				last_offset = offset;
				last_stop = stop;
			}
		} else if (offset < 0.0 && (!negative_stop || offset > neg_offset)) {
			negative_stop = stop;
			neg_offset = offset;
		} else if (offset > 1.0 && (!outofbounds_stop || offset < out_offset)) {
			outofbounds_stop = stop;
			out_offset = offset;
		}
	}
	
	if (negative_stop && first_stop && first_offset != 0.0) { //take care of the negative stop
		Color *neg_color = negative_stop->GetColor ();
		Color *first_color = first_stop->GetColor ();
		double ratio = neg_offset / (neg_offset - first_offset);
		
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, 
			neg_color->r + ratio * (first_color->r - neg_color->r),
			neg_color->g + ratio * (first_color->g - neg_color->g),
			neg_color->b + ratio * (first_color->b - neg_color->b),
			(neg_color->a + ratio * (first_color->a - neg_color->a)) * opacity);
	}
	
	if (outofbounds_stop && last_stop && last_offset != 1.0) { //take care of the >1 stop
		Color *last_color = last_stop->GetColor ();
		Color *out_color = outofbounds_stop->GetColor ();
		double ratio = (1.0 - last_offset) / (out_offset - last_offset);
		
		cairo_pattern_add_color_stop_rgba (pattern, 1.0, 
			last_color->r + ratio * (out_color->r - last_color->r),
			last_color->g + ratio * (out_color->g - last_color->g),
			last_color->b + ratio * (out_color->b - last_color->b),
			(last_color->a + ratio * (out_color->a - last_color->a)) * opacity);	
	}
	
	if (negative_stop && outofbounds_stop && !first_stop && !last_stop) { //only 2 stops, one < 0, the other > 1
		Color *neg_color = negative_stop->GetColor ();
		Color *out_color = outofbounds_stop->GetColor ();
		double ratio = neg_offset / (neg_offset - out_offset);
		
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, 
			neg_color->r + ratio * (out_color->r - neg_color->r),
			neg_color->g + ratio * (out_color->g - neg_color->g),
			neg_color->b + ratio * (out_color->b - neg_color->b),
			(neg_color->a + ratio * (out_color->a - neg_color->a)) * opacity);
		
		ratio = (1.0 - neg_offset) / (out_offset - neg_offset);
		
		cairo_pattern_add_color_stop_rgba (pattern, 1.0, 
			neg_color->r + ratio * (out_color->r - neg_color->r),
			neg_color->g + ratio * (out_color->g - neg_color->g),
			neg_color->b + ratio * (out_color->b - neg_color->b),
			(neg_color->a + ratio * (out_color->a - neg_color->a)) * opacity);	
	}
	
	if (negative_stop && !outofbounds_stop && !first_stop && !last_stop) { //only negative stops
		Color *color = negative_stop->GetColor ();
		
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, color->r, color->g, color->b, color->a * opacity);	
	}
	
	if (outofbounds_stop && !negative_stop && !first_stop && !last_stop) { //only > 1 stops
		Color *color = outofbounds_stop->GetColor ();
		
		cairo_pattern_add_color_stop_rgba (pattern, 1.0, color->r, color->g, color->b, color->a * opacity);	
	}
}

bool
GradientBrush::IsOpaque ()
{
	if (!Brush::IsOpaque ())
		return false;
	
	GradientStopCollection *stops = GetGradientStops ();
	GradientStop *stop;
	Color *c;
	
	for (int i = 0; i < stops->GetCount (); i++) {
		stop = stops->GetValueAt (i)->AsGradientStop ();
		c = stop->GetColor ();
		if (IS_TRANSLUCENT (c->a))
			return false;
	}
	
	return true;
}

//
// GradientStopCollection
//

GradientStopCollection::GradientStopCollection ()
{
	SetObjectType (Type::GRADIENTSTOP_COLLECTION);
}

GradientStopCollection::~GradientStopCollection ()
{
}


//
// GradientStop
//

GradientStop::GradientStop()
{
	SetObjectType (Type::GRADIENTSTOP);
}

GradientStop::~GradientStop()
{
}

//
// LinearGradientBrush
//

LinearGradientBrush::LinearGradientBrush ()
{
	SetObjectType (Type::LINEARGRADIENTBRUSH);
}

LinearGradientBrush::~LinearGradientBrush ()
{
}

void
LinearGradientBrush::SetupBrush (cairo_t *cr, const Rect &area)
{
	Point *start = GetStartPoint ();
	Point *end = GetEndPoint ();
	double x0, y0, x1, y1;
	cairo_matrix_t offset_matrix; 
	Point p = area.GetTopLeft ();
	
	switch (GetMappingMode ()) {
	// unknown (e.g. bad) values are considered to be Absolute to Silverlight
	// even if the default, i.e. *no* value) is RelativeToBoundingBox
	case BrushMappingModeAbsolute:
	default:
		y0 = start ? start->y : 0.0;
		x0 = start ? start->x : 0.0;
		y1 = end ? end->y : area.height;
		x1 = end ? end->x : area.width;
		break;
	case BrushMappingModeRelativeToBoundingBox:
		y0 = start ? (start->y * area.height) : 0.0;
		x0 = start ? (start->x * area.width) : 0.0;
		y1 = end ? (end->y * area.height) : area.height;
		x1 = end ? (end->x * area.width) : area.width;
		break;	
	}

	cairo_pattern_t *pattern = cairo_pattern_create_linear (x0, y0, x1, y1);
	
	cairo_matrix_t matrix;
	cairo_matrix_init_identity (&matrix);

	Transform *transform = GetTransform ();
	if (transform) {
		cairo_matrix_t tm;
		
		transform->GetTransform (&tm);
		// TODO - optimization, check for empty/identity matrix too ?
		cairo_matrix_multiply (&matrix, &matrix, &tm);
	}
	
	Transform *relative_transform = GetRelativeTransform ();
	if (relative_transform) {
		cairo_matrix_t tm;
		transform_get_absolute_transform (relative_transform, area.width, area.height, &tm);
		cairo_matrix_multiply (&matrix, &matrix, &tm);
	}

	if (p.x != 0.0 && p.y != 0.0) {
		cairo_matrix_init_translate (&offset_matrix, p.x, p.y);
		cairo_matrix_multiply (&matrix, &matrix, &offset_matrix);
	}

	cairo_matrix_invert (&matrix);
	cairo_pattern_set_matrix (pattern, &matrix);
	
	bool only_start = (x0 == x1 && y0 == y1);
	GradientBrush::SetupGradient (pattern, area, only_start);
	
	cairo_set_source (cr, pattern);
	cairo_pattern_destroy (pattern);
}

//
// RadialGradientBrush
//

RadialGradientBrush::RadialGradientBrush ()
{
	SetObjectType (Type::RADIALGRADIENTBRUSH);
}

RadialGradientBrush::~RadialGradientBrush()
{
}

void
RadialGradientBrush::SetupBrush (cairo_t *cr, const Rect &area)
{
	Point *origin = GetGradientOrigin ();
	double ox = (origin ? origin->x : 0.5);
	double oy = (origin ? origin->y : 0.5);
	cairo_matrix_t offset_matrix; 
	
	Point *center = GetCenter ();
	double cx = (center ? center->x : 0.5);
	double cy = (center ? center->y : 0.5);
	
	double rx = GetRadiusX ();
	double ry = GetRadiusY ();
	
	cairo_pattern_t *pattern = cairo_pattern_create_radial (ox/rx, oy/ry, 0.0, cx/rx, cy/ry, 1);

	cairo_matrix_t matrix;
	switch (GetMappingMode ()) {
	// unknown (e.g. bad) values are considered to be Absolute to Silverlight
	// even if the default, i.e. *no* value) is RelativeToBoundingBox
	case BrushMappingModeAbsolute:
	default:
		cairo_matrix_init_translate (&matrix, cx, cy);
		cairo_matrix_scale (&matrix, rx, ry);
		cairo_matrix_translate (&matrix, -cx/rx, -cy/ry);
		break;
	case BrushMappingModeRelativeToBoundingBox:
		cairo_matrix_init_translate (&matrix, cx * area.width, cy * area.height);
		cairo_matrix_scale (&matrix, area.width * rx, area.height * ry );
		cairo_matrix_translate (&matrix, -cx/rx, -cy/ry);
		break;
	}
	
	Transform *transform = GetTransform ();
	if (transform) {
		cairo_matrix_t tm;
		
		transform->GetTransform (&tm);
		// TODO - optimization, check for empty/identity matrix too ?
		cairo_matrix_multiply (&matrix, &matrix, &tm);
	}
	
	Transform *relative_transform = GetRelativeTransform ();
	if (relative_transform) {
		cairo_matrix_t tm;
		transform_get_absolute_transform (relative_transform, area.width, area.height, &tm);
		// TODO - optimization, check for empty/identity matrix too ?
		cairo_matrix_multiply (&matrix, &matrix, &tm);
	}

	if (area.x != 0.0 || area.y != 0.0) {
		cairo_matrix_init_translate (&offset_matrix, area.x, area.y);
		cairo_matrix_multiply (&matrix, &matrix, &offset_matrix);
	}

	cairo_status_t status = cairo_matrix_invert (&matrix);
	if (status != CAIRO_STATUS_SUCCESS) {
		printf ("Moonlight: Error inverting matrix falling back\n");
		cairo_matrix_init_identity (&matrix);
	}
	
	cairo_pattern_set_matrix (pattern, &matrix);
	GradientBrush::SetupGradient (pattern, area);
	
	cairo_set_source (cr, pattern);
	cairo_pattern_destroy (pattern);
}

//
// ImageBrush
//

void
ImageBrush::image_progress_changed (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	ImageBrush *brush = (ImageBrush*)closure;
	double progress = brush->image->GetDownloadProgress ();
	
	brush->SetDownloadProgress (progress);
	
	brush->Emit (ImageBrush::DownloadProgressChangedEvent);
}

void
ImageBrush::image_failed (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	((ImageBrush*)closure)->Emit (ImageBrush::ImageFailedEvent, calldata);
}

ImageBrush::ImageBrush ()
{
	SetObjectType (Type::IMAGEBRUSH);

	image = new Image ();

	image->AddHandler (MediaBase::DownloadProgressChangedEvent, image_progress_changed, this);
	image->AddHandler (Image::ImageFailedEvent, image_failed, this);

	image->brush = this;
	
	loaded_count = 0;
}

ImageBrush::~ImageBrush ()
{
	image->brush = NULL;
	image->unref ();
}

void
ImageBrush::SetSource (Downloader *downloader, const char *PartName)
{
	image->SetSource (downloader, PartName);
}

void
ImageBrush::SetSurface (Surface *surface)
{
	if (GetSurface() == surface)
		return;
	
	image->SetSurface (surface);
	
	DependencyObject::SetSurface (surface);
}

void
ImageBrush::TargetLoaded ()
{
	if (++loaded_count == 1)
		image->SetAllowDownloads (true);
}

void
ImageBrush::target_loaded (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	((ImageBrush *) closure)->TargetLoaded ();
}

void
ImageBrush::TargetUnloaded ()
{
	if (--loaded_count == 0)
		image->SetAllowDownloads (false);
}

void
ImageBrush::target_unloaded (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	((ImageBrush *) closure)->TargetUnloaded ();
}

void
ImageBrush::AddTarget (DependencyObject *obj)
{
	if (!obj->Is (Type::UIELEMENT))
		return;
	
	if (((UIElement *) obj)->IsLoaded ())
		TargetLoaded ();
	
	obj->AddHandler (UIElement::UnloadedEvent, target_unloaded, this);
	obj->AddHandler (UIElement::LoadedEvent, target_loaded, this);
}

void
ImageBrush::RemoveTarget (DependencyObject *obj)
{
	if (!obj->Is (Type::UIELEMENT))
		return;
	
	if (((UIElement *) obj)->IsLoaded ())
		TargetUnloaded ();
	
	obj->RemoveHandler (UIElement::UnloadedEvent, target_unloaded, this);
	obj->RemoveHandler (UIElement::LoadedEvent, target_loaded, this);
}

void
ImageBrush::OnPropertyChanged (PropertyChangedEventArgs *args, MoonError *error)
{
	if (args->GetProperty ()->GetOwnerType() != Type::IMAGEBRUSH) {
		TileBrush::OnPropertyChanged (args, error);
		return;
	}

	if (args->GetId () == ImageBrush::DownloadProgressProperty) {
		image->SetValue (Image::DownloadProgressProperty, args->new_value);
	}
	else if (args->GetId () == ImageBrush::ImageSourceProperty) {
		image->SetValue (Image::SourceProperty, args->new_value);
	}

	NotifyListenersOfPropertyChange (args);
}

bool
ImageBrush::IsOpaque ()
{
	// XXX punt for now and return false here.
	return false;
}

cairo_surface_t *
image_brush_create_similar (cairo_t *cairo, int width, int height)
{
#if USE_OPT_IMAGE_ONLY
	return cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
#else
	return cairo_surface_create_similar (cairo_get_target (cairo),
					     CAIRO_CONTENT_COLOR_ALPHA,
					     width,
					     height);
#endif
}

void
image_brush_compute_pattern_matrix (cairo_matrix_t *matrix, double width, double height, int sw, int sh, 
	Stretch stretch, AlignmentX align_x, AlignmentY align_y, Transform *transform, Transform *relative_transform)
{
	// scale required to "fit" for both axes
	double sx = sw / width;
	double sy = sh / height;

	// Fill is the simplest case because AlignementX and AlignmentY don't matter in this case
	if (stretch == StretchFill) {
		// fill extents in both axes
		cairo_matrix_init_scale (matrix, sx, sy);
	} else {
		double scale = 1.0;
		double dx = 0.0;
		double dy = 0.0;

		switch (stretch) {
		case StretchUniform:
			// fill without cuting the image, center the other axes
			scale = (sx < sy) ? sy : sx;
			break;
		case StretchUniformToFill:
			// fill by, potentially, cuting the image on one axe, center on both axes
			scale = (sx < sy) ? sx : sy;
			break;
		case StretchNone:
			break;
		default:
			g_warning ("Invalid Stretch value (%d).", stretch);
			break;
		}

		switch (align_x) {
		case AlignmentXLeft:
			dx = 0.0;
			break;
		case AlignmentXCenter:
			dx = (sw - (scale * width)) / 2;
			break;
		// Silverlight+Javascript default to AlignmentXRight for (some) invalid values (others results in an alert)
		case AlignmentXRight:
		default:
			dx = (sw - (scale * width));
			break;
		}

		switch (align_y) {
		case AlignmentYTop:
			dy = 0.0;
			break;
		case AlignmentYCenter:
			dy = (sh - (scale * height)) / 2;
			break;
		// Silverlight+Javascript default to AlignmentXBottom for (some) invalid values (others results in an alert)
		case AlignmentYBottom:
		default:
			dy = (sh - (scale * height));
			break;
		}

		if (stretch == StretchNone) {
			// no strech, no scale
			cairo_matrix_init_translate (matrix, dx, dy);
		} else {
			// otherwise there's both a scale and translation to be done
			cairo_matrix_init (matrix, scale, 0, 0, scale, dx, dy);
		}
	}

	if (transform || relative_transform) {
		if (transform) {
			cairo_matrix_t tm;
			
			transform->GetTransform (&tm);
			cairo_matrix_invert (&tm);
			cairo_matrix_multiply (matrix, &tm, matrix);
		}
		
		if (relative_transform) {
			cairo_matrix_t tm;
			
			transform_get_absolute_transform (relative_transform, width, height, &tm);
			cairo_matrix_invert (&tm);
			cairo_matrix_multiply (matrix, &tm, matrix);
		}
	}
}

static bool
is_stretch_valid (Stretch stretch)
{
	switch (stretch) {
	case StretchNone:
	case StretchFill:
	case StretchUniform:
	case StretchUniformToFill:
		return true;
	default:
		return false;
	}
}

void
ImageBrush::SetupBrush (cairo_t *cr, const Rect &area)
{
	cairo_surface_t *surface = image->GetCairoSurface ();

	Stretch stretch = GetStretch ();

	if (!surface || !is_stretch_valid (stretch)) {
		// not yet available, draw nothing (!surface) or a bad enum value for stretch
		// XXX Removing this _source_set at all?
		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.0);
		return;
	}
	
	AlignmentX ax = GetAlignmentX ();
	AlignmentY ay = GetAlignmentY ();
	
	Transform *transform = GetTransform ();
	Transform *relative_transform = GetRelativeTransform ();
	cairo_matrix_t matrix;

	cairo_pattern_t *pattern = cairo_pattern_create_for_surface (surface);

	image_brush_compute_pattern_matrix (&matrix, area.width, area.height, image->GetImageWidth (), image->GetImageHeight (), stretch, ax, ay, transform, relative_transform);
	cairo_matrix_translate (&matrix, -area.x, -area.y);
	cairo_pattern_set_matrix (pattern, &matrix);
	
	cairo_set_source (cr, pattern);
	cairo_pattern_destroy (pattern);
}

//
// TileBrush
//

TileBrush::TileBrush ()
{
	SetObjectType (Type::TILEBRUSH);
}

TileBrush::~TileBrush ()
{
}

void
TileBrush::Fill (cairo_t *cr, bool preserve)
{
	double opacity = GetOpacity ();

	if (IS_INVISIBLE (opacity)) {
		if (!preserve)
			cairo_new_path (cr);
		return;
	}

	if (!IS_TRANSLUCENT (opacity)) {
		Brush::Fill (cr, preserve);
		return;
	}
		
	cairo_save (cr);
	cairo_clip (cr);
	cairo_paint_with_alpha (cr, opacity);
	cairo_restore (cr);
	
	if (!preserve)
		cairo_new_path (cr);
}

void
TileBrush::Stroke (cairo_t *cr, bool preserve)
{
	double opacity = GetOpacity ();

	if (IS_INVISIBLE (opacity)) {
		if (!preserve)
			cairo_new_path (cr);
		return;
	}

	if (!IS_TRANSLUCENT (opacity)) {
		Brush::Stroke (cr, preserve);
		return;
	}
		
	cairo_save (cr);
	cairo_push_group_with_content (cr, CAIRO_CONTENT_ALPHA);
	cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, opacity);
	cairo_stroke (cr);

	cairo_pattern_t *mask = cairo_pop_group (cr);
	cairo_restore (cr);
	cairo_mask (cr, mask);
	cairo_pattern_destroy (mask);

	if (!preserve)
		cairo_new_path (cr);
}

//
// VideoBrush
//

VideoBrush::VideoBrush ()
{
	SetObjectType (Type::VIDEOBRUSH);
	media = NULL;
}

VideoBrush::~VideoBrush ()
{
	if (media != NULL) {
		media->RemovePropertyChangeListener (this);
		media->unref ();
	}
}

void
VideoBrush::SetupBrush (cairo_t *cr, const Rect &area)
{
	Stretch stretch = GetStretch ();
	if (!is_stretch_valid (stretch)) {
		// bad enum value for stretch, nothing should be drawn
		// XXX Removing this _source_set at all?
		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.0);
		return;
	}

	MediaPlayer *mplayer = media ? media->GetMediaPlayer () : NULL;
	Transform *transform = GetTransform ();
	Transform *relative_transform = GetRelativeTransform ();
	AlignmentX ax = GetAlignmentX ();
	AlignmentY ay = GetAlignmentY ();
	cairo_surface_t *surface;
	cairo_pattern_t *pattern;
	cairo_matrix_t matrix;
	
	if (media == NULL) {
		DependencyObject *obj;
		const char *name;
		
		name = GetSourceName ();
		
		if (name == NULL || *name == '\0')
			return;
		
		if ((obj = FindName (name)) && obj->Is (Type::MEDIAELEMENT)) {
			obj->AddPropertyChangeListener (this);
			media = (MediaElement *) obj;
			mplayer = media->GetMediaPlayer ();
			obj->ref ();
		} else if (obj == NULL) {
			printf ("could not find element `%s'\n", name);
		} else {
			printf ("obj %p is not of type MediaElement (it is %s)\n", obj,
				obj->GetTypeName ());
		}
	}
	
	if (!mplayer || !(surface = mplayer->GetCairoSurface ())) {
		// not yet available, draw gray-ish shadow where the brush should be applied
		cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.5);
		return;
	}
	
	pattern = cairo_pattern_create_for_surface (surface);
	cairo_pattern_set_filter (pattern, CAIRO_FILTER_FAST);

	image_brush_compute_pattern_matrix (&matrix, area.width, area.height, mplayer->GetVideoWidth (),
					    mplayer->GetVideoHeight (), stretch, ax, ay,
					    transform, relative_transform);
	
	cairo_matrix_translate (&matrix, -area.x, -area.y);
	cairo_pattern_set_matrix (pattern, &matrix);
	
	cairo_set_source (cr, pattern);
	cairo_pattern_destroy (pattern);
}

void
VideoBrush::OnPropertyChanged (PropertyChangedEventArgs *args, MoonError *error)
{
	if (args->GetProperty ()->GetOwnerType() != Type::VIDEOBRUSH) {
		TileBrush::OnPropertyChanged (args, error);
		return;
	}

	if (args->GetId () == VideoBrush::SourceNameProperty) {
		char *name = args->new_value ? args->new_value->AsString () : NULL;
		DependencyObject *obj;
		
		if (media != NULL) {
			media->RemovePropertyChangeListener (this);
			media->unref ();
			media = NULL;
		}
		
		if (name && (obj = FindName (name)) && obj->Is (Type::MEDIAELEMENT)) {
			obj->AddPropertyChangeListener (this);
			media = (MediaElement *) obj;
			obj->ref ();
		} else {
			// Note: This may have failed because the parser hasn't set the
			// toplevel element yet, we'll try again in SetupBrush()
		}
	}

	NotifyListenersOfPropertyChange (args);
}

void
VideoBrush::OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args)
{
	/* this is being handled in the base class */
	/*
	if (subobj_args->GetId () == MediaElement::PositionProperty) {
		// We to changes in this MediaElement property so we
		// can notify whoever is using us to paint that they
		// need to redraw themselves.
		NotifyListenersOfPropertyChange (Brush::ChangedProperty);
	}
	*/

	TileBrush::OnSubPropertyChanged (prop, obj, subobj_args);
}

void
VideoBrush::SetSource (MediaElement *source)
{
	if (source)
		source->ref ();
	
	SetSourceName ("");
	
	if (media != NULL) {
		media->RemovePropertyChangeListener (this);
		media->unref ();
		media = NULL;
	}
	
	media = source;
}

bool
VideoBrush::IsOpaque ()
{
	// XXX punt for now and return false here.
	return false;
}

bool
VideoBrush::IsAnimating ()
{
	if (media && media->IsPlaying ())
		return true;

	return TileBrush::IsAnimating ();
}

//
// VisualBrush
//

VisualBrush::VisualBrush ()
{
	SetObjectType (Type::VISUALBRUSH);
}

VisualBrush::~VisualBrush ()
{
}

void
VisualBrush::SetupBrush (cairo_t *cr, const Rect &area)
{
	UIElement *ui = (UIElement *) GetVisual ();
	if (!ui) {
		// not yet available, draw gray-ish shadow where the brush should be applied
		cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.5);
		return;
	}
	
	// XXX we should cache the surface so that it can be
	// used multiple times without having to re-render each time.
	Rect bounds = ui->GetSubtreeBounds().RoundOut ();
	
	surface = image_brush_create_similar (cr, (int) bounds.width, (int) bounds.height);
	
	cairo_t *surface_cr = cairo_create (surface);
	Region region = Region (0, 0, bounds.width, bounds.height);
	ui->Render (surface_cr, &region);
	cairo_destroy (surface_cr);
	
	Stretch stretch = GetStretch ();
	
	AlignmentX ax = GetAlignmentX ();
	AlignmentY ay = GetAlignmentY ();
	
	Transform *transform = GetTransform ();
	Transform *relative_transform = GetRelativeTransform ();
	
 	cairo_pattern_t *pattern = cairo_pattern_create_for_surface (surface);
	cairo_matrix_t matrix;
 	image_brush_compute_pattern_matrix (&matrix, area.width, area.height,
					    (int) bounds.width, (int) bounds.height,
					    stretch, ax, ay, transform, relative_transform);
	
	cairo_matrix_translate (&matrix, -area.x, -area.y);
 	cairo_pattern_set_matrix (pattern, &matrix);

 	cairo_set_source (cr, pattern);
 	cairo_pattern_destroy (pattern);

	cairo_surface_destroy (surface);
}

void
VisualBrush::update_brush (EventObject *, EventArgs *, gpointer closure)
{
	VisualBrush *b = (VisualBrush*)closure;
	b->NotifyListenersOfPropertyChange (Brush::ChangedProperty);
}

void
VisualBrush::OnPropertyChanged (PropertyChangedEventArgs *args, MoonError *error)
{
	if (args->GetProperty ()->GetOwnerType() != Type::VISUALBRUSH) {
		TileBrush::OnPropertyChanged (args, error);
		return;
	}

	if (args->GetId () == VisualBrush::VisualProperty) {
		// XXX we really need a way to disconnect from the preview visual
		UIElement *v = args->new_value->AsUIElement();
		v->AddHandler (((UIElement*)v)->InvalidatedEvent, update_brush, this);
	}

	NotifyListenersOfPropertyChange (args);
}

bool
VisualBrush::IsOpaque ()
{
	// XXX punt for now and return false here.
	return false;
}

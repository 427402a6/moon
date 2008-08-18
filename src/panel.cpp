/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * panel.cpp
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#include <config.h>

#include "geometry.h"
#include "panel.h"
#include "brush.h"
#include "math.h"
#include "collection.h"
#include "runtime.h"

Panel::Panel ()
{
	SetValue (Panel::ChildrenProperty, Value::CreateUnref (new UIElementCollection ()));
	mouse_over = NULL;
	emitting_loaded = false;
}

Panel::~Panel()
{
}

void
Panel::Dispose ()
{
	UIElementCollection *children = GetChildren();
	for (int i = 0; i < children->GetCount (); i++)
		children->GetValueAt (i)->AsUIElement ()->SetVisualParent (NULL);

	FrameworkElement::Dispose ();
}

Brush *
Panel::GetBackground ()
{
	Value *value = GetValue (Panel::BackgroundProperty);
	
	return value ? value->AsBrush () : NULL;
}

void
Panel::SetBackground (Brush *background)
{
	SetValue (Panel::BackgroundProperty, Value (background));
}

UIElementCollection *
Panel::GetChildren ()
{
	Value *value = GetValue (Panel::ChildrenProperty);
	
	return value ? value->AsUIElementCollection () : NULL;
}

void
Panel::SetChildren (UIElementCollection *children)
{
	SetValue (Panel::ChildrenProperty, children);
}

void
Panel::SetSurface (Surface *s)
{
	UIElementCollection *children;
	UIElement *item;
	
	FrameworkElement::SetSurface (s);
	
	if (!(children = GetChildren ()))
		return;
	
	for (int i = 0; i < children->GetCount (); i++) {
		item = children->GetValueAt (i)->AsUIElement ();
		item->SetSurface (s);
	}
}

void
Panel::AddChild (UIElement *item)
{
	GetChildren ()->Add (item);
}

#define DEBUG_BOUNDS 0
#define CAIRO_CLIP 0

#if DEBUG_BOUNDS
static void space (int n)
{
	for (int i = 0; i < n; i++)
		putchar (' ');
}
static int levelb = 0;
#endif

void
Panel::ComputeBounds ()
{
	UIElementCollection *children = GetChildren ();
	
#if DEBUG_BOUNDS
	levelb += 4;
	space (levelb);
	printf ("Panel: Enter ComputeBounds (%s)\n", GetName());
#endif
	if (children != NULL) {
		for (int i = 0; i < children->GetCount (); i++) {
			UIElement *item = children->GetValueAt (i)->AsUIElement ();
			
			// if the item isn't drawn, skip it
			if (!item->GetRenderVisible ())
				continue;
			
			Rect r = item->GetSubtreeBounds ();
			
			r = IntersectBoundsWithClipPath (r, true);
#if DEBUG_BOUNDS
			space (levelb + 4);
			printf ("Item (%s, %s) bounds %g %g %g %g\n", item->GetName (), item->GetTypeName ()
				r.x, r.y, r.w, r.h);
#endif
			if (i > 0)
				bounds_with_children = bounds_with_children.Union (r);
			else
				bounds_with_children = r;
		}
		bounds_with_children = IntersectBoundsWithClipPath (bounds_with_children, true);
	} else {
		bounds_with_children = Rect (0,0,0,0);
	}

	Value *value = GetValue (Panel::BackgroundProperty);
	if (value) {
		FrameworkElement::ComputeBounds ();
		bounds_with_children = bounds_with_children.Union (bounds);
	} else
		bounds = Rect (0,0,0,0);

#if DEBUG_BOUNDS
	space (levelb);
	printf ("Panel: Leave ComputeBounds (%g %g %g %g)\n", bounds.x, bounds.y, bounds.w, bounds.h);
	space (levelb);
	printf ("Panel: Leave ComputeBounds (%g %g %g %g)\n", bounds_with_children.x, bounds_with_children.y, bounds_with_children.w, bounds_with_children.h);
	levelb -= 4;
#endif
}

void
Panel::ShiftPosition (Point p)
{
	double dx = p.x - bounds.x;
	double dy = p.y - bounds.y;

	// need to do this after computing the delta
	FrameworkElement::ShiftPosition (p);

	bounds_with_children.x += dx;
	bounds_with_children.y += dy;
}

//#define DEBUG_INVALIDATE 1

void
Panel::UpdateTotalRenderVisibility ()
{
	FrameworkElement::UpdateTotalRenderVisibility ();
}

void
Panel::UpdateTotalHitTestVisibility ()
{
#if 1
	// this really shouldn't need to be here, but our dirty code is broken
	UIElementCollection *children = GetChildren ();
	
	for (int i = 0; i < children->GetCount (); i++) {
		UIElement *item = children->GetValueAt (i)->AsUIElement ();
		item->UpdateTotalHitTestVisibility ();
	}
#endif
	
	FrameworkElement::UpdateTotalHitTestVisibility ();
}

bool
Panel::UseBackToFront ()
{
	Collection *children;
	
	if (!(children = GetChildren ()))
		return true;
	
	return children->GetCount () < 25;
}

void
Panel::Render (cairo_t *cr, Region *region)
{
	Brush *background;
	
	cairo_set_matrix (cr, &absolute_xform);
	
	if ((background = GetBackground ())) {
		double fheight = GetHeight ();
		double fwidth = GetWidth ();
		
		if (fwidth > 0 && fheight > 0) {
			background->SetupBrush (cr, this);
			
			// FIXME - UIElement::Opacity may play a role here
			cairo_new_path (cr);
			cairo_rectangle (cr, 0, 0, fwidth, fheight);
			cairo_fill (cr);
		}
	}
}

void
Panel::PostRender (cairo_t *cr, Region *region, bool front_to_back)
{
	// if we didn't render front to back, then render the children here
	if (!front_to_back || !UseBackToFront ()) {
		RenderChildren (cr, region);
	}

	UIElement::PostRender (cr, region, front_to_back);
}


void
Panel::RenderChildren (cairo_t *cr, Region *parent_region)
{
	UIElementCollection *children = GetChildren ();
	
	Region *clipped_region = new Region (bounds_with_children);
	clipped_region->Intersect (parent_region);

	cairo_identity_matrix (cr);
	for (guint i = 0; i < children->z_sorted->len; i++) {
		UIElement *item = (UIElement *) children->z_sorted->pdata[i];
		
		Region *region = new Region (item->GetSubtreeBounds());
		region->Intersect (clipped_region);

		if (!item->GetRenderVisible ()
		    || region->IsEmpty()) {
#ifdef DEBUG_INVALIDATE
			printf ("skipping offscreen object %s: %p (%s)\n", item->GetName (), item, item->GetTypeName());
#endif
			delete region;
			continue;
		}
		
#if CAIRO_CLIP
#if TIME_CLIP
		STARTTIMER(clip, "cairo clip setup");
#endif
		cairo_save (cr);
		
		//printf ("Clipping to %g %g %g %g\n", inter.x, inter.y, inter.w, inter.h);
		// at the very least we need to clip based on the expose area.
		// there's also a UIElement::ClipProperty
		
		region->Draw (cr);
		cairo_clip (cr);
#if TIME_CLIP
		ENDTIMER(clip, "cairo clip setup");
#endif
#endif
		// 			space (levelb);
		// 			printf ("%p %s (%s), bounds = %g %g %g %g, inter = %g %g %g %g\n",
		// 				item, item->GetTypeName(), item->GetName(),
		// 				item->GetBounds().x, item->GetBounds().y, item->GetBounds().w, item->GetBounds().h,
		// 				inter.x, inter.y, inter.w, inter.h);
		
		item->DoRender (cr, region);

#if CAIRO_CLIP
#if TIME_CLIP
		STARTTIMER(endclip, "cairo clip teardown");
#endif			
		cairo_restore (cr);
		
#if TIME_CLIP
		ENDTIMER(endclip, "cairo clip teardown");
#endif
#endif
		delete region;
	}

	delete clipped_region;
}

void
Panel::FrontToBack (Region *surface_region, List *render_list)
{
	double local_opacity = GetValue (OpacityProperty)->AsDouble();

	if (surface_region->RectIn (bounds_with_children.RoundOut()) == GDK_OVERLAP_RECTANGLE_OUT)
		return;

	if (!GetRenderVisible ()
	    || IS_INVISIBLE (local_opacity))
		return;

	if (!UseBackToFront ()) {
		Region *self_region = new Region (surface_region);
		self_region->Intersect (bounds_with_children.RoundOut());
		// we need to include our children in this one, since
		// we'll be rendering them in the PostRender method.
		if (!self_region->IsEmpty())
			render_list->Prepend (new RenderNode (this, self_region, !self_region->IsEmpty(),
							      UIElement::CallPreRender, UIElement::CallPostRender));
		// don't remove the region from surface_region because
		// there are likely holes in it
		return;
	}

	Region *region;
	bool delete_region;
	bool can_subtract_self;
	Value *value;
	
	if ((GetValue (UIElement::ClipProperty) == NULL)
	    && (!(value = GetValue (UIElement::OpacityMaskProperty)) || value->AsBrush () == NULL)
	    && !IS_TRANSLUCENT (GetValue (UIElement::OpacityProperty)->AsDouble ())) {
		region = surface_region;
		delete_region = false;
		can_subtract_self = true;
	}
	else {
		region = new Region (surface_region);
		delete_region = true;
		can_subtract_self = false;
	}

	RenderNode *panel_cleanup_node = new RenderNode (this, NULL, false, NULL, UIElement::CallPostRender);
	
	render_list->Prepend (panel_cleanup_node);

	Region *self_region = new Region (region);

	UIElementCollection *children = GetChildren ();
	for (guint i = children->z_sorted->len; i > 0; i--) {
		UIElement *item = (UIElement *) children->z_sorted->pdata[i - 1];

		item->FrontToBack (region, render_list);
	}

	if (!GetOpacityMask () && !IS_TRANSLUCENT (local_opacity)) {
		delete self_region;
		if (GetValue (Panel::BackgroundProperty) == NULL)
			self_region = new Region ();
		else {
			self_region = new Region (region);
			self_region->Intersect (GetRenderBounds().RoundOut ()); // note the RoundOut
		}
	} else {
		self_region->Intersect (GetSubtreeBounds().RoundOut ()); // note the RoundOut
	}

	if (self_region->IsEmpty() && render_list->First() == panel_cleanup_node) {
		/* we don't intersect the surface region, and none of
		   our children did either, remove the cleanup node */
		render_list->Remove (render_list->First());
		delete self_region;
		if (delete_region)
			delete region;
		return;
	}

	render_list->Prepend (new RenderNode (this, self_region, !self_region->IsEmpty(), UIElement::CallPreRender, NULL));

	if (!self_region->IsEmpty()) {
		bool subtract = ((absolute_xform.yx == 0 && absolute_xform.xy == 0) /* no skew/rotation */
				 && can_subtract_self);

		if (subtract) {
			Brush *background = GetBackground ();
			
			if (background)
				subtract = background->IsOpaque ();
			else
				subtract = false;
		}

 		if (subtract)
			region->Subtract (bounds);
	}

	if (delete_region)
		delete region;
}

void
Panel::CacheInvalidateHint (void)
{
	UIElementCollection *children = GetChildren ();
	
	if (!children)
		return;
	
	for (int i = 0; i < children->GetCount (); i++) {
		UIElement *item = children->GetValueAt (i)->AsUIElement ();
		item->CacheInvalidateHint ();
	}
}

bool
Panel::InsideObject (cairo_t *cr, double x, double y)
{
	bool is_inside_clip = InsideClip (cr, x, y);
	if (!is_inside_clip)
		return false;
	
	/* if we have explicitly set width/height, we check them */
	if (FrameworkElement::InsideObject (cr, x, y)) {
		/* we're inside, check if we're actually painting any background,
		   or, we're just transparent due to no painting. */
		if (GetBackground ())
			return true;
	}
	
	UIElement *mouseover = FindMouseOver (cr, x, y);

	return mouseover != NULL;
}

bool
Panel::CheckOver (cairo_t *cr, UIElement *item, double x, double y)
{
	// if the item isn't visible, it's really easy
	if (!item->GetRenderVisible ())
		return false;

	// if the item doesn't take part in hit testing, it's also easy
	if (!item->GetHitTestVisible ())
		return false;

	// first a quick bounds check
	if (!item->GetSubtreeBounds().PointInside (x, y))
		return false;

	// then, if that passes, a more tailored shape check
	return item->InsideObject (cr, x, y);
}

UIElement *
Panel::FindMouseOver (cairo_t *cr, double x, double y)
{
	UIElementCollection *children = GetChildren ();
	
	// Walk the list in reverse order, since it's sorted in ascending z-index order
	//
	for (guint i = children->z_sorted->len; i > 0; i--) {
		UIElement *item = (UIElement *) children->z_sorted->pdata[i - 1];

		if (CheckOver (cr, item, x, y)) {
			return item;
		}
	}

	return NULL;
}

void
Panel::HitTest (cairo_t *cr, Point p, List *uielement_list)
{
	/* in the interests of not calling FindMouseOver twice, this method
	   cut & pastes from the bodies of both Panel::InsideObject and
	   Panel::FindMouseOver */

	UIElement *mouseover = FindMouseOver (cr, p.x, p.y);

	if (mouseover) {
		uielement_list->Prepend (new UIElementNode (this));
		mouseover->HitTest (cr, p, uielement_list);
	}
	else {
		bool is_inside_clip = InsideClip (cr, p.x, p.y);
		if (!is_inside_clip)
			return;
	
		/* if we have explicitly set width/height, we check them */
		if (FrameworkElement::InsideObject (cr, p.x, p.y)) {
			/* we're inside, check if we're actually painting any background,
			   or, we're just transparent due to no painting. */
			if (GetBackground ())
				uielement_list->Prepend (new UIElementNode (this));
		}
	}
}

void
Panel::HitTest (cairo_t *cr, Rect r, List *uielement_list)
{
}

//
// Intercept any changes to the children property and mirror that into our
// own variable
//
void
Panel::OnPropertyChanged (PropertyChangedEventArgs *args)
{
	if (args->property->GetOwnerType() != Type::PANEL) {
		FrameworkElement::OnPropertyChanged (args);
		return;
	}

	if (args->property == Panel::BackgroundProperty)
		Invalidate ();

	if (args->property == Panel::ChildrenProperty) {
		Collection *collection;
		
		if (args->old_value) {
			collection = args->old_value->AsCollection ();
			for (int i = 0; i < collection->GetCount (); i++)
				ChildRemoved (collection->GetValueAt (i)->AsUIElement());
		}
		
		if (args->new_value) {
			collection = args->new_value->AsCollection ();
			for (int i = 0; i < collection->GetCount (); i++)
				ChildAdded (collection->GetValueAt (i)->AsUIElement ());
			
			if (GetSurface ()) {
				// queue a resort based on ZIndex
				GetSurface ()->AddDirtyElement (this, DirtyChildrenZIndices);
			}
		}

		UpdateBounds();
	}

	NotifyListenersOfPropertyChange (args);
}

void
Panel::ChildAdded (UIElement *child)
{
	UIElement *item = (UIElement *) child;

	item->SetVisualParent (this);
	item->UpdateTransform ();
	item->UpdateTotalRenderVisibility ();
	item->UpdateTotalHitTestVisibility ();
	item->Invalidate ();

	if (flags & UIElement::IS_LOADED)
		item->OnLoaded ();
			
	if (GetSurface ()) {
		// queue a resort based on ZIndex
		GetSurface ()->AddDirtyElement (this, DirtyChildrenZIndices);
	}

	UpdateBounds (true);
}

void
Panel::ChildRemoved (UIElement *child)
{
	UIElement *item = (UIElement *) child;

	Invalidate (item->GetSubtreeBounds());

	item->CacheInvalidateHint ();
	item->SetVisualParent (NULL);
	item->flags &= ~UIElement::IS_LOADED;

	if (GetSurface ()) {
		// queue a resort based on ZIndex
		GetSurface ()->AddDirtyElement (this, DirtyChildrenZIndices);
	}

	UpdateBounds (true);
}

void
Panel::OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args)
{
	if (prop == Panel::BackgroundProperty) {
		Invalidate ();
	}
	else {
		FrameworkElement::OnSubPropertyChanged (prop, obj, subobj_args);
	}
}

void
Panel::OnCollectionChanged (Collection *col, CollectionChangedEventArgs *args)
{
	if (col == GetValue (Panel::ChildrenProperty)->AsCollection ()) {
		switch (args->action) {
		case CollectionChangedActionReplace:
			ChildRemoved (args->old_value->AsUIElement ());
			// now fall thru to Add
		case CollectionChangedActionAdd:
			ChildAdded (args->new_value->AsUIElement ());
			break;
		case CollectionChangedActionRemove:
			ChildRemoved (args->old_value->AsUIElement ());
			break;
		case CollectionChangedActionClearing:
			for (int i = 0; i < col->GetCount (); i++)
				ChildRemoved (col->GetValueAt (i)->AsUIElement ());
			break;
		case CollectionChangedActionCleared:
			// nothing needed here.
			break;
		}
	} else {
		FrameworkElement::OnCollectionChanged (col, args);
	}
}

void
Panel::OnCollectionItemChanged (Collection *col, DependencyObject *obj, PropertyChangedEventArgs *args)
{
	if (col == GetValue (Panel::ChildrenProperty)->AsCollection ()) {
		// if a child changes its ZIndex property we need to resort our Children
		if (args->property == UIElement::ZIndexProperty) {
			((UIElement *) obj)->Invalidate ();
			if (GetSurface ()) {
				// queue a resort based on ZIndex
				GetSurface ()->AddDirtyElement (this, DirtyChildrenZIndices);
			}
		}
	} else {
		FrameworkElement::OnCollectionItemChanged (col, obj, args);
	}
}

void
Panel::OnLoaded ()
{
 	if (emitting_loaded)
 		return;
	
 	emitting_loaded = true;
	
	flags |= UIElement::IS_LOADED;
	
	UIElementCollection *children = GetChildren ();
	
	for (int i = 0; i < children->GetCount (); i++) {
		UIElement *item = children->GetValueAt (i)->AsUIElement ();

		item->OnLoaded ();
	}
	
	FrameworkElement::OnLoaded ();
	
	if (GetSurface ()) {
		// queue a resort based on ZIndex
		GetSurface ()->AddDirtyElement (this, DirtyChildrenZIndices);
	}
	
 	emitting_loaded = false;
}

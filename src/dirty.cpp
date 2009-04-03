/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * dirty.cpp: 
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

#include <glib.h>

#include "canvas.h"
#include "uielement.h"
#include "frameworkelement.h"
#include "panel.h"
#include "control.h"
#include "runtime.h"
#include "clock.h"
#include "dirty.h"
#include "list.h"

class DirtyNode : public List::Node {
public:
	DirtyNode (UIElement *element) 
	{
		this->element = element;
	}
	UIElement *element;
};

class DirtyList : public List::Node
{
public:
	DirtyList (int level)
	{
		this->level = level;
		dirty_list = new List();
	}

	~DirtyList ()
	{
		delete dirty_list;
	}

	List* GetDirtyNodes ()
	{
		return dirty_list;
	}

	int GetLevel ()
	{
		return level;
	}
private:
	List *dirty_list;
	int level;
};

DirtyLists::DirtyLists (bool ascending)
{
	this->ascending = ascending;
	this->lists = new List();
}

DirtyLists::~DirtyLists ()
{
	delete lists;
}

DirtyList*
DirtyLists::GetList (int level, bool create)
{
	DirtyList *dl;

	for (dl = (DirtyList*)lists->First(); dl; dl = (DirtyList*)dl->next) {
		if (dl->GetLevel() == level)
			return dl;
		else if (dl->GetLevel() > level)
			break;
	}

	if (create) {
		DirtyList *new_dl = new DirtyList (level);
		lists->InsertBefore (new_dl, dl);
		return new_dl;
	}

	return NULL;
}

void
DirtyLists::RemoveList (int level)
{
	DirtyList *dl = (DirtyList*)GetList (level, false);
	if (!dl)
		return;
	lists->Remove (dl);
}

void
DirtyLists::AddDirtyNode (int level, List::Node *node)
{
	DirtyList *dl = (DirtyList*)GetList (level, true);
	dl->GetDirtyNodes()->Append(node);
}

void
DirtyLists::RemoveDirtyNode (int level, List::Node *node)
{
	DirtyList *dl = (DirtyList*)GetList (level, false);
	if (!dl)
		return;
	dl->GetDirtyNodes()->Remove(node);
	if (dl->GetDirtyNodes()->IsEmpty())
		lists->Remove (dl);
}

List::Node*
DirtyLists::GetFirst ()
{
	DirtyList *dl;

	if (ascending) {
		dl = (DirtyList*)lists->First();
	}
	else {
		dl = (DirtyList*)lists->Last ();
	}

	if (!dl)
		return NULL;

	return dl->GetDirtyNodes()->First();
}

bool
DirtyLists::IsEmpty ()
{
	return lists->IsEmpty();
}

void
DirtyLists::Clear (bool freeNodes)
{
	lists->Clear (freeNodes);
}

bool
Surface::IsAnythingDirty ()
{
	//return !down_dirty->IsEmpty() || !up_dirty->IsEmpty() || toplevel->dirty_flags & (DirtyMeasure | DirtyArrange);
	return !measure_dirty->IsEmpty () || !arrange_dirty->IsEmpty () || !down_dirty->IsEmpty() || !up_dirty->IsEmpty();
}

void
Surface::AddDirtyElement (UIElement *element, DirtyType dirt)
{
	// there's no point in adding an element to the dirty list if it
	// isn't in the hierarchy.  it will be added to the dirty list when
	// it's added to the hierarchy anyway.
	if (element->GetVisualParent() == NULL && !IsTopLevel(element))
		return;

	// XXX this should really be here...
// 	if (element->dirty_flags & dirt)
// 		return;

	element->dirty_flags |= dirt;

	//printf ("adding element %p (%s) to the dirty list\n", element, element->GetTypeName());

	if (dirt & DirtyMeasure) {
		if (element->measure_dirty_node)
			return;

		element->measure_dirty_node = new DirtyNode (element);
		
		measure_dirty->Append (element->measure_dirty_node);
	}

	if (dirt & DirtyArrange) {
		if (element->arrange_dirty_node)
			return;

		element->arrange_dirty_node = new DirtyNode (element);
		
		arrange_dirty->Append (element->arrange_dirty_node);
	}

	element->dirty_flags |= dirt;

	if (dirt & DownDirtyState) {
		if (element->down_dirty_node)
			return;

		element->down_dirty_node = new DirtyNode (element);

		down_dirty->AddDirtyNode (element->GetVisualLevel (), element->down_dirty_node);
	}

	if (dirt & UpDirtyState) {
		if (element->up_dirty_node)
			return;

		element->up_dirty_node = new DirtyNode (element);

		up_dirty->AddDirtyNode (element->GetVisualLevel (), element->up_dirty_node);
	}
}

void
Surface::RemoveDirtyElement (UIElement *element)
{
	if (element->up_dirty_node)
		up_dirty->RemoveDirtyNode (element->GetVisualLevel(), element->up_dirty_node);
	if (element->down_dirty_node)
		down_dirty->RemoveDirtyNode (element->GetVisualLevel(), element->down_dirty_node);
	if (element->measure_dirty_node)
		measure_dirty->Remove (element->measure_dirty_node);
	if (element->arrange_dirty_node)
		arrange_dirty->Remove (element->arrange_dirty_node);

	element->down_dirty_node = NULL;
	element->up_dirty_node = NULL;
	element->measure_dirty_node = NULL;
	element->arrange_dirty_node = NULL;
}


/*
** There are 2 types of changes that need to propagate around the
** tree.
**
** 1. Those changes that need to be propagated from parent to children
**    (transformation, opacity).  We call these Downward Changes, and
**    the elements are placed in the down_dirty list.
**
** 2. Those changes that need to be propagated from children to parent
**    (bounds, invalidation).  We call these Upward Changes, and the
**    elements are placed in the up_dirty list.
**
**
** Downward Changes can result in new Upward changes (when an
** element's transform changes, usually its bounds change), so when
** processing the dirty list we push changes down the tree before
** handling the Upward Changes.
**
*/

void
Surface::PropagateDirtyFlagToChildren (UIElement *el, DirtyType flags)
{
	VisualTreeWalker walker = VisualTreeWalker (el);
	while (UIElement *child = walker.Step ())
		AddDirtyElement (child, flags);
}

void
Surface::ProcessDownDirtyElements ()
{
	/* push down the transforms opacity, and visibility changes first */
	while (DirtyNode *node = (DirtyNode*)down_dirty->GetFirst()) {
		UIElement* el = (UIElement*)node->element;

		if (el->dirty_flags & DirtyRenderVisibility) {
			el->dirty_flags &= ~DirtyRenderVisibility;

			el->UpdateBounds ();
			// Since we are not included in our parents subtree when we
			// are collapsed we need to notify our parent that things may
			// have changed
			if (el->GetVisualParent ())
				el->GetVisualParent ()->UpdateBounds ();

			el->ComputeTotalRenderVisibility ();
			AddDirtyElement (el, DirtyNewBounds);

			PropagateDirtyFlagToChildren (el, DirtyRenderVisibility);
		}

		if (el->dirty_flags & DirtyHitTestVisibility) {
			el->dirty_flags &= ~DirtyHitTestVisibility;
			
			el->ComputeTotalHitTestVisibility ();

			PropagateDirtyFlagToChildren (el, DirtyHitTestVisibility);
		}
		/*
		** since we cache N's local (from N's parent to N)
		** transform, we need to catch if we're changing
		** something about that local transform and recompute
		** it.
		** 
		** DirtyLocalTransform implies DirtyTransform, since
		** changing N's local transform requires updating the
		** transform of all descendents in the subtree rooted
		** at N.
		*/
		if (el->dirty_flags & DirtyLocalTransform) {
			el->dirty_flags &= ~DirtyLocalTransform;

			el->dirty_flags |= DirtyTransform;

			el->ComputeLocalTransform ();
		}

		if (el->dirty_flags & DirtyTransform) {
			el->dirty_flags &= ~DirtyTransform;
			
			el->Invalidate ();
			el->ComputeTransform ();

			if (el->GetVisualParent ())
				el->GetVisualParent ()->UpdateBounds ();
					
			AddDirtyElement (el, DirtyNewBounds);
			PropagateDirtyFlagToChildren (el, DirtyTransform);
		}

		if (el->dirty_flags & DirtyLocalClip) {
			el->dirty_flags &= ~DirtyLocalClip;
			el->dirty_flags |= DirtyClip;

			// XXX el->ComputeLocalClip ();
		}

		if (el->dirty_flags & DirtyClip) {
			el->dirty_flags &= ~DirtyTransform;

			// XXX el->ComputeClip ();
			// XXX el->UpdateBounds ();

			PropagateDirtyFlagToChildren (el, DirtyClip);
		}

		if (el->dirty_flags & DirtyChildrenZIndices) {
			el->dirty_flags &= ~DirtyChildrenZIndices;
			if (!el->Is(Type::PANEL)) { 
				g_warning ("DirtyChildrenZIndices is only applicable to Panel subclasses");
			}
			else {
				((Panel*)el)->GetChildren ()->ResortByZIndex();
			}
			    
		}

		if (!(el->dirty_flags & DownDirtyState)) {
			down_dirty->RemoveDirtyNode (el->GetVisualLevel (), el->down_dirty_node);
			el->down_dirty_node = NULL;
		}
	}
	
	if (!down_dirty->IsEmpty())
		g_warning ("after down dirty pass, down dirty list is not empty");
}

/*
** Note that since this calls GDK invalidation functions 
** it's a good idea to call it with a GDK lock held (all gtk callbacks
** are automatically protected except for timeouts and idle)
*/
void
Surface::ProcessUpDirtyElements ()
{
	while (DirtyNode *node = (DirtyNode*)up_dirty->GetFirst()) {
		UIElement* el = (UIElement*)node->element;

//   		printf ("up processing element element %p (%s)\n", el, el->GetName());
// 		printf ("el->parent = %p\n", el->parent);

		if (el->dirty_flags & DirtyBounds) {
//			printf (" + bounds\n");
			el->dirty_flags &= ~DirtyBounds;

			Rect obounds = el->GetBounds ();
			Rect osubtreebounds = el->GetSubtreeBounds ();
			bool parent_bounds_updated = false;

			el->ComputeBounds ();

// 				printf (" + + obounds = %f %f %f %f, nbounds = %f %f %f %f\n",
// 					obounds.x, obounds.y, obounds.w, obounds.h,
// 					el->GetBounds().x, el->GetBounds().y, el->GetBounds().w, el->GetBounds().h);

			if (osubtreebounds != el->GetSubtreeBounds ()) {
				if (el->GetVisualParent ()) {
					el->GetVisualParent ()->UpdateBounds ();
					parent_bounds_updated = true;
				}
			}

			if (obounds != el->GetBounds()) {
				if (el->GetVisualParent ()) {
// 						printf (" + + + calling UpdateBounds and Invalidate on parent\n");
					if (!parent_bounds_updated)
						el->GetVisualParent ()->UpdateBounds();

					Region oregion = Region (obounds);
					el->GetVisualParent ()->Invalidate (&oregion);
				}
				el->Invalidate ();
			}
				
			if (el->force_invalidate_of_new_bounds) {
				el->force_invalidate_of_new_bounds = false;
				// Invalidate everything including the
				// visible area of our children.
				el->Invalidate (el->GetSubtreeBounds ());
			}
		}
		if (el->dirty_flags & DirtyNewBounds) {
			el->Invalidate ();
			el->dirty_flags &= ~DirtyNewBounds;
		}
		if (el->dirty_flags & DirtyInvalidate) {
//  			printf (" + invalidating %p (%s) %s, %f %f %f %f\n",
// 				el, el->GetTypeName(), el->GetName(), el->dirty_rect.x, el->dirty_rect.y, el->dirty_rect.w, el->dirty_rect.h);

			el->dirty_flags &= ~DirtyInvalidate;

			Region *dirty = el->dirty_region;

			GdkRectangle *rects;
			int count;
			dirty->GetRectangles (&rects, &count);
			Surface *surface = el->GetSurface ();
			if (surface) {
				while (count--) {
					Rect r = Rect ((double)rects [count].x,
						       (double)rects [count].y,
						       (double)rects [count].width,
						       (double)rects [count].height);
					//printf (" + + invalidating parent (%f,%f,%f,%f)\n",
					//	r.x,
					//	r.y,
					//	r.w,
					//	r.h);

					surface->Invalidate (r);					
				}
				g_free (rects);
			}

			delete el->dirty_region;
			el->dirty_region = new Region ();
		}

		if (!(el->dirty_flags & UpDirtyState)) {
			up_dirty->RemoveDirtyNode (el->GetVisualLevel (), el->up_dirty_node);
			el->up_dirty_node = NULL;
		}
	}
	
	if (!up_dirty->IsEmpty())
		g_warning ("after up dirty pass, up dirty list is not empty");
}

void
Surface::UpdateLayout ()
{
	int i = 0;
	List *size_dirty = new List ();

	Size available = Size (active_window->GetWidth (),
			       active_window->GetHeight ());

	for (int i = 0; i < layers->GetCount (); i++) {
		UIElement *layer = layers->GetValueAt (i)->AsUIElement ();

		Size *last = LayoutInformation::GetLastMeasure (layer);

		if (!last || *last != available) {
			LayoutInformation::SetLastMeasure (layer, &available);
			layer->InvalidateMeasure ();
			layer->InvalidateArrange ();
		}
	}

	for (int i = 0; i < 250; i++) {
		while (!measure_dirty->IsEmpty ()) {
			DirtyNode *node = (DirtyNode *) measure_dirty->First ();
			UIElement *element = node->element;
			element->DoMeasure ();

			if (node != element->measure_dirty_node)
				g_warning ("Dirty node mismatch while measuring");

			measure_dirty->Remove (element->measure_dirty_node);
			element->measure_dirty_node = NULL;
		}
		
		while (!arrange_dirty->IsEmpty () && measure_dirty->IsEmpty ()) {
			DirtyNode *node = (DirtyNode *) arrange_dirty->First ();
			UIElement *element = node->element;
			Size *prev = LayoutInformation::GetLastRenderSize (element);

			element->DoArrange ();

			if (!prev && LayoutInformation::GetLastRenderSize (element))
				size_dirty->Append (new DirtyNode (element));

			if (node != element->arrange_dirty_node)
				g_warning ("Dirty node mismatch while arranging");

			arrange_dirty->Remove (element->arrange_dirty_node);
			element->arrange_dirty_node = NULL;
		}
		
		if (!measure_dirty->IsEmpty ())
			continue;

		while (!size_dirty->IsEmpty () && measure_dirty->IsEmpty () && arrange_dirty->IsEmpty ()) {
			DirtyNode *node = (DirtyNode *) size_dirty->First ();
			UIElement *element = node->element;

			element->UpdateSize ();
			
			size_dirty->Remove (node);
		}

		if (measure_dirty->IsEmpty () && size_dirty->IsEmpty () && arrange_dirty->IsEmpty ()) {
			if (i > 0)
				g_warning ("Got out early %d", i);
			break;
		}
	}
}

void
Surface::ProcessDirtyElements ()
{
	UpdateLayout ();
	ProcessDownDirtyElements ();
	ProcessUpDirtyElements ();
}

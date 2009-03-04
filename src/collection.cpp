/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * collection.cpp: different types of collections
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#include <config.h>
#include <glib.h>
#include <errno.h>

#include "canvas.h"
#include "collection.h"
#include "geometry.h"
#include "transform.h"
#include "frameworkelement.h"
#include "namescope.h"
#include "text.h"
#include "utils.h"
#include "error.h"

//
// Collection
//

Collection::Collection ()
{
	SetObjectType (Type::COLLECTION);
	array = g_ptr_array_new ();
	generation = 0;
}

Collection::~Collection ()
{
	g_ptr_array_free (array, true);
}

void
Collection::Dispose ()
{
	Value *value;
	
	for (guint i = 0; i < array->len; i++) {
		value = (Value *) array->pdata[i];
		RemovedFromCollection (value);
		delete value;
	}
	g_ptr_array_set_size (array, 0);
	
	DependencyObject::Dispose ();
}

CollectionIterator *
Collection::GetIterator ()
{
	return new CollectionIterator (this);
}

int
Collection::Add (Value *value)
{
	MoonError error;
	return AddWithError (value, &error);
}

int
Collection::Add (Value value)
{
	return Add (&value);
}

int
Collection::AddWithError (Value *value, MoonError *error)
{
	bool rv = InsertWithError (array->len, value, error);
	return rv ? array->len - 1 : -1;
}

bool
Collection::Clear ()
{
	EmitChanged (CollectionChangedActionClearing, NULL, NULL, -1);

	guint len = array->len;
	Value** vals = new Value*[len];
	memmove (vals, array->pdata, len * sizeof(Value*));

	g_ptr_array_set_size (array, 0);
	generation++;
	
	SetCount (0);

	for (guint i = 0; i < len; i++) {
		RemovedFromCollection (vals[i]);
		delete vals[i];
	}
	delete[] vals;
	
	EmitChanged (CollectionChangedActionCleared, NULL, NULL, -1);

	return true;
}

bool
Collection::Contains (Value *value)
{
	return IndexOf (value) != -1;
}

int
Collection::IndexOf (Value *value)
{
	Value *v;
	
	for (guint i = 0; i < array->len; i++) {
		v = (Value *) array->pdata[i];
		if (*v == *value)
			return i;
	}
	
	return -1;
}

bool
Collection::Insert (int index, Value value)
{
	return Insert (index, &value);
}

bool
Collection::InsertWithError (int index, Value *value, MoonError *error)
{
	Value *added;
	
	// Check that the item can be added to our collection
	if (!CanAdd (value))
		return false;
	
	// bounds check
	if (index < 0)
		return false;
	
	if (index > GetCount ())
		index = GetCount ();
	
	added = new Value (*value);
	
	if (AddedToCollection (added, error)) {
		g_ptr_array_insert (array, index, added);
	
		SetCount ((int) array->len);
		
		EmitChanged (CollectionChangedActionAdd, added, NULL, index);
	
		return true;
	}
	else {
		delete added;
		return false;
	}
}

bool
Collection::Insert (int index, Value *value)
{
	MoonError error;
	return InsertWithError (index, value, &error);
}

bool
Collection::Remove (Value value)
{
	return Remove (&value);
}

bool
Collection::Remove (Value *value)
{
	int index;
	
	if ((index = IndexOf (value)) == -1)
		return false;
	
	return RemoveAt (index);
}

bool
Collection::RemoveAt (int index)
{
	Value *value;
	
	// check bounds
	if (index < 0 || (guint) index >= array->len)
		return false;
	
	value = (Value *) array->pdata[index];
	
	g_ptr_array_remove_index (array, index);
	SetCount ((int) array->len);
	generation++;
	
	RemovedFromCollection (value);
	
	EmitChanged (CollectionChangedActionRemove, NULL, value, index);
	
	delete value;
	
	return true;
}

bool
Collection::RemoveAtWithError (int index, MoonError *error)
{
	// check bounds
	if (index < 0 || (guint) index >= array->len) {
		MoonError::FillIn (error, MoonError::ARGUMENT_OUT_OF_RANGE, "");
		return false;
	}
	
	return RemoveAt (index);
}

Value *
Collection::GetValueAt (int index)
{
	if (index < 0 || (guint) index >= array->len)
		return NULL;
	
	return (Value *) array->pdata[index];
}

Value *
Collection::GetValueAtWithError (int index, MoonError *error)
{
	// check array bounds
	if (index < 0 || (guint) index >= array->len) {
		MoonError::FillIn (error, MoonError::ARGUMENT_OUT_OF_RANGE, "");
		return NULL;
	}
	
	return GetValueAt (index);
}

bool
Collection::SetValueAt (int index, Value *value)
{
	MoonError error;
	return SetValueAtWithError (index, value, &error);
}

bool
Collection::SetValueAtWithError (int index, Value *value, MoonError *error)
{
	Value *added, *removed;
	
	// Check that the value can be added to our collection
	if (!CanAdd (value)) {
		MoonError::FillIn (error, MoonError::ARGUMENT, "");
		return false;
	}
	
	// check array bounds
	if (index < 0 || (guint) index >= array->len) {
		MoonError::FillIn (error, MoonError::ARGUMENT_OUT_OF_RANGE, "");
		return false;
	}
	
	removed = (Value *) array->pdata[index];
	added = new Value (*value);
	
	if (AddedToCollection (added, error)) {
		array->pdata[index] = added;
	
		RemovedFromCollection (removed);
	
		EmitChanged (CollectionChangedActionReplace, added, removed, index);
	
		delete removed;

		return true;
	}
	else
		return false;
}

void
Collection::EmitChanged (CollectionChangedAction action, Value *new_value, Value *old_value, int index)
{
	CollectionChangedEventArgs *args;
	
	if (GetLogicalParent()) {
		args = new CollectionChangedEventArgs (action, new_value, old_value, index);
		GetLogicalParent()->OnCollectionChanged (this, args);
		args->unref ();
	}
}

bool
Collection::CanAdd (Value *value)
{
	return value->Is (GetElementType ());
}


//
// DependencyObjectCollection
//

DependencyObjectCollection::DependencyObjectCollection ()
{
	SetObjectType (Type::DEPENDENCY_OBJECT_COLLECTION);
}

DependencyObjectCollection::~DependencyObjectCollection ()
{
}

bool
DependencyObjectCollection::AddedToCollection (Value *value, MoonError *error)
{
	DependencyObject *obj = value->AsDependencyObject ();
	
	DependencyObject *parent = obj->GetLogicalParentIncludingCollections();
	
	// Call SetSurface() /before/ setting the logical parent
	// because Storyboard::SetSurface() needs to be able to
	// distinguish between the two cases.
	obj->SetSurface (GetSurface ());

	if (parent) {
		if (parent->Is(Type::COLLECTION) && !obj->PermitsMultipleParents ()) {
			MoonError::FillIn (error, MoonError::INVALID_OPERATION, "Element is already a child of another element.");
			return false;
		}
	}
	else {
		obj->SetLogicalParent (this, error);
		if (error->number)
			return false;
	}

	obj->AddPropertyChangeListener (this);
	
	return Collection::AddedToCollection (value, error);
}

void
DependencyObjectCollection::RemovedFromCollection (Value *value)
{
	DependencyObject *obj = value->AsDependencyObject ();
	
	obj->RemovePropertyChangeListener (this);
	obj->SetLogicalParent (NULL, NULL);
	obj->SetSurface (NULL);
	
	Collection::RemovedFromCollection (value);
}

void
DependencyObjectCollection::SetSurface (Surface *surface)
{
	if (GetSurface() == surface)
		return;

	DependencyObject *obj;
	Value *value;
	
	for (guint i = 0; i < array->len; i++) {
		value = (Value *) array->pdata[i];
		obj = value->AsDependencyObject ();
		obj->SetSurface (surface);
	}
	
	Collection::SetSurface (surface);
}

void
DependencyObjectCollection::OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args)
{
	if (GetLogicalParent())
		GetLogicalParent()->OnCollectionItemChanged (this, obj, subobj_args);
}

void
DependencyObjectCollection::UnregisterAllNamesRootedAt (NameScope *from_ns)
{
	DependencyObject *obj;
	Value *value;
	
	for (guint i = 0; i < array->len; i++) {
		value = (Value *) array->pdata[i];
		obj = value->AsDependencyObject ();
		obj->UnregisterAllNamesRootedAt (from_ns);
	}
	
	Collection::UnregisterAllNamesRootedAt (from_ns);
}

void
DependencyObjectCollection::RegisterAllNamesRootedAt (NameScope *to_ns, MoonError *error)
{
	DependencyObject *obj;
	Value *value;
	
	for (guint i = 0; i < array->len; i++) {
		if (error->number)
			break;

		value = (Value *) array->pdata[i];
		obj = value->AsDependencyObject ();
		obj->RegisterAllNamesRootedAt (to_ns, error);
	}
	
	Collection::RegisterAllNamesRootedAt (to_ns, error);
}

//
// InlineCollection
//

InlineCollection::InlineCollection ()
{
	SetObjectType (Type::INLINE_COLLECTION);
}

InlineCollection::~InlineCollection ()
{
}

bool
InlineCollection::Equals (InlineCollection *inlines)
{
	Inline *run0, *run1;
	
	if (inlines->array->len != array->len)
		return false;
	
	for (guint i = 0; i < array->len; i++) {
		run1 = ((Value *) inlines->array->pdata[i])->AsInline ();
		run0 = ((Value *) array->pdata[i])->AsInline ();
		
		if (!run0->Equals (run1))
			return false;
	}
	
	return true;
}


//
// UIElementCollection
//

UIElementCollection::UIElementCollection ()
{
	SetObjectType (Type::UIELEMENT_COLLECTION);
	z_sorted = g_ptr_array_new ();
}

UIElementCollection::~UIElementCollection ()
{
	g_ptr_array_free (z_sorted, true);
}

static int
UIElementZIndexComparer (gconstpointer ui1, gconstpointer ui2)
{
	int z1 = Canvas::GetZIndex (*((UIElement **) ui1));
	int z2 = Canvas::GetZIndex (*((UIElement **) ui2));
	
	return z1 - z2;
}

void
UIElementCollection::ResortByZIndex ()
{
	g_ptr_array_set_size (z_sorted, array->len);
	
	if (array->len == 0)
		return;
	
	for (guint i = 0; i < array->len; i++)
		z_sorted->pdata[i] = ((Value *) array->pdata[i])->AsUIElement ();
	
	if (array->len > 1)
		g_ptr_array_sort (z_sorted, UIElementZIndexComparer);
}

bool
UIElementCollection::Clear ()
{
	g_ptr_array_set_size (z_sorted, 0);
	return DependencyObjectCollection::Clear ();
}

//
// HitTestCollection
//

HitTestCollection::HitTestCollection ()
{
}

//
// DoubleCollection
//

DoubleCollection::DoubleCollection ()
{
	SetObjectType (Type::DOUBLE_COLLECTION);
}

DoubleCollection::~DoubleCollection ()
{
}

DoubleCollection *
DoubleCollection::FromStr (const char *s)
{
	GArray *values = double_garray_from_str (s, 0);

	if (values->len == 0) {
		g_array_free (values, true);
		return NULL;
	}

	DoubleCollection *doubles = new DoubleCollection ();
	for (guint i = 0; i < values->len; i ++)
		doubles->Add (Value (g_array_index (values, double, i)));
	g_array_free (values, true);

	return doubles;
}

//
// Point Collection
//

PointCollection::PointCollection ()
{
	SetObjectType (Type::POINT_COLLECTION);
}

PointCollection::~PointCollection ()
{
}

PointCollection *
PointCollection::FromStr (const char *s)
{
	int i, j, n = 0;
	GArray *values = double_garray_from_str (s, 0);

	n = values->len / 2;
	if (n == 0 || n % 1 == 1) {
		g_array_free (values, true);
		return NULL;
	}

	PointCollection *points = new PointCollection();
	for (i = 0, j = 0; j < n; j++) {
		double x = g_array_index (values, double, i++);
		double y = g_array_index (values, double, i++);

		points->Add (Point (x, y));
	}

	g_array_free (values, true);
	return points;
}

//
// Item Collection
//

ItemCollection::ItemCollection ()
{
	SetObjectType (Type::ITEM_COLLECTION);
}

ItemCollection::~ItemCollection ()
{
}

//
// Trigger Collection
//

TriggerCollection::TriggerCollection ()
{
	SetObjectType (Type::TRIGGER_COLLECTION);
}

TriggerCollection::~TriggerCollection ()
{
}

//
// TriggerAction Collection
//

TriggerActionCollection::TriggerActionCollection ()
{
	SetObjectType (Type::TRIGGERACTION_COLLECTION);
}

TriggerActionCollection::~TriggerActionCollection ()
{
}

//
// MultiScaleSubImage Collection
//
MultiScaleSubImageCollection::MultiScaleSubImageCollection ()
{
	SetObjectType (Type::MULTISCALESUBIMAGE_COLLECTION);
}

MultiScaleSubImageCollection::~MultiScaleSubImageCollection ()
{
}


//
// CollectionIterator
//

CollectionIterator::CollectionIterator (Collection *c)
{
	generation = c->Generation ();
	collection = c;
	collection->ref ();
	index = -1;
}

CollectionIterator::~CollectionIterator ()
{
	collection->unref ();
}

int
CollectionIterator::Next ()
{
	if (generation != collection->Generation ())
		return -1;
	
	index++;
	
	if (index >= collection->GetCount ())
		return 0;
	
	return 1;
}

bool
CollectionIterator::Reset()
{
	if (generation != collection->Generation ())
		return false;
	
	index = -1;
	
	return true;
}

Value *
CollectionIterator::GetCurrent (int *error)
{
	if (generation != collection->Generation ()) {
		*error = 1;
		return NULL;
	}
	
	if (index < 0) {
		*error = 1;
		return NULL;
	}
	
	*error = 0;
	
	return collection->GetValueAt (index);
}

void
CollectionIterator::Destroy (CollectionIterator *iterator)
{
	delete iterator;
}


//
// VisualTreeWalker
//

VisualTreeWalker::VisualTreeWalker (UIElement *obj, VisualTreeWalkerDirection dir)
{
	index = 0;
	collection = NULL;
	content = obj->GetSubtreeObject ();
	direction = dir;

	if (content != NULL) {
		if (content->Is (Type::COLLECTION)) {
			collection = (Collection *)content;

			if (!collection->Is (Type::UIELEMENT_COLLECTION))
				direction = Logical;
		}

		content->ref ();
	}
}

UIElement *
VisualTreeWalker::Step ()
{
	UIElement *result = NULL;

	if (collection) {
		int count = collection->GetCount ();

		if (index < 0 || index >= count)
			return NULL;

		UIElementCollection *uiecollection = NULL;
		if (direction != Logical) {
			uiecollection = (UIElementCollection *)collection;
			if ((int)uiecollection->z_sorted->len != count) {
				g_warning ("VisualTreeWalker: unexpectedly got an unsorted UIElementCollection");
				uiecollection->ResortByZIndex ();
			}
		}

		switch (direction) {
		case ZForward:
			result = (UIElement*)uiecollection->z_sorted->pdata[index];
			break;
		case ZReverse:
			result = (UIElement *)uiecollection->z_sorted->pdata[count - (index + 1)];
			break;
		default:
			Value *v = collection->GetValueAt (index);
			result = v->AsUIElement ();
		}
		
		index++;
	} else {
		if (index == 0) {
			index++;
			result = (UIElement *)content;
		} else { 
			result = NULL;
		}
	}

	return result;
}

int
VisualTreeWalker::GetCount ()
{
	if (!content)
		return 0;

	if (!collection)
		return 1;

	return collection->GetCount ();
}

VisualTreeWalker::~VisualTreeWalker()
{
	if (content)
		content->unref ();
}


//
// Manual C-Bindings
//
Collection *
collection_new (Type::Kind kind)
{
	Type *t = Type::Find (kind);
	
	if (!t->IsSubclassOf (Type::COLLECTION)) {
		g_warning ("create_collection passed non-collection type");
		return NULL;
	}
	
	return (Collection *) t->CreateInstance();
}

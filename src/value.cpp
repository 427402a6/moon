/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * value.cpp: Implementation of for Value.
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#include <config.h>
#include <string.h>
#include <cairo.h>
#include <malloc.h>
#include <stdlib.h>

#include "value.h"
#include "rect.h"
#include "size.h"
#include "color.h"
#include "clock.h"
#include "animation.h"
#include "control.h"
#include "point.h"
#include "grid.h"
#include "cornerradius.h"
#include "mono/metadata/object.h"

/**
 * Value implementation
 */

static const int NullFlag = 1;
    
Value*
Value::CreateUnrefPtr (DependencyObject* dob)
{
	Value *result = new Value (dob);
	dob->unref ();
	return result;
}

Value
Value::CreateUnref (DependencyObject* dob)
{
	Value result (dob);
	dob->unref ();
	return result;
}

Type::Kind
Value::GetKind ()
{
	return k;
}

bool
Value::GetIsNull ()
{
	return (padding & NullFlag) == NullFlag;
}

void
Value::SetIsNull (bool isNull)
{
	if (isNull)
		padding |= NullFlag;
	else
		padding &= ~NullFlag;
}

void
Value::Init ()
{
	padding = 0;
	memset (&u, 0, sizeof (u));
	SetIsNull (true);
}

Value::Value()
{
	k = Type::INVALID;
	Init ();
}

Value::Value (const Value& v)
{
	padding = v.padding;
	k = v.k;
	u = v.u;

	SetIsNull (((Value&)v).GetIsNull());

	/* make a copy of the string instead of just the pointer */
	switch (k) {
	case Type::STRING:
		u.s = g_strdup (v.u.s);
		break;
	case Type::FONTFAMILY:
		if (v.u.fontfamily) {
			u.fontfamily = g_new (FontFamily, 1);
			u.fontfamily->source = g_strdup (v.u.fontfamily->source);
		}
		break;
	case Type::PROPERTYPATH:
		if (v.u.propertypath) {
			u.propertypath = g_new (PropertyPath, 1);
			u.propertypath->path = g_strdup (v.u.propertypath->path);
		}
		break;
	case Type::COLOR:
		u.color = g_new (Color, 1);
		*u.color = *v.u.color;
		break;
	case Type::POINT:
		u.point = g_new (Point, 1);
		*u.point = *v.u.point;
		break;
	case Type::RECT:
		u.rect = g_new (Rect, 1);
		*u.rect = *v.u.rect;
		break;
	case Type::SIZE:
		u.size = g_new (Size, 1);
		*u.size = *v.u.size;
		break;
	case Type::REPEATBEHAVIOR:
		u.repeat = g_new (RepeatBehavior, 1);
		*u.repeat = *v.u.repeat;
		break;
	case Type::DURATION:
		u.duration = g_new (Duration, 1);
		*u.duration = *v.u.duration;
		break;
	case Type::KEYTIME:
		u.keytime = g_new (KeyTime, 1);
		*u.keytime = *v.u.keytime;
		break;
	case Type::GRIDLENGTH:
		u.grid_length = g_new (GridLength, 1);
		*u.grid_length = *v.u.grid_length;
		break;
	case Type::THICKNESS:
		u.thickness = g_new (Thickness, 1);
		*u.thickness = *v.u.thickness;
		break;
	case Type::CORNERRADIUS:
		u.corner = g_new (CornerRadius, 1);
		*u.corner = *v.u.corner;
		break;
	case Type::MANAGEDTYPEINFO:
		if (u.type_info) {
			u.type_info = g_new (ManagedTypeInfo, 1);
			*u.type_info = *v.u.type_info;
		}
		break;
	default:
		if (Is (Type::EVENTOBJECT) && u.dependency_object)
			u.dependency_object->ref ();
		break;
	}
}

Value::Value (Type::Kind k)
{
	Init();
	this->k = k;
}

Value::Value(bool z)
{
	Init ();
	k = Type::BOOL;
	u.i32 = z;
	SetIsNull (false);
}

Value::Value (double d)
{
	Init ();
	k = Type::DOUBLE;
	u.d = d;
	SetIsNull (false);
}

Value::Value (gint64 i, Type::Kind as)
{
	Init ();
	k = as;
	u.i64 = i;
	SetIsNull (false);
}

Value::Value (gint32 i)
{
	Init ();
	k = Type::INT32;
	u.i32 = i;
	SetIsNull (false);
}


Value::Value (guint32 i)
{
	Init ();
	k = Type::UINT32;
	u.ui32 = i;
	SetIsNull (false);
}

Value::Value (Color c)
{
	Init ();
	k = Type::COLOR;
	u.color = g_new (Color, 1);
	*u.color = Color (c);
	SetIsNull (false);
}

Value::Value (EventObject* obj)
{
	Init ();
	if (obj == NULL) {
		k = Type::EVENTOBJECT;
	}
	else {
		if (!Type::IsSubclassOf (obj->GetObjectType (), Type::EVENTOBJECT)) {
			g_warning ("creating invalid dependency object Value");
			k = Type::INVALID;
			u.dependency_object = NULL;
			return;
		}
		k = obj->GetObjectType ();
		obj->ref ();
		SetIsNull (false);
	}
	u.dependency_object = obj;
}

Value::Value (FontFamily family)
{
	Init ();
	k = Type::FONTFAMILY;
	u.fontfamily = g_new (FontFamily, 1);
	u.fontfamily->source = g_strdup (family.source);
	SetIsNull (false);
}

Value::Value (PropertyPath propertypath)
{
	Init ();
	k = Type::PROPERTYPATH;
	u.propertypath = g_new (PropertyPath, 1);
	u.propertypath->path = g_strdup (propertypath.path);
	SetIsNull (false);
}

Value::Value (Type::Kind kind, void *npobj)
{
	Init ();
	k = kind;
	u.managed_object = npobj;
	SetIsNull (npobj == NULL);
}

Value::Value (Point pt)
{
	Init ();
	k = Type::POINT;
	u.point = g_new (Point, 1);
	*u.point = Point (pt);
	SetIsNull (false);
}

Value::Value (Rect rect)
{
	Init ();
	k = Type::RECT;
	u.rect = g_new (Rect, 1);
	*u.rect = Rect (rect);
	SetIsNull (false);
}

Value::Value (Size size)
{
	Init ();
	k = Type::SIZE;
	u.size = g_new (Size, 1);
	*u.size = Size (size);
	SetIsNull (false);
}

Value::Value (RepeatBehavior repeat)
{
	Init();
	k = Type::REPEATBEHAVIOR;
	u.repeat = g_new (RepeatBehavior, 1);
	*u.repeat = RepeatBehavior (repeat);
	SetIsNull (false);
}

Value::Value (Duration duration)
{
	Init();
	k = Type::DURATION;
	u.duration = g_new (Duration, 1);
	*u.duration = Duration (duration);
	SetIsNull (false);
}

Value::Value (KeyTime keytime)
{
	Init ();
	k = Type::KEYTIME;
	u.keytime = g_new (KeyTime, 1);
	*u.keytime = KeyTime (keytime);
	SetIsNull (false);
}

Value::Value (const char *s, bool take)
{
	Init ();
	k = Type::STRING;
	
	u.s = take ? (char *) s : g_strdup (s);
	SetIsNull (s == NULL);
}

Value::Value (GridLength grid_length)
{
	Init ();
	k = Type::GRIDLENGTH;
	u.grid_length = g_new (GridLength, 1);
	*u.grid_length = GridLength (grid_length);
	SetIsNull (false);
}

Value::Value (Thickness thickness)
{
	Init ();
	k = Type::THICKNESS;
	u.thickness = g_new (Thickness, 1);
	*u.thickness = Thickness (thickness);
	SetIsNull (false);
}

Value::Value (CornerRadius corner)
{
	Init ();
	k = Type::CORNERRADIUS;
	u.corner = g_new (CornerRadius, 1);
	*u.corner = CornerRadius (corner);
	SetIsNull (false);
}

Value::Value (ManagedTypeInfo type_info)
{
	Init ();
	k = Type::MANAGEDTYPEINFO;
	u.type_info = g_new (ManagedTypeInfo, 1);
	*u.type_info = ManagedTypeInfo (type_info);
	SetIsNull (false);
}

void
Value::FreeValue ()
{
	switch (GetKind ()) {
	case Type::STRING:
		g_free (u.s);
		break;
	case Type::COLOR:
		g_free (u.color);
		break;
	case Type::FONTFAMILY:
		g_free (u.fontfamily->source);
		g_free (u.fontfamily);
		break;
	case Type::PROPERTYPATH:
		g_free (u.propertypath->path);
		g_free (u.propertypath);
		break;
	case Type::POINT:
		g_free (u.point);
		break;
	case Type::RECT:
		g_free (u.rect);
		break;
	case Type::SIZE:
		g_free (u.size);
		break;
	case Type::REPEATBEHAVIOR:
		g_free (u.repeat);
		break;
	case Type::DURATION:
		g_free (u.duration);
		break;
	case Type::KEYTIME:
		g_free (u.keytime);
		break;
	case Type::GRIDLENGTH:
		g_free (u.grid_length);
		break;
	case Type::THICKNESS:
		g_free (u.thickness);
		break;
	case Type::CORNERRADIUS:
		g_free (u.corner);
		break;
	default:
		if (Is (Type::EVENTOBJECT) && u.dependency_object)
			u.dependency_object->unref ();
	}
}

char *
Value::ToString ()
{
	GString *str = g_string_new ("");
	
	switch (k) {
	case Type::DOUBLE:
		g_string_append_printf (str, "{ %f }", u.d);
		break;
	case Type::STRING:
		g_string_append (str, u.s);
		break;
	case Type::COLOR:
		g_string_append_printf (str, "{%g/%g/%g/%g}", u.color->r, u.color->g, u.color->b, u.color->a);
		break;
	case Type::POINT:
		g_string_append_printf (str, "{ %g, %g }", (u.point)->x, (u.point)->y);
		break;
	case Type::SIZE:
		g_string_append_printf (str, "{ %g, %g }", (u.size)->width, (u.size)->height);
		break;
	case Type::RECT:
		g_string_append_printf (str, "{ x=%g, y=%g, w=%g, h=%g }", (u.rect)->x, (u.rect)->y, (u.rect)->width, (u.rect)->height);
		break;
	case Type::REPEATBEHAVIOR:
		if (u.repeat->IsForever ())
			g_string_append (str, "{repeat=forever}");
		else if ((u.repeat)->HasDuration ())
			g_string_append_printf (str, "{repeat=duration}");
		else if ((u.repeat)->HasCount ())
			g_string_append_printf (str, "{repeat=count %g}", (u.repeat)->GetCount ());
		break;
	case Type::THICKNESS:
		g_string_append_printf (str, "{ l=%g, t=%g, r=%g, b=%g }", (u.thickness)->left, (u.thickness)->top, (u.thickness)->right, (u.thickness)->bottom);
		break;
	case Type::DURATION:
		g_string_append_printf (str, "{duration/TODO}");
		break;
	case Type::KEYTIME:
		g_string_append_printf (str, "{keytime/TODO}");
		break;
	case Type::GRIDLENGTH:
		g_string_append_printf (str, "{gridlength value:%.2f type:%d}", u.grid_length->val, u.grid_length->type);
		break;
	default:
		if (Is (Type::EVENTOBJECT) && u.dependency_object)
			g_string_append_printf (str, "[%s <%s>]", u.dependency_object->GetTypeName (), Is (Type::DEPENDENCY_OBJECT) ? AsDependencyObject ()->GetName () : "no name");
		else
			g_string_append_printf (str, "UnknownType");
		break;
	}
	
	return g_string_free (str, FALSE);
}

bool
Value::operator!= (const Value &v) const
{
	return !(*this == v);
}

bool 
Value::operator== (const Value &v) const
{
	if (k != v.k)
		return false;
	
	if (padding != v.padding)
		return false;

	switch (k) {
	case Type::STRING:
		if (u.s == NULL){
			return v.u.s == NULL;
		} else if (v.u.s == NULL)
			return FALSE;

		return !strcmp (u.s, v.u.s);
	case Type::FONTFAMILY:
		return *u.fontfamily == *v.u.fontfamily;
	case Type::PROPERTYPATH:
		return *u.propertypath == *v.u.propertypath;
	case Type::COLOR:
		return !memcmp (u.color, v.u.color, sizeof (Color));
	case Type::POINT:
		return !memcmp (u.point, v.u.point, sizeof (Point));
	case Type::RECT:
		return !memcmp (u.rect, v.u.rect, sizeof (Rect));
	case Type::SIZE:
		return !memcmp (u.size, v.u.size, sizeof (Size));
	case Type::REPEATBEHAVIOR:
		// memcmp can't be used since the struct contains unassigned padding value
		return *u.repeat == *v.u.repeat;
	case Type::DURATION:
		// memcmp can't be used since the struct contains unassigned padding value
		return *u.duration == *v.u.duration;
	case Type::KEYTIME:
		// memcmp can't be used since the struct contains unassigned padding value
		return *u.keytime == *v.u.keytime;
	case Type::GRIDLENGTH:
		return !memcmp (u.grid_length, v.u.grid_length, sizeof (GridLength));
	case Type::THICKNESS:
		return !memcmp (u.thickness, v.u.thickness, sizeof (Thickness));
	case Type::CORNERRADIUS:
		return !memcmp (u.corner, v.u.corner, sizeof (CornerRadius));
	case Type::MANAGEDTYPEINFO:
		return !memcmp (u.type_info, v.u.type_info, sizeof (ManagedTypeInfo));
	case Type::MANAGED:
		return mono_gchandle_get_target ((gint32)(gint64)u.managed_object) == mono_gchandle_get_target ((gint32)(gint64)v.u.managed_object);
	
	default:
		return !memcmp (&u, &v.u, sizeof (u));
	}

	return true;
}

void
Value::Unmarshal (Type::Kind desired)
{
	if (desired == k)
		return;

	if (desired == Type::STRING) {
		switch (k) {
		case Type::URI:
			k = desired;
			break;
		default:
			break;
		}
	}
}

//
// This is invoked by managed code to free the contents of the value
//
void 
value_free_value (Value* value)
{
	value->FreeValue ();
}

Value::~Value ()
{
	FreeValue ();
}


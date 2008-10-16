/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * canvas.h: canvas definitions.
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#ifndef __MOON_CANVAS_H__
#define __MOON_CANVAS_H__

#include <glib.h>

#include "panel.h"

//
// Canvas Class, the only purpose is to have the Left/Top properties that
// children can use
//
/* @Namespace=System.Windows.Controls */
class Canvas : public Panel {
 protected:
	virtual ~Canvas () {}
	
 public:
 	/* @PropertyType=double,DefaultValue=0.0,Attached,GenerateAccessors */
	static DependencyProperty *LeftProperty;
 	/* @PropertyType=double,DefaultValue=0.0,Attached,GenerateAccessors */
	static DependencyProperty *TopProperty;
	/* @PropertyType=gint32,DefaultValue=0,Attached,GenerateAccessors */
	static DependencyProperty *ZIndexProperty;
	
	/* @GenerateCBinding,GeneratePInvoke */
	Canvas ();
	
	virtual Type::Kind GetObjectType () { return Type::CANVAS; }

	virtual Point GetTransformOrigin ();
	virtual void OnLoaded ();

	virtual Size MeasureOverride (Size availableSize);
	virtual Size ArrangeOverride (Size availableSize);
	virtual bool ClipToExtents () { return false; }
	virtual void ShiftPosition (Point p);
	virtual void ComputeBounds ();
	virtual void GetTransformFor (UIElement *item, cairo_matrix_t *result);
	
	virtual void ElementAdded (UIElement *item);
	virtual void ElementRemoved (UIElement *item);

	virtual void OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args);
	virtual void OnPropertyChanged (PropertyChangedEventArgs *args);

	virtual void OnCollectionItemChanged (Collection *col, DependencyObject *obj, PropertyChangedEventArgs *args);
	
	//
	// Property Accessors
	//
	static void SetLeft (DependencyObject *item, double left);
	static double GetLeft (DependencyObject *item);
	
	static void SetTop (DependencyObject *item, double top);
	static double GetTop (DependencyObject *item);
	
	static void SetZIndex (DependencyObject *item, int zindex);
	static int GetZIndex (DependencyObject *item);
};

#endif /* __MOON_CANVAS_H__ */

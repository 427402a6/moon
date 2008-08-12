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
 	/* @PropertyType=double,DefaultValue=0.0,Attached */
	static DependencyProperty *LeftProperty;
 	/* @PropertyType=double,DefaultValue=0.0,Attached */
	static DependencyProperty *TopProperty;
	
	/* @GenerateCBinding */
	Canvas ();
	
	virtual Type::Kind GetObjectType () { return Type::CANVAS; }

	virtual Point GetTransformOrigin ();

	virtual void ComputeBounds ();
	virtual void GetTransformFor (UIElement *item, cairo_matrix_t *result);

	virtual void OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args);
	virtual void OnPropertyChanged (PropertyChangedEventArgs *args);
	
	//
	// Property Accessors
	//
	static void SetLeft (UIElement *item, double left);
	static double GetLeft (UIElement *item);
	
	static void SetTop (UIElement *item, double top);
	static double GetTop (UIElement *item);
	
	void SetLeft (double left);
	double GetLeft ();
	
	void SetTop (double top);
	double GetTop ();
};

#endif /* __MOON_CANVAS_H__ */

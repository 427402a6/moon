/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * popup.h: 
 *
 * Copyright 2009 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#ifndef __POPUP_H__
#define __POPUP_H__

#include <glib.h>

#include "panel.h"

/*@Namespace=System.Windows.Controls.Primitives*/
class Popup : public FrameworkElement {
 public:
	/* @PropertyType=UIElement,GenerateAccessors,Validator=ContentControlContentValidator */
	const static int ChildProperty;
	/* @PropertyType=double,GenerateAccessors,DefaultValue=0.0 */
	const static int HorizontalOffsetProperty;
	/* @PropertyType=bool,GenerateAccessors */
	const static int IsOpenProperty;
	/* @PropertyType=double,GenerateAccessors,DefaultValue=0.0 */
	const static int VerticalOffsetProperty;
	
	/* @GenerateCBinding,GeneratePInvoke */
	Popup ();

	//
	// Property Accessors
	//
	void SetChild (UIElement *element);
	UIElement *GetChild ();
	
	void SetHorizontalOffset (double offset);
	double GetHorizontalOffset ();
	
	void SetIsOpen (bool open);
	bool GetIsOpen ();
	
	void SetVerticalOffset (double offset);
	double GetVerticalOffset ();

	virtual void OnPropertyChanged (PropertyChangedEventArgs *args, MoonError *error);

	const static int IsOpenChangedEvent;
 private:
 	void Hide ();
 	void Show ();
	bool visible;
};


#endif /* __POPUP_H__ */

/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * frameworkelement.cpp
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */


#include <config.h>

#include <math.h>

#include "frameworkelement.h"
#include "thickness.h"
#include "collection.h"

DependencyProperty *FrameworkElement::HeightProperty;
DependencyProperty *FrameworkElement::WidthProperty;

// 2.0 only DPs
DependencyProperty *FrameworkElement::ActualHeightProperty;
DependencyProperty *FrameworkElement::ActualWidthProperty;
DependencyProperty *FrameworkElement::DataContextProperty;
DependencyProperty *FrameworkElement::HorizontalAlignmentProperty;
DependencyProperty *FrameworkElement::LanguageProperty;
DependencyProperty *FrameworkElement::MarginProperty;
DependencyProperty *FrameworkElement::MaxHeightProperty;
DependencyProperty *FrameworkElement::MaxWidthProperty;
DependencyProperty *FrameworkElement::MinHeightProperty;
DependencyProperty *FrameworkElement::MinWidthProperty;
DependencyProperty *FrameworkElement::VerticalAlignmentProperty;

FrameworkElement::FrameworkElement ()
{
	// XXX bad bad bad.  no virtual method calls in ctors
	SetValue (FrameworkElement::TriggersProperty, Value::CreateUnref (new TriggerCollection ()));
	SetValue (FrameworkElement::ResourcesProperty, Value::CreateUnref (new ResourceDictionary ()));
}

void
FrameworkElement::OnPropertyChanged (PropertyChangedEventArgs *args)
{
	if (args->property->GetOwnerType() != Type::FRAMEWORKELEMENT) {
		UIElement::OnPropertyChanged (args);
		return;
	}

	if (args->property == FrameworkElement::WidthProperty ||
	    args->property == FrameworkElement::HeightProperty) {
		Point p = GetRenderTransformOrigin ();

		/* normally we'd only update the bounds of this
		   element on a width/height change, but if the render
		   transform is someplace other than (0,0), the
		   transform needs to be updated as well. */
		FullInvalidate (p.x != 0.0 || p.y != 0.0);
	}

	NotifyListenersOfPropertyChange (args);
}

void
FrameworkElement::ComputeBounds ()
{
	double x1, x2, y1, y2;
	
	x1 = y1 = 0.0;
	y2 = GetHeight ();
	x2 = GetWidth ();
	
	if (x2 != 0.0 && y2 != 0.0)
		bounds = IntersectBoundsWithClipPath (Rect (x1,y1,x2,y2), false).Transform (&absolute_xform);
}

bool
FrameworkElement::InsideObject (cairo_t *cr, double x, double y)
{
	double height = GetHeight ();
	double width = GetWidth ();
	double nx = x, ny = y;
	
	uielement_transform_point (this, &nx, &ny);
	if (nx < 0 || ny < 0 || nx > width || ny > height)
		return false;
	
	return UIElement::InsideObject (cr, x, y);
}

void
FrameworkElement::GetSizeForBrush (cairo_t *cr, double *width, double *height)
{
	*height = GetHeight ();
	*width = GetWidth ();
}

void
FrameworkElement::SetHeight (double height)
{
	SetValue (FrameworkElement::HeightProperty, Value (height));
}

double
FrameworkElement::GetHeight ()
{
	return GetValue (FrameworkElement::HeightProperty)->AsDouble ();
}

void
FrameworkElement::SetWidth (double width)
{
	SetValue (FrameworkElement::WidthProperty, Value (width));
}

double
FrameworkElement::GetWidth ()
{
	return GetValue (FrameworkElement::WidthProperty)->AsDouble ();
}

FrameworkElement *
framework_element_new (void)
{
	return new FrameworkElement ();
}

void
framework_element_set_height (FrameworkElement *element, double height)
{
	element->SetHeight (height);
}

double
framework_element_get_height (FrameworkElement *element)
{
	return element->GetHeight ();
}

void
framework_element_set_width (FrameworkElement *element, double width)
{
	element->SetWidth (width);
}

double
framework_element_get_width (FrameworkElement *element)
{
	return element->GetWidth ();
}


void
framework_element_init (void)
{
	FrameworkElement::HeightProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "Height", new Value (0.0));
	FrameworkElement::WidthProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "Width", new Value (0.0));

	// the 2.0 only DPs
	FrameworkElement::ActualHeightProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "ActualHeight", new Value (0.0));
	FrameworkElement::ActualWidthProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "ActualWidth", new Value (0.0));
	FrameworkElement::DataContextProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "DataContext", Type::MANAGED);
	FrameworkElement::HorizontalAlignmentProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "HorizontalAlignment", new Value (HorizontalAlignmentStretch));
	FrameworkElement::LanguageProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "Language", new Value ("en-US"));
	FrameworkElement::MarginProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "Margin", new Value (Thickness (0)));
	FrameworkElement::MaxHeightProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "MaxHeight", new Value (INFINITY));
	FrameworkElement::MaxWidthProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "MaxWidth", new Value (INFINITY));
	FrameworkElement::MinHeightProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "MinHeight", new Value (0.0));
	FrameworkElement::MinWidthProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "MinWidth", new Value (0.0));
	FrameworkElement::VerticalAlignmentProperty = DependencyProperty::Register (Type::FRAMEWORKELEMENT, "VerticalAlignment", new Value (VerticalAlignmentStretch));

}

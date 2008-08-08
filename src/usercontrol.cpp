/*
 * control.cpp:
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#include <config.h>
#include "usercontrol.h"

UIElement *
user_control_get_content (UserControl *user_control)
{
	Value* v =user_control-> GetValue (UserControl::ContentProperty);
	if (!v)
		return NULL;
	return v->AsUIElement ();
}

void
UserControl::OnPropertyChanged (PropertyChangedEventArgs *args)
{
	if (args->property->GetOwnerType() != Type::USERCONTROL) {
		Control::OnPropertyChanged (args);
		return;
	}
	
	if (args->property == UserControl::ContentProperty){
		SetContent (args->new_value->AsUIElement (), GetSurface ());
		UpdateBounds ();
	}
	NotifyListenersOfPropertyChange (args);
}

UserControl::UserControl ()
{
}

UserControl::~UserControl ()
{
}

void 
user_control_init ()
{
	// Don't register any DPs here.
	return;

	UserControl::ContentProperty = DependencyProperty::Register (Type::USERCONTROL, "Content", Type::UIELEMENT);
}

/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bitmapimage.h
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007-2008 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#ifndef __BITMAPIMAGE_H__
#define __BITMAPIMAGE_H__

#include "dependencyobject.h"

/* @Namespace=System.Windows.Media.Imaging */
class BitmapImage : public DependencyObject {
 protected:
	virtual ~BitmapImage ();

 public:
	gpointer buffer;
	gint32 size;

	/* @PropertyType=string,ManagedPropertyType=Uri,GenerateAccessors */
	static DependencyProperty *UriSourceProperty;
	
	/* @GenerateCBinding,GeneratePInvoke */
	BitmapImage ();

	/* @GenerateCBinding,GeneratePInvoke */
	void SetBuffer (gpointer buffer, int size);

	void CleanUp ();

	virtual Type::Kind GetObjectType () { return Type::BITMAPIMAGE; }	
	
	void        SetUriSource (const char *value);
	const char* GetUriSource ();
	
	virtual void OnPropertyChanged (PropertyChangedEventArgs *args);
};

#endif /* __BITMAPIMAGE_H__ */

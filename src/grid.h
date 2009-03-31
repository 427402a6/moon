/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * grid.h
 *
 * Copyright 2008 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#ifndef __MOON_GRID_H__
#define __MOON_GRID_H__

#include <glib.h>
#include "panel.h"

/* @IncludeInKinds */
/* @Namespace=System.Windows */
struct GridLength {
 public:
	double val;
	GridUnitType type;
	
	GridLength () {
		val = 0;
		type = GridUnitTypeAuto;
	}
	
	GridLength (double v, GridUnitType t)
	{
		val = v;
		type = t;
	}
};

/* @Namespace=System.Windows.Controls */
class ColumnDefinition : public DependencyObject {
	// Actual width computed
	double actual;
	
 protected:
	virtual ~ColumnDefinition ();
	
 public:
 	/* @PropertyType=double,DefaultValue=INFINITY,GenerateAccessors */
	const static int MaxWidthProperty;
 	/* @PropertyType=double,DefaultValue=0.0,GenerateAccessors */
	const static int MinWidthProperty;
 	/* @PropertyType=GridLength,DefaultValue=GridLength (1.0\, GridUnitTypeStar),GenerateAccessors */
	const static int WidthProperty;
	
	/* @GenerateCBinding,GeneratePInvoke */
	ColumnDefinition ();
	
	/* @GenerateCBinding,GeneratePInvoke */
	double GetActualWidth () { return actual; }
	void SetActualWidth (double value) { actual = value; }

	// property accessors
	double GetMaxWidth();
	void SetMaxWidth (double value);

	double GetMinWidth();
	void SetMinWidth (double value);

	GridLength* GetWidth();
	void SetWidth (GridLength *value);
};

/* @Namespace=System.Windows.Controls */
class RowDefinition : public DependencyObject {
	// Actual height computed
	double actual;
	
 protected:
	virtual ~RowDefinition ();
	
 public:
 	/* @PropertyType=GridLength,DefaultValue=GridLength (1.0\, GridUnitTypeStar),GenerateAccessors */
	const static int HeightProperty;
 	/* @PropertyType=double,DefaultValue=INFINITY,GenerateAccessors */
	const static int MaxHeightProperty;
 	/* @PropertyType=double,DefaultValue=0.0,GenerateAccessors */
	const static int MinHeightProperty;
	
	/* @GenerateCBinding,GeneratePInvoke */
	RowDefinition ();
	
	/* @GenerateCBinding,GeneratePInvoke */
	double GetActualHeight () { return actual; }
	void SetActualHeight (double value) { actual = value; }

	// property accessors
	double GetMaxHeight();
	void SetMaxHeight (double value);

	double GetMinHeight();
	void SetMinHeight (double value);

	GridLength* GetHeight();
	void SetHeight (GridLength *value);
};

/* @Namespace=System.Windows.Controls */
class ColumnDefinitionCollection : public DependencyObjectCollection {
 protected:
	virtual ~ColumnDefinitionCollection ();
	
 public:
	/* @GenerateCBinding,GeneratePInvoke,ManagedAccess=Internal */
	ColumnDefinitionCollection ();
	
	virtual Type::Kind GetElementType () { return Type::COLUMNDEFINITION; }
};


/* @Namespace=System.Windows.Controls */
class RowDefinitionCollection : public DependencyObjectCollection {
 protected:
	virtual ~RowDefinitionCollection ();
	
 public:
	/* @GenerateCBinding,GeneratePInvoke,ManagedAccess=Internal */
	RowDefinitionCollection ();
	
	virtual Type::Kind GetElementType () { return Type::ROWDEFINITION; }
};


/* @Namespace=System.Windows.Controls */
class Grid : public Panel {
	Size magic;

 protected:
	virtual ~Grid ();

 public:
 	/* @PropertyType=gint32,DefaultValue=0,Attached,GenerateAccessors,Validator=PositiveIntValidator */
	const static int ColumnProperty;
	/* @PropertyType=ColumnDefinitionCollection,AutoCreateValue,ManagedFieldAccess=Internal,ManagedSetterAccess=Internal,GenerateAccessors */
	const static int ColumnDefinitionsProperty;
 	/* @PropertyType=gint32,DefaultValue=1,Attached,GenerateAccessors,Validator=IntGreaterThanZeroValidator */
	const static int ColumnSpanProperty;
 	/* @PropertyType=gint32,DefaultValue=0,Attached,GenerateAccessors,Validator=PositiveIntValidator */
	const static int RowProperty;
	/* @PropertyType=RowDefinitionCollection,AutoCreateValue,ManagedFieldAccess=Internal,ManagedSetterAccess=Internal,GenerateAccessors */
	const static int RowDefinitionsProperty;
 	/* @PropertyType=gint32,DefaultValue=1,Attached,GenerateAccessors,Validator=IntGreaterThanZeroValidator */
	const static int RowSpanProperty;
 	/* @PropertyType=bool,DefaultValue=true,GenerateAccessors */
	const static int ShowGridLinesProperty;
	
	/* @GenerateCBinding,GeneratePInvoke */
	Grid ();
	
	virtual void OnPropertyChanged (PropertyChangedEventArgs *args, MoonError *error);
	virtual void OnCollectionChanged (Collection *col, CollectionChangedEventArgs *args);
	virtual void OnCollectionItemChanged (Collection *col, DependencyObject *obj, PropertyChangedEventArgs *args);
	
	virtual Size MeasureOverride (Size availableSize);
	virtual Size ArrangeOverride (Size finalSize);

	// property accessors
	ColumnDefinitionCollection *GetColumnDefinitions ();
	void SetColumnDefinitions (ColumnDefinitionCollection* value);

	static gint32 GetColumn (DependencyObject *obj);
	static void SetColumn (DependencyObject *obj, gint32 value);

	static gint32 GetColumnSpan (DependencyObject *obj);
	static void SetColumnSpan (DependencyObject *obj, gint32 value);

	RowDefinitionCollection *GetRowDefinitions ();
	void SetRowDefinitions (RowDefinitionCollection* value);

	static gint32 GetRow (DependencyObject *obj);
	static void SetRow (DependencyObject *obj, gint32 value);

	static gint32 GetRowSpan (DependencyObject *obj);
	static void SetRowSpan (DependencyObject *obj, gint32 value);

	bool GetShowGridLines ();
	void SetShowGridLines (bool value);
};

#endif /* __MOON_PANEL_H__ */

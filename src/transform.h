/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * transform.h: 
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include <glib.h>

G_BEGIN_DECLS

#include <cairo.h>
#include "collection.h"

class GeneralTransform : public DependencyObject {
protected:
	cairo_matrix_t _matrix;
	bool need_update;

	virtual ~GeneralTransform () {};

	virtual void UpdateTransform ();
	void MaybeUpdateTransform ();
public:
	GeneralTransform () : need_update (true) { }
	virtual void OnPropertyChanged (PropertyChangedEventArgs *args);
	virtual Type::Kind GetObjectType () { return Type::GENERALTRANSFORM; };

	virtual void GetTransform (cairo_matrix_t *value);

	Point Transform (Point point);
};

GeneralTransform *general_transform_new (void);
void   general_transform_get_transform (GeneralTransform *transform, cairo_matrix_t *value);
void   general_transform_transform_point (GeneralTransform *t, Point *p, Point *r);

class Transform : public GeneralTransform {
protected:
	virtual ~Transform () {}

public:
	Transform () { }

	virtual Type::Kind GetObjectType () { return Type::TRANSFORM; };
};


Transform *transform_new (void);

class RotateTransform : public Transform {
 protected:
	virtual ~RotateTransform () {}
	virtual void UpdateTransform ();
	
 public:
	static DependencyProperty *AngleProperty;
	static DependencyProperty *CenterXProperty;
	static DependencyProperty *CenterYProperty;
	
	RotateTransform () { }
	virtual Type::Kind GetObjectType () { return Type::ROTATETRANSFORM; };
	
	//
	// Property Accessors
	//
	void SetAngle (double angle);
	double GetAngle ();
	
	void SetCenterX (double centerX);
	double GetCenterX ();
	
	void SetCenterY (double centerY);
	double GetCenterY ();
};

RotateTransform *rotate_transform_new (void);

void   rotate_transform_set_angle (RotateTransform *transform, double angle);
double rotate_transform_get_angle (RotateTransform *transform);

void   rotate_transform_set_center_x (RotateTransform *transform, double centerX);
double rotate_transform_get_center_x (RotateTransform *transform);

void   rotate_transform_set_center_y (RotateTransform *transform, double centerY);
double rotate_transform_get_center_y (RotateTransform *transform);


class TranslateTransform : public Transform {
 protected:
	virtual ~TranslateTransform () {}
	virtual void UpdateTransform ();
	
 public:
	static DependencyProperty *XProperty;
	static DependencyProperty *YProperty;
	
	TranslateTransform () {  }
	virtual Type::Kind GetObjectType () { return Type::TRANSLATETRANSFORM; };
	
	//
	// Property Accessors
	//
	void SetX (double x);
	double GetX ();
	
	void SetY (double y);
	double GetY ();
};

TranslateTransform *translate_transform_new (void);
void   translate_transform_set_x (TranslateTransform *transform, double x);
double translate_transform_get_x (TranslateTransform *transform);

void   translate_transform_set_y (TranslateTransform *transform, double y);
double translate_transform_get_y (TranslateTransform *transform);


class ScaleTransform : public Transform {
 protected:
	virtual ~ScaleTransform () {}
	virtual void UpdateTransform ();

 public:

	ScaleTransform () {  }
	virtual Type::Kind GetObjectType () { return Type::SCALETRANSFORM; };

	static DependencyProperty* ScaleXProperty;
	static DependencyProperty* ScaleYProperty;
	static DependencyProperty* CenterXProperty;
	static DependencyProperty* CenterYProperty;
};

ScaleTransform *scale_transform_new (void);
void   scale_transform_set_scale_x (ScaleTransform *transform, double scaleX);
double scale_transform_get_scale_x (ScaleTransform *transform);

void   scale_transform_set_scale_y (ScaleTransform *transform, double scaleY);
double scale_transform_get_scale_y (ScaleTransform *transform);

void   scale_transform_set_center_x (ScaleTransform *transform, double centerX);
double scale_transform_get_center_x (ScaleTransform *transform);

void   scale_transform_set_center_y (ScaleTransform *transform, double centerY);
double scale_transform_get_center_y (ScaleTransform *transform);


class SkewTransform : public Transform {
 protected:
	virtual ~SkewTransform () {}
	virtual void UpdateTransform ();

public:

	SkewTransform () {  }
	virtual Type::Kind GetObjectType () { return Type::SKEWTRANSFORM; };

	static DependencyProperty* AngleXProperty;
	static DependencyProperty* AngleYProperty;
	static DependencyProperty* CenterXProperty;
	static DependencyProperty* CenterYProperty;
};

SkewTransform *skew_transform_new (void);
void   skew_transform_set_angle_x (SkewTransform *transform, double angleX);
double skew_transform_get_angle_x (SkewTransform *transform);

void   skew_transform_set_angle_y (SkewTransform *transform, double angleY);
double skew_transform_get_angle_y (SkewTransform *transform);

void   skew_transform_set_center_x (SkewTransform *transform, double centerX);
double skew_transform_get_center_x (SkewTransform *transform);

void   skew_transform_set_center_y (SkewTransform *transform, double centerY);
double skew_transform_get_center_y (SkewTransform *transform);

class Matrix : public DependencyObject {
private:
	cairo_matrix_t matrix;

protected:
	virtual ~Matrix () {}

public:
	static DependencyProperty *M11Property;
	static DependencyProperty *M12Property;
	static DependencyProperty *M21Property;
	static DependencyProperty *M22Property;
	static DependencyProperty *OffsetXProperty;
	static DependencyProperty *OffsetYProperty;

	Matrix ();

	virtual void OnPropertyChanged (PropertyChangedEventArgs *args);

	virtual Type::Kind GetObjectType () { return Type::MATRIX; }

	cairo_matrix_t GetUnderlyingMatrix ();
};

Matrix *matrix_new (void);
double matrix_get_m11 (Matrix *matrix);
void matrix_set_m11 (Matrix *matrix, double value);
double matrix_get_m12 (Matrix *matrix);
void matrix_set_m12 (Matrix *matrix, double value);
double matrix_get_m21 (Matrix *matrix);
void matrix_set_m21 (Matrix *matrix, double value);
double matrix_get_m22 (Matrix *matrix);
void matrix_set_m22 (Matrix *matrix, double value);
double matrix_get_offset_x (Matrix *matrix);
void matrix_set_offset_x (Matrix *matrix, double value);
double matrix_get_offset_y (Matrix *matrix);
void matrix_set_offset_y (Matrix *matrix, double value);

class MatrixTransform : public Transform {
 protected:
	virtual ~MatrixTransform () {}

	virtual void UpdateTransform ();
 public:
	static DependencyProperty* MatrixProperty;

	MatrixTransform () {}
	virtual Type::Kind GetObjectType () { return Type::MATRIXTRANSFORM; };

	virtual void OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args);
};

MatrixTransform *matrix_transform_new (void);
void	matrix_transform_set_matrix (MatrixTransform *transform, Matrix* matrix);
Matrix*	matrix_transform_get_matrix (MatrixTransform *transform);


class TransformCollection : public DependencyObjectCollection {
 protected:
	virtual ~TransformCollection () {}

 public:
	TransformCollection () {}
	
	virtual Type::Kind GetObjectType () { return Type::TRANSFORM_COLLECTION; }
	virtual Type::Kind GetElementType () { return Type::TRANSFORM; }
};

TransformCollection *transform_collection_new (void);

/* @ContentProperty="Children" */
class TransformGroup : public Transform {
protected:
	virtual ~TransformGroup ();

	virtual void UpdateTransform ();
public:
	static DependencyProperty *ChildrenProperty;

	TransformGroup ();
	virtual Type::Kind GetObjectType() { return Type::TRANSFORMGROUP; };
	
	virtual void OnCollectionItemChanged (Collection *col, DependencyObject *obj, PropertyChangedEventArgs *args);
	virtual void OnCollectionChanged (Collection *col, CollectionChangedEventArgs *args);
	virtual void OnPropertyChanged (PropertyChangedEventArgs *args);
};

TransformGroup *transform_group_new (void);

void transform_init (void);

G_END_DECLS

#endif

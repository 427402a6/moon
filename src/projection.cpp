/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * projection.cpp: 
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2009 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#include <config.h>

#include <math.h>
#include <string.h>

#include "projection.h"
#include "eventargs.h"

//
// Matrix3D
//

Matrix3D::Matrix3D ()
{
	SetObjectType (Type::MATRIX3D);

	// initialize the matrix as the identity
	memset (matrix, 0, sizeof (double) * 16);
	matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0;
}

Matrix3D::Matrix3D (double *m)
{
	SetObjectType (Type::MATRIX3D);

	memcpy (matrix, m, sizeof (double) * 16);
	SetM11 (m[0]);
	SetM12 (m[1]);
	SetM13 (m[2]);
	SetM14 (m[3]);
	SetM21 (m[4]);
	SetM22 (m[5]);
	SetM23 (m[6]);
	SetM24 (m[7]);
	SetM31 (m[8]);
	SetM32 (m[9]);
	SetM33 (m[10]);
	SetM34 (m[11]);
	SetOffsetX (m[12]);
	SetOffsetY (m[13]);
	SetOffsetZ (m[14]);
	SetM44 (m[15]);
}

void
Matrix3D::OnPropertyChanged (PropertyChangedEventArgs *args, MoonError *error)
{
	if (args->GetProperty ()->GetOwnerType() != Type::MATRIX3D) {
		DependencyObject::OnPropertyChanged (args, error);
		return;
	}
	
	if (args->GetId () == Matrix3D::M11Property)
		matrix[0] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M12Property)
		matrix[1] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M13Property)
		matrix[2] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M14Property)
		matrix[3] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M21Property)
		matrix[4] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M22Property)
		matrix[5] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M23Property)
		matrix[6] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M24Property)
		matrix[7] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M31Property)
		matrix[8] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M32Property)
		matrix[9] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M33Property)
		matrix[10] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M34Property)
		matrix[11] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::OffsetXProperty)
		matrix[12] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::OffsetYProperty)
		matrix[13] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::OffsetZProperty)
		matrix[14] = args->GetNewValue()->AsDouble ();
	else if (args->GetId () == Matrix3D::M44Property)
		matrix[15] = args->GetNewValue()->AsDouble ();

	NotifyListenersOfPropertyChange (args, error);
}

void
Matrix3D::TransformPoint (double *out, const double *m, const double *in)
{
	double tmp[4];

#define M(row, col) m[col * 4 + row]
	tmp[0] = M (0, 0) * in[0] + M (0, 1) * in[1] + M (0, 2) * in[2] + M (0, 3) * in[3];
	tmp[1] = M (1, 0) * in[0] + M (1, 1) * in[1] + M (1, 2) * in[2] + M (1, 3) * in[3];
	tmp[2] = M (2, 0) * in[0] + M (2, 1) * in[1] + M (2, 2) * in[2] + M (2, 3) * in[3];
	tmp[3] = M (3, 0) * in[0] + M (3, 1) * in[1] + M (3, 2) * in[2] + M (3, 3) * in[3];
#undef M

	memcpy (out, tmp, 4 * sizeof (double));
}

void
Matrix3D::Multiply (double *out, const double *a, const double *b)
{
	double tmp[16];
	int    i;

#define A(row, col) a[col * 4 + row]
#define B(row, col) b[col * 4 + row]
#define T(row, col) tmp[col * 4 + row]
	for (i = 0; i < 4; i++) {
		T (0, i) = A (0, i) * B (0, 0) + A (1, i) * B (0, 1) + A (2, i) * B (0, 2) + A (3, i) * B (0, 3);
		T (1, i) = A (0, i) * B (1, 0) + A (1, i) * B (1, 1) + A (2, i) * B (1, 2) + A (3, i) * B (1, 3);
		T (2, i) = A (0, i) * B (2, 0) + A (1, i) * B (2, 1) + A (2, i) * B (2, 2) + A (3, i) * B (2, 3);
		T (3, i) = A (0, i) * B (3, 0) + A (1, i) * B (3, 1) + A (2, i) * B (3, 2) + A (3, i) * B (3, 3);
	}
#undef T
#undef B
#undef A

	memcpy (out, tmp, 16 * sizeof (double));
}

void
Matrix3D::Translate (double *out, double tx, double ty, double tz)
{

#define M(row, col) out[col * 4 + row]
	M (0, 0) = 1.0; M (1, 0) = 0.0; M (2, 0) = 0.0; M (3, 0) = 0.0;
	M (0, 1) = 0.0; M (1, 1) = 1.0; M (2, 1) = 0.0; M (3, 1) = 0.0;
	M (0, 2) = 0.0; M (1, 2) = 0.0; M (2, 2) = 1.0; M (3, 2) = 0.0;
	M (0, 3) = tx;  M (1, 3) = ty;  M (2, 3) = tz;  M (3, 3) = 1.0;
#undef M

}

void
Matrix3D::Scale (double *out, double sx, double sy, double sz)
{

#define M(row, col) out[col * 4 + row]
	M (0, 0) = sx;  M (1, 0) = 0.0; M (2, 0) = 0.0; M (3, 0) = 0.0;
	M (0, 1) = 0.0; M (1, 1) = sy;  M (2, 1) = 0.0; M (3, 1) = 0.0;
	M (0, 2) = 0.0; M (1, 2) = 0.0; M (2, 2) = sz;  M (3, 2) = 0.0;
	M (0, 3) = 0.0; M (1, 3) = 0.0; M (2, 3) = 0.0; M (3, 3) = 1.0;
#undef M

}

void
Matrix3D::RotateX (double *out, double theta)
{
	double sr = sin (theta);
	double cr = cos (theta);

#define M(row, col) out[col * 4 + row]
	M (0, 0) = 1.0; M (1, 0) = 0.0; M (2, 0) = 0.0; M (3, 0) = 0.0;
	M (0, 1) = 0.0; M (1, 1) = cr;  M (2, 1) = sr;  M (3, 1) = 0.0;
	M (0, 2) = 0.0; M (1, 2) = -sr; M (2, 2) = cr;  M (3, 2) = 0.0;
	M (0, 3) = 0.0; M (1, 3) = 0.0; M (2, 3) = 0.0; M (3, 3) = 1.0;
#undef M

}

void
Matrix3D::RotateY (double *out, double theta)
{
	double sr = sin (theta);
	double cr = cos (theta);

#define M(row, col) out[col * 4 + row]
	M (0, 0) = cr;  M (1, 0) = 0.0; M (2, 0) = -sr; M (3, 0) = 0.0;
	M (0, 1) = 0.0; M (1, 1) = 1.0; M (2, 1) = 0.0; M (3, 1) = 0.0;
	M (0, 2) = sr;  M (1, 2) = 0.0; M (2, 2) = cr;  M (3, 2) = 0.0;
	M (0, 3) = 0.0; M (1, 3) = 0.0; M (2, 3) = 0.0; M (3, 3) = 1.0;
#undef M

}

void
Matrix3D::RotateZ (double *out, double theta)
{
	double sr = sin (theta);
	double cr = cos (theta);

#define M(row, col) out[col * 4 + row]
	M (0, 0) = cr;  M (1, 0) = sr;  M (2, 0) = 0.0; M (3, 0) = 0.0;
	M (0, 1) = -sr; M (1, 1) = cr;  M (2, 1) = 0.0; M (3, 1) = 0.0;
	M (0, 2) = 0.0; M (1, 2) = 0.0; M (2, 2) = 1.0; M (3, 2) = 0.0;
	M (0, 3) = 0.0; M (1, 3) = 0.0; M (2, 3) = 0.0; M (3, 3) = 1.0;
#undef M
	
}

void
Matrix3D::Perspective (double *out, double fieldOfViewY, double aspectRatio, double zNearPlane, double zFarPlane)
{
	double height = 1.0 / tan (fieldOfViewY / 2.0);
	double width = height / aspectRatio;
	double d = zNearPlane - zFarPlane;

#define M(row, col) out[col * 4 + row]
	M (0, 0) = width; M (1, 0) = 0.0;    M (2, 0) = 0.0;                        M (3, 0) = 0.0;
	M (0, 1) = 0.0;   M (1, 1) = height; M (2, 1) = 0.0;                        M (3, 1) = 0.0;
	M (0, 2) = 0.0;   M (1, 2) = 0.0;    M (2, 2) = zFarPlane / d;              M (3, 2) = -1.0;
	M (0, 3) = 0.0;   M (1, 3) = 0.0;    M (2, 3) = zNearPlane * zFarPlane / d; M (3, 3) = 0.0;
#undef M

}

void
Matrix3D::Viewport (double *out, double width, double height)
{

#define M(row, col) out[col * 4 + row]
	M (0, 0) = width / 2.0; M (1, 0) = 0.0;           M (2, 0) = 0.0; M (3, 0) = 0.0;
	M (0, 1) = 0.0;         M (1, 1) = -height / 2.0; M (2, 1) = 0.0; M (3, 1) = 0.0;
	M (0, 2) = 0.0;         M (1, 2) = 0.0;           M (2, 2) = 1.0; M (3, 2) = 0.0;
	M (0, 3) = width / 2.0; M (1, 3) = height / 2.0;  M (2, 3) = 0.0; M (3, 3) = 1.0;
#undef M

}

bool
Matrix3D::Inverse (double *out, const double *m)
{
	double tmp[16];
	double det;
	int    i;

#define M(row, col) m[col * 4 + row]
#define T(row, col) tmp[col * 4 + row]
	T (0, 0) = M (1, 1) * M (2, 2) * M (3, 3) - M (1, 1) * M (3, 2) * M (2, 3) - M (1, 2) * M (2, 1) * M (3, 3) +
		M (1, 2) * M (3, 1) * M (2, 3) + M (1, 3) * M (2, 1) * M (3, 2) - M (1, 3) * M (3, 1) * M (2, 2);
	T (0, 1) = -M (0, 1) * M (2, 2) * M (3, 3) + M (0, 1) * M (3, 2) * M (2, 3) + M (0, 2) * M (2, 1) * M (3, 3) -
		M (0, 2) * M (3, 1) * M (2, 3) - M (0, 3) * M (2, 1) * M (3, 2) + M (0, 3) * M (3, 1) * M (2, 2);
	T (0, 2) = M (0, 1) * M (1, 2) * M (3, 3) - M (0, 1) * M (3, 2) * M (1, 3) - M (0, 2) * M (1, 1) * M (3, 3) +
		M (0, 2) * M (3, 1) * M (1, 3) + M (0, 3) * M (1, 1) * M (3, 2) - M (0, 3) * M (3, 1) * M (1, 2);
	T (0, 3) = -M (0, 1) * M (1, 2) * M (2, 3) + M (0, 1) * M (2, 2) * M (1, 3) + M (0, 2) * M (1, 1) * M (2, 3) -
		M (0, 2) * M (2, 1) * M (1, 3) - M (0, 3) * M (1, 1) * M (2, 2) + M (0, 3) * M (2, 1) * M (1, 2);
	T (1, 0) = -M (1, 0) * M (2, 2) * M (3, 3) + M (1, 0) * M (3, 2) * M (2, 3) + M (1, 2) * M (2, 0) * M (3, 3) -
		M (1, 2) * M (3, 0) * M (2, 3) - M (1, 3) * M (2, 0) * M (3, 2) + M (1, 3) * M (3, 0) * M (2, 2);
	T (1, 1) =   M (0, 0) * M (2, 2) * M (3, 3) - M (0, 0) * M (3, 2) * M (2, 3) - M (0, 2) * M (2, 0) * M (3, 3) +
		M (0, 2) * M (3, 0) * M (2, 3) + M (0, 3) * M (2, 0) * M (3, 2) - M (0, 3) * M (3, 0) * M (2, 2);
	T (1, 2) = -M (0, 0) * M (1, 2) * M (3, 3) + M (0, 0) * M (3, 2) * M (1, 3) + M (0, 2) * M (1, 0) * M (3, 3) -
		M (0, 2) * M (3, 0) * M (1, 3) - M (0, 3) * M (1, 0) * M (3, 2) + M (0, 3) * M (3, 0) * M (1, 2);
	T (1, 3) = M (0, 0) * M (1, 2) * M (2, 3) - M (0, 0) * M (2, 2) * M (1, 3) - M (0, 2) * M (1, 0) * M (2, 3) +
		M (0, 2) * M (2, 0) * M (1, 3) + M (0, 3) * M (1, 0) * M (2, 2) - M (0, 3) * M (2, 0) * M (1, 2);
	T (2, 0) = M (1, 0) * M (2, 1) * M (3, 3) - M (1, 0) * M (3, 1) * M (2, 3) - M (1, 1) * M (2, 0) * M (3, 3) +
		M (1, 1) * M (3, 0) * M (2, 3) + M (1, 3) * M (2, 0) * M (3, 1) - M (1, 3) * M (3, 0) * M (2, 1);
	T (2, 1) = -M (0, 0) * M (2, 1) * M (3, 3) + M (0, 0) * M (3, 1) * M (2, 3) + M (0, 1) * M (2, 0) * M (3, 3) -
		M (0, 1) * M (3, 0) * M (2, 3) - M (0, 3) * M (2, 0) * M (3, 1) + M (0, 3) * M (3, 0) * M (2, 1);
	T (2, 2) = M (0, 0) * M (1, 1) * M (3, 3) - M (0, 0) * M (3, 1) * M (1, 3) - M (0, 1) * M (1, 0) * M (3, 3) +
		M (0, 1) * M (3, 0) * M (1, 3) + M (0, 3) * M (1, 0) * M (3, 1) - M (0, 3) * M (3, 0) * M (1, 1);
	T (2, 3) = -M (0, 0) * M (1, 1) * M (2, 3) + M (0, 0) * M (2, 1) * M (1, 3) + M (0, 1) * M (1, 0) * M (2, 3) -
		M (0, 1) * M (2, 0) * M (1, 3) - M (0, 3) * M (1, 0) * M (2, 1) + M (0, 3) * M (2, 0) * M (1, 1);
	T (3, 0) = -M (1, 0) * M (2, 1) * M (3, 2) + M (1, 0) * M (3, 1) * M (2, 2) + M (1, 1) * M (2, 0) * M (3, 2) -
		M (1, 1) * M (3, 0) * M (2, 2) - M (1, 2) * M (2, 0) * M (3, 1) + M (1, 2) * M (3, 0) * M (2, 1);
	T (3, 1) = M (0, 0) * M (2, 1) * M (3, 2) - M (0, 0) * M (3, 1) * M (2, 2) - M (0, 1) * M (2, 0) * M (3, 2) +
		M (0, 1) * M (3, 0) * M (2, 2) + M (0, 2) * M (2, 0) * M (3, 1) - M (0, 2) * M (3, 0) * M (2, 1);
	T (3, 2) = -M (0, 0) * M (1, 1) * M (3, 2) + M (0, 0) * M (3, 1) * M (1, 2) + M (0, 1) * M (1, 0) * M (3, 2) -
		M (0, 1) * M (3, 0) * M (1, 2) - M (0, 2) * M (1, 0) * M (3, 1) + M (0, 2) * M (3, 0) * M (1, 1);
	T (3, 3) = M (0, 0) * M (1, 1) * M (2, 2) - M (0, 0) * M (2, 1) * M (1, 2) - M (0, 1) * M (1, 0) * M (2, 2) +
		M (0, 1) * M (2, 0) * M (1, 2) + M (0, 2) * M (1, 0) * M (2, 1) - M (0, 2) * M (2, 0) * M (1, 1);

	det = M (0, 0) * T (0, 0) + M (1, 0) * T (0, 1) + M (2, 0) * T (0, 2) + M (3, 0) * T (0, 3);
#undef T
#undef M

	if (det == 0)
		return false;

	det = 1.0 / det;

	for (i = 0; i < 16; i++)
		out[i] = tmp[i] * det;

	return true;
}

Projection::Projection ()
{
	SetObjectType (Type::PROJECTION);
	need_update = false;
}

Matrix3D *
Projection::GetProjectionMatrix ()
{
	g_warning ("Projection::GetProjectionMatrix has been called. "
		   "The derived class should have overridden it.");
	return NULL;
}

void
Projection::GetTransform (double *value)
{
	Matrix3D *matrix;

	MaybeUpdateProjection ();

	matrix = GetProjectionMatrix ();
	if (matrix) {
		memcpy (value, matrix->GetMatrixValues (), sizeof (double) * 16);
	}
	else {
		memset (value, 0, sizeof (value));
		value[0] = value[5] = value[10] = value[15] = 1.0;
	}
}

static void
lerp (double *fdst, double t, const double *fin, const double *fout)
{
	double tin = t;
	double tout = 1.0 - t;

	fdst[0] = tin * fin[0] + tout * fout[0];
	fdst[1] = tin * fin[1] + tout * fout[1];
	fdst[2] = tin * fin[2] + tout * fout[2];
	fdst[3] = tin * fin[3] + tout * fout[3];
}

static double
dot4 (const double *a, const double *b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

static unsigned
clipmask (const double *clip)
{
	unsigned mask = 0;

	if (-clip[2] + clip[3] < 0) mask |= (1 << 0);
	if ( clip[2] + clip[3] < 0) mask |= (1 << 1);

	return mask;
}

#define MAX_CLIPPED_VERTICES ((2 * 2) + 1)

static Rect
ExtendBoundsTo (Rect     bounds,
		double   *p1,
		double   *p2,
		double   *p3,
		unsigned clipmask)
{
	double *a[MAX_CLIPPED_VERTICES];
	double *b[MAX_CLIPPED_VERTICES];
	double **inlist = a;
	double **outlist = b;
	unsigned tmpnr = 0;
	double tmp0[4], tmp1[4], tmp2[4], tmp3[4];
	double *tmp_vert[] = { tmp0, tmp1, tmp2, tmp3 };
	double plane0[] = { 0.0, 0.0, -1.0, 1.0 };
	double plane1[] = { 0.0, 0.0, 1.0, 1.0 };
	double *clip_plane[] = { plane0, plane1 };
	unsigned n = 3;
	unsigned i;

	inlist[0] = p1;
	inlist[1] = p2;
	inlist[2] = p3;

	while (clipmask && n >= 3) {
		const unsigned plane_idx = ffs (clipmask) - 1;
		const double *plane = clip_plane[plane_idx];
		double *vert_prev = inlist[0];
		double dp_prev = dot4 (vert_prev, plane);
		unsigned outcount = 0;

		clipmask &= ~(1 << plane_idx);

		inlist[n] = inlist[0];

		for (i = 1; i <= n; i++) {
			double *vert = inlist[i];
			double dp = dot4 (vert, plane);

			if (dp_prev >= 0.0)
				outlist[outcount++] = vert_prev;

			if (dp * dp_prev <= 0.0 && dp - dp_prev != 0.0) {
				double *new_vert = tmp_vert[tmpnr++];
				outlist[outcount++] = new_vert;

				if (dp < 0.0) {
					double t = dp / (dp - dp_prev);
					lerp (new_vert, t, vert_prev, vert);
				}
				else {
					double t = dp_prev / (dp_prev - dp);
					lerp (new_vert, t, vert, vert_prev);
				}
			}

			vert_prev = vert;
			dp_prev = dp;
		}

		/* swap in/out lists */
		{
			double **tmp = inlist;
			inlist = outlist;
			outlist = tmp;
			n = outcount;
		}
	}

	if (n >= 3) {
		for (i = 0; i < n; i++) {
			double *v = inlist[i];
			double w = 1.0 / v[3];

			bounds = bounds.ExtendTo (v[0] * w, v[1] * w);
		}
	}

	return bounds;
}

Rect
Projection::ProjectBounds (Rect bounds)
{
	Rect     r = bounds.RoundOut ();
	double   p1[4] = { 0.0, 0.0, 0.0, 1.0 };
	double   p2[4] = { r.width, 0.0, 0.0, 1.0 };
	double   p3[4] = { r.width, r.height, 0.0, 1.0 };
	double   p4[4] = { 0.0, r.height, 0.0, 1.0 };
	double   m[16];
	unsigned cm1;
	unsigned cm2;
	unsigned cm3;
	unsigned cm4;

	SetObjectSize (r.width, r.height);

	GetTransform (m);

	Matrix3D::TransformPoint (p1, m, p1);
	Matrix3D::TransformPoint (p2, m, p2);
	Matrix3D::TransformPoint (p3, m, p3);
	Matrix3D::TransformPoint (p4, m, p4);

	cm1 = clipmask (p1);
	cm2 = clipmask (p2);
	cm3 = clipmask (p3);
	cm4 = clipmask (p4);

	if ((cm1 | cm2 | cm3 | cm4) != 0) {
		bounds = Rect ();
		if ((cm1 & cm2 & cm3 & cm4) == 0) {
			bounds = ExtendBoundsTo (bounds, p1, p2, p3, cm1 | cm2 | cm3);
			bounds = ExtendBoundsTo (bounds, p1, p3, p4, cm1 | cm3 | cm4);
		}
	}
	else {
		p1[0] /= p1[3];
		p1[1] /= p1[3];

		p2[0] /= p2[3];
		p2[1] /= p2[3];

		p3[0] /= p3[3];
		p3[1] /= p3[3];

		p4[0] /= p4[3];
		p4[1] /= p4[3];

		bounds = Rect (p1[0], p1[1], 0, 0);
		bounds = bounds.ExtendTo (p2[0], p2[1]);
		bounds = bounds.ExtendTo (p3[0], p3[1]);
		bounds = bounds.ExtendTo (p4[0], p4[1]);
	}

	bounds.x += r.x;
	bounds.y += r.y;

	return bounds;
}

void
Projection::UpdateProjection ()
{
	g_warning ("Projection::UpdateProjection has been called. "
		   "The derived class should have overridden it.");
}

void
Projection::MaybeUpdateProjection ()
{
	if (need_update) {
		UpdateProjection ();
		need_update = false;
	}
}

PlaneProjection::PlaneProjection ()
{
	SetObjectType (Type::PLANEPROJECTION);
	objectWidth = objectHeight = 1.0;
}

void
PlaneProjection::OnPropertyChanged (PropertyChangedEventArgs *args, MoonError *error)
{
	if (args->GetProperty ()->GetOwnerType() != Type::PLANEPROJECTION) {
		DependencyObject::OnPropertyChanged (args, error);
		return;
	}

	if (args->GetId () != PlaneProjection::ProjectionMatrixProperty)
		need_update = true;

	NotifyListenersOfPropertyChange (args, error);
}

void
PlaneProjection::SetObjectSize (double width, double height)
{
	width = MAX (width, 1.0);
	height = MAX (height, 1.0);

	if (width != objectWidth || height != objectHeight) {
		objectWidth = width;
		objectHeight = height;
		need_update = true;
	}
}

#define FIELD_OF_VIEW (57.0 / 180.0 * M_PI)
#define CAMERA_DIST   999.0
#define NEAR_VAL      1.0
#define FAR_VAL       1000000.0

double
PlaneProjection::DistanceFromXYPlane ()
{
	const double xyPlaneZ = (NEAR_VAL * FAR_VAL / (NEAR_VAL - FAR_VAL) * (1.0 - CAMERA_DIST)) / CAMERA_DIST;
	double p[4] = { objectWidth / 2.0, objectHeight / 2.0, 0.0, 1.0 };
	double m[16];

	GetTransform (m);
	Matrix3D::TransformPoint (p, m, p);

	if (p[3] == 0.0)
		return 0.0;

	return xyPlaneZ - (p[2] / p[3]);
}

void
PlaneProjection::UpdateProjection ()
{
	Value *rotationX = GetValue (RotationXProperty);
	Value *rotationY = GetValue (RotationYProperty);
	Value *rotationZ = GetValue (RotationZProperty);
	double radiansX = rotationX ? rotationX->AsDouble () / 180.0 * M_PI : 0.0;
	double radiansY = rotationY ? rotationY->AsDouble () / 180.0 * M_PI : 0.0;
	double radiansZ = rotationZ ? rotationZ->AsDouble () / 180.0 * M_PI : 0.0;
	Value *globalOffsetX = GetValue (GlobalOffsetXProperty);
	Value *globalOffsetY = GetValue (GlobalOffsetYProperty);
	Value *globalOffsetZ = GetValue (GlobalOffsetZProperty);
	double globalX = globalOffsetX ? globalOffsetX->AsDouble () : 0.0;
	double globalY = globalOffsetY ? globalOffsetY->AsDouble () : 0.0;
	double globalZ = globalOffsetZ ? globalOffsetZ->AsDouble () : 0.0;
	Value *localOffsetX = GetValue (LocalOffsetXProperty);
	Value *localOffsetY = GetValue (LocalOffsetYProperty);
	Value *localOffsetZ = GetValue (LocalOffsetZProperty);
	double localX = localOffsetX ? localOffsetX->AsDouble () : 0.0;
	double localY = localOffsetY ? localOffsetY->AsDouble () : 0.0;
	double localZ = localOffsetZ ? localOffsetZ->AsDouble () : 0.0;

	const double fovY = FIELD_OF_VIEW;
	const double cameraZ = CAMERA_DIST;

	double height = 2.0 * cameraZ * tan (fovY / 2.0);
	double scale = height / objectHeight;

	double toCenter[16];
	double invertY[16];
	double localOffset[16];
	double rotateX[16];
	double rotateY[16];
	double rotateZ[16];
	double toCamera[16];
	double perspective[16];
	double zoom[16];
	double viewport[16];
	double m[16];

	Matrix3D::Translate (toCenter,
			     -objectWidth * GetCenterOfRotationX (),
			     -objectHeight * GetCenterOfRotationY (),
			     -GetCenterOfRotationZ ());
	Matrix3D::Scale (invertY, 1.0, -1.0, 1.0);
	Matrix3D::Translate (localOffset, localX, localY, localZ);
	Matrix3D::RotateX (rotateX, radiansX);
	Matrix3D::RotateY (rotateY, -radiansY);
	Matrix3D::RotateZ (rotateZ, radiansZ);
	Matrix3D::Translate (toCamera,
			     objectWidth * (GetCenterOfRotationX () - 0.5) + globalX,
			     -objectHeight * (GetCenterOfRotationY () - 0.5) - globalY,
			     GetCenterOfRotationZ () - cameraZ + globalZ);
	Matrix3D::Perspective (perspective,
			       fovY,
			       objectWidth / objectHeight,
			       NEAR_VAL,
			       FAR_VAL);
	Matrix3D::Scale (zoom, scale, scale, 1.0);
	Matrix3D::Viewport (viewport, objectWidth, objectHeight);

	Matrix3D::Multiply (m, toCenter, invertY);
	Matrix3D::Multiply (m, m, localOffset);
	Matrix3D::Multiply (m, m, rotateX);
	Matrix3D::Multiply (m, m, rotateY);
	Matrix3D::Multiply (m, m, rotateZ);
	Matrix3D::Multiply (m, m, toCamera);
	Matrix3D::Multiply (m, m, perspective);
	Matrix3D::Multiply (m, m, zoom);
	Matrix3D::Multiply (m, m, viewport);

	SetProjectionMatrix (new Matrix3D (m));
}

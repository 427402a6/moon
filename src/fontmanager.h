/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * fontmanager.h: 
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2009 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#ifndef __FONT_MANAGER_H__
#define __FONT_MANAGER_H__

#include <glib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "moon-path.h"
#include "collection.h"
#include "enums.h"

struct ManagedStreamCallbacks;
class FontManager;
class FontFace;

bool FontWeightIsBold (FontWeights weight);

struct GlyphMetrics {
	double horiBearingX;
	//double horiBearingY;
	double horiAdvance;
	//double height;
	//double width;
};

struct GlyphInfo {
	GlyphMetrics metrics;
	gunichar unichar;
	guint32 index;
	moon_path *path;
	FontFace *face;
	gint64 atime;
};

struct FontFaceExtents {
	double underline_thickness;
	double underline_position;
	double descent;
	double ascent;
	double height;
};


class FontFace {
	FontManager *manager;
	double cur_size;
	int ref_count;
	FT_Face face;
	char *key;
	
 public:
	FontFace (FontManager *manager, FT_Face face, char *key);
	~FontFace ();
	
	void ref ();
	void unref ();
	
	const char *GetFamilyName ();
	const char *GetStyleName ();
	
	int GetMajorVersion ();
	int GetMinorVersion ();
	
	bool IsScalable ();
	bool IsItalic ();
	bool IsBold ();
	
	gunichar GetCharFromIndex (guint32 index);
	guint32 GetCharIndex (gunichar unichar);
	bool HasChar (gunichar unichar);
	
	void GetExtents (double size, bool gapless, FontFaceExtents *extents);
	double Kerning (double size, guint32 left, guint32 right);
	bool LoadGlyph (double size, GlyphInfo *glyph, StyleSimulations simulate = StyleSimulationsNone);
};

/* @IncludeInKinds */
/* @Namespace=None */
class GlyphTypeface {
	char *resource;
	int ver_major;
	int ver_minor;
	
 public:
	GlyphTypeface (const char *path, int index, int major, int minor);
	GlyphTypeface (const GlyphTypeface *typeface);
	~GlyphTypeface ();
	
	const char *GetFontResource () { return resource; }
	
	//
	// Public Accessors
	//
	int GetMajorVersion () { return ver_major; }
	int GetMinorVersion () { return ver_minor; }
	const char *GetFontUri ();
	
	bool operator== (const GlyphTypeface &v) const;
};

/* @IncludeInKinds */
/* @Namespace=None */
class GlyphTypefaceCollection : public Collection {
 protected:
	virtual ~GlyphTypefaceCollection () { }
	
 public:
	GlyphTypefaceCollection ();
	
	virtual Type::Kind GetElementType () { return Type::GLYPHTYPEFACE; }
};

class FontManager {
	friend class FontFace;
	
	GHashTable *resources;
	GHashTable *faces;
	GHashTable *system_faces;
	FT_Library libft2;
	char *root;
	double dpi;
	
	FontFace *OpenFontResource (const char *resource, const char *family, int index, FontStretches stretch, FontWeights weight, FontStyles style);
	FontFace *OpenSystemFont (const char *family, FontStretches stretch, FontWeights weight, FontStyles style);
	FontFace *OpenFontFace (const char *filename, const char *guid, int index);
	
 public:
	FontManager ();
	~FontManager ();
	
	void AddResource (const char *resource, const char *path);
	char *AddResource (ManagedStreamCallbacks *stream);
	
	FontFace *OpenFont (const char *name, FontStretches stretch, FontWeights weight, FontStyles style);
	FontFace *OpenFont (const char *name, int index);
	
	GlyphTypefaceCollection *GetSystemGlyphTypefaces ();
};

#endif /* __FONT_MANAGER_H__ */

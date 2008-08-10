/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * text.h: 
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#ifndef __TEXT_H__
#define __TEXT_H__

#include <glib.h>

G_BEGIN_DECLS

#include <cairo.h>

#include <frameworkelement.h>
#include <downloader.h>
#include <moon-path.h>
#include <layout.h>
#include <brush.h>
#include <font.h>

#define TEXTBLOCK_FONT_FAMILY  "Portable User Interface"
#define TEXTBLOCK_FONT_STRETCH FontStretchesNormal
#define TEXTBLOCK_FONT_WEIGHT  FontWeightsNormal
#define TEXTBLOCK_FONT_STYLE   FontStylesNormal
#define TEXTBLOCK_FONT_SIZE    14.666666984558105

void text_shutdown (void);

class Inline : public DependencyObject {
 protected:
	virtual ~Inline ();

 public:
 	/* @PropertyType=char*,DefaultValue=TEXTBLOCK_FONT_FAMILY */
	static DependencyProperty *FontFamilyProperty;
 	/* @PropertyType=double,DefaultValue=TEXTBLOCK_FONT_SIZE */
	static DependencyProperty *FontSizeProperty;
 	/* @PropertyType=gint32,DefaultValue=TEXTBLOCK_FONT_STRETCH */
	static DependencyProperty *FontStretchProperty;
 	/* @PropertyType=gint32,DefaultValue=TEXTBLOCK_FONT_STYLE */
	static DependencyProperty *FontStyleProperty;
 	/* @PropertyType=gint32,DefaultValue=TEXTBLOCK_FONT_WEIGHT */
	static DependencyProperty *FontWeightProperty;
 	/* @PropertyType=Brush */
	static DependencyProperty *ForegroundProperty;
 	/* @PropertyType=gint32,DefaultValue=TextDecorationsNone */
	static DependencyProperty *TextDecorationsProperty;
	
	TextFontDescription *font;
	
	Brush *foreground;
	bool autogen;
	
	/* @GenerateCBinding */
	Inline ();
	virtual Type::Kind GetObjectType () { return Type::INLINE; }
	virtual Value *GetDefaultValue (DependencyProperty *prop);
	virtual void OnPropertyChanged (PropertyChangedEventArgs *args);
	virtual void OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args);
};

const char *inline_get_font_family (Inline *inline_);
void inline_set_font_family (Inline *inline_, const char *value);

double inline_get_font_size (Inline *inline_);
void inline_set_font_size (Inline *inline_, double value);

FontStretches inline_get_font_stretch (Inline *inline_);
void inline_set_font_stretch (Inline *inline_, FontStretches value);

FontStyles inline_get_font_style (Inline *inline_);
void inline_set_font_style (Inline *inline_, FontStyles value);

FontWeights inline_get_font_weight (Inline *inline_);
void inline_set_font_weight (Inline *inline_, FontWeights value);

Brush *inline_get_foreground (Inline *inline_);
void inline_set_foreground (Inline *inline_, Brush *value);

TextDecorations inline_get_text_decorations (Inline *inline_);
void inline_set_text_decorations (Inline *inline_, TextDecorations value);


class LineBreak : public Inline {
 protected:
	virtual ~LineBreak () {}

 public:
	/* @GenerateCBinding */
	LineBreak () { }
	virtual Type::Kind GetObjectType () { return Type::LINEBREAK; };
};


/* @ContentProperty="Text" */
class Run : public Inline {
 protected:
	virtual ~Run () {}

 public:
 	/* @PropertyType=char* */
	static DependencyProperty *TextProperty;
	
	/* @GenerateCBinding */
	Run () { }
	virtual Type::Kind GetObjectType () { return Type::RUN; };
	
	// property accessors
	void SetText (const char *text);
	const char *GetText ();
};

const char *run_get_text (Run *run);
void run_set_text (Run *run, const char *value);


/* @ContentProperty="Inlines" */
class TextBlock : public FrameworkElement {
	TextFontDescription *font;
	TextLayout *layout;
	Downloader *downloader;
	
	double actual_height;
	double actual_width;
	bool setvalue;
	bool dirty;
	
	void SetActualHeight (double height);
	void SetActualWidth (double width);
	
	void CalcActualWidthHeight (cairo_t *cr);
	void Layout (cairo_t *cr);
	void Paint (cairo_t *cr);
	
	char *GetTextInternal ();
	bool SetTextInternal (const char *text);
	
	double GetBoundingWidth ()
	{
		double actual = GetActualWidth ();
		Value *value;
		
		if (!(value = GetValueNoDefault (FrameworkElement::WidthProperty)))
			return actual;
		
		if (value->AsDouble () > actual)
			return value->AsDouble ();
		
		return actual;
	}
	
	double GetBoundingHeight ()
	{
		double actual = GetActualHeight ();
		Value *value;
		
		if (!(value = GetValueNoDefault (FrameworkElement::HeightProperty)))
			return actual;
		
		if (value->AsDouble () > actual)
			return value->AsDouble ();
		
		return actual;
	}
	
	void DownloaderComplete ();
	
	static void data_write (void *data, gint32 offset, gint32 n, void *closure);
	static void downloader_complete (EventObject *sender, EventArgs *calldata, gpointer closure);
	static void size_notify (gint64 size, gpointer data);
	
 protected:
	virtual ~TextBlock ();

 public:
 	/* @PropertyType=double,ReadOnly */
	static DependencyProperty *ActualHeightProperty;
 	/* @PropertyType=double,ReadOnly */
	static DependencyProperty *ActualWidthProperty;
 	/* @PropertyType=char*,DefaultValue=TEXTBLOCK_FONT_FAMILY */
	static DependencyProperty *FontFamilyProperty;
 	/* @PropertyType=double,DefaultValue=TEXTBLOCK_FONT_SIZE */
	static DependencyProperty *FontSizeProperty;
 	/* @PropertyType=gint32,DefaultValue=TEXTBLOCK_FONT_STRETCH */
	static DependencyProperty *FontStretchProperty;
 	/* @PropertyType=gint32,DefaultValue=TEXTBLOCK_FONT_STYLE */
	static DependencyProperty *FontStyleProperty;
 	/* @PropertyType=gint32,DefaultValue=TEXTBLOCK_FONT_WEIGHT */
	static DependencyProperty *FontWeightProperty;
 	/* @PropertyType=Brush */
	static DependencyProperty *ForegroundProperty;
 	/* @PropertyType=Inlines */
	static DependencyProperty *InlinesProperty;
 	/* @PropertyType=char* */
	static DependencyProperty *TextProperty;
 	/* @PropertyType=gint32,DefaultValue=TextDecorationsNone */
	static DependencyProperty *TextDecorationsProperty;
 	/* @PropertyType=gint32,DefaultValue=TextWrappingNoWrap */
	static DependencyProperty *TextWrappingProperty;
	
	/* @GenerateCBinding */
	TextBlock ();
	virtual Type::Kind GetObjectType () { return Type::TEXTBLOCK; };
	
	void SetFontSource (Downloader *downloader);
	
	//
	// Overrides
	//
	virtual void Render (cairo_t *cr, int x, int y, int width, int height);
	virtual void GetSizeForBrush (cairo_t *cr, double *width, double *height);
	virtual void ComputeBounds ();
	virtual bool InsideObject (cairo_t *cr, double x, double y);
	virtual Point GetTransformOrigin ();
	virtual void OnPropertyChanged (PropertyChangedEventArgs *args);
	virtual void OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args);
	virtual void OnCollectionItemChanged (Collection *col, DependencyObject *obj, PropertyChangedEventArgs *args);
	virtual void OnCollectionChanged (Collection *col, CollectionChangedEventArgs *args);
	
	virtual Value *GetValue (DependencyProperty *property);
	
	//
	// Property Accessors
	//
	double GetActualWidth ()
	{
		if (dirty)
			CalcActualWidthHeight (NULL);
		return actual_width;
	}
	
	double GetActualHeight ()
	{
		if (dirty)
			CalcActualWidthHeight (NULL);
		return actual_height;
	}
	
	void SetFontFamily (const char *family);
	const char *GetFontFamily ();
	
	void SetFontSize (double size);
	double GetFontSize ();
	
	void SetFontStretch (FontStretches stretch);
	FontStretches GetFontStretch ();
	
	void SetFontStyle (FontStyles style);
	FontStyles GetFontStyle ();
	
	void SetFontWeight (FontWeights weight);
	FontWeights GetFontWeight ();
	
	void SetForeground (Brush *fg);
	Brush *GetForeground ();
	
	void SetInlines (Inlines *inlines);
	Inlines *GetInlines ();
	
	void SetText (const char *text);
	const char *GetText ();
	
	void SetTextDecorations (TextDecorations decorations);
	TextDecorations GetTextDecorations ();
	
	void SetTextWrapping (TextWrapping wrapping);
	TextWrapping GetTextWrapping ();
};

double text_block_get_actual_height (TextBlock *textblock);
double text_block_get_actual_width (TextBlock *textblock);

const char *text_block_get_font_family (TextBlock *textblock);
void text_block_set_font_family (TextBlock *textblock, const char *family);

double text_block_get_font_size (TextBlock *textblock);
void text_block_set_font_size (TextBlock *textblock, double size);

FontStretches text_block_get_font_stretch (TextBlock *textblock);
void text_block_set_font_stretch (TextBlock *textblock, FontStretches stretch);

FontStyles text_block_get_font_style (TextBlock *textblock);
void text_block_set_font_style (TextBlock *textblock, FontStyles style);

FontWeights text_block_get_font_weight (TextBlock *textblock);
void text_block_set_font_weight (TextBlock *textblock, FontWeights weight);

Brush *text_block_get_foreground (TextBlock *textblock);
void text_block_set_foreground (TextBlock *textblock, Brush *foreground);

Inlines *text_block_get_inlines (TextBlock *textblock);
void text_block_set_inlines (TextBlock *textblock, Inlines *inlines);

const char *text_block_get_text (TextBlock *textblock);
void text_block_set_text (TextBlock *textblock, const char *text);

TextDecorations text_block_get_text_decorations (TextBlock *textblock);
void text_block_set_text_decorations (TextBlock *textblock, TextDecorations decorations);

TextWrapping text_block_get_text_wrapping (TextBlock *textblock);
void text_block_set_text_wrapping (TextBlock *textblock, TextWrapping wrapping);

void text_block_set_font_source (TextBlock *textblock, Downloader *downloader);


class Glyphs : public FrameworkElement {
	TextFontDescription *desc;
	Downloader *downloader;
	
	moon_path *path;
	gunichar *text;
	List *attrs;
	Brush *fill;
	int index;
	
	double origin_x;
	double origin_y;
	double height;
	double width;
	double left;
	double top;
	
	int origin_y_specified:1;
	int simulation_none:1;
	int uri_changed:1;
	int invalid:1;
	int dirty:1;
	
	void Layout ();
	void SetIndicesInternal (const char *in);
	
	void DownloaderComplete ();
	
	static void data_write (void *data, gint32 offset, gint32 n, void *closure);
	static void downloader_complete (EventObject *sender, EventArgs *calldata, gpointer closure);
	static void size_notify (gint64 size, gpointer data);
	
	void DownloadFont (Surface *surface, const char *url);
	
 protected:
	virtual ~Glyphs ();
	
 public:
 	/* @PropertyType=Brush */
	static DependencyProperty *FillProperty;
 	/* @PropertyType=double,DefaultValue=0.0 */
	static DependencyProperty *FontRenderingEmSizeProperty;
 	/* @PropertyType=char* */
	static DependencyProperty *FontUriProperty;
 	/* @PropertyType=char* */
	static DependencyProperty *IndicesProperty;
 	/* @PropertyType=double,DefaultValue=0.0 */
	static DependencyProperty *OriginXProperty;
 	/* @PropertyType=double,DefaultValue=0.0 */
	static DependencyProperty *OriginYProperty;
 	/* @PropertyType=gint32,DefaultValue=StyleSimulationsNone */
	static DependencyProperty *StyleSimulationsProperty;
 	/* @PropertyType=char* */
	static DependencyProperty *UnicodeStringProperty;
	
	/* @GenerateCBinding */
	Glyphs ();
	
	virtual Type::Kind GetObjectType () { return Type::GLYPHS; };
	
	virtual void GetSizeForBrush (cairo_t *cr, double *width, double *height);
	virtual void Render (cairo_t *cr, int x, int y, int width, int height);
	virtual void ComputeBounds ();
	virtual bool InsideObject (cairo_t *cr, double x, double y);
	virtual Point GetTransformOrigin ();
	virtual Point GetOriginPoint ();
	virtual void SetSurface (Surface *surface);
	virtual void OnPropertyChanged (PropertyChangedEventArgs *args);
	virtual void OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args);
	
	//
	// Property Accessors
	//
	void SetFill (Brush *fill);
	Brush *GetFill ();
	
	void SetFontRenderingEmSize (double size);
	double GetFontRenderingEmSize ();
	
	void SetFontUri (const char *uri);
	const char *GetFontUri ();
	
	void SetIndices (const char *indices);
	const char *GetIndices ();
	
	void SetOriginX (double origin);
	double GetOriginX ();
	
	void SetOriginY (double origin);
	double GetOriginY ();
	
	void SetStyleSimulations (StyleSimulations style);
	StyleSimulations GetStyleSimulations ();
	
	void SetUnicodeString (const char *unicode);
	const char *GetUnicodeString ();
};

Brush *glyphs_get_fill (Glyphs *glyphs);
void glyphs_set_fill (Glyphs *glyphs, Brush *fill);

double glyphs_get_font_rendering_em_size (Glyphs *glyphs);
void glyphs_set_font_rendering_em_size (Glyphs *glyphs, double size);

const char *glyphs_get_font_uri (Glyphs *glyphs);
void glyphs_set_font_uri (Glyphs *glyphs, const char *uri);

const char *glyphs_get_indices (Glyphs *glyphs);
void glyphs_set_indices (Glyphs *glyphs, const char *indices);

double glyphs_get_origin_x (Glyphs *glyphs);
void glyphs_set_origin_x (Glyphs *glyphs, double origin);

double glyphs_get_origin_y (Glyphs *glyphs);
void glyphs_set_origin_y (Glyphs *glyphs, double origin);

StyleSimulations glyphs_get_style_simulations (Glyphs *glyphs);
void glyphs_set_style_simulations (Glyphs *glyphs, StyleSimulations style);

const char *glyphs_get_unicode_string (Glyphs *glyphs);
void glyphs_set_unicode_string (Glyphs *glyphs, const char *unicode);

G_END_DECLS

#endif /* __TEXT_H__ */

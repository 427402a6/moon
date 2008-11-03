/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * layout.cpp: 
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2008 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include "moon-path.h"
#include "layout.h"
#include "debug.h"



#define BBOX_MARGIN 1.0
#define BBOX_PADDING 2.0

/*
 * Silverlight does not apply any kerning on a DOT, so we exclude them
 * 	U+002E FULL STOP
 * 	U+06D4 ARABIC FULL STOP
 *	U+3002 IDEOGRAPHIC FULL STOP
 * Note: this is different than using the "sliding dot" algorithm from
 * http://www.freetype.org/freetype2/docs/glyphs/glyphs-4.html
 */
#define APPLY_KERNING(uc)	((uc != 0x002E) && (uc != 0x06D4) && (uc != 3002))

TextRun::TextRun (const gunichar *ucs4, int len, TextDecorations deco, TextFontDescription *font, Brush **fg)
{
	text = (gunichar *) g_malloc (sizeof (gunichar) * (len + 1));
	memcpy (text, ucs4, sizeof (gunichar) * len);
	text[len] = 0;
	
	this->font = font->GetFont ();
	this->deco = deco;
	this->fg = fg;
}

TextRun::TextRun (const char *utf8, int len, TextDecorations deco, TextFontDescription *font, Brush **fg)
{
	register gunichar *s, *d;
	
	d = this->text = g_utf8_to_ucs4_fast (utf8, len, NULL);
	
	// convert all ascii lwsp into a SPACE, conserving only \n's
	for (s = this->text; *s; s++) {
		if (g_unichar_isspace (*s)) {
			if (*s == '\n')
				*d++ = *s;
			else if (*s < 128)
				*d++ = ' ';
			else
				*d++ = *s;
		} else {
			*d++ = *s;
		}
	}
	
	*d = 0;
	
	this->font = font->GetFont ();
	this->deco = deco;
	this->fg = fg;
}

TextRun::TextRun (TextFontDescription *font)
{
	// This TextRun will represent a LineBreak
	this->deco = TextDecorationsNone;
	this->font = font->GetFont ();
	this->text = NULL;
	this->fg = NULL;
}

TextRun::~TextRun ()
{
	font->unref ();
	g_free (text);
}



class TextSegment : public List::Node {
 public:
	moon_path *path;
	double advance;
	double width;
	TextRun *run;
	int start;
	int end;
	
	TextSegment (TextRun *run, int start);
	~TextSegment ();
};

TextSegment::TextSegment (TextRun *run, int start)
{
	this->advance = 0.0;
	this->width = 0.0;
	this->start = start;
	this->end = start;
	this->path = NULL;
	this->run = run;
}

TextSegment::~TextSegment ()
{
	if (path)
		moon_path_destroy (path);
}




class TextLine : public List::Node {
 public:
	List *segments;
	double descend;
	double height;
	double width;
	
	TextLine ();
	~TextLine ();
};

TextLine::TextLine ()
{
	segments = new List ();
	descend = 0.0;
	height = -1.0;
	width = 0.0;
}

TextLine::~TextLine ()
{
	segments->Clear (true);
	delete segments;
}




TextLayout::TextLayout ()
{
	wrapping = TextWrappingNoWrap;
	max_height = -1.0;
	max_width = -1.0;
	
	runs = NULL;
	
	lines = new List ();
	
	actual_height = -1.0;
	actual_width = -1.0;
}

TextLayout::~TextLayout ()
{
	if (runs) {
		runs->Clear (true);
		delete runs;
	}
	
	lines->Clear (true);
	delete lines;
}

double
TextLayout::GetMaxWidth ()
{
	return max_width;
}

void
TextLayout::SetMaxWidth (double max)
{
	if (max_width == max)
		return;
	
	max_width = max;
	
	actual_height = -1.0;
	actual_width = -1.0;
}

double
TextLayout::GetMaxHeight ()
{
	return max_height;
}

void
TextLayout::SetMaxHeight (double max)
{
	if (max_height == max)
		return;
	
	max_height = max;
	
	actual_height = -1.0;
	actual_width = -1.0;
}

TextWrapping
TextLayout::GetWrapping ()
{
	return wrapping;
}

void
TextLayout::SetWrapping (TextWrapping wrapping)
{
	if (this->wrapping == wrapping)
		return;
	
	this->wrapping = wrapping;
	
	actual_height = -1.0;
	actual_width = -1.0;
}

List *
TextLayout::GetTextRuns ()
{
	return runs;
}

void
TextLayout::SetTextRuns (List *runs)
{
	if (this->runs) {
		this->runs->Clear (true);
		delete this->runs;
	}
	
	this->runs = runs;
	
	actual_height = -1.0;
	actual_width = -1.0;
}

/**
 * TextLayout::GetActualExtents:
 * @width:
 * @height:
 *
 * Gets the actual width and height extents required for rendering the
 * full text.
 **/
void
TextLayout::GetActualExtents (double *width, double *height)
{
	*height = actual_height;
	*width = actual_width;
}

#if 0
/**
 * TextLayout::GetLayoutExtents:
 * @width:
 * @height:
 *
 * Gets the width and height extents suitable for rendering the text
 * w/ the current wrapping model.
 **/
void
TextLayout::GetLayoutExtents (double *width, double *height)
{
	*height = bbox_height;
	*width = bbox_width;
}
#endif


#if DEBUG
static void
print_run_text (const char *msg, gunichar *start, gunichar *end)
{
	GString *str = g_string_new ("");
	
	while (*start && (end ? start < end : true)) {
		g_string_append_unichar (str, *start == 0xA0 ? '_' : *start);
		start++;
	}
	
	printf ("%s = \"%s\"\n", msg, str->str);
	
	g_string_free (str, true);
}

static const char *unicode_break_types[] = {
	"G_UNICODE_BREAK_MANDATORY",
	"G_UNICODE_BREAK_CARRIAGE_RETURN",
	"G_UNICODE_BREAK_LINE_FEED",
	"G_UNICODE_BREAK_COMBINING_MARK",
	"G_UNICODE_BREAK_SURROGATE",
	"G_UNICODE_BREAK_ZERO_WIDTH_SPACE",
	"G_UNICODE_BREAK_INSEPARABLE",
	"G_UNICODE_BREAK_NON_BREAKING_GLUE",
	"G_UNICODE_BREAK_CONTINGENT",
	"G_UNICODE_BREAK_SPACE",
	"G_UNICODE_BREAK_AFTER",
	"G_UNICODE_BREAK_BEFORE",
	"G_UNICODE_BREAK_BEFORE_AND_AFTER",
	"G_UNICODE_BREAK_HYPHEN",
	"G_UNICODE_BREAK_NON_STARTER",
	"G_UNICODE_BREAK_OPEN_PUNCTUATION",
	"G_UNICODE_BREAK_CLOSE_PUNCTUATION",
	"G_UNICODE_BREAK_QUOTATION",
	"G_UNICODE_BREAK_EXCLAMATION",
	"G_UNICODE_BREAK_IDEOGRAPHIC",
	"G_UNICODE_BREAK_NUMERIC",
	"G_UNICODE_BREAK_INFIX_SEPARATOR",
	"G_UNICODE_BREAK_SYMBOL",
	"G_UNICODE_BREAK_ALPHABETIC",
	"G_UNICODE_BREAK_PREFIX",
	"G_UNICODE_BREAK_POSTFIX",
	"G_UNICODE_BREAK_COMPLEX_CONTEXT",
	"G_UNICODE_BREAK_AMBIGUOUS",
	"G_UNICODE_BREAK_UNKNOWN",
	"G_UNICODE_BREAK_NEXT_LINE",
	"G_UNICODE_BREAK_WORD_JOINER",
	"G_UNICODE_BREAK_HANGUL_L_JAMO",
	"G_UNICODE_BREAK_HANGUL_V_JAMO",
	"G_UNICODE_BREAK_HANGUL_T_JAMO",
	"G_UNICODE_BREAK_HANGUL_LV_SYLLABLE",
	"G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE"
};

static void
print_break_info (gunichar *text)
{
	register gunichar *inptr = text;
	GUnicodeBreakType btype;
	char c[7];
	int i;
	
	printf ("Unicode break info:\n");
	
	while (*inptr) {
		btype = g_unichar_break_type (*inptr);
		i = g_unichar_to_utf8 (*inptr, c);
		c[i] = '\0';
		
		printf ("\t%u %s: break type = %s\n", *inptr, c,
			unicode_break_types[btype]);
		
		inptr++;
	}
}
#endif


#define BreakSpace(btype) (btype == G_UNICODE_BREAK_SPACE || btype == G_UNICODE_BREAK_ZERO_WIDTH_SPACE)
#define BreakAfter(btype) (btype == G_UNICODE_BREAK_AFTER || btype == G_UNICODE_BREAK_NEXT_LINE)
#define BreakBefore(btype) (btype == G_UNICODE_BREAK_BEFORE || btype == G_UNICODE_BREAK_PREFIX)

void
TextLayout::LayoutWrapWithOverflow (TextLayoutHints *hints)
{
	double x0 = 0.0, x1 = 0.0, wx = 0.0, dy = 0.0;
	register gunichar *start, *word, *inptr;
	GUnicodeBreakType btype;
	bool underlined = false;
	TextSegment *segment;
	double descend = 0.0;
	double height = 0.0;
	double width = 0.0;
	bool blank = true;
	GlyphInfo *glyph;
	TextLine *line;
	double advance;
	guint32 prev;
	TextRun *run;
	
	if (hints->OverrideLineHeight ())
		height = hints->GetLineHeight ();
	
	line = new TextLine ();
	for (run = (TextRun *) runs->First (); run; run = (TextRun *) run->next) {
		if (run->text == NULL) {
			// LineBreak
			if (blank && !hints->OverrideLineHeight ()) {
				descend = run->font->Descender ();
				height = run->font->Height ();
			}
			
			line->descend = descend;
			line->height = height;
			line->width = width;
			
			lines->Append (line);
			
			dy += height;
			
			// blank lines do not count toward 'ActualHeight' extents
			// unless underlined
			if (!blank || underlined)
				actual_height = dy;
			
			if (run->next)
				line = new TextLine ();
			else
				line = NULL;
			
			underlined = false;
			blank = true;
			
			if (!hints->OverrideLineHeight ()) {
				descend = 0.0;
				height = 0.0;
			}
			
			width = 0.0;
			x0 = 0.0;
			x1 = 0.0;
			
			continue;
		}
		
		if (!underlined)
			underlined = run->IsUnderlined ();
		
		if (!hints->OverrideLineHeight ()) {
			descend = MIN (descend, run->font->Descender ());
			height = MAX (height, run->font->Height ());
		}
		
		segment = new TextSegment (run, 0);
		inptr = start = run->text;
		prev = 0;
		x1 = x0;
		
		do {
			// always include the lwsp, it is allowed to go past max_width
			btype = g_unichar_break_type (*inptr);
			while (BreakSpace (btype)) {
				if ((glyph = run->font->GetGlyphInfo (*inptr))) {
					if ((advance = glyph->metrics.horiAdvance) > 0.0) {
						if ((prev != 0) && APPLY_KERNING (*inptr))
							advance += run->font->Kerning (prev, glyph->index);
						else if (glyph->metrics.horiBearingX < 0)
							advance -= glyph->metrics.horiBearingX;
					}
					
					prev = glyph->index;
					x1 += advance;
				}
				
				inptr++;
				
				btype = g_unichar_break_type (*inptr);
			}
			
			// trailing lwsp only counts toward 'ActualWidth' extents if underlined
			if (run->IsUnderlined ()) {
				actual_width = MAX (actual_width, x1);
				segment->width = x1 - x0;
				width = x1;
			}
			
			if (*inptr == 0)
				break;
			
			segment->advance = x1 - x0;
			word = inptr;
			wx = x1;
			
			if (max_width > 0.0 && x1 >= max_width) {
			linebreak:
				if (segment->start < (word - run->text)) {
					line->segments->Append (segment);
					
					segment = new TextSegment (run, word - run->text);
					start = word;
				} else {
					// reuse the segment
				}
				
				line->descend = descend;
				line->height = height;
				line->width = width;
				
				lines->Append (line);
				
				dy += height;
				
				// blank lines do not count toward 'ActualHeight' extents
				// unless underlined
				if (!blank || underlined)
					actual_height = dy;
				
				line = new TextLine ();
				blank = true;
				
				underlined = run->IsUnderlined ();
				
				if (!hints->OverrideLineHeight ()) {
					descend = run->font->Descender ();
					height = run->font->Height ();
				}
				
				width = 0.0;
				prev = 0;
				x0 = 0.0;
				x1 = 0.0;
				wx = 0.0;
			}
			
			// append this word onto the line
			inptr = word;
			btype = g_unichar_break_type (*inptr);
			while (*inptr && !BreakSpace (btype)) {
				if ((glyph = run->font->GetGlyphInfo (*inptr))) {
					if ((advance = glyph->metrics.horiAdvance) > 0.0) {
						if ((prev != 0) && APPLY_KERNING (*inptr))
							advance += run->font->Kerning (prev, glyph->index);
						else if (glyph->metrics.horiBearingX < 0)
							advance -= glyph->metrics.horiBearingX;
					}
					
					prev = glyph->index;
					x1 += advance;
					width = x1;
					
					if (max_width > 0.0 && x1 >= max_width && wx > 0.0)
						goto linebreak;
				}
				
				inptr++;
				
				btype = g_unichar_break_type (*inptr);
			}
			
			actual_width = MAX (actual_width, x1);
			segment->end = inptr - run->text;
			segment->width = x1 - x0;
			blank = false;
		} while (*inptr);
		
		segment->advance = x1 - x0;
		line->segments->Append (segment);
		
		x0 = x1;
	}
	
	if (line) {
		line->descend = descend;
		line->height = height;
		line->width = width;
		
		lines->Append (line);
		
		dy += height;
		
		// blank lines do not count toward 'ActualHeight' extents
		// unless underlined
		if (!blank || underlined)
			actual_height = dy;
	}
}

void
TextLayout::LayoutNoWrap (TextLayoutHints *hints)
{
	double x0 = 0.0, x1 = 0.0, dy = 0.0;
	register gunichar *inptr;
	GUnicodeBreakType btype;
	bool underlined = false;
	bool clipped = false;
	TextSegment *segment;
	double descend = 0.0;
	double height = 0.0;
	double width = 0.0;
	bool blank = true;
	GlyphInfo *glyph;
	TextLine *line;
	double advance;
	guint32 prev;
	TextRun *run;
	
	if (hints->OverrideLineHeight ())
		height = hints->GetLineHeight ();
	
	line = new TextLine ();
	for (run = (TextRun *) runs->First (); run; run = (TextRun *) run->next) {
		if (run->text == NULL) {
			// LineBreak
			if (blank && !hints->OverrideLineHeight ()) {
				descend = run->font->Descender ();
				height = run->font->Height ();
			}
			
			line->descend = descend;
			line->height = height;
			line->width = width;
			
			lines->Append (line);
			
			dy += height;
			
			// blank lines do not count toward 'ActualHeight' extents
			// unless underlined
			if (!blank || underlined)
				actual_height = dy;
			
			if (run->next)
				line = new TextLine ();
			else
				line = NULL;
			
			underlined = false;
			clipped = false;
			blank = true;
			
			if (!hints->OverrideLineHeight ()) {
				descend = 0.0;
				height = 0.0;
			}
			
			width = 0.0;
			x0 = 0.0;
			x1 = 0.0;
			
			continue;
		} else if (clipped) {
			// once we've clipped, we cannot append anymore text to the line
			// FIXME: Silverlight 2.0 doesn't seem to clip
			continue;
		}
		
		if (!underlined)
			underlined = run->IsUnderlined ();
		
		if (!hints->OverrideLineHeight ()) {
			descend = MIN (descend, run->font->Descender ());
			height = MAX (height, run->font->Height ());
		}
		
		segment = new TextSegment (run, 0);
		inptr = run->text;
		prev = 0;
		x1 = x0;
		
		do {
			// always include the lwsp, it is allowed to go past max_width
			btype = g_unichar_break_type (*inptr);
			while (BreakSpace (btype)) {
				if ((glyph = run->font->GetGlyphInfo (*inptr))) {
					if ((advance = glyph->metrics.horiAdvance) > 0.0) {
						if ((prev != 0) && APPLY_KERNING (*inptr))
							advance += run->font->Kerning (prev, glyph->index);
						else if (glyph->metrics.horiBearingX < 0)
							advance -= glyph->metrics.horiBearingX;
					}
					
					prev = glyph->index;
					x1 += advance;
				}
				
				inptr++;
				
				btype = g_unichar_break_type (*inptr);
			}
			
			// trailing lwsp only counts toward 'ActualWidth' extents if underlined
			if (run->IsUnderlined ()) {
				actual_width = MAX (actual_width, x1);
				segment->width = x1 - x0;
				width = x1;
			}
			
			if (*inptr == 0)
				break;
			
			// append this word onto the line
			btype = g_unichar_break_type (*inptr);
			while (*inptr && !BreakSpace (btype)) {
				if ((glyph = run->font->GetGlyphInfo (*inptr))) {
					if ((advance = glyph->metrics.horiAdvance) > 0.0) {
						if ((prev != 0) && APPLY_KERNING (*inptr))
							advance += run->font->Kerning (prev, glyph->index);
						else if (glyph->metrics.horiBearingX < 0)
							advance -= glyph->metrics.horiBearingX;
					}
					
					prev = glyph->index;
					x1 += advance;
					width = x1;
				}
				
				inptr++;
				
				btype = g_unichar_break_type (*inptr);
			}
			
			actual_width = MAX (actual_width, x1);
			segment->end = inptr - run->text;
			segment->width = x1 - x0;
			blank = false;
			
			if (max_width > 0.0 && x1 >= max_width) {
				// cut the remainder of the run unless it is underlined
				// (in which case we need to underline trailing lwsp).
				if (!run->IsUnderlined ()) {
					clipped = true;
					break;
				}
			}
		} while (*inptr);
		
		segment->advance = x1 - x0;
		line->segments->Append (segment);
		
		x0 = x1;
	}
	
	if (line) {
		line->descend = descend;
		line->height = height;
		line->width = width;
		
		lines->Append (line);
		
		dy += height;
		
		// blank lines do not count toward 'ActualHeight' extents
		// unless underlined
		if (!blank || underlined)
			actual_height = dy;
	}
}

static bool
isLastWord (TextRun *run, gunichar *word, bool *include_lwsp)
{
	register gunichar *inptr = word;
	
	// skip to the end of this word
	while (*inptr && *inptr != ' ')
		inptr++;
	
	// skip over trailing lwsp
	while (*inptr == ' ')
		inptr++;
	
	if (*inptr != 0)
		return false;
	
	// now we need to check following Runs
	while (run->next) {
		run = (TextRun *) run->next;
		
		if (!run->text)
			return true;
		
		inptr = run->text;
		
		// skip over lwsp
		while (*inptr == ' ')
			inptr++;
		
		if (*inptr != 0)
			return false;
	}
	
	*include_lwsp = true;
	
	return true;
}


struct WordChar {
	GUnicodeBreakType btype;
	gunichar *c;
	double x1;
};


/**
 * Notes: The last 'word' of any line must not be broken (bug in
 * Silverlight's text layout)
 **/
void
TextLayout::LayoutWrap (TextLayoutHints *hints)
{
	double x0 = 0.0, x1 = 0.0, wx = 0.0, dy = 0.0;
	register gunichar *start, *word, *inptr;
	GUnicodeBreakType btype;
	bool include_lwsp = false;
	bool underlined = false;
	bool last_word = false;
	bool in_word = false;
	TextSegment *segment;
	double word_end = 0.0;
	double descend = 0.0;
	double height = 0.0;
	double width = 0.0;
	bool blank = true;
	GlyphInfo *glyph;
	TextLine *line;
	double advance;
	GArray *array;
	guint32 prev;
	TextRun *run;
	WordChar wc;
	bool after;
	guint i;
	
	array = g_array_new (false, false, sizeof (WordChar));
	
	if (hints->OverrideLineHeight ())
		height = hints->GetLineHeight ();
	
	line = new TextLine ();
	for (run = (TextRun *) runs->First (); run; run = (TextRun *) run->next) {
		if (run->text == NULL) {
			// LineBreak
			if (blank && !hints->OverrideLineHeight ()) {
				descend = run->font->Descender ();
				height = run->font->Height ();
			}
			
			line->descend = descend;
			line->height = height;
			line->width = width;
			
			lines->Append (line);
			
			dy += height;
			
			// blank lines do not count toward 'ActualHeight' extents
			// unless underlined
			if (!blank || underlined)
				actual_height = dy;
			
			if (run->next)
				line = new TextLine ();
			else
				line = NULL;
			
			underlined = false;
			last_word = false;
			in_word = false;
			blank = true;
			
			if (!hints->OverrideLineHeight ()) {
				descend = 0.0;
				height = 0.0;
			}
			
			word_end = 0.0;
			width = 0.0;
			x0 = 0.0;
			x1 = 0.0;
			
			continue;
		}
		
		if (!underlined)
			underlined = run->IsUnderlined ();
		
		if (!hints->OverrideLineHeight ()) {
			descend = MIN (descend, run->font->Descender ());
			height = MAX (height, run->font->Height ());
		}
		
		segment = new TextSegment (run, 0);
#if DEBUG
		if (debug_flags & RUNTIME_DEBUG_LAYOUT) {
			print_run_text ("Laying out Run.Text", run->text, NULL);
			print_break_info (run->text);
		}
#endif
		inptr = run->text;
		prev = 0;
		x1 = x0;
		
		double bearing_adj = 0.0;
		do {
			// always include the lwsp, it is allowed to go past max_width
			start = inptr;
			btype = g_unichar_break_type (*inptr);
			if (in_word && BreakSpace (btype)) {
				in_word = false;
				word_end = x1;
			}
			
			while (BreakSpace (btype)) {
				if ((glyph = run->font->GetGlyphInfo (*inptr))) {
					advance = glyph->metrics.horiAdvance;
					if ((prev != 0) && APPLY_KERNING (*inptr))
						advance += run->font->Kerning (prev, glyph->index);
					else if (glyph->metrics.horiBearingX < 0) {
						bearing_adj = glyph->metrics.horiBearingX;
						advance += bearing_adj;
					}
					
					prev = glyph->index;
					x1 += advance - bearing_adj;
					bearing_adj = 0.0;
				}
				
				inptr++;
				
				btype = g_unichar_break_type (*inptr);
			}
			
			// trailing lwsp only counts toward 'ActualWidth' extents if underlined
			if (run->IsUnderlined () || include_lwsp) {
				actual_width = MAX (actual_width, x1);
				segment->width = x1 - x0;
				word_end = x1;
				width = x1;
			}
			
			if (*inptr == 0)
				break;
			
			segment->advance = x1 - x0;
			word = inptr;
			wx = x1;
			
			// check to see if this is the last word of the line
			last_word = isLastWord (run, word, &include_lwsp);
			
			if (max_width > 0.0 && x1 >= max_width) {
			linebreak:
				if (segment->advance > 0.0) {
					line->segments->Append (segment);
					
					segment = new TextSegment (run, word - run->text);
					start = word;
				} else {
					// reuse the segment
				}
				
				line->descend = descend;
				line->height = height;
				line->width = width;
				
				lines->Append (line);
				
				dy += height;
				
				// blank lines do not count toward 'ActualHeight' extents
				// unless underlined
				if (!blank || underlined)
					actual_height = dy;
				
				line = new TextLine ();
				blank = true;
				
				underlined = run->IsUnderlined ();
				
				if (!hints->OverrideLineHeight ()) {
					descend = run->font->Descender ();
					height = run->font->Height ();
				}
				
				in_word = false;
				word_end = 0.0;
				
				width = 0.0;
				prev = 0;
				x0 = 0.0;
				x1 = 0.0;
				wx = 0.0;
			}
			
			// append this word onto the line
			inptr = word;
			g_array_set_size (array, 0);
			btype = g_unichar_break_type (*inptr);
			while (*inptr && btype != G_UNICODE_BREAK_SPACE) {
				if (!(glyph = run->font->GetGlyphInfo (*inptr)))
					goto next;
				
				advance = glyph->metrics.horiAdvance;
				if ((prev != 0) && APPLY_KERNING (*inptr))
					advance += run->font->Kerning (prev, glyph->index);
				else if (glyph->metrics.horiBearingX < 0) {
					bearing_adj = glyph->metrics.horiBearingX;
					advance += bearing_adj;
				}
				
				if (max_width > 0.0 && (x1 + advance) > max_width) {
					if (wx == 0.0 && inptr > word && !last_word) {
						// break in the middle of a word
						// FIXME: need to respect unicode breaking
						actual_width = MAX (actual_width, x1);
						segment->end = inptr - run->text;
						segment->advance = x1 - x0;
						segment->width = x1 - x0;
						blank = false;
						word = inptr;
						goto linebreak;
					} else if (wx > 0.0) {
						// scan backwards for a char to break after
						i = array->len;
						after = false;
						
						while (i > 0 && !after) {
							wc = g_array_index (array, WordChar, i - 1);
							
							switch (wc.btype) {
							case G_UNICODE_BREAK_NEXT_LINE:
							case G_UNICODE_BREAK_UNKNOWN:
								after = true;
								break;
							case G_UNICODE_BREAK_BEFORE_AND_AFTER:
							case G_UNICODE_BREAK_EXCLAMATION:
								//case G_UNICODE_BREAK_AFTER:
								// only break after if there are chars before
								after = (i > 1);
								break;
							case G_UNICODE_BREAK_BEFORE:
								if (i > 1) {
									// break after the previous char
									wc = g_array_index (array, WordChar, i - 2);
									after = true;
								}
								break;
							case G_UNICODE_BREAK_WORD_JOINER:
								// only break if there is nothing before it
								after = (i == 1);
								break;
							default:
								// don't break here
								break;
							}
							
							i--;
						}
						
						if (after) {
							// break after a previous char in the word
							inptr = wc.c + 1;
							width = wc.x1;
							x1 = wc.x1;
							
							actual_width = MAX (actual_width, x1);
							segment->end = inptr - run->text;
							segment->advance = x1 - x0;
							segment->width = x1 - x0;
							blank = false;
							word = inptr;
							goto linebreak;
						} else {
							// break before this word
							segment->advance = wx - x0;
							segment->width = wx - x0;
							width = word_end;
							
							goto linebreak;
						}
					}
				}
				
				x1 += advance - bearing_adj;
				bearing_adj = 0.0;
				
			next:
				
				if (!run->font->HasGlyph (*inptr))
					wc.btype = G_UNICODE_BREAK_UNKNOWN;
				else
					wc.btype = btype;
				
				in_word = true;
				wc.c = inptr;
				wc.x1 = x1;
				width = x1;
				
				g_array_append_val (array, wc);
				
				inptr++;
				
				btype = g_unichar_break_type (*inptr);
			}
			
			actual_width = MAX (actual_width, x1);
			segment->end = inptr - run->text;
			segment->width = x1 - x0;
			blank = false;
		} while (*inptr);
		
		segment->advance = x1 - x0;
		line->segments->Append (segment);
		
		x0 = x1;
	}
	
	if (line) {
		line->descend = descend;
		line->height = height;
		line->width = width;
		
		lines->Append (line);
		
		dy += height;
		
		// blank lines do not count toward 'ActualHeight' extents
		// unless underlined
		if (!blank || underlined)
			actual_height = dy;
	}
	
	g_array_free (array, true);
}

#if DEBUG
static void
print_lines (List *lines)
{
	TextSegment *segment;
	TextLine *line;
	GString *str;
	int ln = 0;
	int i;
	
	printf ("layout results:\n");
	
	str = g_string_new ("");
	line = (TextLine *) lines->First ();
	
	while (line) {
		printf ("\tline #%d: ", ln);
		
		segment = (TextSegment *) line->segments->First ();
		
		while (segment) {
			for (i = segment->start; i < segment->end; i++)
				g_string_append_unichar (str, segment->run->text[i] == 0xA0 ? '_' : segment->run->text[i]);
			
			printf ("\"%s\", ", str->str);
			g_string_truncate (str, 0);
			
			segment = (TextSegment *) segment->next;
		}
		
		printf ("\n");
		
		line = (TextLine *) line->next;
		ln++;
	}
}
#endif

void
TextLayout::Layout (TextLayoutHints *hints)
{
	if (actual_width != -1.0)
		return;
	
	lines->Clear (true);
	actual_height = 0.0;
	actual_width = 0.0;
	
	if (!runs || runs->IsEmpty ())
		return;
	
	switch (wrapping) {
	case TextWrappingWrapWithOverflow:
#if DEBUG
		if (debug_flags & RUNTIME_DEBUG_LAYOUT) {
			if (max_width > 0.0)
				printf ("TextLayout::LayoutWrapWithOverflow(%f)\n", max_width);
			else
				printf ("TextLayout::LayoutWrapWithOverflow()\n");
		}
#endif
		LayoutWrapWithOverflow (hints);
		break;
	case TextWrappingNoWrap:
#if DEBUG
		if (debug_flags & RUNTIME_DEBUG_LAYOUT) {
			if (max_width > 0.0)
				printf ("TextLayout::LayoutWrapNoWrap(%f)\n", max_width);
			else
				printf ("TextLayout::LayoutNoWrap()\n");
		}
#endif
		LayoutNoWrap (hints);
		break;
	case TextWrappingWrap:
	// Silverlight default is to wrap for invalid values
	default:
#if DEBUG
		if (debug_flags & RUNTIME_DEBUG_LAYOUT) {
			if (max_width > 0.0)
				printf ("TextLayout::LayoutWrap(%f)\n", max_width);
			else
				printf ("TextLayout::LayoutWrap()\n");
		}
#endif
		LayoutWrap (hints);
		break;
	}
	
#if DEBUG
	if (debug_flags & RUNTIME_DEBUG_LAYOUT) {
		print_lines (lines);
		printf ("actualWidth = %f, actualHeight = %f\n", actual_width, actual_height);
	}
#endif
	
	//bbox_height = actual_height;
	//bbox_width = actual_width;
}

static inline void
RenderLine (cairo_t *cr, const Point &origin, const Point &position, TextLayoutHints *hints, TextLine *line, Brush *default_fg)
{
	TextFont *font = NULL;
	TextDecorations deco;
	TextSegment *segment;
	const gunichar *text;
	guint32 prev = 0;
	GlyphInfo *glyph;
	moon_path *path;
	double x1, y1;
	double x0, y0;
	Brush *fg;
	int size;
	int i;
	
	// set y0 to the line's baseline (descend is a negative value)
	y0 = position.y + line->height + line->descend;
	x0 = position.x;
	
	segment = (TextSegment *) line->segments->First ();
	
	while (segment) {
		text = segment->run->text;
		deco = segment->run->deco;
		font = segment->run->font;
		
		cairo_save (cr);
		cairo_translate (cr, x0, y0 - font->Ascender ());
		
		// set y1 to the baseline relative to the translation matrix
		y1 = font->Ascender ();
		x1 = 0.0;
		
		if (segment->run->fg && *segment->run->fg)
			fg = *segment->run->fg;
		else
			fg = default_fg;
		
		Rect area = Rect (origin.x, origin.y, segment->advance, font->Height ());
		fg->SetupBrush (cr, area);
		
		if (!segment->path) {
			if (font->IsScalable () && segment->start < segment->end) {
				// count how many path data items we'll need to allocate
				for (size = 0, i = segment->start; i < segment->end; i++) {
					if (!(glyph = font->GetGlyphInfo (text[i])))
						continue;
					
					size += glyph->path->cairo.num_data + 1;
				}
				
				path = moon_path_new (size);
				cairo_new_path (cr);
			} else {
				path = NULL;
			}
			
			for (i = segment->start, prev = 0; i < segment->end; i++) {
				gunichar uc = text[i];
				if (!(glyph = font->GetGlyphInfo (uc)))
					continue;
				
				if ((prev != 0) && APPLY_KERNING (uc))
					x1 += font->Kerning (prev, glyph->index);
				else if (glyph->metrics.horiBearingX < 0)
					x1 += glyph->metrics.horiBearingX;
				
				prev = glyph->index;
				
				if (font->IsScalable ()) {
					if (path)
						font->AppendPath (path, glyph, x1, y1);
					
					font->Path (cr, glyph, x1, y1);
				} else {
					font->Render (cr, glyph, x1, y1);
				}
				
				x1 += glyph->metrics.horiAdvance;
			}
			
			if (font->IsScalable () && segment->start < segment->end) {
				moon_close_path (path);
				cairo_close_path (cr);
				segment->path = path;
				cairo_fill (cr);
			}
		} else {
			// it is an error to append a path with no data
			if (segment->path->cairo.data)
				cairo_append_path (cr, &segment->path->cairo);
			
			cairo_fill (cr);
		}
		
		if ((deco & TextDecorationsUnderline) && segment->width > 0.0) {
			cairo_antialias_t aa = cairo_get_antialias (cr);
			double thickness = font->UnderlineThickness ();
			double pos = y1 + font->UnderlinePosition ();
			
			cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
			cairo_set_line_width (cr, thickness);
			x1 = segment->width;
			
			cairo_new_path (cr);
			cairo_move_to (cr, 0.0, pos);
			cairo_line_to (cr, x1, pos);
			cairo_stroke (cr);
			
			cairo_set_antialias (cr, aa);
		}
		
		x0 += segment->advance;
		
		segment = (TextSegment *) segment->next;
		cairo_restore (cr);
	}
}

void
TextLayout::Render (cairo_t *cr, const Point &origin, const Point &offset, TextLayoutHints *hints, Brush *default_fg, TextSelection *selection, int caret)
{
	TextLine *line;
	Point position;
	double deltax;
	
	position.y = offset.y;
	
	Layout (hints);
	
	line = (TextLine *) lines->First ();
	
	while (line) {
		switch (hints->GetTextAlignment ()) {
		case TextAlignmentCenter:
			if (line->width < max_width)
				deltax = (max_width - line->width) / 2.0;
			else
				deltax = 0.0;
			break;
		case TextAlignmentRight:
			if (line->width < max_width)
				deltax = max_width - line->width;
			else
				deltax = 0.0;
			break;
		default:
			deltax = 0.0;
			break;
		}
		
		position.x = offset.x + deltax;
		RenderLine (cr, origin, position, hints, line, default_fg);
		position.y += (double) line->height;
		
		line = (TextLine *) line->next;
	}
}

/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * font.cpp: 
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#include "zip/unzip.h"
#include "moon-path.h"
#include "utils.h"
#include "list.h"
#include "font.h"

#include FT_OUTLINE_H

//#define FONT_DEBUG 1
#ifdef FONT_DEBUG
#define d(x) x
#else
#define d(x)
#endif


#define FONT_FACE_SIZE 41.0

#define DOUBLE_TO_26_6(d) ((FT_F26Dot6)((d) * 64.0))
#define DOUBLE_FROM_26_6(t) ((double)(t) / 64.0)
#define DOUBLE_TO_16_16(d) ((FT_Fixed)((d) * 65536.0))
#define DOUBLE_FROM_16_16(t) ((double)(t) / 65536.0)


static const FT_Matrix invert_y = {
        65535, 0,
        0, -65535,
};

#define LOAD_FLAGS (FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH | FT_LOAD_TARGET_NORMAL)


struct GlyphBitmap {
	cairo_surface_t *surface;
	unsigned char *buffer;
	int height;
	int width;
	int left;
	int top;
};

struct FontStyleInfo {
	int weight;
	int width;
	int slant;
};

struct FontFile : public List::Node {
	GPtrArray *faces;
	char *path;
	
	FontFile (const char *path);
	~FontFile ();
};

struct FontFileFace {
	FontStyleInfo style;
	char *family_name;
	FontFile *file;
	int index;
	
	FontFileFace (FontFile *file, FT_Face face, int index);
	
	~FontFileFace ()
	{
		g_free (family_name);
	}
};

struct FontDir {
	List *fonts;
	char *key;
	char *dir;
	
	FontDir (const char *key)
	{
		this->key = g_strdup (key);
		fonts = new List ();
		dir = NULL;
	}
	
	~FontDir ()
	{
		delete fonts;
		g_free (dir);
		g_free (key);
	}
	
	void CacheFileInfo (const char *filename, FT_Face face);
};

GHashTable *FontFace::cache = NULL;
GHashTable *TextFont::cache = NULL;
static GHashTable *fontdirs = NULL;
static bool initialized = false;
static FT_Library libft2;
static double dpi;


static void
delete_fontdir (FontDir *dir)
{
	delete dir;
}

void
font_init (void)
{
	FcPattern *pattern;
	
	if (initialized)
		return;
	
	if (FT_Init_FreeType (&libft2) != 0) {
		g_warning ("could not init libfreetype2");
		return;
	}
	
	FontFace::Init ();
	TextFont::Init ();
	
	fontdirs = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) delete_fontdir);
	
	pattern = FcPatternBuild (NULL, FC_FAMILY, FcTypeString, "Sans",
				  FC_SIZE, FcTypeDouble, 10.0, NULL);
	
	if (FcPatternGetDouble (pattern, FC_DPI, 0, &dpi) != FcResultMatch)
		dpi = 72.0;
	
	FcPatternDestroy (pattern);
	
	initialized = true;
}

void
font_shutdown (void)
{
	if (!initialized)
		return;
	
	TextFont::Shutdown ();
	FontFace::Shutdown ();
	
	g_hash_table_destroy (fontdirs);
	
	FT_Done_FreeType (libft2);
	
	initialized = false;
}

static int
fc_weight (FontWeights weight)
{
	if ((weight < 0) && (weight > FONT_LOWER_BOLD_LIMIT))
		return FC_WEIGHT_BLACK;
	else if (weight < (FontWeightsThin + FontWeightsLight) / 2)
		return FC_WEIGHT_ULTRALIGHT;
	else if (weight < (FontWeightsLight + FontWeightsNormal) / 2)
		return FC_WEIGHT_LIGHT;
	else if (weight < (FontWeightsNormal + FontWeightsMedium) / 2)
		return FC_WEIGHT_NORMAL;
	else if (weight < (FontWeightsMedium + FontWeightsSemiBold) / 2)
		return FC_WEIGHT_MEDIUM;
	else if (weight < (FontWeightsSemiBold + FontWeightsBold) / 2)
		return FC_WEIGHT_DEMIBOLD;
	else if (weight < (FontWeightsBold + FontWeightsExtraBold) / 2)
		return FC_WEIGHT_BOLD;
	else if (weight < (FontWeightsExtraBold + FontWeightsBlack) / 2)
		return FC_WEIGHT_ULTRABOLD;
	else if (weight < FONT_UPPER_BOLD_LIMIT)
		return FC_WEIGHT_BLACK;
	else
		return FC_WEIGHT_NORMAL;
}

static int
fc_style (FontStyles style)
{
	switch (style) {
	case FontStylesNormal:
		return FC_SLANT_ROMAN;
	// technically Olbique does not exists in SL 1.0 or 2.0 (it's in WPF) but the parser allows it
	case FontStylesOblique:
		return FC_SLANT_OBLIQUE;
	case FontStylesItalic:
	// Silverlight defaults bad values to Italic
	default:
		return FC_SLANT_ITALIC;
	}
}

static int
fc_stretch (FontStretches stretch)
{
	switch (stretch) {
	case FontStretchesUltraCondensed:
		return FC_WIDTH_ULTRACONDENSED;
	case FontStretchesExtraCondensed:
		return FC_WIDTH_EXTRACONDENSED;
	case FontStretchesCondensed:
		return FC_WIDTH_CONDENSED;
	case FontStretchesSemiCondensed:
		return FC_WIDTH_SEMICONDENSED;
	case FontStretchesNormal:
		return FC_WIDTH_NORMAL;
#if 0
	case FontStretchesMedium:
		return FC_WIDTH_NORMAL;
#endif
	case FontStretchesSemiExpanded:
		return FC_WIDTH_SEMIEXPANDED;
	case FontStretchesExpanded:
		return FC_WIDTH_EXPANDED;
	case FontStretchesExtraExpanded:
		return FC_WIDTH_EXTRAEXPANDED;
	case FontStretchesUltraExpanded:
		return FC_WIDTH_ULTRAEXPANDED;
	default:
		return FC_WIDTH_NORMAL;
	}
}


static struct {
	const char *name;
	int value;
} style_weights[] = {
	// FIXME: are these strings all correct (e.g. spaces vs no spaces)?
	{ "Ultra Light", FC_WEIGHT_ULTRALIGHT },
	{ "Light",       FC_WEIGHT_LIGHT      },
	{ "Semi Bold",   FC_WEIGHT_DEMIBOLD   },
	{ "Ultra Bold",  FC_WEIGHT_ULTRABOLD  },
	{ "Medium",      FC_WEIGHT_MEDIUM     },
	{ "Bold",        FC_WEIGHT_BOLD       },
	{ "Black",       FC_WEIGHT_BLACK      }
};

static int
style_weight_parse (const char *style)
{
	unsigned int i;
	
	if (style) {
		for (i = 0; i < G_N_ELEMENTS (style_weights); i++) {
			if (strstr (style, style_weights[i].name))
				return style_weights[i].value;
		}
	}
	
	return FC_WEIGHT_NORMAL;
}

static struct {
	const char *name;
	int value;
} style_widths[] = {
	// FIXME: are these strings all correct (e.g. spaces vs no spaces, Cond vs Condensed)?
	{ "Ultra Condensed", FC_WIDTH_ULTRACONDENSED },
	{ "Extra Condensed", FC_WIDTH_EXTRACONDENSED },
	{ "Semi Condensed",  FC_WIDTH_SEMICONDENSED  },
	{ "Condensed",       FC_WIDTH_CONDENSED      },
	{ "Ultra Expanded",  FC_WIDTH_ULTRAEXPANDED  },
	{ "Extra Expanded",  FC_WIDTH_EXTRAEXPANDED  },
	{ "Semi Expanded",   FC_WIDTH_SEMIEXPANDED   },
	{ "Expanded",        FC_WIDTH_EXPANDED       }
};

static int
style_width_parse (const char *style)
{
	unsigned int i;
	
	if (style) {
		for (i = 0; i < G_N_ELEMENTS (style_widths); i++) {
			if (strstr (style, style_widths[i].name))
				return style_widths[i].value;
		}
	}
	
	return FC_WIDTH_NORMAL;
}

static struct {
	const char *name;
	int value;
} style_slants[] = {
	{ "Oblique", FC_SLANT_OBLIQUE },
	{ "Italic",  FC_SLANT_ITALIC  }
};

static int
style_slant_parse (const char *style)
{
	unsigned int i;
	
	if (style) {
		for (i = 0; i < G_N_ELEMENTS (style_slants); i++) {
			if (strstr (style, style_slants[i].name))
				return style_slants[i].value;
		}
	}
	
	return FC_SLANT_ROMAN;
}

static void
style_info_parse (const char *style, FontStyleInfo *info)
{
	info->weight = style_weight_parse (style);
	info->width = style_width_parse (style);
	info->slant = style_slant_parse (style);
}

#ifdef FONT_DEBUG
static const char *
style_name (FontStyleInfo *style, char *namebuf)
{
	char *p;
	uint i;
	
	strcpy (namebuf, "Regular");
	p = namebuf;
	
	for (i = 0; i < G_N_ELEMENTS (style_widths); i++) {
		if (style_widths[i].value == style->width) {
			p = g_stpcpy (p, style_widths[i].name);
			break;
		}
	}
	
	for (i = 0; i < G_N_ELEMENTS (style_weights); i++) {
		if (style_weights[i].value == style->weight) {
			if (p != namebuf)
				*p++ = ' ';
			
			p = g_stpcpy (p, style_weights[i].name);
			break;
		}
	}
	
	for (i = 0; i < G_N_ELEMENTS (style_slants); i++) {
		if (style_slants[i].value == style->slant) {
			if (p != namebuf)
				*p++ = ' ';
			
			p = g_stpcpy (p, style_slants[i].name);
			break;
		}
	}
	
	return namebuf;
}
#endif

FontFileFace::FontFileFace (FontFile *file, FT_Face face, int index)
{
	d(fprintf (stderr, "\t\t\t* index=%d: family=\"%s\"; style=\"%s\"\n",
		   index, face->family_name, face->style_name));
	
	style_info_parse (face->style_name, &style);
	family_name = g_strdup (face->family_name);
	this->index = index;
	this->file = file;
}

FontFile::FontFile (const char *path)
{
	this->path = g_strdup (path);
	faces = NULL;
}

FontFile::~FontFile ()
{
	if (faces != NULL) {
		FontFileFace *face;
		
		for (uint i = 0; i < faces->len; i++) {
			face = (FontFileFace *) faces->pdata[i];
			delete face;
		}
		
		g_ptr_array_free (faces, true);
	}
}

void
FontDir::CacheFileInfo (const char *filename, FT_Face face)
{
	int i = 0, nfaces = face->num_faces;
	FontFileFace *fface;
	FontFile *file;
	
	file = new FontFile (filename);
	file->faces = g_ptr_array_new ();
	
	do {
		if (i > 0 && FT_New_Face (libft2, filename, i, &face) != 0)
			break;
		
		d(fprintf (stderr, "\t\t* caching font info for `%s'...\n", filename));
		fface = new FontFileFace (file, face, i);
		g_ptr_array_add (file->faces, fface);
		
		FT_Done_Face (face);
		i++;
	} while (i < nfaces);
	
	fonts->Append (file);
}

bool
DecodeObfuscatedFontGUID (const char *in, char *key)
{
	const char *inptr = in;
	int i = 16;
	
	while (i > 0 && *inptr && *inptr != '.') {
		if (*inptr == '-')
			inptr++;
		
		i--;
		
		if (*inptr >= '0' && *inptr <= '9')
			key[i] = (*inptr - '0') * 16;
		else if (*inptr >= 'a' && *inptr <= 'f')
			key[i] = ((*inptr - 'a') + 10) * 16;
		else if (*inptr >= 'A' && *inptr <= 'F')
			key[i] = ((*inptr - 'A') + 10) * 16;
		else
			return false;
		
		inptr++;
		
		if (*inptr >= '0' && *inptr <= '9')
			key[i] += (*inptr - '0');
		else if (*inptr >= 'a' && *inptr <= 'f')
			key[i] += ((*inptr - 'a') + 10);
		else if (*inptr >= 'A' && *inptr <= 'F')
			key[i] += ((*inptr - 'A') + 10);
		else
			return false;
		
		inptr++;
	}
	
	if (i > 0)
		return false;
	
	return true;
}

bool
DeobfuscateFontFileWithGUID (const char *filename, const char *guid, FT_Face *pFace)
{
	char deobfuscated[32], buf[32];
	FT_Face face = NULL;
	size_t nread;
	FILE *fp;
	int i;
	
	if (!(fp = fopen (filename, "r+")))
		return false;
	
	// read the first 32 bytes of the obfuscated font file
	if ((nread = fread (buf, 1, 32, fp)) < 32)
		goto exception;
	
	// XOR the guid with the first 32 bytes of the obfuscated font file
	for (i = 0; i < 32; i++)
		deobfuscated[i] = buf[i] ^ guid[i % 16];
	
	if (fseek (fp, 0, SEEK_SET) != 0)
		goto exception;
	
	if ((nread = fwrite (deobfuscated, 1, 32, fp)) != 32)
		goto exception;
	
	fflush (fp);
	
	if (FT_New_Face (libft2, filename, 0, &face) != 0)
		goto undo;
	
	if (!pFace)
		FT_Done_Face (face);
	else
		*pFace = face;
	
	fclose (fp);
	
	return true;
	
 undo:
	
	if (fseek (fp, 0, SEEK_SET) == 0) {
		fwrite (buf, 1, 32, fp);
		fflush (fp);
	}
	
 exception:
	
	fclose (fp);
	
	return false;
}

static bool
IndexFontSubdirectory (const char *toplevel, GString *path, FontDir **out)
{
	FontDir *fontdir = *out;
	struct dirent *dent;
	struct stat st;
	FT_Face face;
	size_t len;
	DIR *dir;
	
	if (!(dir = opendir (path->str)))
		return fontdir != NULL;
	
	g_string_append_c (path, G_DIR_SEPARATOR);
	len = path->len;
	
	while ((dent = readdir (dir))) {
		if (!strcmp (dent->d_name, "..") ||
		    !strcmp (dent->d_name, "."))
			continue;
		
		g_string_append (path, dent->d_name);
		
		if (stat (path->str, &st) == -1)
			goto next;
		
		if (S_ISDIR (st.st_mode)) {
			IndexFontSubdirectory (toplevel, path, &fontdir);
			goto next;
		}
		
		if (FT_New_Face (libft2, path->str, 0, &face) != 0) {
			// not a valid font file... is it maybe an obfuscated font?
			char guid[16];
			
			if (!DecodeObfuscatedFontGUID (dent->d_name, guid))
				goto next;
			
			if (!DeobfuscateFontFileWithGUID (path->str, guid, &face))
				goto next;
		}
		
		if (fontdir == NULL)
			fontdir = new FontDir (toplevel);
		
		// cache font info
		fontdir->CacheFileInfo (path->str, face);
		
	 next:
		g_string_truncate (path, len);
	}
	
	closedir (dir);
	
	*out = fontdir;
	
	return fontdir != NULL;
}

static FontDir *
IndexFontDirectory (const char *dirname)
{
	FontDir *fontdir = NULL;
	GString *path;
	size_t len;
	
	path = g_string_new (dirname);
	len = path->len;
	
	if (!IndexFontSubdirectory (dirname, path, &fontdir)) {
		g_string_free (path, true);
		return NULL;
	}
	
	g_string_truncate (path, len);
	fontdir->dir = path->str;
	
	g_string_free (path, false);
	
	return fontdir;
}


struct FontFamilyInfo {
	char *name;
	int weight;
	int width;
	int slant;
};

struct Token {
	Token *next;
	const char *value;
	size_t len;
};

enum StyleType {
	Weight,
	Width,
	Slant
};

static struct {
	const char *name;
	StyleType type;
	int value;
} style_values[] = {
	// FIXME: what other names can be specified?
	{ "Light",    Weight, FC_WEIGHT_LIGHT        },
	{ "SemiBold", Weight, FC_WEIGHT_DEMIBOLD     },
	{ "Bold",     Weight, FC_WEIGHT_BOLD         },
	{ "Black",    Weight, FC_WEIGHT_BLACK        },
	
	{ "SemiCond", Width,  FC_WIDTH_SEMICONDENSED },
	{ "Cond",     Width,  FC_WIDTH_CONDENSED     },
	
	{ "Oblique",  Slant,  FC_SLANT_OBLIQUE       },
	{ "Italic",   Slant,  FC_SLANT_ITALIC        }
};


static FontFamilyInfo *
parse_font_family (const char *in)
{
	register const char *inptr = in;
	Token *tokens, *token, *tail;
	const char *start, *end;
	FontFamilyInfo *family;
	bool style_info;
	uint i;
	
	// tokenize the font name so we can check for stretch/weight/style info in the name
	tail = tokens = token = new Token ();
	token->value = start = inptr;
	
	while (*inptr) {
		if (*inptr == ' ') {
			token->len = (inptr - token->value);
			while (*inptr == ' ')
				inptr++;
			
			if (*inptr) {
				token = new Token ();
				token->value = inptr;
				tail->next = token;
				tail = token;
			}
		} else {
			inptr++;
		}
	}
	
	token->len = (inptr - token->value);
	token->next = NULL;
	
	family = new FontFamilyInfo ();
	family->weight = FC_WEIGHT_NORMAL;
	family->width = FC_WIDTH_NORMAL;
	family->slant = FC_SLANT_ROMAN;
	
	// assume at least the first token is part of the face name
	end = tokens->value + tokens->len;
	token = tokens->next;
	style_info = false;
	delete tokens;
	
	while (token != NULL) {
		for (i = 0; i < G_N_ELEMENTS (style_values); i++) {
			if (!strncmp (style_values[i].name, token->value, token->len)) {
				switch (style_values[i].type) {
				case Weight:
					family->weight = style_values[i].value;
					break;
				case Width:
					family->width = style_values[i].value;
					break;
				case Slant:
					family->slant = style_values[i].value;
					break;
				}
				
				style_info = true;
				break;
			}
		}
		
		if (!style_info)
			end = token->value + token->len;
		
		tail = token->next;
		delete token;
		token = tail;
	}
	
	family->name = g_strndup (start, (end - start));
	
	return family;
}


//
// FontFace
//

struct FontFaceSimilarity {
	FontFileFace *face;
	int weight;
	int width;
	int slant;
};


bool
FontFace::OpenFontDirectory (FT_Face *face, FcPattern *pattern, const char *path, const char **families)
{
#ifdef FONT_DEBUG
	char stylebuf1[256], stylebuf2[256];
#endif
	FontFaceSimilarity similar;
	FontFamilyInfo *family;
	FontStyleInfo style;
	FontFileFace *fface;
	GPtrArray *array;
	FontFile *file;
	FontDir *dir;
	uint i, j;
	
	if (FcPatternGetInteger (pattern, FC_WEIGHT, 0, &style.weight) != FcResultMatch)
		style.weight = FC_WEIGHT_NORMAL;
	
	if (FcPatternGetInteger (pattern, FC_WIDTH, 0, &style.width) != FcResultMatch)
		style.width = FC_WIDTH_NORMAL;
	
	if (FcPatternGetInteger (pattern, FC_SLANT, 0, &style.slant) != FcResultMatch)
		style.slant = FC_SLANT_ROMAN;
	
	if (!(dir = (FontDir *) g_hash_table_lookup (fontdirs, path))) {
		d(fprintf (stderr, "\t* indexing font directory...\n"));
		if (!(dir = IndexFontDirectory (path)))
			return false;
		
		g_hash_table_insert (fontdirs, dir->key, dir);
	} else {
		d(fprintf (stderr, "\t* reusing an extracted zip archive...\n"));
	}
	
	array = g_ptr_array_new ();
	for (i = 0; families && families[i]; i++) {
		// parse the family name to extract any additional style info
		family = parse_font_family ((const char *) families[i]);
		g_ptr_array_add (array, family);
		
		// FIXME: which style info should take prioriy?
		if (family->weight == FC_WEIGHT_NORMAL)
			family->weight = style.weight;
		if (family->width == FC_WIDTH_NORMAL)
			family->width = style.width;
		if (family->slant == FC_SLANT_ROMAN)
			family->slant = style.slant;
	}
	
	similar.weight = INT_MAX;
	similar.width = INT_MAX;
	similar.slant = INT_MAX;
	similar.face = NULL;
	
	file = (FontFile *) dir->fonts->First ();
	while (file != NULL) {
		for (i = 0; i < file->faces->len; i++) {
			fface = (FontFileFace *) file->faces->pdata[i];
			if (!fface->family_name)
				continue;
			
			for (j = 0; j < array->len; j++) {
				family = (FontFamilyInfo *) array->pdata[i];
				style.weight = family->weight;
				style.width = family->width;
				style.slant = family->slant;
				
				d(fprintf (stderr, "\t* checking if '%s' matches '%s'... ",
					   fface->family_name, family->name));
				
				// compare against parsed family name (don't want to include style info)
				if (strcmp (family->name, fface->family_name) != 0) {
					d(fprintf (stderr, "no\n"));
					continue;
				}
				
				d(fprintf (stderr, "yes\n\t\t* checking if '%s' matches '%s'... ",
					   style_name (&fface->style, stylebuf1), style_name (&style, stylebuf2)));
				
				if (fface->style.weight == style.weight &&
				    fface->style.width == style.width &&
				    fface->style.slant == style.slant) {
					// found an exact match
					d(fprintf (stderr, "yes\n"));
					goto found;
				}
				
				// not an exact match, but similar...
				if (abs (fface->style.weight - style.weight) <= similar.weight &&
				    abs (fface->style.width - style.width) <= similar.width &&
				    abs (fface->style.slant - style.slant) <= similar.slant) {
					d(fprintf (stderr, "no, but closest match\n"));
					similar.weight = abs (fface->style.weight - style.weight);
					similar.width = abs (fface->style.width - style.width);
					similar.slant = abs (fface->style.slant - style.slant);
					similar.face = fface;
				} else {
					d(fprintf (stderr, "no\n"));
				}
			}
		}
		
		file = (FontFile *) file->next;
	}
	
	// we were unable to find an exact match...
	
	if (similar.face == NULL) {
		// no similar fonts, I guess we just fail?
		for (i = 0; i < array->len; i++)
			delete (FontFamilyInfo *) array->pdata[i];
		g_ptr_array_free (array, true);
		return false;
	}
	
	// use the closest match...
	file = similar.face->file;
	fface = similar.face;
	
 found:
	
	d(fprintf (stderr, "\t* using font '%s, %s'\n", fface->family_name, style_name (&fface->style, stylebuf1)));
	
	for (i = 0; i < array->len; i++)
		delete (FontFamilyInfo *) array->pdata[i];
	g_ptr_array_free (array, true);
	
	return FT_New_Face (libft2, file->path, fface->index, face) == 0;
}

bool
FontFace::LoadFontFace (FT_Face *face, FcPattern *pattern, const char **families)
{
	FcPattern *matched = NULL, *fallback = NULL;
	FcChar8 *filename = NULL;
	bool try_nofile = false;
	FT_Face ftface = NULL;
	FcResult result;
	FT_Error err;
	int index, i;
	
	if (FcPatternGetString (pattern, FC_FILE, 0, &filename) == FcResultMatch) {
		struct stat st;
		int rv;
		
		try_nofile = true;
		
		if ((rv = stat ((const char *) filename, &st)) == -1 || S_ISDIR (st.st_mode)) {
			if (rv != -1 && OpenFontDirectory (face, pattern, (const char *) filename, families)) {
				// we found the font in the directory...
				return true;
			}
			
			if (families)
				goto try_nofile;
		}
	} else {
		// original pattern does not reference a file or directory path,
		// need to query FontConfig to find us a match...
		matched = FcFontMatch (NULL, pattern, &result);
	}
	
	if (matched == NULL) {
		FcPatternReference (pattern);
		matched = pattern;
	}
	
	do {
		if (FcPatternGetString (matched, FC_FILE, 0, &filename) != FcResultMatch)
			goto fail;
		
		if (FcPatternGetInteger (matched, FC_INDEX, 0, &index) != FcResultMatch)
			goto fail;
		
		d(fprintf (stderr, "\t* loading font from `%s' (index=%d)... ", filename, index));
		if ((err = FT_New_Face (libft2, (const char *) filename, index, &ftface)) == 0) {
			if (FT_IS_SCALABLE (ftface)) {
				if (!families || !ftface->family_name) {
					d(fprintf (stderr, "success!\n"));
					break;
				}
				
				// make sure the font family name matches what was requested...
				for (i = 0; families[i]; i++) {
					if (!g_ascii_strcasecmp (ftface->family_name, families[i]))
						break;
				}
				
				if (families[i]) {
					d(fprintf (stderr, "success!\n"));
					break;
				}
				
#if d(!)0
				fprintf (stderr, "no\n\t\t* incorrect family: '%s' does not match any of: ",
					 ftface->family_name);
				for (i = 0; families[i]; i++) {
					fputs (families[i], stderr);
					if (families[i+1])
						fputs (", ", stderr);
				}
				
				fputc ('\n', stderr);
#endif
			} else {
				d(fprintf (stderr, "no\n\t\t* not a scalable font\n"));
			}
			
			FT_Done_Face (ftface);
			ftface = NULL;
		} else {
			d(fprintf (stderr, "failed :(\n"));
		}
		
	 fail:
		
		if (try_nofile && families) {
		 try_nofile:
			// We couldn't find a matching font in the font directory, so let's try
			// removing the filename from the pattern and see if that gets us what
			// we are looking for.
#if d(!)0
			d(fprintf (stderr, "\t* falling back to matching by family: "));
			for (i = 0; families[i]; i++) {
				fputs (families[i], stderr);
				if (families[i+1])
					fputs (", ", stderr);
			}
			
			fputc ('\n', stderr);
#endif
			
			fallback = FcPatternDuplicate (pattern);
			FcPatternDel (fallback, FC_FILE);
			
			if (matched != NULL)
				FcPatternDestroy (matched);
			
			matched = FcFontMatch (NULL, fallback, &result);
			FcPatternDestroy (fallback);
			fallback = NULL;
			filename = NULL;
			
			try_nofile = false;
			continue;
		}
		
		// epic fail.
		ftface = NULL;
		break;
	} while (true);
	
	FcPatternDestroy (matched);
	
	if (ftface != NULL) {
		*face = ftface;
		return true;
	}
	
	return false;
}

FontFace::FontFace (FT_Face face, FcPattern *pattern, bool own)
{
	this->own_face = own;
	this->ref_count = 1;
	
	g_hash_table_insert (FontFace::cache, pattern, this);
	FT_Set_Pixel_Sizes (face, 0, (int) FONT_FACE_SIZE);
	this->cur_size = FONT_FACE_SIZE;
	this->pattern = pattern;
	this->face = face;
}

FontFace::~FontFace ()
{
	g_hash_table_remove (FontFace::cache, pattern);
	FcPatternDestroy (pattern);
	
	if (face && own_face)
		FT_Done_Face (face);
}

static FT_Face default_face;

void
FontFace::Init ()
{
	FontFace::cache = g_hash_table_new ((GHashFunc) FcPatternHash, (GEqualFunc) FcPatternEqual);
	default_face = NULL;
}

void
FontFace::Shutdown ()
{
	g_hash_table_destroy (FontFace::cache);
	
	if (default_face)
		FT_Done_Face (default_face);
}

static FcPattern *
create_default_pattern (const char **families)
{
	FcPattern *pattern;
	int i;
	
	pattern = FcPatternCreate ();
	FcPatternAddDouble (pattern, FC_DPI, dpi);
	
	for (i = 0; families[i]; i++)
		FcPatternAddString (pattern, FC_FAMILY, (FcChar8 *) families[i]);
	
	FcDefaultSubstitute (pattern);
	
	return pattern;
}

static struct {
	const char *source;
	const char *families[2];
} default_font_names[] = {
	// Use the Microsoft Lucida Sans Unicode font if available (in
	// an attempt to be a pixel-perfect match with Windows/Mac)
	{ "Microsoft", { "Lucida Sans Unicode", NULL } },
	// GNOME's Bitstream Vera Sans font are probably the next best
	{ "GNOME", { "Bitstream Vera Sans", NULL } },
	// Liberation fonts are supposed to be really nice, let's fall back to them next
	{ "Liberation", { "Liberation Sans", NULL } },
	// DejaVu Sans is another common Sans font found on Linux desktop systems
	{ "DejaVu", { "DejaVu Sans", NULL } },
	// Finally, fall back to X.Org default Sans font
	{ "X.Org", { "Luxi Sans", NULL } }
};

void
FontFace::LoadDefaultFace ()
{
	bool loaded = false;
	FcPattern *pattern;
	
	d(fprintf (stderr, "Attempting to load default system font\n"));
	for (guint i = 0; i < G_N_ELEMENTS (default_font_names) && !loaded; i++) {
		const char **families = default_font_names[i].families;
		
		d(fprintf (stderr, "    %s\n", default_font_names[i].source));
		pattern = create_default_pattern (families);
		loaded = LoadFontFace (&default_face, pattern, families);
		FcPatternDestroy (pattern);
	}
}

FontFace *
FontFace::GetDefault (FcPattern *pattern)
{
	if (!default_face)
		LoadDefaultFace ();
	
	return new FontFace (default_face, pattern, false);
}

FontFace *
FontFace::Load (const TextFontDescription *desc)
{
	FcPattern *pattern = desc->CreatePattern (false);
	FontFace *face;
	
	if (!(face = (FontFace *) g_hash_table_lookup (FontFace::cache, pattern))) {
		bool loaded = false;
		
		if (!desc->IsDefault ()) {
			char **families = desc->GetFamilies ();
			FT_Face ftface;
			
#if d(!)0
			char *debug = desc->ToString ();
			fprintf (stderr, "Attempting to load %s\n", debug);
			g_free (debug);
#endif
			
			if ((loaded = LoadFontFace (&ftface, pattern, (const char **) families)))
				face = new FontFace (ftface, pattern, true);
			else
				d(fprintf (stderr, "\t* falling back to default system font...\n"));
			
			g_strfreev (families);
		}
		
		if (!loaded)
			face = GetDefault (pattern);
	} else {
		FcPatternDestroy (pattern);
		face->ref ();
	}
	
	return face;
}

void
FontFace::ref ()
{
	ref_count++;
}

void
FontFace::unref ()
{
	ref_count--;
	
	if (ref_count == 0)
		delete this;
}

bool
FontFace::IsScalable ()
{
	if (!face)
		return false;
	
	return FT_IS_SCALABLE (face);
}

void
FontFace::GetExtents (double size, FontFaceExtents *extents)
{
	double scale = 1.0 / 64.0;
	FT_Long thickness;
	FT_Long position;
	
	if (!face) {
		extents->underline_thickness = 1.0;
		extents->underline_position = 0.0;
		extents->descent = 0.0;
		extents->ascent = 0.0;
		extents->height = 0.0;
		
		return;
	}
	
	if (size <= FONT_FACE_SIZE) {
		if (!own_face || cur_size != FONT_FACE_SIZE) {
			FT_Set_Pixel_Sizes (face, 0, (int) FONT_FACE_SIZE);
			cur_size = FONT_FACE_SIZE;
		}
		
		scale *= (size / FONT_FACE_SIZE);
	} else if (!own_face || cur_size != size) {
		FT_Set_Pixel_Sizes (face, 0, (int) size);
		cur_size = size;
	}
	
	thickness = FT_MulFix (face->underline_thickness, face->size->metrics.y_scale);
	position = FT_MulFix (-face->underline_position, face->size->metrics.y_scale);
	
	extents->underline_thickness = thickness * scale;
	extents->underline_position = (position * scale) + ((extents->underline_thickness + 1) / 2.0);
	if (extents->underline_thickness < 1.0)
		extents->underline_thickness = 1.0;
	
	extents->descent = face->size->metrics.descender * scale;
	extents->ascent = face->size->metrics.ascender * scale;
	extents->height = FT_MulFix (face->height, face->size->metrics.y_scale) * scale;
}

static int
font_move_to (FT_Vector *to, void *user_data)
{
	moon_path *path = (moon_path *) user_data;
	double x, y;
	
	x = DOUBLE_FROM_26_6 (to->x);
	y = DOUBLE_FROM_26_6 (to->y);
	
	moon_move_to (path, x, y);
	
	return 0;
}

static int
font_line_to (FT_Vector *to, void *user_data)
{
	moon_path *path = (moon_path *) user_data;
	double x, y;
	
	x = DOUBLE_FROM_26_6 (to->x);
	y = DOUBLE_FROM_26_6 (to->y);
	
	moon_line_to (path, x, y);
	
	return 0;
}

static int
font_conic_to (FT_Vector *control, FT_Vector *to, void *user_data)
{
	moon_path *path = (moon_path *) user_data;
	double x3, y3;
	double x, y;
	
	x = DOUBLE_FROM_26_6 (control->x);
	y = DOUBLE_FROM_26_6 (control->y);
	
	x3 = DOUBLE_FROM_26_6 (to->x);
	y3 = DOUBLE_FROM_26_6 (to->y);
	
	moon_quad_curve_to (path, x, y, x3, y3);
	
	return 0;
}

static int
font_cubic_to (FT_Vector *control1, FT_Vector *control2, FT_Vector *to, void *user_data)
{
	moon_path *path = (moon_path *) user_data;
	double x0, y0;
	double x1, y1;
	double x2, y2;
	
	x0 = DOUBLE_FROM_26_6 (control1->x);
	y0 = DOUBLE_FROM_26_6 (control1->y);
	
	x1 = DOUBLE_FROM_26_6 (control2->x);
	y1 = DOUBLE_FROM_26_6 (control2->y);
	
	x2 = DOUBLE_FROM_26_6 (to->x);
	y2 = DOUBLE_FROM_26_6 (to->y);
	
	moon_curve_to (path, x0, y0, x1, y1, x2, y2);
	
	return 0;
}

static const FT_Outline_Funcs outline_funcs = {
        (FT_Outline_MoveToFunc) font_move_to,
        (FT_Outline_LineToFunc) font_line_to,
        (FT_Outline_ConicToFunc) font_conic_to,
        (FT_Outline_CubicToFunc) font_cubic_to,
        0, /* shift */
        0, /* delta */
};

bool
FontFace::LoadGlyph (double size, GlyphInfo *glyph)
{
	FT_Glyph_Metrics *metrics;
	FT_Matrix matrix;
	double scale;
	
	if (!face)
		return false;
	
	if (size <= FONT_FACE_SIZE) {
		if (!own_face || cur_size != FONT_FACE_SIZE) {
			FT_Set_Pixel_Sizes (face, 0, (int) FONT_FACE_SIZE);
			cur_size = FONT_FACE_SIZE;
		}
		
		scale = size / FONT_FACE_SIZE;
	} else {
		if (!own_face || cur_size != size) {
			FT_Set_Pixel_Sizes (face, 0, (int) size);
			cur_size = size;
		}
		
		scale = 1.0;
	}
	
	if (FT_Load_Glyph (face, glyph->index, LOAD_FLAGS) != 0)
		return false;
	
	if (FT_Render_Glyph (face->glyph, FT_RENDER_MODE_NORMAL) != 0)
		return false;
	
	matrix.xx = (FT_Fixed) (65535 * scale);
	matrix.xy = 0;
	matrix.yy = (FT_Fixed) (-65535 * scale);
	matrix.yx = 0;
	
	glyph->path = moon_path_new (8);
	FT_Outline_Transform (&face->glyph->outline, &matrix);
	FT_Outline_Decompose (&face->glyph->outline, &outline_funcs, glyph->path);
	
	metrics = &face->glyph->metrics;
	
	glyph->metrics.horiBearingX = DOUBLE_FROM_26_6 (metrics->horiBearingX) * scale;
	glyph->metrics.horiBearingY = DOUBLE_FROM_26_6 (metrics->horiBearingY) * scale;
	// always prefer linearHoriAdvance over horiAdvance since the later is rounded to an integer
	glyph->metrics.horiAdvance = DOUBLE_FROM_16_16 (face->glyph->linearHoriAdvance) * scale;
	glyph->metrics.height = DOUBLE_FROM_26_6 (metrics->height) * scale;
	glyph->metrics.width = DOUBLE_FROM_26_6 (metrics->width) * scale;
	
	return true;
}

double
FontFace::Kerning (double size, gunichar left, gunichar right)
{
	FT_Vector kerning;
	
	if (!face || !FT_HAS_KERNING (face) || left == 0 || right == 0)
		return 0.0;
	
	if (size <= FONT_FACE_SIZE) {
		if (!own_face || cur_size != FONT_FACE_SIZE) {
			FT_Set_Pixel_Sizes (face, 0, (int) FONT_FACE_SIZE);
			cur_size = FONT_FACE_SIZE;
		}
		
		FT_Get_Kerning (face, left, right, FT_KERNING_DEFAULT, &kerning);
		
		return (kerning.x * size) / (FONT_FACE_SIZE * 64.0);
	} else {
		if (!own_face || cur_size != size) {
			FT_Set_Pixel_Sizes (face, 0, (int) size);
			cur_size = size;
		}
		
		FT_Get_Kerning (face, left, right, FT_KERNING_DEFAULT, &kerning);
		
		return kerning.x / 64.0;
	}
}

gunichar
FontFace::GetCharFromIndex (guint32 index)
{
	gunichar unichar;
	guint32 idx;
	
	if (!face || index == 0)
		return 0;
	
	unichar = FT_Get_First_Char (face, &idx);
	while (idx != index && idx != 0)
		unichar = FT_Get_Next_Char (face, unichar, &idx);
	
	if (idx == 0)
		unichar = 0;
	
	return unichar;
}

guint32
FontFace::GetCharIndex (gunichar unichar)
{
	return face ? FT_Get_Char_Index (face, unichar) : 0;
}

bool
FontFace::HasChar (gunichar unichar)
{
	if (!face)
		return false;
	
	return FcFreeTypeCharIndex (face, unichar) != 0;
}


//
// TextFont
//

TextFont::TextFont (FontFace *face, FcPattern *pattern)
{
	FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &size);
	g_hash_table_insert (TextFont::cache, pattern, this);
	face->GetExtents (size, &extents);
	this->pattern = pattern;
	this->face = face;
	ref_count = 1;
	nglyphs = 0;
}

TextFont::~TextFont ()
{
	int i;
	
	for (i = 0; i < nglyphs; i++) {
		if (glyphs[i].path)
			moon_path_destroy (glyphs[i].path);
	}
	
	g_hash_table_remove (TextFont::cache, pattern);
	FcPatternDestroy (pattern);
	face->unref ();
}

void
TextFont::Init ()
{
	TextFont::cache = g_hash_table_new ((GHashFunc) FcPatternHash, (GEqualFunc) FcPatternEqual);
}

void
TextFont::Shutdown ()
{
	g_hash_table_destroy (TextFont::cache);
}

TextFont *
TextFont::Load (const TextFontDescription *desc)
{
	FcPattern *pattern = desc->CreatePattern (true);
	TextFont *font;
	FontFace *face;
	
	if ((font = (TextFont *) g_hash_table_lookup (TextFont::cache, pattern))) {
		FcPatternDestroy (pattern);
		font->ref ();
		return font;
	}
	
	face = FontFace::Load (desc);
	
	return new TextFont (face, pattern);
}

void
TextFont::ref ()
{
	ref_count++;
}

void
TextFont::unref ()
{
	ref_count--;
	
	if (ref_count == 0)
		delete this;
}

bool
TextFont::IsScalable ()
{
	return face->IsScalable ();
}

double
TextFont::Kerning (gunichar left, gunichar right)
{
	return face->Kerning (size, left, right);
}

double
TextFont::Descender ()
{
	return extents.descent;
}

double
TextFont::Ascender ()
{
	return extents.ascent;
}

double
TextFont::Height ()
{
	return extents.height;
}

static int
glyphsort (const void *v1, const void *v2)
{
	GlyphInfo *g1 = (GlyphInfo *) v1;
	GlyphInfo *g2 = (GlyphInfo *) v2;
	
	return g2->requested - g1->requested;
}

GlyphInfo *
TextFont::GetGlyphInfo (gunichar unichar, guint32 index)
{
	GlyphInfo glyph, *slot;
	int i;
	
	if (!face)
		return NULL;
	
	for (i = 0; i < nglyphs; i++) {
		if (glyphs[i].index == index) {
			slot = &glyphs[i];
			slot->requested++;
			return slot;
		}
	}
	
	glyph.unichar = unichar;
	glyph.index = index;
	glyph.requested = 1;
	glyph.path = NULL;
	
	if (!face->LoadGlyph (size, &glyph))
		return NULL;
	
	if (nglyphs == 256) {
		// need to expire the least requested glyph (which will be the last element in the array after sorting)
		qsort (glyphs, nglyphs, sizeof (GlyphInfo), glyphsort);
		slot = &glyphs[nglyphs - 1];
		
		if (slot->path)
			moon_path_destroy (slot->path);
	} else {
		slot = &glyphs[nglyphs++];
	}
	
	memcpy (slot, &glyph, sizeof (GlyphInfo));
	
	return slot;
}

GlyphInfo *
TextFont::GetGlyphInfo (gunichar unichar)
{
	guint32 index;
	
	index = face->GetCharIndex (unichar);
	
	return GetGlyphInfo (unichar, index);
}

GlyphInfo *
TextFont::GetGlyphInfoByIndex (guint32 index)
{
	gunichar unichar;
	
	unichar = face->GetCharFromIndex (index);
	
	return GetGlyphInfo (unichar, index);
}

bool
TextFont::HasGlyph (gunichar unichar)
{
	return face->HasChar (unichar);
}

double
TextFont::UnderlinePosition ()
{
	return extents.underline_position;
}

double
TextFont::UnderlineThickness ()
{
	return extents.underline_thickness;
}

void
TextFont::RenderGlyphPath (cairo_t *cr, GlyphInfo *glyph, double x, double y)
{
       cairo_new_path (cr);
       Path (cr, glyph, x, y);
       cairo_close_path (cr);
       cairo_fill (cr);
}

void
TextFont::Render (cairo_t *cr, GlyphInfo *glyph, double x, double y)
{
	if (glyph->path)
		RenderGlyphPath (cr, glyph, x, y);
}

void
TextFont::Render (cairo_t *cr, gunichar unichar, double x, double y)
{
	GlyphInfo *glyph;
	
	if (!(glyph = GetGlyphInfo (unichar)))
		return;
	
	Render (cr, glyph, x, y);
}

void
TextFont::Path (cairo_t *cr, GlyphInfo *glyph, double x, double y)
{
	if (!glyph->path || !(&glyph->path->cairo)->data)
		return;
	
	cairo_translate (cr, x, y);
	cairo_append_path (cr, &glyph->path->cairo);
	cairo_translate (cr, -x, -y);
}

void
TextFont::Path (cairo_t *cr, gunichar unichar, double x, double y)
{
	GlyphInfo *glyph;
	
	if (!(glyph = GetGlyphInfo (unichar)))
		return;
	
	Path (cr, glyph, x, y);
}

static void
moon_append_path_with_origin (moon_path *mpath, cairo_path_t *path, double x, double y)
{
	cairo_path_data_t *data;
	
	moon_move_to (mpath, x, y);
	
	for (int i = 0; i < path->num_data; i += path->data[i].header.length) {
		data = &path->data[i];
		
		switch (data->header.type) {
		case CAIRO_PATH_MOVE_TO:
			moon_move_to (mpath, data[1].point.x + x, data[1].point.y + y);
			break;
		case CAIRO_PATH_LINE_TO:
			moon_line_to (mpath, data[1].point.x + x, data[1].point.y + y);
			break;
		case CAIRO_PATH_CURVE_TO:
			moon_curve_to (mpath, data[1].point.x + x, data[1].point.y + y,
				       data[2].point.x + x, data[2].point.y + y,
				       data[3].point.x + x, data[3].point.y + y);
			break;
		case CAIRO_PATH_CLOSE_PATH:
			break;
		}
	}
}

void
TextFont::AppendPath (moon_path *path, GlyphInfo *glyph, double x, double y)
{
	if (!glyph->path || !(&glyph->path->cairo)->data)
		return;
	
	moon_append_path_with_origin (path, &glyph->path->cairo, x, y);
}

void
TextFont::AppendPath (moon_path *path, gunichar unichar, double x, double y)
{
	GlyphInfo *glyph;
	
	if (!(glyph = GetGlyphInfo (unichar)))
		return;
	
	AppendPath (path, glyph, x, y);
}


TextFontDescription::TextFontDescription ()
{
	font = NULL;
	
	set = 0;
	family = NULL;
	filename = NULL;
	style = FontStylesNormal;
	weight = FontWeightsNormal;
	stretch = FontStretchesNormal;
	size = 14.666666984558105;
	index = 0;
}

TextFontDescription::TextFontDescription (const char *str)
{
	// FIXME: implement me
}

TextFontDescription::~TextFontDescription ()
{
	if (font != NULL)
		font->unref ();
	
	g_free (filename);
	g_free (family);
}

FcPattern *
TextFontDescription::CreatePattern (bool sized) const
{
	FcPattern *pattern;
	char **families;
	int i;
	
	pattern = FcPatternCreate ();
	FcPatternAddDouble (pattern, FC_DPI, dpi);
	
	if (set & FontMaskFilename) {
		FcPatternAddString (pattern, FC_FILE, (FcChar8 *) filename);
		FcPatternAddInteger (pattern, FC_INDEX, index);
	}
	
	if (!(set & FontMaskFilename) || (set & FontMaskFamily)) {
		families = g_strsplit (GetFamily (), ",", -1);
		for (i = 0; families[i]; i++)
			FcPatternAddString (pattern, FC_FAMILY, (FcChar8 *) g_strstrip (families[i]));
		g_strfreev (families);
	}
	
	if (!IsDefault ()) {
		FcPatternAddInteger (pattern, FC_SLANT, fc_style (style));
		FcPatternAddInteger (pattern, FC_WEIGHT, fc_weight (weight));
		FcPatternAddInteger (pattern, FC_WIDTH, fc_stretch (stretch));
	}
	
	if (sized)
		FcPatternAddDouble (pattern, FC_PIXEL_SIZE, size);
	
	FcDefaultSubstitute (pattern);
	
	return pattern;
}

TextFont *
TextFontDescription::GetFont ()
{
	if (font == NULL)
		font = TextFont::Load (this);
	
	if (font)
		font->ref ();
	
	return font;
}

guint8
TextFontDescription::GetFields () const
{
	return set;
}

void
TextFontDescription::UnsetFields (guint8 mask)
{
	if (!(set & mask))
		return;
	
	if (font != NULL) {
		font->unref ();
		font = NULL;
	}
	
	if (mask & FontMaskFilename) {
		g_free (filename);
		filename = NULL;
		index = 0;
	}
	
	if (mask & FontMaskFamily) {
		g_free (family);
		family = NULL;
	}
	
	if (mask & FontMaskStretch)
		stretch = FontStretchesNormal;
	
	if (mask & FontMaskWeight)
		weight = FontWeightsNormal;
	
	if (mask & FontMaskStyle)
		style = FontStylesNormal;
	
	if (mask & FontMaskSize)
		size = 14.666f;
	
	set &= ~mask;
}

void
TextFontDescription::Merge (TextFontDescription *desc, bool replace)
{
	bool changed = false;
	
	if ((desc->set & FontMaskFilename) && (!(set & FontMaskFilename) || replace)) {
		if (!filename || strcmp (filename, desc->filename) != 0) {
			g_free (filename);
			filename = g_strdup (desc->filename);
			changed = true;
		}
		
		index = desc->index;
		
		set |= FontMaskFilename;
	}
	
	if ((desc->set & FontMaskFamily) && (!(set & FontMaskFamily) || replace)) {
		if (!family || strcmp (family, desc->family) != 0) {
			g_free (family);
			if (desc->family)
				family = g_strdup (desc->family);
			else
				family = NULL;
			changed = true;
		}
		
		set |= FontMaskFamily;
	}
	
	if ((desc->set & FontMaskStyle) && (!(set & FontMaskStyle) || replace)) {
		if (style != desc->style) {
			style = desc->style;
			changed = true;
		}
		
		set |= FontMaskStyle;
	}
	
	if ((desc->set & FontMaskWeight) && (!(set & FontMaskWeight) || replace)) {
		if (weight != desc->weight) {
			weight = desc->weight;
			changed = true;
		}
		
		set |= FontMaskWeight;
	}
	
	if ((desc->set & FontMaskStretch) && (!(set & FontMaskStretch) || replace)) {
		if (stretch != desc->stretch) {
			stretch = desc->stretch;
			changed = true;
		}
		
		set |= FontMaskStretch;
	}
	
	if ((desc->set & FontMaskSize) && (!(set & FontMaskSize) || replace)) {
		if (size != desc->size) {
			size = desc->size;
			changed = true;
		}
		
		set |= FontMaskSize;
	}
	
	if (changed && font != NULL) {
		font->unref ();
		font = NULL;
	}
}

bool
TextFontDescription::IsDefault () const
{
	if (set & FontMaskFilename)
		return false;
	
	if (!(set & FontMaskFamily) || !family)
		return true;
	
	return false;
}

const char *
TextFontDescription::GetFilename () const
{
	if (set & FontMaskFilename)
		return filename;
	
	return NULL;
}

void
TextFontDescription::SetFilename (const char *filename)
{
	bool changed;
	
	if (filename) {
		if (!this->filename || strcmp (this->filename, filename) != 0) {
			g_free (this->filename);
			this->filename = g_strdup (filename);
			set |= FontMaskFilename;
			changed = true;
		} else {
			changed = false;
		}
	} else {
		changed = this->filename != NULL;
		set &= ~FontMaskFilename;
		g_free (this->filename);
		this->filename = NULL;
	}
	
	if (changed && font != NULL) {
		font->unref ();
		font = NULL;
	}
}

int
TextFontDescription::GetIndex () const
{
	return index;
}

void
TextFontDescription::SetIndex (int index)
{
	bool changed = this->index != index;
	
	this->index = index;
	
	if (changed && font != NULL) {
		font->unref ();
		font = NULL;
	}
}

char **
TextFontDescription::GetFamilies () const
{
	char **families;
	
	if (!family)
		return NULL;
	
	if ((families = g_strsplit (family, ",", -1))) {
		for (int i = 0; families[i]; i++)
			g_strstrip (families[i]);
	}
	
	return families;
}

const char *
TextFontDescription::GetFamily () const
{
	if ((set & FontMaskFamily) && family)
		return family;
	
	// either no family is set or it is "Portable User Interface"
	return "Lucida Sans Unicode, Lucida Sans";
}

void
TextFontDescription::SetFamily (const char *family)
{
	bool changed;
	
	if (family) {
		if (!this->family || g_ascii_strcasecmp (this->family, family) != 0) {
			g_free (this->family);
			if (g_ascii_strcasecmp (family, "Portable User Interface") != 0)
				this->family = g_strdup (family);
			else
				this->family = NULL;
			set |= FontMaskFamily;
			changed = true;
		} else {
			changed = false;
		}
	} else {
		changed = this->family != NULL;
		set &= ~FontMaskFamily;
		g_free (this->family);
		this->family = NULL;
	}
	
	if (changed && font != NULL) {
		font->unref ();
		font = NULL;
	}
}

FontStyles
TextFontDescription::GetStyle () const
{
	return style;
}

void
TextFontDescription::SetStyle (FontStyles style)
{
	bool changed = this->style != style;
	
	this->style = style;
	set |= FontMaskStyle;
	
	if (changed && font != NULL) {
		font->unref ();
		font = NULL;
	}
}

FontWeights
TextFontDescription::GetWeight () const
{
	return weight;
}

void
TextFontDescription::SetWeight (FontWeights weight)
{
	bool changed = this->weight != weight;
	
	this->weight = weight;
	set |= FontMaskWeight;
	
	if (changed && font != NULL) {
		font->unref ();
		font = NULL;
	}
}

FontStretches
TextFontDescription::GetStretch () const
{
	return stretch;
}

void
TextFontDescription::SetStretch (FontStretches stretch)
{
	bool changed = this->stretch != stretch;
	
	this->stretch = stretch;
	set |= FontMaskStretch;
	
	if (changed && font != NULL) {
		font->unref ();
		font = NULL;
	}
}

double
TextFontDescription::GetSize () const
{
	return size;
}

void
TextFontDescription::SetSize (double size)
{
	bool changed = this->size != size;
	
	this->size = size;
	set |= FontMaskSize;
	
	if (changed && font != NULL) {
		font->unref ();
		font = NULL;
	}
}

char *
TextFontDescription::ToString () const
{
	bool attrs = false;
	GString *str;
	
	if (set == 0)
		return NULL;
	
	str = g_string_new ("");
	
	if (set & FontMaskFilename) {
		g_string_append (str, "font:");
		g_string_append (str, filename);
		g_string_append_printf (str, "?index=%d", index);
	}
	
	if ((set & FontMaskFamily) && family) {
		if (set & FontMaskFilename)
			g_string_append (str, "?family=");
		
		if (strchr (family, ',')) {
			g_string_append_c (str, '"');
			g_string_append (str, family);
			g_string_append_c (str, '"');
		} else {
			g_string_append (str, family);
		}
	} else if (!(set & FontMaskFilename)) {
		g_string_append (str, "\"Lucida Sans Unicode, Lucida Sans\"");
	}
	
	if ((set & FontMaskStretch) && stretch != FontStretchesNormal) {
		if (!attrs) {
			g_string_append_c (str, ',');
			attrs = true;
		}
		
		g_string_append_c (str, ' ');
		switch (stretch) {
		case FontStretchesUltraCondensed:
			g_string_append (str, "UltraCondensed");
			break;
		case FontStretchesExtraCondensed:
			g_string_append (str, "ExtraCondensed");
			break;
		case FontStretchesCondensed:
			g_string_append (str, "Condensed");
			break;
		case FontStretchesSemiCondensed:
			g_string_append (str, "SemiCondensed");
			break;
		case FontStretchesNormal:
			break;
#if 0
		case FontStretchesMedium:
			g_string_append (str, "Medium");
			break;
#endif
		case FontStretchesSemiExpanded:
			g_string_append (str, "SemiExpanded");
			break;
		case FontStretchesExpanded:
			g_string_append (str, "Expanded");
			break;
		case FontStretchesExtraExpanded:
			g_string_append (str, "ExtraExpanded");
			break;
		case FontStretchesUltraExpanded:
			g_string_append (str, "UltraExpanded");
			break;
		}
	}
	
	if ((set & FontMaskWeight) && weight != FontWeightsNormal) {
		if (!attrs) {
			g_string_append_c (str, ',');
			attrs = true;
		}
		
		g_string_append_c (str, ' ');
		switch (weight) {
		case FontWeightsThin:
			g_string_append (str, "Thin");
			break;
		case FontWeightsExtraLight:
			g_string_append (str, "ExtraLight");
			break;
		case FontWeightsLight:
			g_string_append (str, "Light");
			break;
		case FontWeightsNormal:
			break;
		case FontWeightsSemiBold:
			g_string_append (str, "SemiBold");
			break;
		case FontWeightsBold:
			g_string_append (str, "Bold");
			break;
		case FontWeightsExtraBold:
			g_string_append (str, "ExtraBold");
			break;
		case FontWeightsBlack:
			g_string_append (str, "Black");
			break;
		case FontWeightsExtraBlack:
			g_string_append (str, "ExtraBlack");
			break;
		default:
			g_string_append_printf (str, "%d", (int) weight);
			break;
		}
	}
	
	if ((set & FontMaskStyle) && style != FontStylesNormal) {
		if (!attrs) {
			g_string_append_c (str, ',');
			attrs = true;
		}
		
		g_string_append_c (str, ' ');
		switch (style) {
		case FontStylesNormal:
			break;
		case FontStylesOblique:
			g_string_append (str, "Oblique");
			break;
		case FontStylesItalic:
			g_string_append (str, "Italic");
			break;
		}
	}
	
	g_string_append_printf (str, " %.3fpx", size);
	
	return g_string_free (str, false);
}

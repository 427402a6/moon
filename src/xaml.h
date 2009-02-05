/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * xaml.h: xaml parser
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#ifndef __MOON_XAML_H__
#define __MOON_XAML_H__

#include <glib.h>

#include "enums.h"
#include "uielement.h"
#include "error.h"
#include "value.h"

class XamlLoader;

typedef bool (*xaml_lookup_object_callback) (void *parser, void *top_level, const char *xmlns, const char *name, bool create, Value *value);
typedef void (*xaml_create_gchandle_callback) ();
typedef bool (*xaml_set_property_callback) (void *parser, void *top_level, const char* xmlns, void *target, void *target_data, void *target_parent, const char *name, Value *value);
typedef void (*xaml_import_xaml_xmlns_callback) (void *parser, const char* xmlns);
typedef const char* (*xaml_get_content_property_name_callback) (void *parser, Type::Kind kind);

struct XamlLoaderCallbacks {
	xaml_lookup_object_callback lookup_object;
	xaml_create_gchandle_callback create_gchandle;
	xaml_set_property_callback set_property;
	xaml_import_xaml_xmlns_callback import_xaml_xmlns;
	xaml_get_content_property_name_callback get_content_property_name;

	XamlLoaderCallbacks () :
		lookup_object (NULL),
		set_property (NULL),
		import_xaml_xmlns (NULL),
		get_content_property_name (NULL)
	{
	}
};


//
// Used by the templates
//

class XamlContextInternal;

class XamlContext {

 public:
	XamlContextInternal *internal;

	XamlContext (XamlContextInternal *internal);
	~XamlContext ();
};


G_BEGIN_DECLS

void        xaml_init (void);

bool        xaml_set_property_from_str (DependencyObject *obj, DependencyProperty *prop, const char *value, bool sl2);

bool        time_span_from_str (const char *str, TimeSpan *res);
bool        value_from_str_with_typename (const char *type_name, const char *prop_name, const char *str, Value **v, bool sl2);
bool        value_from_str (Type::Kind type, const char *prop_name, const char *str, Value **v, bool sl2);
bool        convert_property_value_to_enum_str (DependencyProperty *prop, Value *v, const char **s);

void	    xaml_parse_xmlns (const char *xmlns, char **type_name, char **ns, char **assembly);

bool        xaml_is_valid_event_name (const char *name);

XamlLoader *xaml_loader_new (const char *filename, const char *str, Surface *surface);
void	    xaml_loader_free (XamlLoader *loader);

void        xaml_loader_set_callbacks (XamlLoader *loader, XamlLoaderCallbacks callbacks);

char*       xaml_uri_for_prefix (void *parser, char* prefix);


Value*      xaml_lookup_named_item (void *parser, void *element_instance, const char* name);


G_END_DECLS

/*

  Plugin:
    - calls PluginXamlLoader::TryLoad to try to load some xaml.
    -  calls xaml_create_from_*
    -     calls XamlLoader::LookupObject (,) if it encounters xmlns/name
    -      parses the xmlns and name
    -       calls XamlLoader::LoadVM.
    -        PluginXamlLoader::LoadVM will load the vm and create a ManagedXamlLoader (which will set the callbacks in XamlLoader)
    -       calls XamlLoader::CreateObject (,,,) with the parsed xml
    -        calls the create_managed_object callback (if any).
    -          will try to load the assembly, if it fails, it's requested.
    -  if XamlLoader::CreateObject failed, try to download the missing assembly (if any).
    -  if no missing assembly, the xaml load fails.

  Deskop:
    - calls System.Windows.XamlReader::Load
    -  creates a ManagedXamlLoader and a native XamlLoader (setting the callbacks).
    -  calls xaml_create_from_str
    -     calls XamlLoader::CreateObject (,) if it encounters xmlns/name
    -      parses the xmlns and name
    -       calls XamlLoader::LoadVM (which does nothing).
    -       calls XamlLoader::CreateObject (,,,) with the parsed xml
    -        calls the create_managed_object callback (if any).
    -          will try to load the assembly, if it fails, it's requested.
    -    destroy the native/managed XamlLoader. Any requested assemblies are ignored, no retries are done.
*/


class XamlLoader {
	Surface *surface;
	char *filename;
	char *str;
	XamlContext *context;

 public:
	enum AssemblyLoadResult {
		SUCCESS = -1,
		MissingAssembly = 1,
		LoadFailure = 2
	};

	XamlLoader (const char *filename, const char *str, Surface *surface, XamlContext *context = NULL);
	virtual ~XamlLoader ();
	
	virtual bool LoadVM ();

	virtual bool LookupObject (void *p, void *top_element, const char* xmlns, const char* name, bool create, Value *value);
	virtual bool SetProperty (void *p, void *top_level, const char* xmlns, void *target, void *target_data, void *target_parent, const char *name, Value *value);

	virtual const char *GetContentPropertyName (void *p, Type::Kind kind);

	char *GetFilename () { return filename; }
	char *GetString () { return str; }
	Surface *GetSurface () { return surface; }
	XamlContext *GetContext () { return context; }

	bool vm_loaded;

	DependencyObject* CreateFromFile (const char *xaml, bool create_namescope, Type::Kind *element_type);
	DependencyObject* CreateFromString  (const char *xaml, bool create_namescope, Type::Kind *element_type);
	DependencyObject* HydrateFromString (const char *xaml, DependencyObject *object, bool create_namescope, Type::Kind *element_type);

	/* @GenerateCBinding,GeneratePInvoke */
	DependencyObject* CreateFromFileWithError (const char *xaml, bool create_namescope, Type::Kind *element_type, MoonError *error);
	/* @GenerateCBinding,GeneratePInvoke */
	DependencyObject* CreateFromStringWithError  (const char *xaml, bool create_namescope, Type::Kind *element_type, MoonError *error);
	/* @GenerateCBinding,GeneratePInvoke */
	DependencyObject* HydrateFromStringWithError (const char *xaml, DependencyObject *obj, bool create_namescope, Type::Kind *element_type, MoonError *error);
	
	XamlLoaderCallbacks callbacks;
	ParserErrorEventArgs *error_args;
};


#endif /* __MOON_XAML_H__ */

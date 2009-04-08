/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * plugin.h: MoonLight browser plugin.
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 *
 */

#ifndef MOON_PLUGIN
#define MOON_PLUGIN

#include "moonlight.h"

#if PLUGIN_SL_2_0
#include <mono/jit/jit.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
G_BEGIN_DECLS
/* because this header sucks */
#include <mono/metadata/mono-debug.h>
G_END_DECLS
#include <mono/metadata/mono-config.h>
#endif

class MoonlightScriptControlObject;
class PluginXamlLoader;
class PluginInstance;
class BrowserBridge;
#if PLUGIN_SL_2_0
class Xap;
#endif

char *NPN_strdup (const char *val);

class PluginInstance
{
 public:
	PluginInstance (NPMIMEType pluginType, NPP instance, guint16 mode);
	~PluginInstance ();
	
	void Initialize (int argc, char *const argn[], char *const argv[]);
	void Finalize ();
	
	// Mozilla plugin related methods
	NPError GetValue (NPPVariable variable, void *result);
	NPError SetValue (NPNVariable variable, void *value);
	NPError SetWindow (NPWindow *window);
	NPError NewStream (NPMIMEType type, NPStream *stream, NPBool seekable, guint16 *stype);
	NPError DestroyStream (NPStream *stream, NPError reason);
	void StreamAsFile (NPStream *stream, const char *fname);
	gint32 WriteReady (NPStream *stream);
	gint32 Write (NPStream *stream, gint32 offset, gint32 len, void *buffer);
	void UrlNotify (const char *url, NPReason reason, void *notifyData);
	void Print (NPPrint *platformPrint);
	int16_t EventHandle (void *event);
	void ReportException (char *msg, char *details, char **stack_trace, int num_frames);
	void *Evaluate (const char *code);
	
	NPObject *GetHost ();
	
	void      AddWrappedObject    (EventObject *obj, NPObject *wrapper);
	void      RemoveWrappedObject (EventObject *obj);
	NPObject *LookupWrappedObject (EventObject *obj);
	
	void      AddCleanupPointer    (gpointer p);
	void      RemoveCleanupPointer (gpointer p);
	
	// [Obselete (this is obsolete in SL b2)]
	guint32 TimeoutAdd (gint32 interval, GSourceFunc callback, gpointer data);
	// [Obselete (this is obsolete in SL b2)]
	void    TimeoutStop (guint32 source_id);
	
	void Properties ();
	
	// Property getters and setters
	char *GetInitParams () { return this->initParams; }
	char *GetSource () { return this->source; }
	char *GetSourceLocation () { return this->source_location; }
	char *GetId () { return this->id; }
	
	void SetSource (const char *value);
	
	char *GetBackground ();
	bool SetBackground (const char *value);
	bool GetEnableFramerateCounter ();
	bool GetEnableRedrawRegions ();
	void SetEnableRedrawRegions (bool value);
	bool GetEnableHtmlAccess ();
	bool GetAllowHtmlPopupWindow ();
	bool GetWindowless ();
	void SetMaxFrameRate (int value);
	int  GetMaxFrameRate ();
	Deployment *GetDeployment ();
	
	BrowserBridge *GetBridge () { return bridge; }
	
	MoonlightScriptControlObject *GetRootObject ();
	NPP GetInstance ();
	NPWindow *GetWindow ();
	Surface *GetSurface () { return surface; }
	
	gint32 GetActualHeight ();
	gint32 GetActualWidth ();
	
	static gboolean plugin_button_press_callback (GtkWidget *widget, GdkEventButton *event, gpointer user_data);
	
	static Downloader *CreateDownloader (PluginInstance *instance);
	
#if DEBUG
	struct moon_source : List::Node {
		char *uri;
		char *filename;
		virtual ~moon_source ()
		{
			g_free (uri);
			g_free (filename);
		}
	};
	void AddSource (const char *uri, const char *filename);
	List *GetSources ();
#endif

#if PLUGIN_SL_2_0
	static bool MonoIsLoaded ();
	static bool DeploymentInit ();

	bool InitializePluginAppDomain ();
	bool CreatePluginDeployment ();

	gpointer ManagedCreateXamlLoaderForFile (XamlLoader* loader, const char *file);
	gpointer ManagedCreateXamlLoaderForString (XamlLoader* loader, const char *str);
	void ManagedLoaderDestroy (gpointer loader_object);
#endif
	
 private:
#if DEBUG
	List *moon_sources;
#endif

	// Gtk controls
	GtkWidget *container;  // plugin container object
 	Surface *surface;      // plugin surface object
	MoonWindow *moon_window;
	GdkDisplay *display;

	GSList *timers;

  	guint16 mode;          // NP_EMBED, NP_FULL, or NP_BACKGROUND
	NPWindow *window;      // Mozilla window object
	NPP instance;          // Mozilla instance object
	NPObject *rootobject;  // Mozilla jscript object wrapper
	guint32 xembed_supported; // XEmbed Extension supported

	GHashTable *wrapped_objects; // wrapped object cache

	GSList *cleanup_pointers;

	// Property fields
	char *initParams;
	char *source;
	char *source_location;
	guint source_idle;
	char *onLoad;
	char *background;
	char *onError;
	char *onResize;
	char *id;

	bool windowless;
	bool enable_html_access;
	bool allow_html_popup_window;
	int maxFrameRate;

	BrowserBridge *bridge;

	GtkWidget *properties_fps_label;
	GtkWidget *properties_cache_label;

	//
	// The XAML loader, contains a handle to a MonoObject *
	//
	PluginXamlLoader *xaml_loader;
	Deployment   *deployment;
#if PLUGIN_SL_2_0
	bool xap_loaded;

	MonoAssembly *system_windows_assembly;

	static bool mono_is_loaded;

	// Methods
	MonoMethod   *moon_load_xaml;
	MonoMethod   *moon_load_xap;
	MonoMethod   *moon_destroy_application;

	void LoadXAP  (const char*url, const char *fname);
	void DestroyApplication ();

	MonoMethod *MonoGetMethodFromName (MonoClass *klass, const char *name);

	bool ManagedInitializeDeployment (const char *file);
	void ManagedDestroyApplication ();

	gpointer ManagedCreateXamlLoader (XamlLoader* native_loader, const char *file, const char *str);
#endif

	// The name of the file that we are missing, and we requested to be loaded
	char *vm_missing_file;

	// Private methods
	void CreateWindow ();
	void UpdateSource ();
	void UpdateSourceByReference (const char *value);
	void LoadXAML ();
	void SetPageURL ();
	
	void TryLoadBridge (const char *prefix);
	
	static gboolean IdleUpdateSourceByReference (gpointer data);

	static void ReportFPS (Surface *surface, int nframes, float nsecs, void *user_data);
	static void ReportCache (Surface *surface, long bytes, void *user_data);
	static void properties_dialog_response (GtkWidget *dialog, int response, PluginInstance *plugin);
};

extern GSList *plugin_instances;

#define NPID(x) NPN_GetStringIdentifier (x)

#define STRDUP_FROM_VARIANT(v) (g_strndup ((char *) NPVARIANT_TO_STRING (v).utf8characters, NPVARIANT_TO_STRING (v).utf8length))
#define STRLEN_FROM_VARIANT(v) ((size_t) NPVARIANT_TO_STRING (v).utf8length)

#define STREAM_NOTIFY(x) ((StreamNotify*) x)

#define STREAM_NOTIFY_DATA(x) ((StreamNotify*) x)->pdata

#define IS_NOTIFY_SOURCE(x) \
	(!x ? true : (((StreamNotify*) x)->type == StreamNotify::SOURCE))

#define IS_NOTIFY_DOWNLOADER(x) \
	(!x ? StreamNotify::NONE : (((StreamNotify*) x)->type == StreamNotify::DOWNLOADER))

#define IS_NOTIFY_REQUEST(x) \
	(!x ? StreamNotify::NONE : (((StreamNotify*) x)->type == StreamNotify::REQUEST))

class StreamNotify
{
 public:
	enum StreamNotifyFlags {
		NONE = 0,
		SOURCE = 1,
		DOWNLOADER = 2,
		REQUEST = 3
	};
	
	StreamNotifyFlags type;
	void *pdata;
	
	StreamNotify () : type (NONE), pdata (NULL) {};
	StreamNotify (void *data) : type (NONE), pdata (data) {};
	StreamNotify (StreamNotifyFlags type) : type (type), pdata (NULL) {};
	StreamNotify (StreamNotifyFlags type, void *data) : type (type), pdata (data) {};
	StreamNotify (StreamNotifyFlags type, DependencyObject *dob) : type (type), pdata (dob)
	{
		if (dob)
			dob->ref ();
	}
	
	~StreamNotify ()
	{
		if (type == DOWNLOADER && pdata)
			((DependencyObject *) pdata)->unref ();
	}
};

class PluginXamlLoader : public XamlLoader
{
	PluginXamlLoader (const char *filename, const char *str, PluginInstance *plugin, Surface *surface);
	bool InitializeLoader ();
	PluginInstance *plugin;
	bool initialized;
	bool xaml_is_managed;
	
#if PLUGIN_SL_2_0
	gpointer managed_loader;
	Xap *xap;
#endif
 public:
	virtual ~PluginXamlLoader ();
	const char *TryLoad (int *error);

	bool SetProperty (void *parser, void *top_level, const char *xmlns, Value *target, void *target_data, void *target_parent, const char *name, Value* value, void* value_data);

	static PluginXamlLoader *FromFilename (const char *filename, PluginInstance *plugin, Surface *surface)
	{
		return new PluginXamlLoader (filename, NULL, plugin, surface);
	}
	
	static PluginXamlLoader *FromStr (const char *str, PluginInstance *plugin, Surface *surface)
	{
		return new PluginXamlLoader (NULL, str, plugin, surface);
	}
	
	bool IsManaged () { return xaml_is_managed; }
	virtual bool LoadVM ();
};

G_BEGIN_DECLS

const char *get_plugin_dir (void);

gint32 plugin_instance_get_actual_width  (PluginInstance *instance);
gint32 plugin_instance_get_actual_height (PluginInstance *instance);

Surface *plugin_instance_get_surface (PluginInstance *instance);

char *plugin_instance_get_init_params  (PluginInstance *instance);
char *plugin_instance_get_source  (PluginInstance *instance);
char *plugin_instance_get_source_location  (PluginInstance *instance);
char *plugin_instance_get_id (PluginInstance *instance);
NPObject *plugin_instance_get_host (PluginInstance *instance);

void plugin_instance_get_browser_runtime_settings (bool *debug, bool *html_access,
						   bool *httpnet_access, bool *script_access);

void plugin_instance_report_exception (PluginInstance *instance, char *msg, char *details, char **stack_trace, int num_frames);
void *plugin_instance_load_url (PluginInstance *instance, char *url, gint32 *length);

void *plugin_instance_evaluate (PluginInstance *instance, const char *code);

gboolean plugin_instance_get_enable_html_access (PluginInstance *instance);
gboolean plugin_instance_get_allow_html_popup_window (PluginInstance *instance);
gboolean plugin_instance_get_windowless (PluginInstance *instance);

PluginXamlLoader *plugin_xaml_loader_from_str (const char *str, PluginInstance *plugin, Surface *surface);

G_END_DECLS

#endif /* MOON_PLUGIN */

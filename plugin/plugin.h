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

class MoonlightScriptControlObject;
class PluginXamlLoader;
class PluginInstance;
class BrowserBridge;
#if PLUGIN_SL_2_0
class Xap;
#endif

typedef void plugin_unload_callback (PluginInstance *plugin);
char *NPN_strdup (const char *val);

class PluginInstance
{
 public:
	PluginInstance (NPMIMEType pluginType, NPP instance, uint16_t mode);
	~PluginInstance ();
	
	void SetUnloadCallback (plugin_unload_callback *puc);
	void Initialize (int argc, char *const argn[], char *const argv[]);
	void Finalize ();
	
	// Mozilla plugin related methods
	NPError GetValue (NPPVariable variable, void *result);
	NPError SetValue (NPNVariable variable, void *value);
	NPError SetWindow (NPWindow *window);
	NPError NewStream (NPMIMEType type, NPStream *stream, NPBool seekable, uint16_t *stype);
	NPError DestroyStream (NPStream *stream, NPError reason);
	void StreamAsFile (NPStream *stream, const char *fname);
	int32_t WriteReady (NPStream *stream);
	int32_t Write (NPStream *stream, int32_t offset, int32_t len, void *buffer);
	void UrlNotify (const char *url, NPReason reason, void *notifyData);
	void Print (NPPrint *platformPrint);
	int16_t EventHandle (void *event);
	void ReportException (char *msg, char *details, char **stack_trace, int num_frames);
	void *LoadUrl (char *url, int32_t *length);
	void *Evaluate (const char *code);
	
	NPObject *GetHost ();
	
	void      AddWrappedObject    (EventObject *obj, NPObject *wrapper);
	void      RemoveWrappedObject (EventObject *obj);
	NPObject *LookupWrappedObject (EventObject *obj);
	
	void      AddCleanupPointer    (gpointer p);
	void      RemoveCleanupPointer (gpointer p);
	
	// [Obselete (this is obsolete in SL b2)]
	uint32_t TimeoutAdd (int32_t interval, GSourceFunc callback, gpointer data);
	// [Obselete (this is obsolete in SL b2)]
	void     TimeoutStop (uint32_t source_id);
	
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
	bool GetWindowless ();
	void SetMaxFrameRate (int value);
	int  GetMaxFrameRate ();
	
	BrowserBridge *GetBridge () { return bridge; }
	
	MoonlightScriptControlObject *GetRootObject ();
	NPP GetInstance ();
	NPWindow *GetWindow ();
	Surface *GetSurface () { return surface; }
	
	int32_t GetActualHeight ();
	int32_t GetActualWidth ();
	
	void GetBrowserInformation (char **name, char **version,
				    char **platform, char **userAgent,
				    bool *cookieEnabled);
	bool IsSilverlight2 () { return silverlight2; } 
	
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

  	uint16_t mode;         // NP_EMBED, NP_FULL, or NP_BACKGROUND
	NPWindow *window;      // Mozilla window object
	NPP instance;          // Mozilla instance object
	NPObject *rootobject;  // Mozilla jscript object wrapper
	bool xembed_supported; // XEmbed Extension supported

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
	bool silverlight2;
	int maxFrameRate;

	BrowserBridge *bridge;

	GtkWidget *properties_fps_label;
	GtkWidget *properties_cache_label;

	//
	// The XAML loader, contains a handle to a MonoObject *
	//
	PluginXamlLoader *xaml_loader;
#if PLUGIN_SL_2_0
	bool xap_loaded;
#endif
	
	//
	// A (managed) callback to call when the plugin is unloaded.
	//
	plugin_unload_callback *plugin_unload;

	// The name of the file that we are missing, and we requested to be loaded
	char *vm_missing_file;

	// Private methods
	void CreateWindow ();
	void UpdateSource ();
	void UpdateSourceByReference (const char *value);
	void LoadXAML ();
#if PLUGIN_SL_2_0
	void LoadXAP  (const char *fname);
	void DestroyApplication ();
#endif
	void SetPageURL ();

	void TryLoadBridge (const char *prefix);
	
	static gboolean IdleUpdateSourceByReference (gpointer data);

	static void ReportFPS (Surface *surface, int nframes, float nsecs, void *user_data);
	static void ReportCache (Surface *surface, long bytes, void *user_data);
	static void properties_dialog_response (GtkWidget *dialog, int response, PluginInstance *plugin);
};

extern GSList *plugin_instances;

#define NPID(x) NPN_GetStringIdentifier (x)

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
	
	static PluginXamlLoader *FromFilename (const char *filename, PluginInstance *plugin, Surface *surface)
	{
		return new PluginXamlLoader (filename, NULL, plugin, surface);
	}
	
	static PluginXamlLoader *FromStr (const char *str, PluginInstance *plugin, Surface *surface)
	{
		return new PluginXamlLoader (NULL, str, plugin, surface);
	}
	
	bool IsManaged () { return xaml_is_managed; }
	virtual bool HookupEvent (void *target, void *dest, const char *name, const char *value);
	virtual bool LoadVM ();
};

G_BEGIN_DECLS

int32_t plugin_instance_get_actual_width  (PluginInstance *instance);
int32_t plugin_instance_get_actual_height (PluginInstance *instance);

Surface *plugin_instance_get_surface (PluginInstance *instance);

char *plugin_instance_get_init_params  (PluginInstance *instance);
char *plugin_instance_get_source  (PluginInstance *instance);
char *plugin_instance_get_source_location  (PluginInstance *instance);
char *plugin_instance_get_id (PluginInstance *instance);
NPObject *plugin_instance_get_host (PluginInstance *instance);

void plugin_instance_get_browser_information (PluginInstance *instance,
					      char **name, char **version,
					      char **platform, char **userAgent,
					      bool *cookieEnabled);

void plugin_instance_get_browser_runtime_settings (bool *debug, bool *html_access,
						   bool *httpnet_access, bool *script_access);

void plugin_instance_report_exception (PluginInstance *instance, char *msg, char *details, char **stack_trace, int num_frames);
void *plugin_instance_load_url (PluginInstance *instance, char *url, int32_t *length);

void *plugin_instance_evaluate (PluginInstance *instance, const char *code);

void     plugin_set_unload_callback (PluginInstance *instance, plugin_unload_callback *puc);
PluginXamlLoader *plugin_xaml_loader_from_str (const char *str, PluginInstance *plugin, Surface *surface);

G_END_DECLS

#endif /* MOON_PLUGIN */

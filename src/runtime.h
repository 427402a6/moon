/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * runtime.h: Core surface and canvas definitions.
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#ifndef __RUNTIME_H__
#define __RUNTIME_H__

#include <glib.h>

#include <stdint.h>
#include <cairo.h>
#include <gtk/gtkwidget.h>

#include "point.h"
#include "uielement.h"
#include "dependencyobject.h"
#include "dirty.h"
#include "value.h"
#include "type.h"
#include "list.h"
#include "error.h"
#include "window.h"

#define MAXIMUM_CACHE_SIZE 6000000

#define FRONT_TO_BACK_STATS 0

#define TIMERS 0
#define DEBUG_MARKER_KEY 0
#if TIMERS
#define STARTTIMER(id,str) TimeSpan id##_t_start = get_now(); printf ("timing of '%s' started at %lld\n", str, id##_t_start)
#define ENDTIMER(id,str) TimeSpan id##_t_end = get_now(); printf ("timing of '%s' ended at %lld (%f seconds)\n", str, id##_t_end, (double)(id##_t_end - id##_t_start) / 10000000)
#else
#define STARTTIMER(id,str)
#define ENDTIMER(id,str)
#endif

#if SANITY
#define VERIFY_MAIN_THREAD \
	if (!Surface::InMainThread ()) {	\
		printf ("Moonlight: This method should be only be called from the main thread (%s)\n", __PRETTY_FUNCTION__);	\
	}
#else
#define VERIFY_MAIN_THREAD
#endif

enum RuntimeInitFlags {
	RUNTIME_INIT_PANGO_TEXT_LAYOUT     = 1 << 0,
	// (not used)                      = 1 << 1,
	RUNTIME_INIT_MANUAL_TIMESOURCE     = 1 << 2,
	RUNTIME_INIT_DISABLE_AUDIO         = 1 << 3,
	RUNTIME_INIT_SHOW_EXPOSE           = 1 << 4,
	RUNTIME_INIT_SHOW_CLIPPING         = 1 << 5,
	RUNTIME_INIT_SHOW_BOUNDING_BOXES   = 1 << 6,
	RUNTIME_INIT_SHOW_TEXTBOXES        = 1 << 7,
	RUNTIME_INIT_SHOW_FPS              = 1 << 8,
	RUNTIME_INIT_RENDER_FRONT_TO_BACK  = 1 << 9,
	RUNTIME_INIT_SHOW_CACHE_SIZE	   = 1 << 10,
	RUNTIME_INIT_FFMPEG_YUV_CONVERTER  = 1 << 11,
	RUNTIME_INIT_USE_SHAPE_CACHE	   = 1 << 12,
	RUNTIME_INIT_USE_UPDATE_POSITION   = 1 << 13,
	RUNTIME_INIT_ALLOW_WINDOWLESS      = 1 << 14,
	RUNTIME_INIT_AUDIO_ALSA_MMAP       = 1 << 15,
	RUNTIME_INIT_AUDIO_ALSA_RW         = 1 << 16,
	RUNTIME_INIT_AUDIO_ALSA            = 1 << 17,
	RUNTIME_INIT_AUDIO_PULSE           = 1 << 18,
	RUNTIME_INIT_USE_IDLE_HINT         = 1 << 19,
	RUNTIME_INIT_USE_BACKEND_XLIB      = 1 << 20,
	RUNTIME_INIT_KEEP_MEDIA            = 1 << 21,
	RUNTIME_INIT_ENABLE_MS_CODECS      = 1 << 22,
	RUNTIME_INIT_DISABLE_FFMPEG_CODECS = 1 << 23,
	RUNTIME_INIT_ALL_IMAGE_FORMATS     = 1 << 24,
	RUNTIME_INIT_CREATE_ROOT_DOMAIN    = 1 << 25,
	RUNTIME_INIT_DESKTOP_EXTENSIONS    = 1 << 26,
};

extern guint64 moonlight_flags;


#if DEBUG
enum RuntimeDebugFlags {
	RUNTIME_DEBUG_ALSA              = 1 << 0,
	RUNTIME_DEBUG_ALSA_EX           = 1 << 1,
	RUNTIME_DEBUG_AUDIO             = 1 << 2,
	RUNTIME_DEBUG_AUDIO_EX          = 1 << 3,
	RUNTIME_DEBUG_PULSE             = 1 << 4,
	RUNTIME_DEBUG_PULSE_EX          = 1 << 5,
	RUNTIME_DEBUG_HTTPSTREAMING     = 1 << 6,
	RUNTIME_DEBUG_MARKERS           = 1 << 7,
	RUNTIME_DEBUG_MARKERS_EX        = 1 << 8,
	RUNTIME_DEBUG_MMS               = 1 << 9,
	RUNTIME_DEBUG_MEDIAPLAYER       = 1 << 10,
	RUNTIME_DEBUG_MEDIAPLAYER_EX    = 1 << 11,
	RUNTIME_DEBUG_PIPELINE          = 1 << 12,
	RUNTIME_DEBUG_PIPELINE_ERROR    = 1 << 13,
	RUNTIME_DEBUG_FRAMEREADERLOOP   = 1 << 14,
	RUNTIME_DEBUG_FFMPEG            = 1 << 15,
	RUNTIME_DEBUG_UI                = 1 << 16,
	RUNTIME_DEBUG_CODECS            = 1 << 17,
	RUNTIME_DEBUG_DP                = 1 << 18,
	RUNTIME_DEBUG_DOWNLOADER        = 1 << 19,
	RUNTIME_DEBUG_FONT              = 1 << 20,
	RUNTIME_DEBUG_LAYOUT            = 1 << 21,
	RUNTIME_DEBUG_MEDIA             = 1 << 22,
	RUNTIME_DEBUG_MEDIAELEMENT      = 1 << 23,
	RUNTIME_DEBUG_MEDIAELEMENT_EX   = 1 << 24,
	RUNTIME_DEBUG_BUFFERING         = 1 << 25,
	RUNTIME_DEBUG_ASF               = 1 << 26,
	RUNTIME_DEBUG_PLAYLIST          = 1 << 27,
	RUNTIME_DEBUG_PLAYLIST_WARN     = 1 << 28,
	RUNTIME_DEBUG_TEXT              = 1 << 29,
	RUNTIME_DEBUG_XAML              = 1 << 30,
	RUNTIME_DEBUG_DEPLOYMENT        = 1ULL << 31,
	/* Add more as RUNTIME_DEBUG_XXX = 1ULL << 32, */
	RUNTIME_DEBUG_MSI		= 1ULL << 32,
	RUNTIME_DEBUG_MP3               = 1ULL << 33,
};

extern guint64 debug_flags;
#endif


class TimeManager;
class Surface;
class Downloader;

typedef void (* MoonlightFPSReportFunc) (Surface *surface, int nframes, float nsecs, void *user_data);
typedef void (* MoonlightCacheReportFunc) (Surface *surface, long size, void *user_data);
typedef bool (* MoonlightEventEmitFunc) (UIElement *element, GdkEvent *event);
typedef void (* MoonlightExposeHandoffFunc) (Surface *surface, TimeSpan time, void *user_data);

class Surface : public EventObject {
public:
	/* @GenerateCBinding,GeneratePInvoke */
	Surface (MoonWindow *window);
	virtual void Dispose ();
	
	/* @GenerateCBinding */
	MoonWindow *GetWindow () { return active_window; }
	
	// allows you to redirect painting of the surface to an
	// arbitrary cairo context.
	void Paint (cairo_t *ctx, Region *region);
	/* @GenerateCBinding,GeneratePInvoke */
	void Paint (cairo_t *ctx, int x, int y, int width, int height);

	/* @GenerateCBinding,GeneratePInvoke */
	void Attach (UIElement *toplevel);

	void AttachLayer (UIElement *layer);

	void DetachLayer (UIElement *layer);
	
	void SetCursor (MouseCursor cursor);

	bool SetMouseCapture (UIElement *capture);
	
	/* @GenerateCBinding,GeneratePInvoke */
	void Resize (int width, int height);

	void EmitError (ErrorEventArgs *args);
	void EmitLoad ();
	
	void SetBackgroundColor (Color *color);
	/* @GenerateCBinding,GeneratePInvoke */
	Color *GetBackgroundColor ();

	int GetFrameCount () { return frames; }
	void ResetFrameCount () { frames = 0; }

	virtual void Invalidate (Rect r);
	virtual void ProcessUpdates ();

	UIElement *GetToplevel() { return toplevel; }
	bool IsTopLevel (UIElement *top);

	/* @GenerateCBinding,GeneratePInvoke,Version=2.0 */
	UIElement *GetFocusedElement () { return focused_element; }

	bool FocusElement (UIElement *element);

	/* @GenerateCBinding,GeneratePInvoke */
	bool IsLoaded () { return toplevel != NULL; }

	/* @GenerateCBinding,GeneratePInvoke */
	static bool IsVersionSupported (const char *version);

	const static int ResizeEvent;
	const static int FullScreenChangeEvent;
	const static int ErrorEvent;
	const static int LoadEvent;

	/* @GenerateCBinding,GeneratePInvoke,Version=2.0 */
	bool GetFullScreen () { return full_screen; }
	/* @GenerateCBinding,GeneratePInvoke,Version=2.0 */
	void SetFullScreen (bool value);
	void SetCanFullScreen (bool value) { can_full_screen = value; }
	const char* GetSourceLocation ();
	void SetSourceLocation (const char *location);
	bool FullScreenKeyHandled (GdkEventKey *key);
	
	/* @GenerateCBinding,GeneratePInvoke */
	TimeManager *GetTimeManager () { return time_manager; }

	void SetDownloaderContext (gpointer context) { downloader_context = context; }
	gpointer GetDownloaderContext () { return downloader_context; }
	
	/* @GenerateCBinding,GeneratePInvoke */
	Downloader *CreateDownloader ();
	static Downloader *CreateDownloader (EventObject *obj);

	void SetFPSReportFunc (MoonlightFPSReportFunc report, void *user_data);
	void SetCacheReportFunc (MoonlightCacheReportFunc report, void *user_data);
	void SetExposeHandoffFunc (MoonlightExposeHandoffFunc func, void *user_data);

	bool VerifyWithCacheSizeCounter (int w, int h);
	gint64 AddToCacheSizeCounter (int w, int h);
	void RemoveFromCacheSizeCounter (gint64 size);

	// called from the plugin if the surface is headed for death.
	// stops event emission (since the plugin counterparts to xaml
	// objects will be destroyed)
	void Zombify ();
	
	void DetachDownloaders ();
	
#if FRONT_TO_BACK_STATS
	int uielements_rendered_front_to_back;
	int uielements_rendered_back_to_front;
#endif

#ifdef DEBUG
	UIElement *debug_selected_element;
#endif

	void PaintToDrawable (GdkDrawable *drawable, GdkVisual *visual, GdkEventExpose *event, int off_x, int off_y, bool transparent, bool clear_transparent);


	gboolean HandleUIMotion (GdkEventMotion *event);
	gboolean HandleUICrossing (GdkEventCrossing *event);
	gboolean HandleUIKeyPress (GdkEventKey *event);
	gboolean HandleUIKeyRelease (GdkEventKey *event);
	gboolean HandleUIButtonRelease (GdkEventButton *event);
	gboolean HandleUIButtonPress (GdkEventButton *event);
	gboolean HandleUIScroll (GdkEventScroll *event);
	gboolean HandleUIFocusIn (GdkEventFocus *event);
	gboolean HandleUIFocusOut (GdkEventFocus *event);
	void HandleUIWindowAllocation (bool emit_resize);
	void HandleUIWindowAvailable ();
	void HandleUIWindowUnavailable ();
	void HandleUIWindowDestroyed (MoonWindow *window);

	// bad, but these live in dirty.cpp, not runtime.cpp
	void AddDirtyElement (UIElement *element, DirtyType dirt);
	void UpdateLayout ();
	void RemoveDirtyElement (UIElement *element);
	void ProcessDirtyElements ();
	void PropagateDirtyFlagToChildren (UIElement *element, DirtyType dirt);
	bool IsAnythingDirty ();

	static pthread_t main_thread;
	/* @GenerateCBinding,GeneratePInvoke */
	static bool InMainThread () { return (pthread_equal (main_thread, pthread_self ()) || pthread_equal (main_thread, NULL)); }
	
protected:
	// The current window we are drawing to
	MoonWindow *active_window;
	
	virtual ~Surface();

private:
	// are we headed for death?
	bool zombie;

	// bad, but these two live in dirty.cpp, not runtime.cpp
	void ProcessDownDirtyElements ();
	void ProcessUpDirtyElements ();

	DirtyLists *down_dirty;
	DirtyLists *up_dirty;
	
	List *measure_dirty;
	List *arrange_dirty;

	gpointer downloader_context;
	List *downloaders;
	static void OnDownloaderDestroyed (EventObject *sender, EventArgs *args, gpointer closure);
	
	Color *background_color;
	
	// This is the normal-sized window
	MoonWindow *normal_window;
	
	// We set active_window to this whenever we are in fullscreen mode.
	MoonWindow *fullscreen_window;
	
	// We can have multiple top level elements, these are stored as layers
	HitTestCollection *layers;
	
	// This currently can only be a canvas.
	UIElement *toplevel;

	// The element holding the keyboard focus, and the one that
	// held it previously (so we can emit lostfocus events async)
	UIElement *focused_element;
	UIElement *prev_focused_element;

	// the list of elements (from most deeply nested to the
	// toplevel) we've most recently sent a mouse event to.
	List *input_list;
	
	// is the mouse captured?  if it is, it'll be by the first element in input_list.
	bool captured;
	UIElement *pendingCapture;
	bool pendingReleaseCapture;
	
	// are we currently emitting a mouse event?
	bool emittingMouseEvent;
	
	// the currently shown cursor
	MouseCursor cursor;
	
	// Fullscreen support
	bool full_screen;
	Canvas *full_screen_message;
	char *source_location;
	// Should be set to true only while executing MouseLeftButtonDown, 
	// MouseLeftButtonUp, KeyDown, and KeyUp event handlers
	bool can_full_screen; 
	
	void UpdateFullScreen (bool value);
	
	TimeManager *time_manager;
	
	int frames;
	
	GdkEvent *mouse_event;
	
	// Variables for reporting FPS
	MoonlightFPSReportFunc fps_report;
	gint64 fps_start;
	int fps_nframes;
	void *fps_data;
	
	// Variables for reporting cache size
	MoonlightCacheReportFunc cache_report;
	gint64 cache_size_in_bytes;
	int cache_size_ticker;
	void *cache_data;
	int cache_size_multiplier;

	// Expose handoff
	TimeSpan expose_handoff_last_timespan;
	MoonlightExposeHandoffFunc expose_handoff;
	void *expose_handoff_data;
	
	void Realloc ();
	void ShowFullScreenMessage ();
	void HideFullScreenMessage ();
	
	void CreateSimilarSurface ();
	
	static void render_cb (EventObject *sender, EventArgs *calldata, gpointer closure);
	static void update_input_cb (EventObject *sender, EventArgs *calldata, gpointer closure);
	static void widget_destroyed (GtkWidget *w, gpointer data);
	
	EventArgs* CreateArgsForEvent (int event_id, GdkEvent *event);

	List* ElementPathToRoot (UIElement *source);
	void GenerateFocusChangeEvents();
	static void generate_focus_change_events (EventObject *object);
	bool focus_tick_call_added;

	void FindFirstCommonElement (List *l1, int *index1, List *l2, int *index2);
	bool EmitEventOnList (int event_id, List *element_list, GdkEvent *event, int end_idx);
	void UpdateCursorFromInputList ();
	bool HandleMouseEvent (int event_id, bool emit_leave, bool emit_enter, bool force_emit, GdkEvent *event);
	void PerformCapture (UIElement *capture);
	void PerformReleaseCapture ();
};

/* for hit testing */
class UIElementNode : public List::Node {
public:
	UIElement *uielement;
		
	UIElementNode (UIElement *el);
	virtual ~UIElementNode ();
};

/* for rendering */
typedef void (*RenderFunc) (cairo_t *ctx, UIElement *uielement, Region *region, bool front_to_back);

class RenderNode : public List::Node {
public:
	RenderNode (UIElement *el, Region *region, bool render_element, RenderFunc pre, RenderFunc post);
	
	void Render (cairo_t *cr);

	virtual ~RenderNode ();

private:
	UIElement *uielement;
	Region *region;
	bool render_element;
	RenderFunc pre_render;
	RenderFunc post_render;

};

G_BEGIN_DECLS

void     runtime_init (const char *platform_dir, guint64 flags);

void     runtime_init_browser (const char *plugin_dir);
void     runtime_init_desktop (void);

GList   *runtime_get_surface_list (void);

void	 runtime_flags_set_manual_timesource (gboolean flag);
void	 runtime_flags_set_show_fps (gboolean flag);
void	 runtime_flags_set_use_shapecache (gboolean flag);

void     runtime_shutdown (void);

G_END_DECLS

#endif

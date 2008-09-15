/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * runtime.cpp: Core surface and canvas definitions.
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include <gtk/gtk.h>
#include <glib.h>

#define Visual _XxVisual
#define Region _XxRegion
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <cairo-xlib.h>
#undef Visual
#undef Region

#include "runtime.h"
#include "canvas.h"
#include "color.h"
#include "shape.h"
#include "transform.h"
#include "animation.h"
#include "downloader.h"
#include "frameworkelement.h"
#include "media.h"
#include "stylus.h"
#include "rect.h"
#include "text.h"
#include "panel.h"
#include "value.h"
#include "namescope.h"
#include "xaml.h"
#include "dirty.h"
#include "fullscreen.h"
#include "utils.h"
#include "window-gtk.h"

#if SL_2_0
#include "contentcontrol.h"
#include "usercontrol.h"
#include "deployment.h"
#include "grid.h"
#endif

//#define DEBUG_INVALIDATE 1
//#define RENDER_INDIVIDUALLY 1
#define DEBUG_REFCNT 0

#define CAIRO_CLIP 0
#define TIME_CLIP 0
#define TIME_REDRAW 1

#define NO_EVENT_ID -1

pthread_t Surface::main_thread = 0;

static bool inited = false;
static bool g_type_inited = false;
guint32 moonlight_flags = 0;
static GList* surface_list = NULL;

static struct {
	const char *override;
	guint32 flag;
	bool set;
} overrides[] = {
	{ "codecs=microsoft",  RUNTIME_INIT_MICROSOFT_CODECS,      true  },
	{ "codecs=ffmpeg",     RUNTIME_INIT_MICROSOFT_CODECS,      false },
	{ "timesource=manual", RUNTIME_INIT_MANUAL_TIMESOURCE,     true  },
	{ "timesource=system", RUNTIME_INIT_MANUAL_TIMESOURCE,     false },
	{ "expose=show",       RUNTIME_INIT_SHOW_EXPOSE,           true  },
	{ "expose=hide",       RUNTIME_INIT_SHOW_EXPOSE,           false },
	{ "clipping=show",     RUNTIME_INIT_SHOW_CLIPPING,         true  },
	{ "clipping=hide",     RUNTIME_INIT_SHOW_CLIPPING,         false },
	{ "bbox=show",         RUNTIME_INIT_SHOW_BOUNDING_BOXES,   true  },
	{ "bbox=hide",         RUNTIME_INIT_SHOW_BOUNDING_BOXES,   false },
	{ "textbox=show",      RUNTIME_INIT_SHOW_TEXTBOXES,        true  },
	{ "textbox=hide",      RUNTIME_INIT_SHOW_TEXTBOXES,        false },
	{ "fps=show",          RUNTIME_INIT_SHOW_FPS,              true  },
	{ "fps=hide",          RUNTIME_INIT_SHOW_FPS,              false },
	{ "render=ftb",        RUNTIME_INIT_RENDER_FRONT_TO_BACK,  true  },
	{ "render=btf",        RUNTIME_INIT_RENDER_FRONT_TO_BACK,  false },
	{ "cache=show",	       RUNTIME_INIT_SHOW_CACHE_SIZE,       true  },
	{ "cache=hide",        RUNTIME_INIT_SHOW_CACHE_SIZE,       false },
	{ "converter=default", RUNTIME_INIT_FFMPEG_YUV_CONVERTER,  false },
	{ "converter=ffmpeg",  RUNTIME_INIT_FFMPEG_YUV_CONVERTER,  true  },
	{ "shapecache=yes",    RUNTIME_INIT_USE_SHAPE_CACHE,	   true  },
	{ "shapecache=no",     RUNTIME_INIT_USE_SHAPE_CACHE,	   false },
	{ "updatepos=yes",     RUNTIME_INIT_USE_UPDATE_POSITION,   true  },
	{ "updatepos=no",      RUNTIME_INIT_USE_UPDATE_POSITION,   false },
	{ "windowless=yes",    RUNTIME_INIT_ALLOW_WINDOWLESS,      true  },
	{ "windowless=no",     RUNTIME_INIT_ALLOW_WINDOWLESS,      false },
	{ "audio=alsa",        RUNTIME_INIT_AUDIO_ALSA,            true  },
	{ "audio=alsa-mmap",   RUNTIME_INIT_AUDIO_ALSA_MMAP,       true  },
	{ "audio=alsa-rw",     RUNTIME_INIT_AUDIO_ALSA_RW,         true  },
	{ "audio=pulseaudio",  RUNTIME_INIT_AUDIO_PULSE,           true  },
	{ "audio=debug",       RUNTIME_INIT_AUDIO_DEBUG,           true  },
	{ "idlehint=yes",      RUNTIME_INIT_USE_IDLE_HINT,         false },
	{ "idlehint=no",       RUNTIME_INIT_USE_IDLE_HINT,         true  },
	{ "backend=xlib",      RUNTIME_INIT_USE_BACKEND_XLIB,      true  },
	{ "backend=image",     RUNTIME_INIT_USE_BACKEND_XLIB,      false }
};

#define RENDER_EXPOSE (moonlight_flags & RUNTIME_INIT_SHOW_EXPOSE)


static void
fps_report_default (Surface *surface, int nframes, float nsecs, void *user_data)
{
	printf ("Rendered %d frames in %.3fs = %.3f FPS\n", nframes, nsecs, nframes / nsecs);
}

static void
cache_report_default (Surface *surface, long bytes, void *user_data)
{
	printf ("Cache size is ~%.3f MB\n", bytes / 1048576.0);
}

GList *
runtime_get_surface_list (void)
{
	if (!Surface::InMainThread ()) {
		g_warning ("This method can be only called from the main thread!\n");
		return NULL;
	}
	
	return surface_list;
}

static cairo_t *
runtime_cairo_create (GdkWindow *drawable, GdkVisual *visual, bool native)
{
	int width, height;
	cairo_surface_t *surface;
	cairo_t *cr;

	gdk_drawable_get_size (drawable, &width, &height);

	if (native)
		surface = cairo_xlib_surface_create (gdk_x11_drawable_get_xdisplay (drawable),
						     gdk_x11_drawable_get_xid (drawable),
						     GDK_VISUAL_XVISUAL (visual),
						     width, height);
	else 
		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

	cr = cairo_create (surface);
	cairo_surface_destroy (surface);
			    
	return cr;
}

static gboolean
flags_can_be_modifed (void)
{
	if (g_list_length (surface_list) != 0) {
		g_warning ("Flags can be dynamically modified only when there are no surfaces created!");
		return FALSE;
	} else if (inited == FALSE) {
		g_warning ("Runtime has not been initialized yet, your flags will be overriden!");
		return FALSE;
	} else 
		return TRUE;
}

void
runtime_flags_set_manual_timesource (gboolean flag)
{
	if (flags_can_be_modifed ())
		moonlight_flags |= RUNTIME_INIT_MANUAL_TIMESOURCE;
}

void
runtime_flags_set_use_shapecache (gboolean flag)
{
	if (flags_can_be_modifed ())
		moonlight_flags |= RUNTIME_INIT_USE_SHAPE_CACHE;
}

void
runtime_flags_set_show_fps (gboolean flag)
{
	if (flags_can_be_modifed ())
		moonlight_flags |= RUNTIME_INIT_SHOW_FPS;
}

/* FIXME More flag setters here */

Surface::Surface (MoonWindow *window, bool silverlight2)
{
	main_thread = pthread_self ();

	this->silverlight2 = silverlight2;

	zombie = false;
	downloader_context = NULL;
	downloaders = NULL;
	transparent = false;
	background_color = NULL;
	cursor = MouseCursorDefault;
	mouse_event = NULL;
	
	background_color = new Color (1, 1, 1, 0);

	time_manager = new TimeManager ();
	time_manager->Start ();

	fullscreen_window = NULL;
	normal_window = active_window = window;
	if (active_window->IsFullScreen())
		g_warning ("Surfaces cannot be initialized with fullscreen windows.");
	window->SetSurface (this);
	
	toplevel = NULL;
	input_list = new List ();
	captured = false;
	
	focused_element = NULL;
	prev_focused_element = NULL;
	focus_tick_call_added = false;

	full_screen = false;
	can_full_screen = false;

	full_screen_message = NULL;
	source_location = NULL;

	fps_report = fps_report_default;
	fps_data = NULL;

	fps_nframes = 0;
	fps_start = 0;

	cache_report = cache_report_default;
	cache_data = NULL;

	cache_size_in_bytes = 0;
	cache_size_ticker = 0;
	cache_size_multiplier = -1;

	expose_handoff = NULL;
	expose_handoff_data = NULL;
	expose_handoff_last_timespan = G_MAXINT64; 

	emittingMouseEvent = false;
	pendingCapture = NULL;
	pendingReleaseCapture = false;

#ifdef DEBUG
	debug_selected_element = NULL;
#endif
	if (!(moonlight_flags & RUNTIME_INIT_USE_BACKEND_XLIB))
		g_warning ("using software backend");

	up_dirty = new List ();
	down_dirty = new List ();
	
	surface_list = g_list_append (surface_list, this);
}

Surface::~Surface ()
{
	//
	// This removes one source of problems: the unrealize handler is not
	// being called when Mozilla destroys our window, so we remove it here.
	//
	// This is easy to trigger, open two clocks and try to load a different
	// page on one of the pages (that keeps the timer ticking, and eventually
	// kills this
	//
	// There is still another problem: sometimes we are getting:
	//     The error was 'RenderBadPicture (invalid Picture parameter)'.
	//
	// And I have yet to track what causes this, the stack trace is not 
	// very useful
	//
	time_manager->RemoveHandler (TimeManager::RenderEvent, render_cb, this);
	time_manager->RemoveHandler (TimeManager::UpdateInputEvent, update_input_cb, this);
		
	if (toplevel) {
		toplevel->SetSurface (NULL);
		toplevel->unref ();
	}
	
#if DEBUG
	if (debug_selected_element) {
		debug_selected_element->unref ();
		debug_selected_element = NULL;
	}
#endif
	
	if (full_screen_message)
		HideFullScreenMessage ();
	
	delete input_list;
	
	g_free (source_location);

	if (fullscreen_window)
		delete fullscreen_window;
	
	if (normal_window)
		delete normal_window;
	
	delete background_color;
	
	time_manager->unref ();
	
	EventObject::DrainUnrefs ();
	
	delete up_dirty;
	delete down_dirty;
	
	delete downloaders;
	
	surface_list = g_list_remove (surface_list, this);
}

void
Surface::Zombify ()
{
	time_manager->Shutdown ();
	DetachDownloaders ();
	zombie = true;
}

/* XPM */
static const char *dot[] = {
	"18 18 4 1",
	"       c None",
	".      c #808080",
	"+      c #303030",
	"@      c #000000",
	".+.               ",
	"@@@               ",
	".@.               ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  ",
	"                  "
};

/* XPM */
static const char *eraser[] = {
	"20 20 49 1",
	"       c None",
	".      c #000000",
	"+      c #858585",
	"@      c #E8E8E8",
	"#      c #E9E9E9",
	"$      c #E7E7E7",
	"%      c #E2E2E2",
	"&      c #D6D6D6",
	"*      c #7D7D7D",
	"=      c #565656",
	"-      c #E1E1E1",
	";      c #E0E0E0",
	">      c #DEDEDE",
	",      c #DFDFDF",
	"'      c #474747",
	")      c #6C6C6C",
	"!      c #B0B0B0",
	"~      c #E3E3E3",
	"{      c #4E4E4E",
	"]      c #636363",
	"^      c #E6E6E6",
	"/      c #505050",
	"(      c #4A4A4A",
	"_      c #C7C7C7",
	":      c #272727",
	"<      c #797979",
	"[      c #E5E5E5",
	"}      c #DDDDDD",
	"|      c #9C9C9C",
	"1      c #232323",
	"2      c #E4E4E4",
	"3      c #656565",
	"4      c #313131",
	"5      c #EAEAEA",
	"6      c #ECECEC",
	"7      c #EEEEEE",
	"8      c #EFEFEF",
	"9      c #F0F0F0",
	"0      c #999999",
	"a      c #5D5D5D",
	"b      c #343434",
	"c      c #757575",
	"d      c #383838",
	"e      c #CECECE",
	"f      c #A9A9A9",
	"g      c #6F6F6F",
	"h      c #B3B3B3",
	"i      c #787878",
	"j      c #3F3F3F",
	"                    ",
	"                    ",
	"                    ",
	"                    ",
	"                    ",
	"                    ",
	"       ...........  ",
	"      .+@#@@@$$%&*. ",
	"      =-;%>>>>>>>,' ",
	"     )!~>>>>>>>>>>{ ",
	"     ]^>>>>>>>>>,,/ ",
	"    (_;>>>>>>>>,>&: ",
	"    <[,}>>>>>>>-,|  ",
	"   1[-;>>>>>>>$2,3  ",
	"   45678999998550a  ",
	"   b~,,,,,,,,,;$c   ",
	"   de-,,,,,,,,,fg   ",
	"   bh%%,,;}}}>>ij   ",
	"    ............    ",
	"                    "
};

void
Surface::SetCursor (MouseCursor new_cursor)
{
	if (new_cursor != cursor) {
		cursor = new_cursor;

		GdkCursor *c = NULL;
		GdkPixmap *empty = gdk_bitmap_create_from_data (NULL, "0x00", 1, 1);
		GdkColor empty_color = {0, 0, 0, 0};
		switch (cursor) {
		case MouseCursorDefault:
			c = NULL;
			break;
		case MouseCursorArrow:
			c = gdk_cursor_new (GDK_LEFT_PTR);
			break;
		case MouseCursorHand:
			c = gdk_cursor_new (GDK_HAND2);
			break;
		case MouseCursorWait:
			c = gdk_cursor_new (GDK_WATCH);
			break;
		case MouseCursorIBeam:
			c = gdk_cursor_new (GDK_XTERM);
			break;
		case MouseCursorStylus:
			c = gdk_cursor_new_from_pixbuf (gdk_display_get_default (), gdk_pixbuf_new_from_xpm_data ((const char**) dot), 0, 0);
			break;
		case MouseCursorEraser:
			c = gdk_cursor_new_from_pixbuf (gdk_display_get_default (), gdk_pixbuf_new_from_xpm_data ((const char**) eraser), 8, 8);
			break;
		case MouseCursorNone:
		// Silverlight display no cursor if the enumeration value is invalid (e.g. -1)
		default:
			//from gdk-cursor doc :"To make the cursor invisible, use gdk_cursor_new_from_pixmap() to create a cursor with no pixels in it."
			c = gdk_cursor_new_from_pixmap (empty, empty, &empty_color, &empty_color, 0, 0);
			break;
		}

		SetCursor (c);

		// XXX unref the cursor?
	}
}

void
Surface::SetCursor (GdkCursor *c)
{
	active_window->SetCursor (c);
}

void
Surface::Attach (UIElement *element)
{
	bool first = false;

#if DEBUG
	// Attach must be called with NULL to clear out the old canvas 
	// before attaching another canvas, otherwise the new canvas 
	// might get loaded with data from the old canvas (when parsing
	// xaml ticks will get added to the timemanager of the surface,
	// if the old canvas isn't gone when the new canvas is parsed,
	// the ticks will be added to the old timemanager).
	if (toplevel != NULL && element != NULL)
		g_warning ("Surface::Attach (NULL) should be called to clear out the old canvas before adding a new canvas.");
#endif

				
	if (toplevel) {
		toplevel->SetSurface (NULL);
		time_manager->RemoveHandler (TimeManager::RenderEvent, render_cb, this);
		time_manager->RemoveHandler (TimeManager::UpdateInputEvent, update_input_cb, this);
		time_manager->Stop ();
		time_manager->unref ();
		toplevel->unref ();
		time_manager = new TimeManager ();
		time_manager->AddHandler (TimeManager::RenderEvent, render_cb, this);
		time_manager->AddHandler (TimeManager::UpdateInputEvent, update_input_cb, this);
		time_manager->NeedRedraw ();
		time_manager->Start ();
	} else 
		first = true;

	if (!element) {
		DetachDownloaders ();

		if (first)
			active_window->EnableEvents (first);

		active_window->Invalidate();

		toplevel = NULL;
		return;
	}

	if (!element->Is (Type::UIELEMENT)) {
		printf ("Surface::Attach Unsupported toplevel %s\n", Type::Find (element->GetObjectType ())->GetName ());
		return;
	}

	UIElement *canvas = element;
	canvas->ref ();

	// make sure we have a namescope at the toplevel so that names
	// can be registered/resolved properly.
	if (NameScope::GetNameScope (canvas) == NULL) {
		NameScope::SetNameScope (canvas, new NameScope());
	}

	canvas->SetSurface (this);
	toplevel = canvas;

	// First time we connect the surface, start responding to events
	if (first)
		active_window->EnableEvents (first);

	canvas->OnLoaded ();
	
	if (zombie)
		return;

	Emit (Surface::LoadEvent);
	
	if (active_window->HasFocus())
		canvas->EmitGotFocus ();
	
	//
	// If the did not get a size specified
	//
	if (normal_window->GetWidth() == 0 && normal_window->GetHeight() == 0 && toplevel) {
		/*
		 * this should only be hit in the nonplugin case ans is
		 * simply here to give a reasonable default size
		 */
		Value *vh, *vw;
		vw = toplevel->GetValue (FrameworkElement::WidthProperty);
		vh = toplevel->GetValue (FrameworkElement::HeightProperty);
		if (vh || vw)
			normal_window->Resize (MAX (vw ? (int)vw->AsDouble () : 0, 0),
					       MAX (vh ? (int)vh->AsDouble () : 0, 0));
	}

	Emit (ResizeEvent);

	toplevel->UpdateTotalRenderVisibility ();
	toplevel->UpdateTotalHitTestVisibility ();
	toplevel->FullInvalidate (true);
}


void
Surface::Invalidate (Rect r)
{
	active_window->Invalidate (r);
}

void
Surface::ProcessUpdates ()
{
	active_window->ProcessUpdates();
}

void
Surface::Paint (cairo_t *ctx, int x, int y, int width, int height)
{
	Rect r = Rect (x, y, width, height);
	Region region = Region (r);
	Paint (ctx, &region);
}

void
Surface::Paint (cairo_t *ctx, Region *region)
{
	if (!toplevel)
		return;

#if FRONT_TO_BACK_STATS
	uielements_rendered_front_to_back = 0;
	uielements_rendered_back_to_front = 0;
#endif

	if (IsAnythingDirty())
		ProcessDirtyElements ();

	bool did_front_to_back = false;

	List *render_list = new List ();
	Region *copy = new Region (region);

	if (moonlight_flags & RUNTIME_INIT_RENDER_FRONT_TO_BACK) {
		if (full_screen_message)
			full_screen_message->FrontToBack (copy, render_list);

		toplevel->FrontToBack (copy, render_list);

		if (!render_list->IsEmpty ()) {
			while (RenderNode *node = (RenderNode*)render_list->First()) {
#if FRONT_TO_BACK_STATS
				uielements_rendered_front_to_back ++;
#endif
				node->Render (ctx);

				render_list->Remove (node);
			}

			did_front_to_back = true;
		}

		delete render_list;
		delete copy;
	}

	if (!did_front_to_back) {
		toplevel->DoRender (ctx, region);

		if (full_screen_message) {
			full_screen_message->DoRender (ctx, region);
		}
	}

#if FRONT_TO_BACK_STATS
	printf ("UIElements rendered front-to-back: %d\n", uielements_rendered_front_to_back);
	printf ("UIElements rendered back-to-front: %d\n", uielements_rendered_back_to_front);
#endif

#ifdef DEBUG
		if (debug_selected_element) {
			Rect bounds = debug_selected_element->GetSubtreeBounds();
// 			printf ("debug_selected_element is %s\n", debug_selected_element->GetName());
// 			printf ("bounds is %g %g %g %g\n", bounds.x, bounds.y, bounds.w, bounds.h);
			cairo_save (ctx);
			//RenderClipPath (ctx);
			cairo_new_path (ctx);
			cairo_identity_matrix (ctx);
			cairo_set_source_rgba (ctx, 1.0, 0.5, 0.2, 1.0);
			cairo_set_line_width (ctx, 1);
			cairo_rectangle (ctx, bounds.x, bounds.y, bounds.width, bounds.height);
			cairo_stroke (ctx);
			cairo_restore (ctx);
		}
#endif
}

//
// This will resize the surface (merely a convenience function for
// resizing the widget area that we have.
//
// This will not change the Width and Height properties of the 
// toplevel canvas, if you want that, you must do that yourself
//
void
Surface::Resize (int width, int height)
{
	if (width == normal_window->GetWidth()
	    && height == normal_window->GetHeight())
		return;

	normal_window->Resize (width, height);
}

void
Surface::EmitError (ErrorEventArgs *args)
{
	Emit (ErrorEvent, args);
}

void
Surface::Realloc ()
{
	if (toplevel)
		toplevel->UpdateBounds();
}

void
Surface::SetFullScreen (bool value)
{
	if (value && !can_full_screen) {
		g_warning ("You're not allowed to switch to fullscreen from where you're doing it.");
		return;
	}
	
	UpdateFullScreen (value);
}

bool
Surface::IsTopLevel (UIElement* top)
{
	if (top == NULL)
		return false;
		
	return top == toplevel || top == full_screen_message;
}

void
Surface::ShowFullScreenMessage ()
{
	g_return_if_fail (full_screen_message == NULL);
	g_return_if_fail (toplevel && toplevel->Is (Type::PANEL));
	
	Type::Kind dummy;
	XamlLoader *loader = new XamlLoader (NULL, FULLSCREEN_MESSAGE, this);
	DependencyObject* message = loader->CreateFromString (FULLSCREEN_MESSAGE, false, &dummy);
	delete loader;
	
	if (!message) {
		printf ("Unable to create fullscreen message.\n");
		return;
	}
	
	if (!message->Is (Type::CANVAS)) {
		printf ("Unable to create fullscreen message, got a %s, expected at least a UIElement.\n", message->GetTypeName ());
		message->unref ();
		return;
	}
	
	full_screen_message = (Canvas*) message;
	full_screen_message->SetSurface (this);
	
	DependencyObject* message_object = full_screen_message->FindName ("message");
	DependencyObject* url_object = full_screen_message->FindName ("url");
	TextBlock* message_block = (message_object != NULL && message_object->Is (Type::TEXTBLOCK)) ? (TextBlock*) message_object : NULL;
	TextBlock* url_block = (url_object != NULL && url_object->Is (Type::TEXTBLOCK)) ? (TextBlock*) url_object : NULL;
	
	Transform* transform = full_screen_message->GetRenderTransform ();
	
	double box_height = full_screen_message->GetHeight ();
	double box_width = full_screen_message->GetWidth ();
	
	// Set the url in the box
	if (url_block != NULL)  {
		char *url = NULL;
		if (g_str_has_prefix (source_location, "http://")) {
			char* path = strchr (source_location + 7, '/');
			if (path != NULL && path > source_location + 7) {
				url = g_strndup (source_location, path - source_location);  
			} else {
				url = g_strdup (source_location);
			}
		} else if (g_str_has_prefix (source_location, "file://")) {
			url = g_strdup ("file://");
		} else {
			url = g_strdup (source_location);
		}
		
		url_block->SetValue (TextBlock::TextProperty, url ? url : (char *) "file://");
		g_free (url);
	}
	
	// The box is not made bigger if the url doesn't fit.
	// MS has an interesting text rendering if the url doesn't
	// fit: the text is overflown to the left.
	// Since only the server is shown, this shouldn't
	// happen on a regular basis though.
	
	// Center the url block
	if (url_block != NULL) {
		double url_width = url_block->GetActualWidth ();
		Canvas::SetLeft (url_block, (box_width - url_width) / 2);
	}

	// Center the message block
	if (message_block != NULL) {
		double message_width = message_block->GetActualWidth ();
		Canvas::SetLeft (message_block, (box_width - message_width) / 2);
	}	

	// Put the box in the middle of the screen
	transform->SetValue (TranslateTransform::XProperty, Value ((active_window->GetWidth() - box_width) / 2));
	transform->SetValue (TranslateTransform::YProperty, Value ((active_window->GetHeight() - box_height) / 2));
	
	full_screen_message->UpdateTotalRenderVisibility ();
	full_screen_message->UpdateTotalHitTestVisibility ();
	full_screen_message->FullInvalidate (true);
	full_screen_message->OnLoaded ();
}

void
Surface::SetSourceLocation (const char* location)
{
	if (source_location)
		g_free (source_location);
	source_location = g_strdup (location);
}

void 
Surface::HideFullScreenMessage ()
{
	if (full_screen_message) {
		full_screen_message->unref ();
		full_screen_message = NULL;
	}
}

void
Surface::UpdateFullScreen (bool value)
{
	if (value == full_screen)
		return;
	
	if (value) {
		fullscreen_window = new MoonWindowGtk (true);
		fullscreen_window->SetSurface (this);

		active_window = fullscreen_window;
		
		ShowFullScreenMessage ();

		fullscreen_window->EnableEvents (false);
	} else {
		active_window = normal_window;

		HideFullScreenMessage ();

		delete fullscreen_window;
		fullscreen_window = NULL;
	}

	full_screen = value;
	
	Realloc ();

	time_manager->GetSource()->Stop();
	Emit (FullScreenChangeEvent);
	time_manager->GetSource()->Start();
}

void
Surface::render_cb (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	Surface *s = (Surface *) closure;
	gint64 now;
	bool dirty = false;

	GDK_THREADS_ENTER ();
	if (s->IsAnythingDirty ()) {
		if (s->zombie) {
			s->up_dirty->Clear (true);
			s->down_dirty->Clear (true);
		} else {
			s->ProcessDirtyElements ();
			dirty = true;
		}
	}

	if (s->expose_handoff) {
		TimeSpan time = s->GetTimeManager ()->GetCurrentTime ();
		if (time != s->expose_handoff_last_timespan) {
			s->expose_handoff (s, time , s->expose_handoff_data);
			s->expose_handoff_last_timespan = time;
		}
	}

	GDK_THREADS_LEAVE ();

	if ((moonlight_flags & RUNTIME_INIT_SHOW_FPS) && s->fps_start == 0)
		s->fps_start = get_now ();
	
	if (dirty) {
		s->ProcessUpdates ();
	}

	if ((moonlight_flags & RUNTIME_INIT_SHOW_FPS) && s->fps_report) {
		s->fps_nframes++;
		
		if ((now = get_now ()) > (s->fps_start + TIMESPANTICKS_IN_SECOND)) {
			float nsecs = (now - s->fps_start) / TIMESPANTICKS_IN_SECOND_FLOAT;
			
			s->fps_report (s, s->fps_nframes, nsecs, s->fps_data);
			s->fps_nframes = 0;
			s->fps_start = now;
		}
	}

	if ((moonlight_flags & RUNTIME_INIT_SHOW_CACHE_SIZE) && s->cache_report) {
		// By default we report cache status every 50 render's.
		// Should be enough for everybody, but syncing to ie. 1s sounds
		// better.
		if (s->cache_size_ticker == 50) {
			s->cache_report (s, s->cache_size_in_bytes, s->cache_data);
			s->cache_size_ticker = 0;
		} else
			s->cache_size_ticker++;
	}
}

void
Surface::update_input_cb (EventObject *sender, EventArgs *calldata, gpointer closure)
{
#if notyet
	Surface *s = (Surface *) closure;

	s->HandleMouseEvent (UIElement::MouseMoveEvent, true, true, false, s->mouse_event_state, s->mouse_event_x, s->mouse_event_y);
	s->UpdateCursorFromInputList ()
#endif
}

void
Surface::HandleUIWindowAvailable ()
{
	time_manager->AddHandler (TimeManager::RenderEvent, render_cb, this);
	time_manager->AddHandler (TimeManager::UpdateInputEvent, update_input_cb, this);

	time_manager->NeedRedraw ();
}

void
Surface::HandleUIWindowUnavailable ()
{
	time_manager->RemoveHandler (TimeManager::RenderEvent, render_cb, this);
	time_manager->RemoveHandler (TimeManager::UpdateInputEvent, update_input_cb, this);
}

void
Surface::PaintToDrawable (GdkDrawable *drawable, GdkVisual *visual, GdkEventExpose *event, int off_x, int off_y, bool clear_transparent)
{
	frames++;

	if (event->area.x > (off_x + active_window->GetWidth()) || event->area.y > (off_y + active_window->GetHeight()))
		return;

#if TIME_REDRAW
	STARTTIMER (expose, "redraw");
#endif
	if (cache_size_multiplier == -1)
		cache_size_multiplier = gdk_drawable_get_depth (drawable) / 8 + 1;

#ifdef DEBUG_INVALIDATE
	printf ("Got a request to repaint at %d %d %d %d\n", event->area.x, event->area.y, event->area.width, event->area.height);
#endif
	cairo_t *ctx = runtime_cairo_create (drawable, visual, moonlight_flags & RUNTIME_INIT_USE_BACKEND_XLIB);
	Region *region = new Region (event->region);

	region->Offset (-off_x, -off_y);
	cairo_surface_set_device_offset (cairo_get_target (ctx),
					 off_x - event->area.x, 
					 off_y - event->area.y);
	region->Draw (ctx);
	//
	// These are temporary while we change this to paint at the offset position
	// instead of using the old approach of modifying the topmost Canvas (a no-no),
	//
	// The flag "transparent" is here because I could not
	// figure out what is painting the background with white now.
	// The change that made the white painting implicit instead of
	// explicit is patch 80632.   I would appreciate any help in tracking down
	// the proper way of making the background white when not running in 
	// "transparent" mode.    
	//
	// Either exposing surface_set_trans to turn the next code is a hack, 
	// or it is normal to request all code that wants to paint to manually
	// clear the background to white beforehand.    For now am going with
	// making this an explicit surface API.
	//
	// The second part is for coping with the future: when we support being 
	// windowless
	//
	cairo_set_operator (ctx, CAIRO_OPERATOR_OVER);

	if (transparent) {
		if (clear_transparent) {
			cairo_set_operator (ctx, CAIRO_OPERATOR_CLEAR);
			cairo_fill_preserve (ctx);
			cairo_set_operator (ctx, CAIRO_OPERATOR_OVER);
		}

		cairo_set_source_rgba (ctx,
				       background_color->r,
				       background_color->g,
				       background_color->b,
				       background_color->a);
	}
	else {
		cairo_set_source_rgb (ctx,
				      background_color->r,
				      background_color->g,
				      background_color->b);
	}

	cairo_fill_preserve (ctx);
	cairo_clip (ctx);

	cairo_save (ctx);
	Paint (ctx, region);
	cairo_restore (ctx);

	if (RENDER_EXPOSE) {
		cairo_new_path (ctx);
		region->Draw (ctx);
		cairo_set_line_width (ctx, 2.0);
		cairo_set_source_rgb (ctx, (double)(frames % 2), (double)((frames + 1) % 2), (double)((frames / 3) % 2));
		cairo_stroke (ctx);
	}

	if (!(moonlight_flags & RUNTIME_INIT_USE_BACKEND_XLIB)) {
		cairo_surface_flush (cairo_get_target (ctx));
		cairo_t *native = runtime_cairo_create (drawable, visual, true);

		cairo_surface_set_device_offset (cairo_get_target (native),
						 0, 0);
		cairo_surface_set_device_offset (cairo_get_target (ctx),
						 0, 0);

		cairo_set_source_surface (native, cairo_get_target (ctx),
					  0, 0);

		region->Offset (off_x, off_y);
		region->Offset (-event->area.x, -event->area.y);
		region->Draw (native);

		cairo_fill (native);
		cairo_destroy (native);
	} else
		cairo_destroy (ctx);

	delete region;


#if TIME_REDRAW
	ENDTIMER (expose, "redraw");
#endif

}

RenderNode::RenderNode (UIElement *el,
			Region *region,
			bool render_element,
			RenderFunc pre,
			RenderFunc post)

{
	uielement = el;
	uielement->ref();
	this->region = region ? region : new Region ();
	this->render_element = render_element;
	this->pre_render = pre;
	this->post_render = post;
}

void
RenderNode::Render (cairo_t *ctx)
{
	bool front_to_back = uielement->UseBackToFront ();

	if (pre_render)
		pre_render (ctx, uielement, region, front_to_back);

	if (render_element)
		uielement->Render (ctx, region);
	
	if (post_render)
		post_render (ctx, uielement, region, front_to_back);
}

RenderNode::~RenderNode ()
{
	if (uielement) {
		uielement->unref ();
		uielement = NULL;
	}

	if (region)
		delete region;
}

UIElementNode::UIElementNode (UIElement *el)
{
	uielement = el;
	uielement->ref();
}

UIElementNode::~UIElementNode ()
{
	uielement->unref();
	uielement = NULL;
}

void
Surface::PerformCapture (UIElement *capture)
{
	// "Capturing" the mouse pointer at an element forces us to
	// use the path up the hierarchy from that element to the root
	// as the input list, regardless of where the pointer actually
	// is.

	// XXX we should check if the input_list already starts at
	// @capture.
	List *new_input_list = new List();
	while (capture) {
		new_input_list->Append (new UIElementNode (capture));
		capture = capture->GetVisualParent();
	}

	delete input_list;
	input_list = new_input_list;
	captured = true;
	pendingCapture = NULL;
}

void
Surface::PerformReleaseCapture ()
{
	// These need to be set before calling HandleMouseEvent as
	// "captured" determines the input_list calculation, and
	// "pendingReleaseCapture", when set, causes an infinite
	// recursive loop.
	captured = false;
	pendingReleaseCapture = false;

	// this causes any new elements we're over to be Enter'ed.  MS
	// doesn't Leave the element that had the mouse captured,
	// though.
	HandleMouseEvent (NO_EVENT_ID, false, true, false, mouse_event);
}

bool
Surface::SetMouseCapture (UIElement *capture)
{
	if (capture != NULL && (captured || pendingCapture))
		return false;

	// we delay the actual capture/release until the end of the
	// event bubbling.  need more testing on MS here to see what
	// happens if through the course of a single event bubbling up
	// to the root, one element captures and another releases.
	// Our current implementation has it so that any "release"
	// causes all captures to be ignored.
	if (capture == NULL) {
		if (emittingMouseEvent)
			pendingReleaseCapture = true;
		else
			PerformReleaseCapture ();
	}
	else {
		if (emittingMouseEvent) {
			pendingCapture = capture;
		}
		else {
			PerformCapture (capture);
		}
	}

	return true;
}

EventArgs*
Surface::CreateArgsForEvent (int event_id, GdkEvent *event)
{
	if (event_id == UIElement::MouseLeaveEvent
	    || event_id ==UIElement::InvalidatedEvent
	    || event_id ==UIElement::GotFocusEvent
	    || event_id ==UIElement::LostFocusEvent)
		return new EventArgs ();
	else if (event_id == UIElement::MouseMoveEvent
		 || event_id ==UIElement::MouseLeftButtonDownEvent
		 || event_id ==UIElement::MouseLeftButtonUpEvent
		 || event_id ==UIElement::MouseEnterEvent)
		return new MouseEventArgs(event);
	else if (event_id == UIElement::KeyDownEvent
		 || event_id == UIElement::KeyUpEvent)
		return new KeyEventArgs((GdkEventKey*)event);
	else {
		g_warning ("Unknown event id %d\n", event_id);
		return new EventArgs();
	}
}

bool
Surface::EmitEventOnList (int event_id, List *element_list, GdkEvent *event, int end_idx)
{
	bool handled = false;

	int idx;
	UIElementNode *node;

	if (end_idx == -1)
		end_idx = element_list->Length();

	EmitContext** emit_ctxs = g_new (EmitContext*, end_idx + 1);
	for (node = (UIElementNode*)element_list->First(), idx = 0; node && idx < end_idx; node = (UIElementNode*)node->next, idx++) {
		emit_ctxs[idx] = node->uielement->StartEmit (event_id);
	}

	emittingMouseEvent = true;

	EventArgs *args = CreateArgsForEvent(event_id, event);
	bool args_are_routed = args->Is (Type::ROUTEDEVENTARGS);

	if (args_are_routed && element_list->First())
		((RoutedEventArgs*)args)->SetSource(((UIElementNode*)element_list->First())->uielement);

	for (node = (UIElementNode*)element_list->First(), idx = 0; node && idx < end_idx; node = (UIElementNode*)node->next, idx++) {
		bool h = node->uielement->DoEmit (event_id, emit_ctxs[idx], args);
		if (h)
			handled = true;
		if (zombie) {
			handled = false;
			break;
		}
		if (silverlight2 && args_are_routed && ((RoutedEventArgs*)args)->GetHandled())
			break;
	}
	emittingMouseEvent = false;
	args->unref();

	for (node = (UIElementNode*)element_list->First(), idx = 0; node && idx < end_idx; node = (UIElementNode*)node->next, idx++) {
		node->uielement->FinishEmit (event_id, emit_ctxs[idx]);
	}
	g_free (emit_ctxs);

	return handled;
}

void
Surface::FindFirstCommonElement (List *l1, int *index1,
				 List *l2, int *index2)
{
	// we exploit the fact that for a list with any common
	// elements, the lists will be identical from the first common
	// element to the end of the lists.  So, we start from the
	// last elements in both lists and walk backward to the start,
	// looking for the first elements that don't match.
	//
	// this algorithm is O(MAX(n,m)), but it's unclear whether or
	// not this is actually better than the O(n*m) approach, since
	// the O(n*m) approach will often find a match on the first
	// comparison (especially when the user is slowly moving the
	// mouse around in the same element), and skip the rest.
	int i1, i2;
	UIElementNode *ui1, *ui2;

	*index1 = -1;
	*index2 = -1;

	ui1 = (UIElementNode*)l1->Last();
	i1 = l1->Length() - 1;

	ui2 = (UIElementNode*)l2->Last();
	i2 = l2->Length() - 1;

	while (ui1 && ui2) {

		if (ui1->uielement == ui2->uielement) {
			*index1 = i1;
			*index2 = i2;
		}
		else {
			return;
		}

		ui1 = (UIElementNode*)ui1->prev;
		ui2 = (UIElementNode*)ui2->prev;
		i1--;
		i2--;
	}
}

static List*
copy_input_list_with_visibility_check (List *input_list)
{
	List *list = new List ();
	UIElementNode *node;

	for (node = (UIElementNode*) input_list->First(); node; node = (UIElementNode*)node->next) {
		if (node->uielement->GetActualTotalRenderVisibility () &&
		    node->uielement->GetActualTotalHitTestVisibility ()) 
			list->Append (new UIElementNode (node->uielement));
	}

	return list;
}

bool
Surface::HandleMouseEvent (int event_id, bool emit_leave, bool emit_enter, bool force_emit, GdkEvent *event)
{
	bool handled = false;

	// we can end up here if mozilla pops up the JS timeout
	// dialog.  The problem is that JS might have registered a
	// handler for the event we're going to emit, so when we end
	// up tripping the timeout while in JS, mozilla pops up the
	// dialog, which causes a crossing-notify event to be emitted.
	// This causes HandleMouseEvent to be called, and the original
	// input_list is deleted.  the crossing-notify event is
	// handled, then we return to the event that tripped the
	// timeout, we crash.
	if (emittingMouseEvent)
		return false;

	if (zombie)
		return false;

	if (toplevel == NULL || event == NULL)
		return false;

	// FIXME this should probably use mouse event args
	if (IsAnythingDirty())
		ProcessDirtyElements();

	if (captured) {
		// if the mouse is captured, the input_list doesn't ever
		// change, and we don't emit enter/leave events.  just emit
		// the event on the input_list.
		if (event_id != NO_EVENT_ID)
			handled = EmitEventOnList (event_id, input_list, event, -1);
	}
	else {
		int surface_index;
		int new_index;

		// Accumulate a new input_list, which contains the
		// most deeply nested hit testable UIElement covering
		// the point (x,y), and all visual parents up the
		// hierarchy to the root.
		List *new_input_list = new List ();
		double x, y;

		gdk_event_get_coords (event, &x, &y);

		Point p (x,y);

		cairo_t *ctx = measuring_context_create ();
		toplevel->HitTest (ctx, p, new_input_list);
		
		// for 2 lists:
		//   l1:  [a1, a2, a3, a4, ... ]
		//   l2:  [b1, b2, b3, b4, ... ]
		//
		// For identical lists:
		// 
		//   only the primary event is emitted for all
		//   elements of l2, in order.
		//
		// For lists that differ, Enter/Leave events must be
		// emitted.
		//
		//   If the first few nodes in each list differ, and,
		//   for instance bn == am, we know that [am...] ==
		//   [bn...]
		//
		//   when emitting a given event on b1, MS generally
		//   emits Leave events on [a1, a2, a3, ... am-1], and
		//   Enter events on [b1, b2, ... bn-1].
		//
		//   For most event types, that's all that happens if
		//   the lists differ.  For MouseLeftButtonDown (we
		//   also do it for MouseLeftButtonUp), we also emit
		//   the primary event on l2 after the enter/leave
		//   events.
		//
		FindFirstCommonElement (input_list, &surface_index,
					new_input_list, &new_index);

		if (emit_leave)
			handled = EmitEventOnList (UIElement::MouseLeaveEvent, input_list, event, surface_index);

		if (emit_enter)
			handled = EmitEventOnList (UIElement::MouseEnterEvent, new_input_list, event, new_index) || handled;

		if (event_id != NO_EVENT_ID && ((surface_index == 0 && new_index == 0) || force_emit)) {
			handled = EmitEventOnList (event_id, new_input_list, event, -1) || handled;

			if (silverlight2 && event_id == UIElement::MouseLeftButtonDownEvent) {
				UIElement *el = new_input_list->First() ? ((UIElementNode*)new_input_list->First())->uielement : NULL;

				if (el != focused_element)
					FocusElement (el);
			}
		}

		// We need to remove from the new_input_list the events which have just 
		// became invisible/unhittable as the result of the event. 
		// (ie. element visibility was changed in the mouse enter).

		if (handled) {
			UIElementNode *node;

			for (node = (UIElementNode*)new_input_list->First(); node; node = (UIElementNode*)node->next) {
				if (! node->uielement->GetActualTotalRenderVisibility () || 
				    ! node->uielement->GetActualTotalHitTestVisibility ()) {
					// Ooops, looks like something changed.
					// We need to copy the list with some elements removed.
					List *list = copy_input_list_with_visibility_check (new_input_list);
					delete new_input_list;
					new_input_list = list;
					break;
				}
			}
		}

		measuring_context_destroy (ctx);

		delete input_list;
		input_list = new_input_list;
	}

	// Perform any captures/releases that are pending after the
	// event is bubbled.
	if (pendingCapture)
		PerformCapture (pendingCapture);
	else if (pendingReleaseCapture)
		PerformReleaseCapture ();

	return handled;
}

void
Surface::UpdateCursorFromInputList ()
{
	MouseCursor new_cursor = MouseCursorDefault;
	
	// loop over the input list in order until we hit a node that
	// has its cursor set to the non-default.
	UIElementNode *node;
	for (node = (UIElementNode*)input_list->First(); node; node = (UIElementNode*)node->next) {
		new_cursor = node->uielement->GetCursor ();
		if (new_cursor != MouseCursorDefault)
			break;
	}

	SetCursor (new_cursor);
}

void
Surface::SetFPSReportFunc (MoonlightFPSReportFunc report, void *user_data)
{
	fps_report = report;
	fps_data = user_data;
}

void
Surface::SetCacheReportFunc (MoonlightCacheReportFunc report, void *user_data)
{
	cache_report = report;
	cache_data = user_data;
}

void 
Surface::SetExposeHandoffFunc (MoonlightExposeHandoffFunc func, void *user_data)
{
	expose_handoff = func;
	expose_handoff_data = user_data;
	expose_handoff_last_timespan = G_MAXINT64; 
}

class DownloaderNode : public List::Node {
public:
	Downloader *downloader;
	DownloaderNode (Downloader *dl) { downloader = dl; }		
};

void
Surface::DetachDownloaders ()
{
	DownloaderNode *node;
	if (downloaders == NULL)
		return;
		
	node = (DownloaderNode *) downloaders->First ();
	while (node != NULL) {
		node->downloader->RemoveHandler (Downloader::DestroyedEvent, OnDownloaderDestroyed, this);
		node->downloader->SetSurface (NULL);
		node = (DownloaderNode *) node->next;
	}
	downloaders->Clear (true);
}

void
Surface::OnDownloaderDestroyed (EventObject *sender, EventArgs *args, gpointer closure)
{
	DownloaderNode *node;
	Surface *surface = (Surface *) closure;
	List *downloaders = surface->downloaders;
	
	if (downloaders == NULL) {
		printf ("Surface::OnDownloaderDestroyed (): The list of downloaders is empty.\n");
		return;
	}
	
	node = (DownloaderNode *) downloaders->First ();
	while (node != NULL) {
		if (node->downloader == sender) {
			downloaders->Remove (node);
			return;
		}
		node = (DownloaderNode *) node->next;
	}
	
	printf ("Surface::OnDownloaderDestroyed (): Couldn't find the downloader %p in the list of downloaders\n", sender);
}

Downloader *
Surface::CreateDownloader (void) 
{
	if (zombie) {
		g_warning ("Surface::CreateDownloader (): Trying to create a downloader on a zombified surface.\n");
		return NULL;
	}
	
	Downloader *downloader = new Downloader ();
	downloader->SetSurface (this);
	downloader->SetContext (downloader_context);
	downloader->AddHandler (Downloader::DestroyedEvent, OnDownloaderDestroyed, this);
	if (downloaders == NULL)
		downloaders = new List ();
	downloaders->Append (new DownloaderNode (downloader));
	
	return downloader;
}

Downloader *
Surface::CreateDownloader (UIElement *element)
{
	Surface *surface = element ? element->GetSurface () : NULL;
	
	if (surface)
		return surface->CreateDownloader ();
	
	g_warning ("Surface::CreateDownloader (%p, ID: %i): Unable to create contextual downloader.\n",
		   element, GET_OBJ_ID (element));
	
	return NULL;
}

bool 
Surface::VerifyWithCacheSizeCounter (int w, int h)
{
	if (! (moonlight_flags & RUNTIME_INIT_USE_SHAPE_CACHE))
		return false;

	if (cache_size_multiplier == -1)
		return false;

	if (cache_size_in_bytes + (w * h * cache_size_multiplier) < MAXIMUM_CACHE_SIZE)
		return true;
	else
		return false;
}

gint64
Surface::AddToCacheSizeCounter (int w, int h)
{
	gint64 new_size = w * h * cache_size_multiplier;
	cache_size_in_bytes += new_size;
	return new_size;
}

void 
Surface::RemoveFromCacheSizeCounter (gint64 size)
{
	cache_size_in_bytes -= size;
}

bool
Surface::FullScreenKeyHandled (GdkEventKey *key)
{
	if (!GetFullScreen ())
		return false;
		
	// If we're in fullscreen mode no key events are passed through.
	// We only handle Esc, to exit fullscreen mode.
	if (key->keyval == GDK_Escape)
		SetFullScreen (false);
	
	return true;
}

gboolean
Surface::HandleUIFocusIn (GdkEventFocus *event)
{
	if (toplevel)
		toplevel->EmitGotFocus ();

	return false;
}

gboolean
Surface::HandleUIFocusOut (GdkEventFocus *event)
{
	if (toplevel)
		toplevel->EmitLostFocus ();

	return false;
}

gboolean
Surface::HandleUIButtonRelease (GdkEventButton *event)
{
	if (event->button != 1)
		return false;
	
	SetCanFullScreen (true);
	
	if (mouse_event)
		gdk_event_free (mouse_event);
	
	mouse_event = gdk_event_copy ((GdkEvent *) event);

	HandleMouseEvent (UIElement::MouseLeftButtonUpEvent, true, true, true, mouse_event);

	UpdateCursorFromInputList ();
	SetCanFullScreen (false);

	// XXX MS appears to do this here, which is completely stupid.
	if (captured)
		PerformReleaseCapture ();

	return true;
}

gboolean
Surface::HandleUIButtonPress (GdkEventButton *event)
{
	active_window->GrabFocus ();
	
	if (event->button != 1)
		return false;

	SetCanFullScreen (true);

	if (mouse_event)
		gdk_event_free (mouse_event);
	
	mouse_event = gdk_event_copy ((GdkEvent *) event);

	bool handled = HandleMouseEvent (UIElement::MouseLeftButtonDownEvent, true, true, true, mouse_event);

	UpdateCursorFromInputList ();
	SetCanFullScreen (false);

	return handled;
}

gboolean
Surface::HandleUIMotion (GdkEventMotion *event)
{
	if (mouse_event)
		gdk_event_free (mouse_event);
	
	mouse_event = gdk_event_copy ((GdkEvent *) event);

	bool handled = false;

	if (event->is_hint) {
#if GTK_CHECK_VERSION(2,12,0)
	  if (gtk_check_version (2, 12, 0))
	  	gdk_event_request_motions (event);
	  else
#endif
	    {
		int ix, iy;
		GdkModifierType state;
		gdk_window_get_pointer (event->window, &ix, &iy, (GdkModifierType*)&state);
		((GdkEventMotion *) mouse_event)->x = ix;
		((GdkEventMotion *) mouse_event)->y = iy;
	    }    
	}

	handled = HandleMouseEvent (UIElement::MouseMoveEvent, true, true, true, mouse_event);
	UpdateCursorFromInputList ();

	return handled;
}

gboolean
Surface::HandleUICrossing (GdkEventCrossing *event)
{
	bool handled;
	
	// Ignore the enter/leave events coming as a result of grab/press
	// started finished
	if (event->mode != GDK_CROSSING_NORMAL)
		return true;
	
	if (event->type == GDK_ENTER_NOTIFY) {
		if (mouse_event)
			gdk_event_free (mouse_event);
		mouse_event = gdk_event_copy ((GdkEvent *) event);
		
		handled = HandleMouseEvent (UIElement::MouseMoveEvent, true, true, false, mouse_event);

		UpdateCursorFromInputList ();
	
	} else {
		// forceably emit MouseLeave on the current input
		// list..  the "new" list computed by HandleMouseEvent
		// should be the same as the current one since we pass
		// in the same x,y but I'm not sure that's something
		// we can rely on.
		handled = HandleMouseEvent (UIElement::MouseLeaveEvent, false, false, true, mouse_event);

		// MS specifies that mouse capture is lost when you mouse out of the control
		if (captured)
			PerformReleaseCapture ();

		// clear out the input list so we emit the right
		// events when the pointer reenters the control.
		if (!emittingMouseEvent) {
			delete input_list;
			input_list = new List();
		}
	}

	return handled;
}

void
Surface::GenerateFocusChangeEvents()
{
	focus_tick_call_added = false;

	List *el_list;
	if (prev_focused_element) {
		el_list = ElementPathToRoot (prev_focused_element);
		EmitEventOnList (UIElement::LostFocusEvent, el_list, NULL, -1);
		delete (el_list);
	}

	if (focused_element) {
		el_list = ElementPathToRoot (focused_element);
		EmitEventOnList (UIElement::GotFocusEvent, el_list, NULL, -1);
		delete (el_list);
	}
}

void
Surface::generate_focus_change_events (EventObject *object)
{
	Surface *s = (Surface*)object;
	s->GenerateFocusChangeEvents();
}

bool
Surface::FocusElement (UIElement *focused)
{
	if (focused == focused_element)
		return true;

	/* according to msdn, these three things must be true for an element to be focusable:

	   1. the element must be visible
	   2. the element must have IsTabStop = true
	   3. the element must be part of the plugin's visual tree, and must have had its Loaded event fired.
	*/
	if (!focused->GetRenderVisible()
#if SL_2_0
	    || (!focused->Is(Type::CONTROL) || !((Control*)focused)->GetIsTabStop())
#endif
	    || !focused->IsLoaded()
	    || focused->GetSurface () != this)
		return false;

	if (!focus_tick_call_added) {
		prev_focused_element = focused_element;
	}
	focused_element = focused;
	if ((focused_element || prev_focused_element) && !focus_tick_call_added) {
		time_manager->AddTickCall (generate_focus_change_events, this);
		focus_tick_call_added = true;
	}

	return true;
}

List*
Surface::ElementPathToRoot (UIElement *source)
{
	List *list = new List();
	while (source) {
		list->Append (new UIElementNode (source));
		source = source->GetVisualParent();
	}
	return list;
}

gboolean 
Surface::HandleUIKeyPress (GdkEventKey *event)
{
	if (FullScreenKeyHandled (event))
		return true;
	
#if DEBUG_MARKER_KEY
	static int debug_marker_key_in = 0;
	if (event->keyval == GDK_d || event->keyval == GDK_D) {
		if (!debug_marker_key_in)
			printf ("<--- DEBUG MARKER KEY IN (%f) --->\n", get_now () / 10000000.0);
		else
			printf ("<--- DEBUG MARKER KEY OUT (%f) --->\n", get_now () / 10000000.0);
		debug_marker_key_in = ! debug_marker_key_in;
		return true;
	}
#endif
	
	SetCanFullScreen (true);
	bool handled;
	if (silverlight2 && focused_element) {
		List *focus_to_root = ElementPathToRoot (focused_element);
		handled = EmitEventOnList (UIElement::KeyDownEvent, focus_to_root, (GdkEvent*)event, -1);
		delete focus_to_root;
	}
	else {
		// in silverlight 1.0, key events are only ever delivered to the toplevel
		toplevel->EmitKeyDown (event);
		handled = true;
	}
				    
	SetCanFullScreen (false);
	
	return handled;
}

gboolean 
Surface::HandleUIKeyRelease (GdkEventKey *event)
{
	if (FullScreenKeyHandled (event))
		return true;

	SetCanFullScreen (true);

	bool handled;
	if (silverlight2 && focused_element) {
		List *focus_to_root = ElementPathToRoot (focused_element);
		handled = EmitEventOnList (UIElement::KeyUpEvent, focus_to_root, (GdkEvent*)event, -1);
		delete focus_to_root;
	}
	else {
		// in silverlight 1.0, key events are only ever delivered to the toplevel
		toplevel->EmitKeyUp (event);
		handled = true;
	}
	
	SetCanFullScreen (false);
	
	return handled;
}

void
Surface::HandleUIWindowAllocation (bool emit_resize)
{
	Realloc ();
	if (emit_resize)
		Emit (ResizeEvent);
}

void
Surface::HandleUIWindowDestroyed (MoonWindow *window)
{
	if (window == fullscreen_window) {
		// switch out of fullscreen mode, as something has
		// destroyed our fullscreen window.
		UpdateFullScreen (false);
	}
	else if (window == normal_window) {
		// something destroyed our normal window
		normal_window = NULL;
	}

	if (window == active_window)
		active_window = NULL;
}

void
Surface::SetTransparent (bool transparent)
{
	this->transparent = transparent;
	
	active_window->Invalidate ();
}

void
Surface::SetBackgroundColor (Color *color)
{
	if (background_color)
		delete background_color;
		
	background_color = new Color (*color);

	active_window->Invalidate ();
}


void
runtime_init (guint32 flags)
{
	const char *env;
	
	if (inited)
		return;
	
	if (cairo_version () < CAIRO_VERSION_ENCODE(1,4,0)) {
		printf ("*** WARNING ***\n");
		printf ("*** Cairo versions < 1.4.0 should not be used for Moon.\n");
		printf ("*** Moon was configured to use Cairo version %d.%d.%d, but\n",
			CAIRO_VERSION_MAJOR, CAIRO_VERSION_MINOR, CAIRO_VERSION_MICRO);
		printf ("*** is being run against version %s.\n", cairo_version_string ());
		printf ("*** Proceed at your own risk\n");
	}
	
	flags |= RUNTIME_INIT_SHOW_FPS;
	
	// Allow the user to override the flags via his/her environment
	if ((env = g_getenv ("MOONLIGHT_OVERRIDES"))) {
		const char *flag = env;
		const char *inptr;
		size_t n;
		uint i;
		
		while (*flag == ',')
			flag++;
		
		inptr = flag;
		
		while (*flag) {
			while (*inptr && *inptr != ',')
				inptr++;
			
			n = (inptr - flag);
			for (i = 0; i < G_N_ELEMENTS (overrides); i++) {
				if (n != strlen (overrides[i].override))
					continue;
				
				if (!strncmp (overrides[i].override, flag, n)) {
					if (!overrides[i].set)
						flags &= ~overrides[i].flag;
					else
						flags |= overrides[i].flag;
				}
			}
			
			while (*inptr == ',')
				inptr++;
			
			flag = inptr;
		}
	}
	
#if OBJECT_TRACKING
	if (EventObject::objects_created == EventObject::objects_destroyed) {
		printf ("Runtime created (no leaked objects so far).\n");
	} else {
		printf ("Runtime created. Object tracking summary:\n");
		printf ("\tObjects created: %i\n", EventObject::objects_created);
		printf ("\tObjects destroyed: %i\n", EventObject::objects_destroyed);
		printf ("\tDifference: %i\n", EventObject::objects_created - EventObject::objects_destroyed);
	}
#endif

	inited = true;
	
	if (!g_type_inited) {
		g_type_inited = true;
		g_type_init ();
	}
	
	moonlight_flags = flags;
	
	types_init ();
	dependency_property_g_init ();
	xaml_init ();
	font_init ();
	downloader_init ();
	Media::Initialize ();
}

#if OBJECT_TRACKING
static int
IdComparer (gconstpointer base1, gconstpointer base2)
{
	int id1 = (*(EventObject **) base1)->id;
	int id2 = (*(EventObject **) base2)->id;

	int iddiff = id1 - id2;
	
	if (iddiff == 0)
		return 0;
	else if (iddiff < 0)
		return -1;
	else
		return 1;
}

static void
accumulate_last_n (gpointer key,
		   gpointer value,
		   gpointer user_data)
{
	GPtrArray *last_n = (GPtrArray*)user_data;

	g_ptr_array_insert_sorted (last_n, IdComparer, key);
}
#endif

void
runtime_shutdown (void)
{
	if (!inited)
		return;

	EventObject::DrainUnrefs ();
	
	Media::Shutdown ();
	
	animation_shutdown ();
	text_shutdown ();
	font_shutdown ();
	
	DependencyObject::Shutdown ();
	DependencyProperty::Shutdown ();

#if OBJECT_TRACKING
	if (EventObject::objects_created == EventObject::objects_destroyed) {
		printf ("Runtime destroyed, no leaked objects.\n");
	} else {
		printf ("Runtime destroyed, with leaked EventObjects:\n");
		printf ("\tObjects created: %i\n", EventObject::objects_created);
		printf ("\tObjects destroyed: %i\n", EventObject::objects_destroyed);
		printf ("\tDifference: %i (%.1f%%)\n", EventObject::objects_created - EventObject::objects_destroyed, (100.0 * EventObject::objects_destroyed) / EventObject::objects_created);

		GPtrArray* last_n = g_ptr_array_new ();

		g_hash_table_foreach (EventObject::objects_alive, accumulate_last_n, last_n);

	 	uint counter = 10;
		counter = MIN(counter, last_n->len);
		if (counter) {
			printf ("\tOldest %d objects alive:\n", counter);
			for (uint i = 0; i < MIN (counter, last_n->len); i ++) {
				EventObject* obj = (EventObject *) last_n->pdata[i];
				printf ("\t\t%i = %s, refcount: %i\n", obj->id, obj->GetTypeName (), obj->GetRefCount ());
			}
		}

		g_ptr_array_free (last_n, true);
	}
	g_hash_table_destroy (EventObject::objects_alive);
	EventObject::objects_alive = NULL;
#elif DEBUG
	if (EventObject::objects_created != EventObject::objects_destroyed) {
		printf ("Runtime destroyed, with %i leaked EventObjects.\n", EventObject::objects_created - EventObject::objects_destroyed);
	}
#endif

	inited = false;
	

}

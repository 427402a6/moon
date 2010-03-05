/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#include "pal-gtk.h"

#include "runtime.h"
#include "window-gtk.h"
#include "pixbuf-gtk.h"
#include "im-gtk.h"

#define Visual _XxVisual
#define Region _XxRegion
#define Window _XxWindow
#include <gdk/gdkx.h>
#include <cairo-xlib.h>
#undef Visual
#undef Region
#undef Window

#include <gdk/gdkkeysyms.h>


static Key
MapKeyvalToKey (guint keyval)
{
	switch (keyval) {
	case GDK_BackSpace:				return KeyBACKSPACE;
	case GDK_Tab: case GDK_ISO_Left_Tab:		return KeyTAB;
	case GDK_Return: case GDK_KP_Enter:		return KeyENTER;
	case GDK_Shift_L: case GDK_Shift_R:		return KeySHIFT;
	case GDK_Control_L: case GDK_Control_R:		return KeyCTRL;
	case GDK_Alt_L: case GDK_Alt_R:			return KeyALT;
	case GDK_Caps_Lock:				return KeyCAPSLOCK;
	case GDK_Escape:				return KeyESCAPE;
	case GDK_space: case GDK_KP_Space:		return KeySPACE;
	case GDK_Page_Up: case GDK_KP_Page_Up:		return KeyPAGEUP;
	case GDK_Page_Down: case GDK_KP_Page_Down:	return KeyPAGEDOWN;
	case GDK_End: case GDK_KP_End:			return KeyEND;
	case GDK_Home: case GDK_KP_Home:		return KeyHOME;
	case GDK_Left: case GDK_KP_Left:		return KeyLEFT;
	case GDK_Up: case GDK_KP_Up:			return KeyUP;
	case GDK_Right: case GDK_KP_Right:		return KeyRIGHT;
	case GDK_Down: case GDK_KP_Down:		return KeyDOWN;
	case GDK_Insert: case GDK_KP_Insert:		return KeyINSERT;
	case GDK_Delete: case GDK_KP_Delete:		return KeyDELETE;
	case GDK_0: case GDK_parenright:		return KeyDIGIT0;
	case GDK_1: case GDK_exclam:			return KeyDIGIT1;
	case GDK_2: case GDK_at:			return KeyDIGIT2;
	case GDK_3: case GDK_numbersign:		return KeyDIGIT3;
	case GDK_4: case GDK_dollar:			return KeyDIGIT4;
	case GDK_5: case GDK_percent:			return KeyDIGIT5;
	case GDK_6: case GDK_asciicircum:		return KeyDIGIT6;
	case GDK_7: case GDK_ampersand:			return KeyDIGIT7;
	case GDK_8: case GDK_multiply:			return KeyDIGIT8;
	case GDK_9: case GDK_parenleft:			return KeyDIGIT9;
	case GDK_a: case GDK_A:				return KeyA;
	case GDK_b: case GDK_B:				return KeyB;
	case GDK_c: case GDK_C:				return KeyC;
	case GDK_d: case GDK_D:				return KeyD;
	case GDK_e: case GDK_E:				return KeyE;
	case GDK_f: case GDK_F:				return KeyF;
	case GDK_g: case GDK_G:				return KeyG;
	case GDK_h: case GDK_H:				return KeyH;
	case GDK_i: case GDK_I:				return KeyI;
	case GDK_j: case GDK_J:				return KeyJ;
	case GDK_k: case GDK_K:				return KeyK;
	case GDK_l: case GDK_L:				return KeyL;
	case GDK_m: case GDK_M:				return KeyM;
	case GDK_n: case GDK_N:				return KeyN;
	case GDK_o: case GDK_O:				return KeyO;
	case GDK_p: case GDK_P:				return KeyP;
	case GDK_q: case GDK_Q:				return KeyQ;
	case GDK_r: case GDK_R:				return KeyR;
	case GDK_s: case GDK_S:				return KeyS;
	case GDK_t: case GDK_T:				return KeyT;
	case GDK_u: case GDK_U:				return KeyU;
	case GDK_v: case GDK_V:				return KeyV;
	case GDK_w: case GDK_W:				return KeyW;
	case GDK_x: case GDK_X:				return KeyX;
	case GDK_y: case GDK_Y:				return KeyY;
	case GDK_z: case GDK_Z:				return KeyZ;
		
	case GDK_F1: case GDK_KP_F1:			return KeyF1;
	case GDK_F2: case GDK_KP_F2:			return KeyF2;
	case GDK_F3: case GDK_KP_F3:			return KeyF3;
	case GDK_F4: case GDK_KP_F4:			return KeyF4;
	case GDK_F5:					return KeyF5;
	case GDK_F6:					return KeyF6;
	case GDK_F7:					return KeyF7;
	case GDK_F8:					return KeyF8;
	case GDK_F9:					return KeyF9;
	case GDK_F10:					return KeyF10;
	case GDK_F11:					return KeyF11;
	case GDK_F12:					return KeyF12;
		
	case GDK_KP_0:					return KeyNUMPAD0;
	case GDK_KP_1:					return KeyNUMPAD1;
	case GDK_KP_2:					return KeyNUMPAD2;
	case GDK_KP_3:					return KeyNUMPAD3;
	case GDK_KP_4:					return KeyNUMPAD4;
	case GDK_KP_5:					return KeyNUMPAD5;
	case GDK_KP_6:					return KeyNUMPAD6;
	case GDK_KP_7:					return KeyNUMPAD7;
	case GDK_KP_8:					return KeyNUMPAD8;
	case GDK_KP_9:					return KeyNUMPAD9;
		
	case GDK_KP_Multiply:				return KeyMULTIPLY;
	case GDK_KP_Add:				return KeyADD;
	case GDK_KP_Subtract:				return KeySUBTRACT;
	case GDK_KP_Decimal:				return KeyDECIMAL;
	case GDK_KP_Divide:				return KeyDIVIDE;
		
	default:
		return KeyUNKNOWN;
	}
}

static int
MapGdkToVKey (GdkEventKey *event)
{
	if (event->keyval >= GDK_A && event->keyval <= GDK_Z)
		return event->keyval;
	if (event->keyval >= GDK_a && event->keyval <= GDK_z)
		return event->keyval - GDK_a + GDK_A;

	if (event->keyval >= GDK_F1 && event->keyval <= GDK_F24)
		return event->keyval - GDK_F1 + 0x70;

	if (event->keyval >= GDK_KP_0 && event->keyval <= GDK_KP_9)
		return event->keyval - GDK_KP_0 + 0x60;

	switch (event->keyval) {
	case GDK_Delete:
		return 0x2e;

	case GDK_parenright:
	case GDK_0:
		return 0x30;

	case GDK_exclam:
	case GDK_1:
		return 0x31;

	case GDK_at:
	case GDK_2:
		return 0x32;

	case GDK_numbersign:
	case GDK_3:
		return 0x33;

	case GDK_dollar:
	case GDK_4:
		return 0x34;

	case GDK_percent:
	case GDK_5:
		return 0x35;

	case GDK_asciicircum:
	case GDK_6:
		return 0x36;

	case GDK_ampersand:
	case GDK_7:
		return 0x37;

	case GDK_multiply:
	case GDK_8:
		return 0x38;

	case GDK_parenleft:
	case GDK_9:
		return 0x39;

	case GDK_Num_Lock:
		return 0x90;

	case GDK_colon:
	case GDK_semicolon:
		return 0xba;

	case GDK_equal:
	case GDK_plus:
		return 0xbb;

	case GDK_comma:
	case GDK_less:
		return 0xbc;

	case GDK_minus:
	case GDK_underscore:
		return 0xbd;

	case GDK_period:
	case GDK_greater:
		return 0xbe;

	case GDK_slash:
	case GDK_question:
		return 0xbf;

	case GDK_grave:
	case GDK_asciitilde:
		return 0xc0;

	case GDK_bracketleft:
	case GDK_braceleft:
		return 0xdb;

	case GDK_backslash:
	case GDK_bar:
		return 0xdc;

	case GDK_bracketright:
	case GDK_braceright:
		return 0xdd;

	case GDK_quotedbl:
	case GDK_apostrophe:
		return 0xde;

	default:
		printf ("default case for keyval 0x%0x keycode %d\n", event->keyval, event->hardware_keycode);
		return event->hardware_keycode;
	}
}

class MoonKeyEventGtk : public MoonKeyEvent {
public:
	MoonKeyEventGtk (GdkEvent *event)
	{
		this->event = (GdkEventKey*)gdk_event_copy (event);

		key = MapKeyvalToKey (this->event->keyval);
		keycode = (moonlight_flags & RUNTIME_INIT_EMULATE_KEYCODES) ? MapGdkToVKey (this->event) : this->event->hardware_keycode;
	}

	virtual ~MoonKeyEventGtk ()
	{
		gdk_event_free ((GdkEvent*)event);
	}

	virtual MoonEvent* Clone ()
	{
		return new MoonKeyEventGtk ((GdkEvent*)event);
	}

	virtual gpointer GetPlatformEvent ()
	{
		return event;
	}

	virtual Key GetSilverlightKey ()
	{
		return key;
	}

	virtual int GetPlatformKeycode ()
	{
		return keycode;
	}

	virtual int GetPlatformKeyval ()
	{
		return event->keyval;
	}

	virtual gunichar GetUnicode ()
	{
		return gdk_keyval_to_unicode (event->keyval);
	}

	virtual MoonModifier GetModifiers ()
	{
		return (MoonModifier) event->state;
	}


	virtual bool IsModifier ()
	{
#if GTK_CHECK_VERSION(2,10,0)
	if (gtk_check_version(2,10,0))
		return event->is_modifier;
	else
#endif
		switch (event->keyval) {
		case GDK_Shift_L:
		case GDK_Shift_R:
		case GDK_Control_L:
		case GDK_Control_R:
		case GDK_Meta_L:
		case GDK_Meta_R:
		case GDK_Alt_L:
		case GDK_Alt_R:
		case GDK_Super_L:
		case GDK_Super_R:
		case GDK_Hyper_L:
		case GDK_Hyper_R:
			return true;
		default:
			return false;
		}
	}

private:
	GdkEventKey *event;

	Key key;
	int keycode;
};

static void GetStylusInfoFromDevice (GdkDevice *gdk_device, TabletDeviceType *type, bool *is_inverted)
{
	if (!gdk_device)
		return;

	switch (gdk_device->source) {
	case GDK_SOURCE_PEN:
	case GDK_SOURCE_ERASER:
		*type = TabletDeviceTypeStylus;
		break;
	case GDK_SOURCE_MOUSE:
	case GDK_SOURCE_CURSOR: /* XXX not sure where to lump this in..  in the stylus block? */
	default:
		*type = TabletDeviceTypeMouse;
		break;
	}

	*is_inverted = (gdk_device->source == GDK_SOURCE_ERASER);
}

class MoonButtonEventGtk : public MoonButtonEvent {
public:
	MoonButtonEventGtk (GdkEvent *event)
	{
		this->event = (GdkEventButton*)gdk_event_copy (event);
	}

	virtual ~MoonButtonEventGtk ()
	{
		gdk_event_free ((GdkEvent*)event);
	}

	virtual MoonEvent* Clone ()
	{
		return new MoonButtonEventGtk ((GdkEvent*)event);
	}

	virtual gpointer GetPlatformEvent ()
	{
		return event;
	}
	
	virtual Point GetPosition ()
	{
		return Point (event->x, event->y);
	}

	virtual double GetPressure ()
	{
		double pressure = 0.0;
		if (!event->device || !gdk_event_get_axis ((GdkEvent*)event, GDK_AXIS_PRESSURE, &pressure))
			pressure = 0.0;
		return pressure;
	}

	virtual void GetStylusInfo (TabletDeviceType *type, bool *is_inverted)
	{
		GetStylusInfoFromDevice (event->device, type, is_inverted);
	}

	virtual MoonModifier GetModifiers ()
	{
		return (MoonModifier) event->state;
	}

	bool IsRelease ()
	{
		return event->type == GDK_BUTTON_RELEASE;
	}

	int GetButton ()
	{
		return event->button;
	}

	// the number of clicks.  gdk provides them as event->type ==
	// GDK_3BUTTON_PRESS/GDK_2BUTTON_PRESS/GDK_BUTTON_PRESS
	virtual int GetNumberOfClicks ()
	{
		switch (event->type) {
		case GDK_BUTTON_PRESS: return 1;
		case GDK_2BUTTON_PRESS: return 2;
		case GDK_3BUTTON_PRESS: return 3;
		default: return 0;
		}
	}

private:
	GdkEventButton *event;
};


class MoonMotionEventGtk : public MoonMotionEvent {
public:
	MoonMotionEventGtk (GdkEvent *event)
	{
		this->event = (GdkEventMotion*)gdk_event_copy (event);
	}

	virtual ~MoonMotionEventGtk ()
	{
		gdk_event_free ((GdkEvent*)event);
	}

	virtual MoonEvent* Clone ()
	{
		return new MoonMotionEventGtk ((GdkEvent*)event);
	}

	virtual gpointer GetPlatformEvent ()
	{
		return event;
	}

	virtual Point GetPosition ()
	{
		return Point (event->x, event->y);
	}

	virtual double GetPressure ()
	{
		double pressure = 0.0;
		if (!event->device || !gdk_event_get_axis ((GdkEvent*)event, GDK_AXIS_PRESSURE, &pressure))
			pressure = 0.0;
		return pressure;
	}

	virtual void GetStylusInfo (TabletDeviceType *type, bool *is_inverted)
	{
		GetStylusInfoFromDevice (event->device, type, is_inverted);
	}

	virtual MoonModifier GetModifiers ()
	{
		return (MoonModifier) event->state;
	}

private:
	GdkEventMotion *event;
};

class MoonCrossingEventGtk : public MoonCrossingEvent {
public:
	MoonCrossingEventGtk (GdkEvent *event)
	{
		this->event = (GdkEventCrossing*)gdk_event_copy (event);
	}

	virtual ~MoonCrossingEventGtk ()
	{
		gdk_event_free ((GdkEvent*)event);
	}

	virtual MoonEvent* Clone ()
	{
		return new MoonCrossingEventGtk ((GdkEvent*)event);
	}

	virtual gpointer GetPlatformEvent ()
	{
		return event;
	}

	virtual Point GetPosition ()
	{
		return Point (event->x, event->y);
	}

	virtual double GetPressure ()
	{
		return 0.0;
	}

	virtual void GetStylusInfo (TabletDeviceType *type, bool *is_inverted)
	{
	}

	virtual MoonModifier GetModifiers ()
	{
		g_assert_not_reached ();
	}
	
	virtual bool IsEnter ()
	{
		return event->type == GDK_ENTER_NOTIFY;
	}

private:
	GdkEventCrossing *event;
};

class MoonFocusEventGtk : public MoonFocusEvent {
public:
	MoonFocusEventGtk (GdkEvent *event)
	{
		this->event = (GdkEventFocus*)gdk_event_copy (event);
	}

	virtual ~MoonFocusEventGtk ()
	{
		gdk_event_free ((GdkEvent*)event);
	}

	virtual MoonEvent *Clone ()
	{
		return new MoonFocusEventGtk ((GdkEvent*)event);
	}

	virtual gpointer GetPlatformEvent()
	{
		return event;
	}

	virtual bool IsIn ()
	{
		return event->in != 0;
	}

private:
	GdkEventFocus *event;
};

class MoonScrollWheelEventGtk : public MoonScrollWheelEvent {
public:
	MoonScrollWheelEventGtk (GdkEvent *event)
	{
		this->event = (GdkEventScroll*)gdk_event_copy (event);
	}

	virtual ~MoonScrollWheelEventGtk ()
	{
		gdk_event_free ((GdkEvent*)event);
	}

	virtual MoonEvent* Clone ()
	{
		return new MoonScrollWheelEventGtk ((GdkEvent*)event);
	}

	virtual gpointer GetPlatformEvent ()
	{
		return event;
	}

	virtual Point GetPosition ()
	{
		return Point (event->x, event->y);
	}

	virtual double GetPressure ()
	{
		double pressure = 0.0;
		if (!event->device || !gdk_event_get_axis ((GdkEvent*)event, GDK_AXIS_PRESSURE, &pressure))
			pressure = 0.0;
		return pressure;
	}

	virtual void GetStylusInfo (TabletDeviceType *type, bool *is_inverted)
	{
		GetStylusInfoFromDevice (event->device, type, is_inverted);
	}

	virtual MoonModifier GetModifiers ()
	{
		return (MoonModifier) event->state;
	}

#define MOON_SCROLL_WHEEL_DELTA 10

	virtual int GetWheelDelta ()
	{
		/* we only handle UP/DOWN scroll events for the time being */
		switch (event->direction) {
		case GDK_SCROLL_UP:
			return MOON_SCROLL_WHEEL_DELTA;
		case GDK_SCROLL_DOWN:
			return -MOON_SCROLL_WHEEL_DELTA;

		default:
		case GDK_SCROLL_LEFT:
		case GDK_SCROLL_RIGHT:
			return 0;
		}
	}

private:
	GdkEventScroll *event;
};

/// our windowing system

MoonWindowingSystemGtk::MoonWindowingSystemGtk ()
{
	if (!(moonlight_flags & RUNTIME_INIT_USE_BACKEND_IMAGE) && RunningOnNvidia ()) {
		printf ("Moonlight: Forcing client-side rendering because we detected binary drivers which are known to suffer performance problems.\n");
		moonlight_flags |= RUNTIME_INIT_USE_BACKEND_IMAGE;
	}
	
	LoadSystemColors ();
}

MoonWindowingSystemGtk::~MoonWindowingSystemGtk ()
{
	for (int i = 0; i < (int) NumSystemColors; i++)
		delete system_colors[i];
}


cairo_surface_t *
MoonWindowingSystemGtk::CreateSurface ()
{
	// FIXME...
	g_assert_not_reached ();
}

MoonWindow *
MoonWindowingSystemGtk::CreateWindow (bool fullscreen, int width, int height, MoonWindow *parentWindow, Surface *surface)
{
	MoonWindowGtk *gtkwindow = new MoonWindowGtk (fullscreen, width, height, parentWindow, surface);
	RegisterWindow (gtkwindow);
	return gtkwindow;
}

MoonWindow *
MoonWindowingSystemGtk::CreateWindowless (int width, int height, PluginInstance *forPlugin)
{
	MoonWindowGtk *gtkwindow = (MoonWindowGtk*)MoonWindowingSystem::CreateWindowless (width, height, forPlugin);
	if (gtkwindow)
		RegisterWindow (gtkwindow);
	return gtkwindow;
}

// older gtk+ (like 2.8 used in SLED10) don't support icon-less GTK_MESSAGE_OTHER
#ifndef GTK_MESSAGE_OTHER
#define GTK_MESSAGE_OTHER	GTK_MESSAGE_INFO
#endif

int
MoonWindowingSystemGtk::ShowMessageBox (const char *caption, const char *text, int buttons)
{
	if (!caption || !text)
		return MESSAGE_BOX_RESULT_NONE;

	// NOTE: this dialog is displayed even WITHOUT any user action
	//if (!Deployment::GetCurrent ()->GetSurface ()->IsUserInitiatedEvent ())
	//	return MESSAGE_BOX_RESULT_NONE;

	GtkButtonsType bt = buttons == MESSAGE_BOX_BUTTON_OK ? GTK_BUTTONS_OK : GTK_BUTTONS_OK_CANCEL;

	GtkWidget *widget = gtk_message_dialog_new (NULL,
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_OTHER,
						bt,
						text);

	gtk_window_set_title (GTK_WINDOW (widget), caption);
	
	gint result = gtk_dialog_run (GTK_DIALOG (widget));
	gtk_widget_destroy (widget);

	switch (result) {
	case GTK_RESPONSE_OK:
		return MESSAGE_BOX_RESULT_OK;
	case GTK_RESPONSE_CANCEL:
		return MESSAGE_BOX_RESULT_CANCEL;
	case GTK_RESPONSE_YES:
		return MESSAGE_BOX_RESULT_YES;
	case GTK_RESPONSE_NO:
		return MESSAGE_BOX_RESULT_NO;
	case GTK_RESPONSE_NONE:
	default:
		return MESSAGE_BOX_RESULT_NONE;
	}
}

static void
set_filters (GtkFileChooser *chooser, const char* filter, int idx)
{
	if (!filter || (strlen (filter) <= 1))
		return;

	char **filters = g_strsplit (filter, "|", 0);

	// to be valid (managed code) we know we have an even number of items
	// (if not we're still safe by dropping the last one)
	int pos = 0;
	int n = g_strv_length (filters) >> 1;
	for (int i=0; i < n; i++) {
		char *name = g_strstrip (filters[pos++]);
		if (strlen (name) < 1)
			continue;

		char *pattern = g_strstrip (filters[pos++]);
		if (strlen (pattern) < 1)
			continue;

		GtkFileFilter *ff = gtk_file_filter_new ();
		gtk_file_filter_set_name (ff, g_strdup (name));
		// there can be multiple patterns in a single string
		if (g_strrstr (pattern, ";")) {
			int n = 0;
			char **patterns = g_strsplit (pattern, ";", 0);
			while (char *p = patterns[n++])
				gtk_file_filter_add_pattern (ff, g_strdup (p));
			g_strfreev (patterns);
		} else {
			// or a single one
			gtk_file_filter_add_pattern (ff, g_strdup (pattern));
		}
		gtk_file_chooser_add_filter (chooser, ff);
		// idx (FilterIndex) is 1 (not 0) based
		if (i == (idx - 1))
			gtk_file_chooser_set_filter (chooser, ff);
	}
	g_strfreev (filters);
}

char**
MoonWindowingSystemGtk::ShowOpenFileDialog (const char *title, bool multsel, const char *filter, int idx)
{
	GtkWidget *widget = gtk_file_chooser_dialog_new (title, NULL, 
					    GTK_FILE_CHOOSER_ACTION_OPEN, 
					    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	GtkFileChooser *chooser = GTK_FILE_CHOOSER (widget);
	set_filters (chooser, filter, idx);
	gtk_file_chooser_set_select_multiple (chooser, multsel ? TRUE : FALSE);

	gchar **ret = NULL;
	if (gtk_dialog_run (GTK_DIALOG (widget)) == GTK_RESPONSE_ACCEPT){
		GSList *k, *l = gtk_file_chooser_get_filenames (chooser);
		int i, count = g_slist_length (l);

		ret = g_new (gchar *, count + 1);
		ret [count] = NULL;
		
		for (i = 0, k = l; k; k = k->next)
			ret [i++] = (gchar *) k->data;

		g_slist_free (l);
	}

	gtk_widget_destroy (widget);

	return ret;
}

char*
MoonWindowingSystemGtk::ShowSaveFileDialog (const char *title, const char *filter, int idx)
{
	GtkWidget *widget = gtk_file_chooser_dialog_new (title, NULL, 
					    GTK_FILE_CHOOSER_ACTION_SAVE, 
					    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	GtkFileChooser *chooser = GTK_FILE_CHOOSER (widget);
	set_filters (chooser, filter, idx);
	gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

	char* ret = NULL;
	if (gtk_dialog_run (GTK_DIALOG (widget)) == GTK_RESPONSE_ACCEPT)
		ret = gtk_file_chooser_get_filename (chooser);

	gtk_widget_destroy (widget);

	return ret;
}

void
MoonWindowingSystemGtk::RegisterWindow (MoonWindow *window)
{
}

void
MoonWindowingSystemGtk::UnregisterWindow (MoonWindow *window)
{
}

static Color *
color_from_gdk (GdkColor color)
{
	return new Color ((color.red >> 8) & 0xff, (color.green >> 8) & 0xff, (color.blue >> 8) & 0xff, 255);
}

void
MoonWindowingSystemGtk::LoadSystemColors ()
{
	GtkSettings *settings = gtk_settings_get_default ();
	GtkWidget *widget;
	GtkStyle *style;
	
	widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_ensure_style (widget);
	style = gtk_widget_get_style (widget);
	
	// AppWorkspace colors (FIXME: wtf is an Application Workspace?)
	system_colors[AppWorkspaceColor] = color_from_gdk (style->bg[GTK_STATE_ACTIVE]);
	
	// Border colors (the Window's border - FIXME: get this from the WM?)
	system_colors[ActiveBorderColor] = color_from_gdk (style->bg[GTK_STATE_ACTIVE]);
	system_colors[InactiveBorderColor] = color_from_gdk (style->bg[GTK_STATE_INSENSITIVE]);
	
	// Caption colors (the Window's title bar - FIXME: get this from the WM?)
	system_colors[ActiveCaptionColor] = color_from_gdk (style->bg[GTK_STATE_ACTIVE]);
	system_colors[ActiveCaptionTextColor] = color_from_gdk (style->fg[GTK_STATE_ACTIVE]);
	system_colors[InactiveCaptionColor] = color_from_gdk (style->bg[GTK_STATE_INSENSITIVE]);
	system_colors[InactiveCaptionTextColor] = color_from_gdk (style->fg[GTK_STATE_INSENSITIVE]);
	
	// Desktop colors (FIXME: get this from gconf?)
	system_colors[DesktopColor] = color_from_gdk (style->bg[GTK_STATE_ACTIVE]);
	
	// Window colors (GtkWindow)
	system_colors[WindowColor] = color_from_gdk (style->bg[GTK_STATE_NORMAL]);
	system_colors[WindowFrameColor] = color_from_gdk (style->bg[GTK_STATE_NORMAL]);
	system_colors[WindowTextColor] = color_from_gdk (style->fg[GTK_STATE_NORMAL]);
	
	gtk_widget_destroy (widget);
	
	// Control colors (FIXME: what widget should we use? Does it matter?)
	widget = gtk_button_new ();
	gtk_widget_ensure_style (widget);
	style = gtk_widget_get_style (widget);
	
	system_colors[ControlColor] = color_from_gdk (style->bg[GTK_STATE_ACTIVE]);
	system_colors[ControlTextColor] = color_from_gdk (style->fg[GTK_STATE_ACTIVE]);
	
	system_colors[ControlDarkColor] = color_from_gdk (style->dark[GTK_STATE_ACTIVE]);
	system_colors[ControlDarkDarkColor] = color_from_gdk (style->dark[GTK_STATE_ACTIVE]);
	system_colors[ControlDarkDarkColor]->Darken ();
	
	system_colors[ControlLightColor] = color_from_gdk (style->light[GTK_STATE_ACTIVE]);
	system_colors[ControlLightLightColor] = color_from_gdk (style->light[GTK_STATE_ACTIVE]);
	system_colors[ControlLightLightColor]->Lighten ();
	
	// Gray Text colors (disabled text)
	system_colors[GrayTextColor] = color_from_gdk (style->fg[GTK_STATE_INSENSITIVE]);
	
	gtk_widget_destroy (widget);
	
	// Highlight colors (selected items - FIXME: what widget should we use? Does it matter?)
	widget = gtk_entry_new ();
	gtk_widget_ensure_style (widget);
	style = gtk_widget_get_style (widget);
	system_colors[HighlightColor] = color_from_gdk (style->bg[GTK_STATE_SELECTED]);
	system_colors[HighlightTextColor] = color_from_gdk (style->fg[GTK_STATE_SELECTED]);
	gtk_widget_destroy (widget);
	
	// Info colors (GtkTooltip)
	if (!(style = gtk_rc_get_style_by_paths (settings, "gtk-tooltip", "GtkWindow", GTK_TYPE_WINDOW))) {
		widget = gtk_window_new (GTK_WINDOW_POPUP);
		gtk_widget_ensure_style (widget);
		style = gtk_widget_get_style (widget);
	} else {
		widget = NULL;
	}
	system_colors[InfoColor] = color_from_gdk (style->bg[GTK_STATE_NORMAL]);
	system_colors[InfoTextColor] = color_from_gdk (style->fg[GTK_STATE_NORMAL]);
	if (widget)
		gtk_widget_destroy (widget);
	
	// Menu colors (GtkMenu)
	widget = gtk_menu_new ();
	gtk_widget_ensure_style (widget);
	style = gtk_widget_get_style (widget);
	system_colors[MenuColor] = color_from_gdk (style->bg[GTK_STATE_NORMAL]);
	system_colors[MenuTextColor] = color_from_gdk (style->fg[GTK_STATE_NORMAL]);
	gtk_widget_destroy (widget);
	
	// ScrollBar colors (GtkScrollbar)
	widget = gtk_vscrollbar_new (NULL);
	gtk_widget_ensure_style (widget);
	style = gtk_widget_get_style (widget);
	system_colors[ScrollBarColor] = color_from_gdk (style->bg[GTK_STATE_NORMAL]);
	gtk_widget_destroy (widget);
}

Color *
MoonWindowingSystemGtk::GetSystemColor (SystemColor id)
{
	if (id < 0 || id >= (int) NumSystemColors)
		return NULL;
	
	return system_colors[id];
}

guint
MoonWindowingSystemGtk::AddTimeout (gint priority, gint ms, MoonSourceFunc timeout, gpointer data)
{
	return g_timeout_add_full (priority, ms, (GSourceFunc)timeout, data, NULL);
}

void
MoonWindowingSystemGtk::RemoveTimeout (guint timeoutId)
{
	g_source_remove (timeoutId);
}

MoonIMContext*
MoonWindowingSystemGtk::CreateIMContext ()
{
	return new MoonIMContextGtk ();
}

MoonEvent*
MoonWindowingSystemGtk::CreateEventFromPlatformEvent (gpointer platformEvent)
{
	GdkEvent *gdk = (GdkEvent*)platformEvent;

	switch (gdk->type) {
	case GDK_MOTION_NOTIFY: {
		GdkEventMotion *mev = (GdkEventMotion*)gdk;
		if (mev->is_hint) {
#if GTK_CHECK_VERSION(2,12,0)
			if (gtk_check_version (2, 12, 0)) {
				gdk_event_request_motions (mev);
			}
			else
#endif
			{
				int ix, iy;
				GdkModifierType state;
				gdk_window_get_pointer (mev->window, &ix, &iy, (GdkModifierType*)&state);
				mev->x = ix;
				mev->y = iy;
			}    
		}

		return new MoonMotionEventGtk (gdk);
	}
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		return new MoonButtonEventGtk (gdk);

	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		return new MoonKeyEventGtk (gdk);

	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
		return new MoonCrossingEventGtk (gdk);

	case GDK_FOCUS_CHANGE:
		return new MoonFocusEventGtk (gdk);

	case GDK_SCROLL:
		return new MoonScrollWheelEventGtk (gdk);
	default:
		printf ("unhandled gtk event %d\n", gdk->type);
		return NULL;
	}
}

guint
MoonWindowingSystemGtk::GetCursorBlinkTimeout (MoonWindow *moon_window)
{
	GdkScreen *screen;
	GdkWindow *window;
	GtkSettings *settings;
	guint timeout;

	if (!(window = GDK_WINDOW (moon_window->GetPlatformWindow ())))
		return CURSOR_BLINK_TIMEOUT_DEFAULT;
	
	if (!(screen = gdk_drawable_get_screen (window)))
		return CURSOR_BLINK_TIMEOUT_DEFAULT;
	
	if (!(settings = gtk_settings_get_for_screen (screen)))
		return CURSOR_BLINK_TIMEOUT_DEFAULT;
	
	g_object_get (settings, "gtk-cursor-blink-time", &timeout, NULL);
	
	return timeout;
}


MoonPixbufLoader*
MoonWindowingSystemGtk::CreatePixbufLoader (const char *imageType)
{
	if (imageType)
		return new MoonPixbufLoaderGtk (imageType);
	else
		return new MoonPixbufLoaderGtk ();
}

bool
MoonWindowingSystemGtk::RunningOnNvidia ()
{
	int event, error, opcode;

	Display *display = XOpenDisplay (NULL);
	bool result = XQueryExtension (display, "NV-GLX", &opcode, &event, &error);
	XCloseDisplay (display);

	return result;
}

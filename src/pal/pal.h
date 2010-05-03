/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#ifndef MOON_PAL_H
#define MOON_PAL_H

#include <glib.h>
#include <time.h>

#include "enums.h"
#include "cairo.h"
#include "color.h"
#include "point.h"
#include "rect.h"
#include "error.h"
#include "list.h"

// I hate X11
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif

// the default for MoonWindowingSystem::GetCursorBlinkTimeout
#define CURSOR_BLINK_TIMEOUT_DEFAULT  900

class Surface;
class UIElement;
class Deployment;
class Downloader;
class EventObject;
class EventArgs;
class PluginInstance;

class MoonEvent;
class MoonWindow;
class MoonClipboard;

enum MoonModifier {
	MoonModifier_Shift    = 1 << 0,
	MoonModifier_Lock     = 1 << 1,
	MoonModifier_Control  = 1 << 2,

	MoonModifier_Mod1	    = 1 << 3,
	MoonModifier_Mod2	    = 1 << 4,
	MoonModifier_Mod3	    = 1 << 5,
	MoonModifier_Mod4	    = 1 << 6,
	MoonModifier_Mod5	    = 1 << 7,

	MoonModifier_Super    = 1 << 26,
	MoonModifier_Hyper    = 1 << 27,
	MoonModifier_Meta     = 1 << 28,
};

// useful abstractions for porting moonlight to other platforms.

// returns true if the timeout/idle should be removed
typedef bool (*MoonSourceFunc) (gpointer data);

typedef bool (*MoonCallback) (gpointer sender, gpointer data);

typedef void (*MoonClipboardGetTextCallback) (MoonClipboard *clipboard, const char *text, gpointer data);

class MoonEvent {
public:
	virtual ~MoonEvent () {}
	virtual MoonEvent *Clone () = 0;

	// returns a platform event so that other
	// platform interfaces which consume events can get at the actual data.
	virtual gpointer GetPlatformEvent() = 0;
};

class MoonKeyEvent : public MoonEvent {
public:
	virtual Key GetSilverlightKey () = 0; // returns the enum value.  this requires platform specific mapping

	virtual int GetPlatformKeycode () = 0; // FIXME: do we really need both of these?
	virtual int GetPlatformKeyval () = 0;

	virtual gunichar GetUnicode () = 0;

	virtual MoonModifier GetModifiers () = 0; // FIXME: should this be separate bool getters instead (like IsShiftDown, IsCtrlDown, IsAltDown)?

	virtual bool IsModifier () = 0;
};

class MoonMouseEvent : public MoonEvent {
public:
	virtual Point GetPosition () = 0;

	virtual double GetPressure () = 0;

	virtual void GetStylusInfo (TabletDeviceType *type, bool *is_inverted) = 0;

	virtual MoonModifier GetModifiers () = 0; // FIXME: should this be separate bool getters instead (like IsShiftDown, IsCtrlDown, IsAltDown)?
};

class MoonButtonEvent : public MoonMouseEvent {
public:
	virtual bool IsRelease () = 0;

	virtual int GetButton () = 0;

	// the number of clicks.  gdk provides them as event->type ==
	// GDK_3BUTTON_PRESS/GDK_2BUTTON_PRESS/GDK_BUTTON_PRESS
	virtual int GetNumberOfClicks () = 0; // FIXME: will this api work?
};

class MoonMotionEvent : public MoonMouseEvent {
};

class MoonCrossingEvent : public MoonMouseEvent {
public:
	virtual bool IsEnter () = 0;
};

class MoonScrollWheelEvent : public MoonMouseEvent {
public:
	virtual int GetWheelDelta () = 0;
};

class MoonFocusEvent : public MoonEvent {
public:
	virtual bool IsIn() = 0;
};


class MoonIMContext {
public:
	virtual void SetUsePreedit (bool flag) = 0;
	virtual void SetClientWindow (MoonWindow*  window) = 0;
	virtual void SetSurroundingText (const char *text, int offset, int length) = 0;
	virtual void Reset () = 0;

	virtual void FocusIn () = 0;
	virtual void FocusOut () = 0;

	virtual void SetCursorLocation (Rect r) = 0;

	virtual bool FilterKeyPress (MoonKeyEvent* event) = 0;

	virtual void SetRetrieveSurroundingCallback (MoonCallback cb, gpointer data) = 0;
	virtual void SetDeleteSurroundingCallback (MoonCallback cb, gpointer data) = 0;
	virtual void SetCommitCallback (MoonCallback cb, gpointer data) = 0;

	virtual gpointer GetPlatformIMContext () = 0;
	virtual ~MoonIMContext () {}
};

enum MoonClipboardType {
	MoonClipboard_Clipboard,
	MoonClipboard_Primary
};

class MoonClipboard {
public:
	virtual bool ContainsText () = 0;
	virtual void SetSelection (const char *text, int length) = 0;
	virtual void SetText (const char *text, int length) = 0;
	virtual void AsyncGetText (MoonClipboardGetTextCallback cb, gpointer data) = 0;
	virtual char* GetText () = 0;
	virtual ~MoonClipboard () {}
};

class MoonPixbuf {
public:
	virtual gint GetWidth () = 0;
	virtual gint GetHeight () = 0;
	virtual gint GetRowStride () = 0;
	virtual gint GetNumChannels () = 0;
	virtual guchar *GetPixels () = 0;
	virtual ~MoonPixbuf () {}
};

class MoonPixbufLoader {
public:
	virtual void Write (const guchar *buffer, int buflen, MoonError **error = NULL) = 0;
	virtual void Close (MoonError **error = NULL) = 0;
	virtual MoonPixbuf *GetPixbuf () = 0;
	virtual ~MoonPixbufLoader () {}
};

// must match values from System.Windows.MessageBoxButtons
#define MESSAGE_BOX_BUTTON_OK		0
#define MESSAGE_BOX_BUTTON_OK_CANCEL	1

// must match values from System.Windows.MessageBoxResult
#define MESSAGE_BOX_RESULT_NONE		0
#define MESSAGE_BOX_RESULT_OK		1
#define MESSAGE_BOX_RESULT_CANCEL	2
#define MESSAGE_BOX_RESULT_YES		6
#define MESSAGE_BOX_RESULT_NO		7

typedef MoonWindow* (*MoonWindowlessCtor)(int width, int height, PluginInstance *forPlugin);

/* @Version=2 */
class MoonWindowingSystem {
public:
	MoonWindowingSystem() { windowless_ctor = NULL; }

	virtual ~MoonWindowingSystem () {}

	// creates a platform/windowing system specific surface
	virtual cairo_surface_t *CreateSurface () = 0;

	/* @GenerateCBinding,GeneratePInvoke */
	virtual MoonWindow *CreateWindow (bool fullscreen, int width = -1, int height = -1, MoonWindow *parentWindow = NULL, Surface* surface = NULL) = 0;
	virtual MoonWindow *CreateWindowless (int width, int height, PluginInstance *forPlugin);

	/* @GenerateCBinding,GeneratePInvoke */
	virtual int ShowMessageBox (const char *caption, const char *text, int buttons) = 0;

	/* @GenerateCBinding,GeneratePInvoke */
	virtual gchar** ShowOpenFileDialog (const char *title, bool multsel, const char *filter, int idx) = 0;
	/* @GenerateCBinding,GeneratePInvoke */
	virtual char* ShowSaveFileDialog (const char *title, const char *filter, int idx) = 0;
	
	/* @GenerateCBinding,GeneratePInvoke */
	virtual Color *GetSystemColor (SystemColor id) = 0;
	
	virtual guint AddTimeout (gint priority, gint ms, MoonSourceFunc timeout, gpointer data) = 0;
	virtual void RemoveTimeout (guint timeoutId) = 0;

	virtual MoonIMContext* CreateIMContext () = 0;

	virtual MoonEvent* CreateEventFromPlatformEvent (gpointer platformEvent) = 0;

	virtual guint GetCursorBlinkTimeout (MoonWindow *window) = 0;

	virtual MoonPixbufLoader* CreatePixbufLoader (const char *imageType) = 0;

	void SetWindowlessCtor (MoonWindowlessCtor ctor);

private:
	MoonWindowlessCtor windowless_ctor;
};

struct MoonAppRecord {
	char *origin;
	time_t mtime;
	char *uid;
	
	MoonAppRecord ();
	~MoonAppRecord ();
	
	bool Equal (const MoonAppRecord *app) const;
	bool Save (FILE *db) const;
};

class MoonAppDatabase {
	char *base_dir;
	char *path;
	
public:
	MoonAppDatabase (const char *base_dir);
	~MoonAppDatabase ();
	
	MoonAppRecord *CreateAppRecord (const char *origin);
	
	bool AddAppRecord (MoonAppRecord *record);
	bool SyncAppRecord (const MoonAppRecord *record);
	bool RemoveAppRecord (const MoonAppRecord *record);
	
	MoonAppRecord *GetAppRecordByOrigin (const char *origin);
	MoonAppRecord *GetAppRecordByUid (const char *uid);
};

typedef void (* UpdateCompletedCallback) (bool updated, const char *error, gpointer user_data);

/* @Version=2 */
class MoonInstallerService {
	UpdateCompletedCallback completed;
	Downloader *downloader;
	MoonAppDatabase *db;
	gpointer user_data;
	MoonAppRecord *app;
	GByteArray *xap;
	
	MoonAppRecord *GetAppRecord (Deployment *deployment);
	void CloseDownloader (bool abort);
	bool InitDatabase ();
	
	static void downloader_notify_size (gint64 size, gpointer user_data);
	static void downloader_write (void *buf, gint32 offset, gint32 n, gpointer user_data);
	static void downloader_completed (EventObject *sender, EventArgs *args, gpointer user_data);
	static void downloader_failed (EventObject *sender, EventArgs *args, gpointer user_data);
	
protected:
	MoonAppRecord *CreateAppRecord (const char *origin);
	
public:
	MoonInstallerService ();
	virtual ~MoonInstallerService ();
	
	void CheckAndDownloadUpdateAsync (Deployment *deployment, UpdateCompletedCallback completed, gpointer user_data);
	
	virtual bool Install (Deployment *deployment, bool unattended) = 0;
	virtual bool Uninstall (Deployment *deployment);
	
	virtual const char *GetBaseInstallDir () = 0;
	
	MoonAppRecord *GetAppRecord (const char *uid);
	
	bool IsRunningOutOfBrowser (Deployment *deployment);
	bool CheckInstalled (Deployment *deployment);
	
	void UpdaterNotifySize (gint64 size);
	void UpdaterWrite (void *buf, gint32 offset, gint32 n);
	void UpdaterCompleted ();
	void UpdaterFailed ();
};

// XXX we need to think about multitouch events/tablets/accelerometers/gtk extension events, etc.

typedef char* (*MessageReceivedCallback) (const char *message, gpointer data);
typedef void (*MessageSentCallback) (const char *message, const char *response, gpointer managedUserState, gpointer data);

/* @Version=2 */
class MoonMessageListener {
public:
	MoonMessageListener () {};
	virtual ~MoonMessageListener () {};
	
	virtual void AddMessageReceivedCallback (MessageReceivedCallback messageReceivedCallback, gpointer data) = 0;
	virtual void RemoveMessageReceivedCallback () = 0;
};

/* @Version=2 */
class MoonMessageSender {
public:
	MoonMessageSender () {};
	virtual ~MoonMessageSender () {};
	
	virtual void AddMessageSentCallback (MessageSentCallback messageSentCallback, gpointer data) = 0;
	virtual void RemoveMessageSentCallback () = 0;

	virtual void SendMessageAsync (const char *msg, gpointer managedUserState, MoonError *error) = 0;
};

/* @Version=2 */
class MoonMessagingService {
public:
	MoonMessagingService () {};
	virtual ~MoonMessagingService () {};

	virtual MoonMessageListener* CreateMessagingListener (const char *domain, const char *listenerName, MoonError *error) = 0;
	virtual MoonMessageSender* CreateMessagingSender (const char *listenerName, const char *listenerDomain, const char *domain, MoonError *error) = 0;
};


//
// XXX media capture api below not finished
//
struct MoonVideoFormat {
public:
	MoonVideoFormat (MoonPixelFormat format,
			 int framesPerSecond,
			 int stride,
			 int width,
			 int height) :
		format (format),
		framesPerSecond (framesPerSecond),
		stride (stride),
		width (width),
		height (height)
	{ }

	MoonPixelFormat GetPixelFormat () { return format; }
	int GetFramesPerSecond () { return framesPerSecond; }
	int GetStride () { return stride; }
	int GetHeight () { return height; }
	int GetWidth () { return width; }
	
private:
	MoonPixelFormat format;
	int framesPerSecond;
	int stride;
	int width;
	int height;
};

class MoonAudioFormat {
public:
	MoonAudioFormat (int bitsPerSample,
			 int channels,
			 int samplesPerSecond,
			 MoonWaveFormatType waveFormatType) :
		bitsPerSample (bitsPerSample),
		channels (channels),
		samplesPerSecond (samplesPerSecond),
		waveFormatType (waveFormatType)
	{ }

	int GetBitsPerSample () { return bitsPerSample; }
	int GetChannels () { return channels; }
	int GetSamplesPerSecond () { return samplesPerSecond; }
	MoonWaveFormatType GetWaveFormatType () { return waveFormatType; }

private:
	int bitsPerSample;
	int channels;
	int samplesPerSecond;
	MoonWaveFormatType waveFormatType;
};

class MoonCaptureDevice {
public:
	MoonCaptureDevice () {};
	virtual ~MoonCaptureDevice () {};

	virtual const char GetFriendlyName () = 0;
	virtual bool GetIsDefaultDevice () = 0;
};

class MoonVideoCaptureDevice : public MoonCaptureDevice {
public:
	MoonVideoCaptureDevice () {};
	virtual ~MoonVideoCaptureDevice () {};

	virtual MoonVideoFormat* GetDesiredFormat () = 0;

	// FIXME this should just return an array of structs
	virtual List* GetSupportedFormats () = 0;
};

class MoonAudioCaptureDevice : public MoonCaptureDevice {
	MoonAudioCaptureDevice () {};
	virtual ~MoonAudioCaptureDevice () {};

	virtual int GetAudioFrameSize () = 0;
	virtual MoonAudioFormat* GetDesiredFormat () = 0;
	// FIXME this should just return an array of structs
	virtual List* GetSupportedFormats () = 0;
};

class MoonVideoCaptureService {
public:
	MoonVideoCaptureService () {};
	virtual ~MoonVideoCaptureService () {}

	virtual MoonVideoCaptureDevice* GetDefaultCaptureDevice () = 0;
	// FIXME this should just return an array of MoonVideoCaptureDevice*
	virtual List* GetAvailableCaptureDevices () = 0;
};

class MoonAudioCaptureService {
public:
	MoonAudioCaptureService () {};
	virtual ~MoonAudioCaptureService () {}

	virtual MoonAudioCaptureDevice* GetDefaultCaptureDevice () = 0;
	// FIXME this should just return an array of MoonAudioCaptureDevice*
	virtual List* GetAvailableCaptureDevices () = 0;
};

class MoonCaptureService {
public:
	MoonCaptureService () {};
	virtual ~MoonCaptureService () {};

	virtual MoonVideoCaptureService *GetVideoCaptureService() = 0;
	virtual MoonAudioCaptureService *GetAudioCaptureService() = 0;

	// return true if the platform requires its own user
	// interaction to enable access to video/audio capture devices
	virtual bool RequiresSystemPermissionForDeviceAccess () = 0;

	// it's alright to block waiting on a response here, return
	// true if the user has allowed access.
	virtual bool RequestSystemAccess () = 0;
};

#endif /* MOON_PAL_H */

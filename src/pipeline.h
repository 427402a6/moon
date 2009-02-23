/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * pipeline.h: Pipeline for the media
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#ifndef __MOON_PIPELINE_H_
#define __MOON_PIPELINE_H_

#include <glib.h>
#include <stdio.h>
#include <pthread.h>
#include "utils.h"

#define MAKE_CODEC_ID(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))

#define CODEC_WMV1	MAKE_CODEC_ID ('W', 'M', 'V', '1')
#define CODEC_WMV2	MAKE_CODEC_ID ('W', 'M', 'V', '2')
#define CODEC_WMV3	MAKE_CODEC_ID ('W', 'M', 'V', '3')
#define CODEC_WMVA	MAKE_CODEC_ID ('W', 'M', 'V', 'A')
#define CODEC_WVC1	MAKE_CODEC_ID ('W', 'V', 'C', '1')
#define CODEC_MP3	0x55
#define CODEC_WMAV1 0x160
#define CODEC_WMAV2 0x161
#define CODEC_WMAV3 0x162

#define MAX_VIDEO_HEIGHT	2048
#define MAX_VIDEO_WIDTH		2048

typedef void (*register_codec) (int abi_version);

/*
 *	Should be capable of:
 *	- play files and live streams
 *	- open a file and get information about its streams
 *	    - audio: format, initial_pts, # of channels, sample rate, sample size
 *	    - video: format, initial_pts, height, width, frame rate
 *	- needs to be able to pause playback when there's no more source (reached end of downloaded data / progressive downloading)
 *	- informing back about streamed markers (these are quite rare, so it doesn't make sense to poll on every AdvanceFrame if there are any new markers)
 */

/*
 *	
 *	Playing media more or less goes like this with async frame reading (from MediaPlayer's perspective)
 *		Open ():
 *			Stop ()
 *			set state to paused
 *			Open the file, read data/headers, initialize whatever has to be initialized
 *			if any video streams, request first frame to be decoded (sync) and show it
 *		Play ():
 *			set state to playing
 *			set flag that we need more frames
 *			enqueue a few frame requests
 *		Pause ():
 *			set state to paused
 *			clear the queue of requested frames (no need to wait until frames being decoded are finished)
 *		Stop ():
 *			set state to stopped
 *			EmptyQueues ()
 *		AdvanceFrame ():
 *			if not playing, return
 *			if no decoded frames, return
 *			aquire queue-lock
 *			pop a decoded video+audio frame
 *			release queue-lock
 *			draw/play a decoded video/audio frame(s)
 *			enqueue more frame requests (one for each drawn/played)
 *		Seek ():
 *			EmptyQueues ()
 *			seek to the desired position
 *			enqueue a few frame requests
 *		EmptyQueues ():
 *			set flag that we need no more frames (saving old state)
 *			clear the queue of requested frames and wait until no more frames are being decoded
 *			// No need to lock here, since we know that nobody will call FrameDecodedCallback now (there are no requested frames)
 *			empty the queue of decoded frames
 *			set flag to saved state
 *		
 *		FrameDecodedCallback () -> called on another thread
 *			if flag that we need no more frames is set, do nothing
 *			aquire queue-lock
 *			add the decoded frame to the queue of decoded frames
 *			release queue-lock
 *			
 *			
 *
 */

class Media;
class IMediaSource;
class IMediaStream;
class IMediaDemuxer;
class IMediaDecoder;
class IMediaObject;
class FileSource;
class LiveSource;
class MediaClosure;
class MediaFrame;
class VideoStream;
class AudioStream;
class MarkerStream;
class IImageConverter;
class MediaMarker;
class ProgressiveSource;
class MediaMarkerFoundClosure;
class Playlist;

typedef gint32 MediaResult;

#define MEDIA_SUCCESS ((MediaResult) 0)
#define MEDIA_FAIL ((MediaResult) 1)
#define MEDIA_INVALID_STREAM ((MediaResult) 2)
#define MEDIA_UNKNOWN_CODEC ((MediaResult) 3)
#define MEDIA_INVALID_MEDIA ((MediaResult) 4)
#define MEDIA_FILE_ERROR ((MediaResult) 5)
#define MEDIA_CODEC_ERROR ((MediaResult) 6)
#define MEDIA_OUT_OF_MEMORY ((MediaResult) 7)
#define MEDIA_DEMUXER_ERROR ((MediaResult) 8)
#define MEDIA_CONVERTER_ERROR ((MediaResult) 9)
#define MEDIA_UNKNOWN_CONVERTER ((MediaResult) 10)
#define MEDIA_UNKNOWN_MEDIA_TYPE ((MediaResult) 11)
#define MEDIA_CODEC_DELAYED ((MediaResult) 12)
#define MEDIA_NO_MORE_DATA ((MediaResult) 13)
#define MEDIA_CORRUPTED_MEDIA ((MediaResult) 14)
#define MEDIA_NO_CALLBACK ((MediaResult) 15)
#define MEDIA_INVALID_DATA ((MediaResult) 16)
#define MEDIA_READ_ERROR ((MediaResult) 17)
// The pipeline returns this value in GetNextFrame if the
// buffer is empty.
#define MEDIA_BUFFER_UNDERFLOW ((MediaResult) 18)
// This value might be returned by the pipeline for an open
// request, indicating that there is not enough data available
// to open the media.
// It is also used internally in the pipeline whenever something
// can't be done because of not enough data.
// Note: this value must not be used for eof condition.
#define MEDIA_NOT_ENOUGH_DATA ((MediaResult) 19)

#define MEDIA_SUCCEEDED(x) (((x) <= 0))

#define FRAME_PLANAR (1 << 0)
#define FRAME_DECODED (1 << 1)
#define FRAME_DEMUXED (1 << 2)
#define FRAME_CONVERTED (1 << 3)
#define FRAME_KEYFRAME (1 << 4)
#define FRAME_MARKER (1 << 6)

enum MediaSourceType {
	MediaSourceTypeFile = 1,
	MediaSourceTypeLive = 2,
	MediaSourceTypeProgressive = 3,
	MediaSourceTypeMemory = 4,
	MediaSourceTypeQueueMemory = 5,
	MediaSourceTypeManagedStream = 6,
};

enum MediaStreamSourceDiagnosticKind {
    BufferLevelInMilliseconds = 1,
    BufferLevelInBytes = 2
};

enum MoonPixelFormat {
	MoonPixelFormatNone = 0,
	MoonPixelFormatRGB32,
	MoonPixelFormatYUV420P
};

enum MediaStreamType {
	MediaTypeAudio = 0,
	MediaTypeVideo = 1,
	MediaTypeMarker = 2
};

typedef MediaResult MediaCallback (MediaClosure *closure);

#include "list.h"
#include "debug.h"
#include "dependencyobject.h"
#include "error.h"
#include "type.h"
#include "enums.h"

/*
 * MediaClosure: 
 */ 
class MediaClosure : public EventObject {
private:
	MediaCallback *callback;
	MediaResult result;

	Media *media;
	EventObject *context; // The property of whoever creates the closure.
	MediaCallback *finished; // Calls this method after calling base class 'callback'
	
protected:
	virtual ~MediaClosure () {}
	
public:
	MediaClosure (Media *media, MediaCallback *callback, MediaCallback *finished, EventObject *context);
	MediaClosure (Media *media, MediaCallback *callback, EventObject *context);
	virtual void Dispose ();
	
	void Call (); // Calls the callback and stores the result in 'result', then calls the 'finished' callback

	MediaResult GetResult () { return result; }
	Media *GetMedia () { return media; }
	EventObject *GetContext () { return context; }
};


/*
 * MediaGetFrameClosure
 */
class MediaGetFrameClosure : public MediaClosure {
private:
	IMediaStream *stream;

protected:
	virtual ~MediaGetFrameClosure ();
		
public:
	MediaGetFrameClosure (Media *media, MediaCallback *callback, IMediaDemuxer *context, IMediaStream *stream);
	virtual void Dispose ();
	
	IMediaStream *GetStream () { return stream; }
	IMediaDemuxer *GetDemuxer () { return (IMediaDemuxer *) GetContext (); }
};

/*
 * MediaDecodeFrameClosure
 */
class MediaDecodeFrameClosure : public MediaClosure {
private:
	MediaFrame *frame;

protected:
	virtual ~MediaDecodeFrameClosure () {}
	
public:
	MediaDecodeFrameClosure (Media *media, MediaCallback *callback, IMediaDemuxer *context, MediaFrame *frame);
	virtual void Dispose ();
	
	MediaFrame *GetFrame () { return frame; }
};


/*
 * MediaMarkerFoundClosure
 */
class MediaMarkerFoundClosure : public MediaClosure {
private:
	MediaMarker *marker;

protected:
	virtual ~MediaMarkerFoundClosure () {}
	
public:
	MediaMarkerFoundClosure (Media *media, MediaCallback *callback, MediaElement *context);
	virtual void Dispose ();
	
	MediaMarker *GetMarker () { return marker; }
	void SetMarker (MediaMarker *marker);
};

/*
 * MediaSeekClosure
 */
class MediaSeekClosure : public MediaClosure {
private:
	guint64 pts;

protected:
	virtual ~MediaSeekClosure () {}
	
public:
	MediaSeekClosure (Media *media, MediaCallback *callback, IMediaDemuxer *context, guint64 pts);
	
	IMediaDemuxer *GetDemuxer () { return (IMediaDemuxer *) GetContext (); }
	guint64 GetPts () { return pts; }
};

/*
 * *Info classes used to register codecs, demuxers and converters.
 */

class MediaInfo {
public:
	MediaInfo *next; // Used internally by Media.
	MediaInfo () : next (NULL) {}
	virtual ~MediaInfo () {}
	virtual const char *GetName () { return "Unknown"; }
};

class DecoderInfo : public MediaInfo {
public:
	virtual bool Supports (const char *codec) = 0;
	virtual IMediaDecoder *Create (Media *media, IMediaStream *stream) = 0;
};

class DemuxerInfo : public MediaInfo  {
public:
	// <buffer> points to the first <length> bytes of a file. 
	// <length> is guaranteed to be at least 16 bytes.
	// Possible return values:
	// - MEDIA_SUCCESS: the demuxer supports this source
	// - MEDIA_FAIL: the demuxer does not support this source
	// - MEDIA_NOT_ENOUGH_DATA: the source doesn't have enough data available for the demuxer to know if it's a supported format or not.
	virtual MediaResult Supports (IMediaSource *source) = 0; 
	virtual IMediaDemuxer *Create (Media *media, IMediaSource *source) = 0;
};

class ConverterInfo : public MediaInfo  {
public:
	virtual bool Supports (MoonPixelFormat input, MoonPixelFormat output) = 0;
	virtual IImageConverter *Create (Media *media, VideoStream *stream) = 0;
};

/*
 * MediaWork
 */ 

class MediaWork : public List::Node {
public:
	MediaClosure *closure;
	MediaWork (MediaClosure *closure);
	virtual ~MediaWork ();
};

/*
 * Media
 */
class Media : public EventObject {
private:	
	static ConverterInfo *registered_converters;
	static DemuxerInfo *registered_demuxers;
	static DecoderInfo *registered_decoders;
	static bool registering_ms_codecs;
	static bool registered_ms_codecs;

	List *queued_requests;
	pthread_t queue_thread;
	pthread_cond_t queue_condition;
	pthread_mutex_t queue_mutex;
	
	guint64 buffering_time;

	char *uri;
	char *file;
	IMediaSource *source;
	IMediaDemuxer *demuxer;
	List *markers;
	bool initialized;
	bool opened;
	bool opening;
	bool stopping;
	bool stopped; // If the worker thread has been stopped.
	bool error_reported; // If an error has been reported.
	bool buffering_enabled;
	bool in_open_internal; // detect recursive calls to OpenInternal
	double download_progress;
	double buffering_progress;
	
	Downloader *downloader;
	PlaylistRoot *playlist;

	//	Called on another thread, loops the queue of requested frames 
	//	and calls GetNextFrame and FrameReadCallback.
	//	If there are any requests for audio frames in the queue
	//	they are always (and all of them) satisfied before any video frame request.
	void WorkerLoop ();
	static void *WorkerLoop (void *data);
	void StopThread (); // Stops the worker thread.
	
	// Determines the container type and selects a demuxer. We have support for mp3 and asf demuxers.
	// Also opens the demuxer.
	// This method is supposed to be called multiple times, until either 'error_reported' is true or this method
	// returns true. It will pick up wherever it left working last time.
	bool SelectDemuxerAsync ();
	
	//	Selects decoders according to stream info.
	//	- Default is to use any MS decoder if available (and applicable), otherwise ffmpeg. 
	//    Overridable by MOONLIGHT_OVERRIDES, set ms-codecs=no to disable ms codecs, and ffmpeg-codecs=no to disable ffempg codecs
	// This method is supposed to be called multiple times, until either 'error_reported' is true or this method
	// returns true. It will pick up wherever it left working last time.
	bool SelectDecodersAsync ();
	
	// This method is supposed to be called multiple times, until the media has been successfully opened or an error occurred.
	void OpenInternal ();
	static MediaResult OpenInternal (MediaClosure *closure);

protected:
	virtual ~Media ();

public:
	Media (PlaylistRoot *root);
	
	virtual void Dispose ();
	
	bool InMediaThread ();
	void EnqueueWork (MediaClosure *closure, bool wakeup = true);
	
	// Initialize the Media.
	// These methods may raise MediaError events.
	void Initialize (Downloader *downloader, const char *PartName); // MediaElement.SetSource (dl, 'PartName');
	void Initialize (const char *uri); // MediaElement.Source = 'uri';
	void Initialize (IMediaDemuxer *demuxer); // MediaElement.SetSource (demuxer); 
	void Initialize (IMediaSource *source);

	// Start opening the media.
	// When done, OpenCompleted event is raised.
	// In case of failure, MediaError event is raised	
	void OpenAsync ();
	
	void ReportOpenDemuxerCompleted (); // This method is called by the demuxer when it has opened.
	void ReportOpenDecoderCompleted (IMediaDecoder *decoder); // This method is called by any of the decoders when it has opened.
	void ReportOpenCompleted (); // Raise the OpenCompleted event.
	
	void ReportDownloadProgress (double progress);
	void ReportBufferingProgress (double progress);
	
	// Media playback
	void PlayAsync (); // Raises CurrentStateChanged
	void PauseAsync (); // Raises CurrentStateChanged
	void StopAsync (); // Raises CurrentStateChanged
	// Seek to the specified pts
	// When done, SeekCompleted is raised
	// In case of failure, MediaError is raised.
	void SeekAsync (guint64 pts);
	void ReportSeekCompleted (guint64 pts); // This method is called by IMediaDemuxer when the seek is completed. Raises the SeekCompleted event.
		
	void ClearQueue (); // Clears the queue and make sure the thread has finished processing what it's doing
	void WakeUp ();
	
	void SetBufferingTime (guint64 buffering_time);
	guint64 GetBufferingTime ();

	void SetBufferingEnabled (bool value);

	IMediaSource *GetSource () { return source; }
	IMediaDemuxer *GetDemuxer () { return demuxer; }
	const char *GetFile () { return file; }
	const char *GetUri () { return uri; }
	void SetFileOrUrl (const char *value);
	
	static void Warning (MediaResult result, const char *format, ...);
	// A list of MediaMarker::Node.
	// This is the list of markers found in the metadata/headers (not as a separate stream).
	// Will never return NULL.
	List *GetMarkers ();
	Downloader *GetDownloader () { return downloader; }
	double GetDownloadProgress () { return download_progress; }
	double GetBufferingProgress () { return buffering_progress; }
	
	PlaylistRoot *GetPlaylistRoot ();
	
	bool IsOpened () { return opened; }
	bool IsOpening () { return opening; }
	
	void ReportErrorOccurred (ErrorEventArgs *args);
	void ReportErrorOccurred (const char *message);
	void ReportErrorOccurred (MediaResult result);
	
	const static int OpeningEvent;
	const static int OpenCompletedEvent;
	const static int SeekingEvent;
	const static int SeekCompletedEvent;
	const static int CurrentStateChangedEvent;
	const static int MediaErrorEvent;
	const static int DownloadProgressChangedEvent;
	const static int BufferingProgressChangedEvent;
	
	// Registration functions
	// This class takes ownership of the infos and will delete them (not free) when the Media is shutdown.
	static void RegisterDemuxer (DemuxerInfo *info);
	static void RegisterDecoder (DecoderInfo *info);
	static void RegisterConverter (ConverterInfo *info);
	
	static void RegisterMSCodecs ();
	static bool IsMSCodecsInstalled ();
	
	static void Initialize ();
	static void Shutdown ();

	static Queue* media_objects;
	static int media_thread_count;
};
 
class MediaFrame : public EventObject {
private:
	void Initialize ();
	
protected:
	virtual ~MediaFrame ();
	
public:
	MediaFrame (IMediaStream *stream);
	/* @GenerateCBinding,GeneratePInvoke */
	MediaFrame (IMediaStream *stream, guint8 *buffer, guint32 buflen, guint64 pts);
	void Dispose ();
	
	void AddState (guint16 state) { this->state |= state; } // There's no way of "going back" to an earlier state 
	bool IsDecoded () { return (state & FRAME_DECODED) == FRAME_DECODED; }
	bool IsDemuxed () { return (state & FRAME_DEMUXED) == FRAME_DEMUXED; }
	bool IsConverted () { return (state & FRAME_CONVERTED) == FRAME_CONVERTED; }
	bool IsPlanar () { return (state & FRAME_PLANAR) == FRAME_PLANAR; }
	bool IsKeyFrame () { return (state & FRAME_KEYFRAME) == FRAME_KEYFRAME; }
	bool IsMarker () { return (state & FRAME_MARKER) == FRAME_MARKER; }
	
	IMediaStream *stream;
	MediaMarker *marker;
	void *decoder_specific_data; // data specific to the decoder
	guint64 pts; // Set by the demuxer
	guint64 duration; // Set by the demuxer
	
	guint16 state; // Current state of the frame
	guint16 event; // special frame event if non-0
	
	// The demuxer sets these to the encoded data which the
	// decoder then uses and replaces with the decoded data.
	guint8 *buffer;
	guint32 buflen;
	
	// planar data
	guint8 *data_stride[4]; // Set by the decoder
	int srcSlideY; // Set by the decoder
	int srcSlideH; // Set by the decoder
	int srcStride[4]; // Set by the decoder
};

class MediaMarker : public EventObject {
	guint64 pts; // 100-nanosecond units (pts)
	char *type;
	char *text;

protected:
	virtual ~MediaMarker ();
	
public:
	class Node : public List::Node {
	public:
		Node (MediaMarker *m)
		{
			marker = m;
			marker->ref ();
		}
		
		virtual ~Node ()
		{
			marker->unref ();
		}
		
		MediaMarker *marker;
	};
	
	MediaMarker (const char *type, const char *text, guint64 pts);
	
	const char *Type () { return type; }
	const char *Text () { return text; }
	guint64 Pts () { return pts; }
};

// Interfaces

class IMediaObject : public EventObject {
protected:
	Media *media;
	virtual ~IMediaObject () {}

public:
	IMediaObject (Type::Kind kind, Media *media);
	virtual void Dispose ();
	
	// TODO: media should be protected with a mutex, and GetMedia should return a refcounted media.
	Media *GetMedia () { return media; }
	/* @GenerateCBinding,GeneratePInvoke */
	Media *GetMediaReffed ();
	void SetMedia (Media *value);

	void ReportErrorOccurred (ErrorEventArgs *args);
	void ReportErrorOccurred (const char *message);
	void ReportErrorOccurred (MediaResult result);
};

class IMediaStream : public IMediaObject {
private:
	void *context;
	bool enabled;
	bool selected;
	bool ended; // end of stream reached.
	gint32 get_frame_pending_count;
	guint64 first_pts; // The first pts in the stream, initialized to G_MAXUINT64
	guint64 last_popped_pts; // The pts of the last frame returned, initialized to G_MAXUINT64
	guint64 last_enqueued_pts; // The pts of the last frae enqueued, initialized to G_MAXUINT64
	guint64 last_available_pts; // The last pts available, initialized to 0. Note that this field won't be correct for streams which CanSeekToPts.
	Queue queue; // Our queue of demuxed frames
	IMediaDecoder *decoder;

protected:
	virtual ~IMediaStream () {}
	virtual void FrameEnqueued () {}

	static char *CreateCodec (int codec_id); // converts fourcc int value into a string
public:
	class StreamNode : public List::Node {
	 private:
	 	MediaFrame *frame;
	 public:
		StreamNode (MediaFrame *frame);
		virtual ~StreamNode ();
		MediaFrame *GetFrame () { return frame; }
	};
	
	IMediaStream (Type::Kind kind, Media *media);
	virtual void Dispose ();
	
	//	Video, Audio, Markers, etc.
	virtual MediaStreamType GetType () = 0; // TODO: This should be removed, it clashes with GetType in EventObject.
	virtual MediaStreamType GetStreamType () { return GetType (); }
	const char *GetStreamTypeName ();
	
	IMediaDecoder *GetDecoder ();
	void SetDecoder (IMediaDecoder *value);
	
	//	If this stream is enabled (producing output). 
	//	A file might have several audio streams, 
	//	and live streams might have several video streams with different bitrates.
	bool IsEnabled () { return enabled; }
	const char *GetCodec () { return codec; }
	
	//	User defined context value.
	void *GetContext () { return context; }
	void  SetContext (void *context) { this->context = context; }
	
	bool GetSelected () { return selected; }
	void SetSelected (bool value);

	guint32 GetBitrate ();

	void *extra_data;
	int extra_data_size;
	int codec_id;
	guint64 duration; // 100-nanosecond units (pts)
	char *codec; // freed upon destruction
	// The minimum amount of padding any other part of the pipeline needs for frames from this stream.
	// Used by the demuxer when reading frames, ensures that there are at least min_padding extra bytes
	// at the end of the frame data (all initialized to 0).
	int min_padding;
	// 0-based index of the stream in the media
	// set by the demuxer, until then its value must be -1
	int index; 
	
	void EnqueueFrame (MediaFrame *frame);
	MediaFrame *PopFrame ();
	bool IsQueueEmpty ();
	void ClearQueue ();
	guint64 GetFirstPts () { return first_pts; }
	guint64 GetLastPoppedPts () { return last_popped_pts; }
	guint64 GetLastEnqueuedPts () { return last_enqueued_pts; }
	void SetLastAvailablePts (guint64 value) { last_available_pts = MAX (value, last_available_pts); }
	guint64 GetLastAvailablePts () { return last_available_pts; }
	guint64 GetBufferedSize (); // Returns the time between the last frame returned and the last frame available (buffer time)
	
	bool GetPendingFrameCount () { return get_frame_pending_count; }
	void IncPendingFrameCount () { g_atomic_int_inc (&get_frame_pending_count); }
	void DecPendingFrameCount () { g_atomic_int_dec_and_test (&get_frame_pending_count); }
	
	bool GetEnded () { return ended; }
	void SetEnded (bool value) { ended = value; }
	
	IMediaDemuxer *GetDemuxer ();
	
#if DEBUG
	void PrintBufferInformation ();
#endif
	const static int FirstFrameEnqueuedEvent;
};

class IMediaDemuxer : public IMediaObject {
private:
	IMediaStream **streams;
	int stream_count;
	bool opened;
	bool opening;
	
	static MediaResult GetFrameCallback (MediaClosure *closure);
	static MediaResult FillBuffersCallback (MediaClosure *closure);
	static MediaResult OpenCallback (MediaClosure *closure);
	static MediaResult SeekCallback (MediaClosure *closure);
	
	void FillBuffersInternal ();
	
protected:
	IMediaSource *source;
	
	IMediaDemuxer (Type::Kind kind, Media *media, IMediaSource *source);
	IMediaDemuxer (Type::Kind kind, Media *media);
	
	virtual ~IMediaDemuxer () {}
	
	void SetStreams (IMediaStream **streams, int count);
	gint32 AddStream (IMediaStream *stream);
	
	virtual void CloseDemuxerInternal () {};
	virtual void GetDiagnosticAsyncInternal (MediaStreamSourceDiagnosticKind diagnosticKind) {}
	virtual void GetFrameAsyncInternal (IMediaStream *stream) = 0;
	virtual void OpenDemuxerAsyncInternal () = 0;
	virtual void SeekAsyncInternal (guint64 pts) = 0;
	virtual void SwitchMediaStreamAsyncInternal (IMediaStream *stream) = 0;
	
	void EnqueueOpen ();
	void EnqueueGetFrame (IMediaStream *stream);
	
public:
	virtual void Dispose ();

	void CloseDemuxer () {};
	void GetDiagnosticAsync (MediaStreamSourceDiagnosticKind diagnosticKind) {}
	void GetFrameAsync (IMediaStream *stream);
	void OpenDemuxerAsync ();
	void SeekAsync (guint64 pts);
	void SwitchMediaStreamAsync (IMediaStream *stream);
	
	/* @GenerateCBinding,GeneratePInvoke */
	void ReportOpenDemuxerCompleted ();
	/* @GenerateCBinding,GeneratePInvoke */
	void ReportGetFrameCompleted (MediaFrame *frame);
	/* @GenerateCBinding,GeneratePInvoke */
	void ReportGetFrameProgress (double bufferingProgress);
	/* @GenerateCBinding,GeneratePInvoke */
	void ReportSeekCompleted (guint64 pts);
	/* @GenerateCBinding,GeneratePInvoke */
	void ReportSwitchMediaStreamCompleted (IMediaStream *stream);
	/* @GenerateCBinding,GeneratePInvoke */
	void ReportGetDiagnosticCompleted (MediaStreamSourceDiagnosticKind diagnosticKind, gint64 diagnosticValue);
	
	guint64 GetBufferedSize ();
	void FillBuffers ();
	
	void PrintBufferInformation ();
	
	int GetStreamCount () { return stream_count; }
	IMediaStream *GetStream (int index);
	// Gets the longest duration from all the streams
	virtual guint64 GetDuration (); // 100-nanosecond units (pts)
	virtual const char *GetName () = 0;
	virtual void UpdateSelected (IMediaStream *stream) {};
	
	guint64 GetLastAvailablePts ();
	IMediaSource *GetSource () { return source; }
	bool IsOpened () { return opened; }
	bool IsOpening () { return opening; }
	
	virtual bool IsPlaylist () { return false; }
	virtual Playlist *GetPlaylist () { return NULL; }
};

class IMediaDecoder : public IMediaObject {
private:
	bool opening;
	bool opened;
	MoonPixelFormat pixel_format; // The pixel format this codec outputs. Open () should fill this in.
	IMediaStream *stream;
		
protected:
	virtual ~IMediaDecoder () {}

	virtual void DecodeFrameAsyncInternal (MediaFrame *frame) = 0;
	virtual void OpenDecoderAsyncInternal () = 0;
	void ReportDecodeFrameCompleted (MediaFrame *frame);
	void ReportOpenDecoderCompleted ();
	
	void SetPixelFormat (MoonPixelFormat value) { pixel_format = value; }
	
public:
	IMediaDecoder (Type::Kind kind, Media *media, IMediaStream *stream);
	virtual void Dispose ();
	
	// If MediaFrame->decoder_specific_data is non-NULL, this method is called in ~MediaFrame.
	virtual void Cleanup (MediaFrame *frame) {}
	virtual void CleanState () {}
	virtual bool HasDelayedFrame () { return false; }
	
	virtual const char *GetName () { return GetTypeName (); }
	
	void DecodeFrameAsync (MediaFrame *frame);
	void OpenDecoderAsync ();
	
	bool IsOpening () { return opening; }
	bool IsOpened () { return opened; }
	MoonPixelFormat GetPixelFormat () { return pixel_format; }
	IMediaStream *GetStream () { return stream; }	
};


/*
 * Inherit from this class to provide image converters (yuv->rgb for instance) 
 */
class IImageConverter : public IMediaObject {
protected:
	virtual ~IImageConverter () {}

public:
	MoonPixelFormat output_format;
	MoonPixelFormat input_format;
	VideoStream *stream;
	
	IImageConverter (Type::Kind kind, Media *media, VideoStream *stream);
	
	virtual MediaResult Open () = 0;
	virtual MediaResult Convert (guint8 *src[], int srcStride[], int srcSlideY, int srcSlideH, guint8 *dest[], int dstStride []) = 0;
};

/*
 * IMediaSource
 */
class IMediaSource : public IMediaObject {
private:
	// General locking behaviour:
	// All protected virtual methods must be called with the mutex
	// locked. If a derived virtual method needs to lock, it needs
	// to be implemented as a protected virtual method xxxInternal
	// which requires the mutex to be locked, and then a public 
	// method in IMediaSource which does the locking. No public method
	// in IMediaSource may be called from the xxxInternal methods.
	pthread_mutex_t mutex;
	pthread_cond_t condition;

protected:
	virtual ~IMediaSource ();

	void Lock ();
	void Unlock ();

	// All these methods must/will be called with the lock locked.	
	virtual gint32 ReadInternal (void *buf, guint32 n) = 0;
	virtual gint32 PeekInternal (void *buf, guint32 n) = 0;
	virtual bool SeekInternal (gint64 offset, int mode) = 0;
	virtual gint64 GetLastAvailablePositionInternal () { return -1; }
	virtual gint64 GetPositionInternal () = 0;
	virtual gint64 GetSizeInternal () = 0;

public:
	IMediaSource (Type::Kind kind, Media *media);
	virtual void Dispose ();
	
	// Initializes this stream (and if it succeeds, it can be read from later on).
	// streams based on remote content (live/progress) should contact the server
	// and try to start downloading content
	// file streams should try to open the file
	virtual MediaResult Initialize () = 0;
	virtual MediaSourceType GetType () = 0;
	
	// Reads 'n' bytes into 'buf'. If data isn't available it will 
	// read the amount of data available. Returns the number of bytes read.
	// This method will lock the mutex.
	gint32 ReadSome (void *buf, guint32 n);

	// Reads 'n' bytes into 'buf'.
	// Returns false if 'n' bytes couldn't be read.
	// This method will lock the mutex.
	bool ReadAll (void *buf, guint32 n);

	// Reads 'n' bytes into 'buf', starting at position 'start'. If 'start' is -1,
	// then start at the current position. If data isn't available it will
	// read the amount of data available. Returns false if 'n' bytes couldn't be
	// read.
	// This method will lock the mutex.
	bool Peek (void *buf, guint32 n);
	
	virtual bool CanSeek () { return true; }

	// Seeks to the specified 'offset', using the specified 'mode'. 
	// This method will lock the mutex.
	bool Seek (gint64 offset, int mode = SEEK_CUR);
	
	// Seeks to the specified 'pts'.
	virtual bool CanSeekToPts () { return false; }
	virtual MediaResult SeekToPts (guint64 pts) { return MEDIA_FAIL; }

	// Returns the current reading position
	// This method will lock the mutex.
	gint64 GetPosition ();

	// Returns the size of the source. This method may return -1 if the
	// size isn't known.
	// This method will lock the mutex.
	gint64 GetSize ();

	virtual bool Eof () = 0;

	// Returns the last available position
	// If the returned value is -1, then everything is available.
	// This method will lock the mutex.
	gint64 GetLastAvailablePosition ();

	// Checks if the specified position can be read
	// upon return, and if the position is not availble eof determines whether the position is not available because
	// the file isn't that big (eof = true), or the position hasn't been read yet (eof = false).
	// if the position is available, eof = false
	bool IsPositionAvailable (gint64 position, bool *eof);

	virtual const char *ToString () { return "IMediaSource"; }
};

class ManagedStreamSource : public IMediaSource {
private:
	ManagedStreamCallbacks stream;
	
protected:	
	virtual ~ManagedStreamSource ();

	virtual gint32 ReadInternal (void *buf, guint32 n);
	virtual gint32 PeekInternal (void *buf, guint32 n);
	virtual bool SeekInternal (gint64 offset, int mode);
	virtual gint64 GetPositionInternal ();
	virtual gint64 GetSizeInternal ();

public:
	ManagedStreamSource (Media *media, ManagedStreamCallbacks *stream);
	
	virtual MediaResult Initialize () { return MEDIA_SUCCESS; }
	virtual MediaSourceType GetType () { return MediaSourceTypeManagedStream; }
	
	virtual bool Eof () { return GetPositionInternal () == GetSizeInternal (); }

	virtual const char *ToString () { return "ManagedStreamSource"; }
};
 
class FileSource : public IMediaSource {
public: // private:
	gint64 size;
	FILE *fd;
	bool temp_file;
	char buffer [1024];

	void UpdateSize ();
protected:
	char *filename;
	
	virtual ~FileSource ();
	FileSource (Media *media, bool temp_file);
	
	MediaResult Open (const char *filename);

	virtual gint32 ReadInternal (void *buf, guint32 n);
	virtual gint32 PeekInternal (void *buf, guint32 n);
	virtual bool SeekInternal (gint64 offset, int mode);
	virtual gint64 GetPositionInternal ();
	virtual gint64 GetSizeInternal ();

public:
	FileSource (Media *media, const char *filename);
	virtual void Dispose ();
	
	virtual MediaResult Initialize (); 
	virtual MediaSourceType GetType () { return MediaSourceTypeFile; }
	
	virtual bool Eof ();

	virtual const char *ToString () { return filename; }

	const char *GetFileName () { return filename; }
};

class ProgressiveSource : public FileSource {
private:
	gint64 write_pos;
	gint64 size;
	// To avoid locking while reading and writing (since reading is done on 
	// the media thread and writing on the main thread), we open two file
	// handlers, one for reading (in FileSource) and the other one here
	// for writing.
	FILE *write_fd;
	Downloader *downloader;
	
	virtual gint64 GetLastAvailablePositionInternal () { return size == write_pos ? write_pos : write_pos & ~(1024*4-1); }
	virtual gint64 GetSizeInternal () { return size; }

	EVENTHANDLER (ProgressiveSource, DownloadFailed,   EventObject, ErrorEventArgs);
	EVENTHANDLER (ProgressiveSource, DownloadComplete, EventObject, EventArgs);
	
	static void data_write (void *data, gint32 offset, gint32 n, void *closure);
	static void size_notify (gint64 size, gpointer data);

	void DataWrite (void *data, gint32 offset, gint32 n);
	void NotifySize (gint64 size);
	void SetTotalSize (gint64 size);
	
	void CloseWriteFile ();
	
protected:
	virtual ~ProgressiveSource () {}

public:
	ProgressiveSource (Media *media, Downloader *downloader);
	virtual void Dispose ();
	
	virtual MediaResult Initialize (); 
	virtual MediaSourceType GetType () { return MediaSourceTypeProgressive; }
	
	Downloader *GetDownloader ();
};

/*
 * MemorySource
 */
class MemorySource : public IMediaSource {
private:
	void *memory;
	gint32 size;
	gint64 start;
	gint64 pos;
	bool owner;

protected:
	virtual ~MemorySource ();

	virtual gint32 ReadInternal (void *buf, guint32 n);
	virtual gint32 PeekInternal (void *buf, guint32 n);
	virtual bool SeekInternal (gint64 offset, int mode);
	virtual gint64 GetSizeInternal () { return size; }
	virtual gint64 GetPositionInternal () { return pos + start; }
	virtual gint64 GetLastAvailablePositionInternal () { return start + size; }

public:
	MemorySource (Media *media, void *memory, gint32 size, gint64 start = 0);

	void *GetMemory () { return memory; }
	void Release (void) { delete this; }

	void SetOwner (bool value) { owner = value; }
	gint64 GetStart () { return start; }

	virtual MediaResult Initialize () { return MEDIA_SUCCESS; }
	virtual MediaSourceType GetType () { return MediaSourceTypeMemory; }
	
	virtual bool CanSeek () { return true; }
	virtual bool Eof () { return pos >= size; }

	virtual const char *ToString () { return "MemorySource"; }
};


// MemoryNestedSource is used to allow independent reading/seeking
// into an already created MemorySource. This is required when we 
// read data to calculate bufferingprogress (on main thread), while
// the same data might get read on the worker thread. Using the same 
// MemorySource would corrupt the current position.
class MemoryNestedSource : public MemorySource {
private:
	MemorySource *src;

protected:
	virtual ~MemoryNestedSource ();
	
public:
	MemoryNestedSource (MemorySource *src);
};

class VideoStream : public IMediaStream {
protected:
	virtual ~VideoStream ();

public:
	void Dispose ();
	
	IImageConverter *converter; // This stream has the ownership of the converter, it will be deleted upon destruction.
	guint32 bits_per_sample;
	guint64 pts_per_frame; // Duration (in pts) of each frame. Set to 0 if unknown.
	guint64 initial_pts;
	guint32 height;
	guint32 width;
	guint32 bit_rate;
	
	VideoStream (Media *media);
	
	/* @GenerateCBinding,GeneratePInvoke */
	VideoStream (Media *media, int codec_id, guint32 width, guint32 height, guint64 duration, gpointer extra_data, guint32 extra_data_size);
	
	virtual MediaStreamType GetType () { return MediaTypeVideo; } 
	guint32 GetBitRate () { return (guint32) bit_rate; }
};
 
class AudioStream : public IMediaStream {
protected:
	virtual ~AudioStream () {}

public:
	int bits_per_sample;
	int block_align;
	int sample_rate;
	int channels;
	int bit_rate;
	
	AudioStream (Media *media);
	
	/* @GenerateCBinding,GeneratePInvoke */
	AudioStream (Media *media, int codec_id, int bits_per_sample, int block_align, int sample_rate, int channels, int bit_rate, gpointer extra_data, guint32 extra_data_size);
	
	virtual MediaStreamType GetType () { return MediaTypeAudio; }
	guint32 GetBitRate () { return (guint32) bit_rate; }
};

/*
 * ExternalDemuxer
 */

typedef void (* CloseDemuxerCallback) (void *instance);
typedef void (* GetDiagnosticAsyncCallback) (void *instance, MediaStreamSourceDiagnosticKind diagnosticKind);
typedef void (* GetFrameAsyncCallback) (void *instance, MediaStreamType mediaStreamType);
typedef void (* OpenDemuxerAsyncCallback) (void *instance, IMediaDemuxer *demuxer);
typedef void (* SeekAsyncCallback) (void *instance, guint64 seekToTime);
typedef void (* SwitchMediaStreamAsyncCallback) (void *instance, IMediaStream *mediaStreamDescription);
		
class ExternalDemuxer : public IMediaDemuxer {
private:
	void *instance;
	CloseDemuxerCallback close_demuxer_callback;
	GetDiagnosticAsyncCallback get_diagnostic_async_callback;
	GetFrameAsyncCallback get_sample_async_callback;
	OpenDemuxerAsyncCallback open_demuxer_async_callback;
	SeekAsyncCallback seek_async_callback;
	SwitchMediaStreamAsyncCallback switch_media_stream_async_callback;
	
protected:
	virtual ~ExternalDemuxer () {}

	virtual void CloseDemuxerInternal ();
	virtual void GetDiagnosticAsyncInternal (MediaStreamSourceDiagnosticKind diagnosticsKind);
	virtual void GetFrameAsyncInternal (IMediaStream *stream);
	virtual void OpenDemuxerAsyncInternal ();
	virtual void SeekAsyncInternal (guint64 seekToTime);
	virtual void SwitchMediaStreamAsyncInternal (IMediaStream *mediaStreamDescription);
	
public:
	ExternalDemuxer (Media *media, void *instance, CloseDemuxerCallback close_demuxer, 
		GetDiagnosticAsyncCallback get_diagnostic, GetFrameAsyncCallback get_sample, OpenDemuxerAsyncCallback open_demuxer, 
		SeekAsyncCallback seek, SwitchMediaStreamAsyncCallback switch_media_stream);
	
	virtual void Dispose ();
		
	/* @GenerateCBinding,GeneratePInvoke */
	void SetCanSeek (bool value);
	
	/* @GenerateCBinding,GeneratePInvoke */
	gint32 AddStream (IMediaStream *stream);
	
	virtual const char *GetName () { return "ExternalDemuxer"; }
};

/*
 * ASX demuxer
 */ 
class ASXDemuxer : public IMediaDemuxer {
private:
	Playlist *playlist;

protected:
	virtual ~ASXDemuxer ();
	virtual MediaResult SeekInternal (guint64 pts) { return MEDIA_FAIL; }

	virtual void GetFrameAsyncInternal (IMediaStream *stream) { ReportErrorOccurred ("GetFrameAsync isn't supported for ASXDemuxer"); }
	virtual void OpenDemuxerAsyncInternal ();
	virtual void SeekAsyncInternal (guint64 seekToTime) {}
	virtual void SwitchMediaStreamAsyncInternal (IMediaStream *stream) {}
	
public:
	ASXDemuxer (Media *media, IMediaSource *source);
	virtual void Dispose ();
	
	virtual bool IsPlaylist () { return true; }
	virtual Playlist *GetPlaylist () { return playlist; }
	virtual const char *GetName () { return "ASXDemuxer"; }
};

class ASXDemuxerInfo : public DemuxerInfo {
public:
	virtual MediaResult Supports (IMediaSource *source);
	virtual IMediaDemuxer *Create (Media *media, IMediaSource *source); 
	virtual const char *GetName () { return "ASXDemuxer"; }
};

class MarkerStream : public IMediaStream {
private:
	MediaMarkerFoundClosure *closure;

protected:
	virtual ~MarkerStream () {}

public:
	MarkerStream (Media *media);
	virtual void Dispose ();
	
	virtual MediaStreamType GetType () { return MediaTypeMarker; }
	
	void SetCallback (MediaMarkerFoundClosure *closure);

	// Since the markers are taking the wrong way through the pipeline 
	// (it's the pipeline who is pushing the markers up to the consumer, 
	// not the consumer reading new markers), this works in the following way:
	// The demuxer reaches a marker somehow, creates a MediaFrame with the marker data and calls MarkerFound on the MarkerStream.
	// The MarkerStream calls the decoder to decode the frame.
	// The decoder must create an instance of MediaMarker and store it in the frame's buffer.
	// The MarkerStream then calls the closure with the MediaMarker.
	// Cleanup:
	// 	- The stream (in MarkerFound) frees the MediaMarker.
	//  - The demuxer frees the MediaFrame, and the original frame buffer (before decoding).
	void MarkerFound (MediaFrame *frame);
	
	virtual void FrameEnqueued ();
};

class NullDecoder : public IMediaDecoder {
private:
	// Video data
	guint8 *logo;
	guint32 logo_size;

	// Audio datarf
	guint64 prev_pts;
	
	MediaResult DecodeVideoFrame (MediaFrame *frame);
	MediaResult DecodeAudioFrame (MediaFrame *frame);
	MediaResult OpenAudio ();
	MediaResult OpenVideo ();
	
protected:
	virtual ~NullDecoder () {}
	virtual void DecodeFrameAsyncInternal (MediaFrame *frame);
	virtual void OpenDecoderAsyncInternal ();

public:
	NullDecoder (Media *media, IMediaStream *stream);
	virtual void Dispose ();
	
	virtual const char *GetName () { return "NullDecoder"; }
};

class NullDecoderInfo : public DecoderInfo {	
public:
	virtual bool Supports (const char *codec);
	
	virtual IMediaDecoder *Create (Media *media, IMediaStream *stream)
	{
		return new NullDecoder (media, stream);
	}	
	virtual const char *GetName () { return "NullDecoder"; }
};

#endif

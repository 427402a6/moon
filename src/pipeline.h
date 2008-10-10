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

#define MAKE_CODEC_ID(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))

#define CODEC_WMV1	MAKE_CODEC_ID ('W', 'M', 'V', '1')
#define CODEC_WMV2	MAKE_CODEC_ID ('W', 'M', 'V', '2')
#define CODEC_WMV3	MAKE_CODEC_ID ('W', 'M', 'V', '3')
#define CODEC_WMVA	MAKE_CODEC_ID ('W', 'M', 'V', 'A')
#define CODEC_WVC1	MAKE_CODEC_ID ('W', 'V', 'C', '1')
#define CODEC_MP3	0x55
#define CODEC_WMAV1 0x160
#define CODEC_WMAV2 0x161

#define MOONLIGHT_CODEC_ABI_VERSION 3
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
struct ManagedStreamCallbacks;

typedef gint32 MediaResult;

#define MEDIA_SUCCESS ((MediaResult) 0)
#define MEDIA_FAIL ((MediaResult) 1)
#define MEDIA_INVALID_PROTOCOL ((MediaResult) 2)
#define MEDIA_INVALID_ARGUMENT ((MediaResult) 3)
#define MEDIA_INVALID_STREAM ((MediaResult) 4)
#define MEDIA_UNKNOWN_CODEC ((MediaResult) 5)
#define MEDIA_INVALID_MEDIA ((MediaResult) 6)
#define MEDIA_SEEK_ERROR ((MediaResult) 7)
#define MEDIA_FILE_ERROR ((MediaResult) 8)
#define MEDIA_CODEC_ERROR ((MediaResult) 9)
#define MEDIA_OUT_OF_MEMORY ((MediaResult) 10)
#define MEDIA_DEMUXER_ERROR ((MediaResult) 11)
#define MEDIA_CONVERTER_ERROR ((MediaResult) 12)
#define MEDIA_UNKNOWN_CONVERTER ((MediaResult) 13)
#define MEDIA_UNKNOWN_MEDIA_TYPE ((MediaResult) 14)
#define MEDIA_CODEC_DELAYED ((MediaResult) 15)
#define MEDIA_NO_MORE_DATA ((MediaResult) 16)
#define MEDIA_CORRUPTED_MEDIA ((MediaResult) 17)
#define MEDIA_NO_CALLBACK ((MediaResult) 18)
#define MEDIA_INVALID_DATA ((MediaResult) 19)
#define MEDIA_READ_ERROR ((MediaResult) 20)
// The pipeline returns this value in GetNextFrame if the
// buffer is empty.
#define MEDIA_BUFFER_UNDERFLOW ((MediaResult) 21)
// This value might be returned by the pipeline for an open
// request, indicating that there is not enough data available
// to open the media.
// It is also used internally in the pipeline whenever something
// can't be done because of not enough data.
// Note: this value must not be used for eof condition.
#define MEDIA_NOT_ENOUGH_DATA ((MediaResult) 22)

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

enum FrameEvent {
	FrameEventNone,
	FrameEventEOF
};

enum MoonPixelFormat {
	MoonPixelFormatNone = 0,
	MoonPixelFormatRGB32,
	MoonPixelFormatYUV420P
};

enum MoonMediaType {
	MediaTypeNone = 0,
	MediaTypeVideo,
	MediaTypeAudio,
	MediaTypeMarker
};

enum MoonWorkType {
	// The order is important, the most important work comes first (lowest number).
	// Seek is most important (as it will invalidate all other work), 
	// and will always be put first in the queue.
	// No more than one seek request should be in the queue at the same time either.
	WorkTypeSeek = 1, 
	// All audio work is done before any video work, since glitches in the audio is worse 
	// than glitches in the video.
	WorkTypeAudio, 
	WorkTypeVideo,
	WorkTypeMarker,
	// No other work should be in the queue until this has finished, so priority doesn't matter.
	WorkTypeOpen
};

typedef MediaResult MediaCallback (MediaClosure *closure);

#include "list.h"
#include "asf.h"
#include "debug.h"
#include "dependencyobject.h"
#include "playlist.h"
#include "error.h"

class MediaClosure {
private:
	Media *media; // Set when this is the callback in Media::GetNextFrameAsync
	EventObject *context; // The property of whoever creates the closure.
	MediaCallback *callback; // The callback to call
	bool context_refcounted; // If we hold a ref to context.

public:
	MediaClosure (MediaCallback *callback);
	~MediaClosure ();
	
	// If this is a GetNextFrameAsync callback, and the result is positive,
	// this contains the resulting frame.
	MediaFrame *frame; 
	// Set when this is the callback in a MarkerStream
	MediaMarker *marker; 
	// The result of the work done in GetNextFrameAsync, OpenAsync or SeekAsync.
	MediaResult result;

	// Calls the callback and returns the callback's return value
	// If no callback is set, returns MEDIA_NO_CALLBACK
	MediaResult Call ();

	void SetMedia (Media *media);
	Media *GetMedia ();

	void SetContextUnsafe (EventObject *context); // Sets the context, but doesn't add a ref.
	void SetContext (EventObject *context);
	EventObject *GetContext ();

	MediaClosure *Clone ();
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

class MediaWork : public List::Node {
public:
	MoonWorkType type;
	MediaClosure *closure;
	union {
		struct {
			guint64 seek_pts;
		} seek;
		struct {
			guint16 states;
			IMediaStream *stream;
		} frame;
		struct {
			IMediaSource *source;
		} open;
	} data;
	
	MediaWork (MediaClosure *closure, IMediaStream *stream, guint16 states); // GetNextFrame
	MediaWork (MediaClosure *closure, guint64 seek_pts); // Seek
	MediaWork (MediaClosure *closure, IMediaSource *source); // Open
	virtual ~MediaWork ();
};

class Media : public EventObject {
private:	
	static ConverterInfo *registered_converters;
	static DemuxerInfo *registered_demuxers;
	static DecoderInfo *registered_decoders;

	List *queued_requests;
	pthread_t queue_thread;
	pthread_cond_t queue_condition;
	pthread_mutex_t queue_mutex;
	
	guint64 buffering_time;

	char *file_or_url;
	IMediaSource *source;
	IMediaDemuxer *demuxer;
	List *markers;
	bool opened;
	bool stopping;
	bool stopped; // If the worker thread has been stopped.
	bool buffering_enabled;
	static bool registering_ms_codecs;
	static bool registered_ms_codecs;
	
	MediaElement *element;
	Downloader *downloader;

	//	Called on another thread, loops the queue of requested frames 
	//	and calls GetNextFrame and FrameReadCallback.
	//	If there are any requests for audio frames in the queue
	//	they are always (and all of them) satisfied before any video frame request.
	void WorkerLoop ();
	static void *WorkerLoop (void *data);
	void EnqueueWork (MediaWork *work);	
	void StopThread (); // Stops the worker thread.

	void Init (MediaElement *element, Downloader *dl);
	
protected:
	virtual ~Media ();

public:
	Media (MediaElement *element, Downloader *dl = NULL);
	virtual void Dispose ();
	
	//	Determines the container type and selects a demuxer
	//	- Default is to use our own ASF demuxer (if it's an ASF file), otherwise use ffmpeg (if available). 
	//    Overridable by MOONLIGHT_OVERRIDES, set demuxer=ffmpeg to force the ffmpeg demuxer.
	//	Makes the demuxer read the data header and fills in stream info, etc.
	//	Selects decoders according to stream info.
	//	- Default is Media *media to use MS decoder if available, otherwise ffmpeg. 
	//    Overridable by MOONLIGHT_OVERRIDES, set ms-codecs=no to disable ms codecs, and ffmpeg-codecs=no to disable ffempg codecs
	// Possible return values:
	// - MEDIA_SUCCESS: media opened successfully
	// - MEDIA_INVALID_MEDIA: invalid media.
	// - MEDIA_FAIL: media can't be opened for whatever reason
	// - MEDIA_NOT_ENOUGH_DATA: the source doesn't have enough data yet to open the media. try again when the source has more data
	//   the source will still be positioned at position 0 when this value is returned, and this value will not be returned
	//   if the source is fully available (GetLastAvailablePosition == -1 || GetLastAvailablePosition == GetSize)
	MediaResult Open (); // Should only be called if Initialize has already been called (which will create the source)
	MediaResult Open (IMediaSource *source); // Called if you have your own source
	MediaResult OpenAsync (IMediaSource *source, MediaClosure *closure);
	
	// Seeks to the specified pts (if seekable).
	MediaResult Seek (guint64 pts);
	MediaResult SeekAsync (guint64 pts, MediaClosure *closure);
	
	//	Reads the next frame from the demuxer
	//	Requests the decoder to decode the frame
	//	Returns the decoded frame
	MediaResult GetNextFrame (MediaWork *work);
	
	//	Requests reading of the next frame
	void GetNextFrameAsync (MediaClosure *closure, IMediaStream *stream, guint16 states); 
	void ClearQueue (); // Clears the queue and make sure the thread has finished processing what it's doing
	void WakeUp ();
	
	void SetBufferingTime (guint64 buffering_time);
	guint64 GetBufferingTime ();

	void SetBufferingEnabled (bool value);

	IMediaSource *GetSource ();
	void SetSource (IMediaSource *value);
	IMediaDemuxer *GetDemuxer () { return demuxer; }
	const char *GetFileOrUrl () { return file_or_url; }
	void SetFileOrUrl (const char *value);
	MediaElement *GetElement () { return element; }
	
	static void Warning (MediaResult result, const char *format, ...);
	void AddError (MediaErrorEventArgs *args);

	// A list of MediaMarker::Node.
	// This is the list of markers found in the metadata/headers (not as a separate stream).
	// Will never return NULL.
	List *GetMarkers ();
	
	bool IsOpened () { return opened; }
	
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
	
	virtual const char* GetTypeName () { return "Media"; }
	Downloader *GetDownloader () { return downloader; }
};
 
class MediaFrame {
public:
	MediaFrame (IMediaStream *stream);
	~MediaFrame ();
	
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
private:
	guint64 pts; // 100-nanosecond units (pts)
	char *type;
	char *text;

protected:
	~MediaMarker ();

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
	IMediaObject (Media *media);
	virtual void Dispose ();
	
	// TODO: media should be protected with a mutex, and GetMedia should return a refcounted media.
	Media *GetMedia () { return media; }
	void SetMedia (Media *value);
	
	virtual const char* GetTypeName () { return "IMediaObject"; }
};


class IMediaStream : public IMediaObject {
private:
	void *context;
	bool enabled;
	bool selected;
	guint64 first_pts; // The first pts in the stream, initialized to G_MAXUINT64
	guint64 last_popped_pts; // The pts of the last frame returned, initialized to G_MAXUINT64
	guint64 last_enqueued_pts; // The pts of the last frame enqueued, initialized to G_MAXUINT64
	Queue *queue; // Our queue of demuxed frames
	IMediaDecoder *decoder;

protected:
	virtual ~IMediaStream ();

public:
	class StreamNode : public List::Node {
	 public:
		MediaFrame *frame;
		StreamNode (MediaFrame *frame) { this->frame = frame; }
		virtual ~StreamNode () { delete frame; }
	};
	
	IMediaStream (Media *media);
	virtual void Dispose ();
	
	//	Video, Audio, Markers, etc.
	virtual MoonMediaType GetType () = 0;
	
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
	gint32 msec_per_frame;
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
	void ClearQueue ();
	guint64 GetFirstPts () { return first_pts; }
	guint64 GetLastPoppedPts () { return last_popped_pts; }
	guint64 GetLastEnqueuedPts () { return last_enqueued_pts; }
	guint64 GetBufferedSize (); // Returns the time between the last frame returned and the last frame available (buffer time)
#if DEBUG
	void PrintBufferInformation ();
#endif
	virtual const char* GetTypeName () { return "IMediaStream"; }
};

class IMediaDemuxer : public IMediaObject {
private:
	IMediaStream **streams;
	int stream_count;
	
protected:
	IMediaSource *source;
	
	void SetStreams (IMediaStream **streams, int count);
	virtual ~IMediaDemuxer ();
	virtual MediaResult SeekInternal (guint64 pts) = 0;
	
public:
	IMediaDemuxer (Media *media, IMediaSource *source);
	
	virtual MediaResult ReadHeader () = 0;
	// Fills the uncompressed_data field in the frame with data.
	// Tries to read a frame.
	// Must return failure (MEDIA_NOT_ENOUGH_DATA)
	// if not enough available data in the source
	virtual MediaResult TryReadFrame (IMediaStream *stream, MediaFrame **frame) = 0;
	MediaResult Seek (guint64 pts);
	void FillBuffers ();
	guint64 GetBufferedSize ();
#if DEBUG
	void PrintBufferInformation ();
#endif
	int GetStreamCount () { return stream_count; }
	IMediaStream *GetStream (int index);
	// Gets the longest duration from all the streams
	virtual guint64 GetDuration (); // 100-nanosecond units (pts)
	virtual const char *GetName () = 0;
	virtual void UpdateSelected (IMediaStream *stream) {};
	
	virtual const char* GetTypeName () { return GetName (); }
};

class IMediaDecoder : public IMediaObject {
protected:
	virtual ~IMediaDecoder () {}

public:
	IMediaDecoder (Media *media, IMediaStream *stream);
	
	virtual MediaResult DecodeFrame (MediaFrame *frame) = 0;
	virtual MediaResult Open () = 0;
	
	// If MediaFrame->decoder_specific_data is non-NULL, this method is called in ~MediaFrame.
	virtual void Cleanup (MediaFrame *frame) {}
	virtual void CleanState () {}
	virtual bool HasDelayedFrame () { return false; }
	MoonPixelFormat pixel_format; // The pixel format this codec outputs. Open () should fill this in.
	IMediaStream *stream;
	
	virtual const char* GetTypeName () { return "IMediaDecoder"; }
	virtual const char *GetName () { return GetTypeName (); }
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
	
	IImageConverter (Media *media, VideoStream *stream);
	
	virtual MediaResult Open () = 0;
	virtual MediaResult Convert (guint8 *src[], int srcStride[], int srcSlideY, int srcSlideH, guint8 *dest[], int dstStride []) = 0;
	
	virtual const char* GetTypeName () { return "IImageConverter"; }
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
	IMediaSource (Media *media);
	
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

	virtual void NotifySize (gint64 size) { return; }
	virtual void NotifyFinished () { return; }

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

	virtual void Write (void *buf, gint64 offset, gint32 n) { return; }
	
	virtual const char* GetTypeName () { return "IMediaSource"; }
};

// Implementations

typedef bool   Stream_CanSeek  (void *handle);
typedef bool   Stream_CanRead  (void *handle);
typedef gint64 Stream_Length   (void *handle);
typedef gint64 Stream_Position (void *handle);
typedef gint32 Stream_Read     (void *handle,  void *buffer, gint32 offset, gint32 count);
typedef void   Stream_Seek     (void *handle, gint64 offset, gint32 origin);

struct ManagedStreamCallbacks {
	void *handle;
	Stream_CanSeek *CanSeek;
	Stream_CanRead *CanRead;
	Stream_Length *Length;
	Stream_Position *Position;
	Stream_Read *Read;
	Stream_Seek *Seek;
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

	virtual const char* GetTypeName () { return "ManagedStreamSource"; }
};
 
class FileSource : public IMediaSource {
public: // private:
	gint64 size;
	FILE *fd;
	bool temp_file;
	char buffer [1024];

protected:
	char *filename;
	
	virtual ~FileSource ();
	FileSource (Media *media, bool temp_file);

	virtual gint32 ReadInternal (void *buf, guint32 n);
	virtual gint32 PeekInternal (void *buf, guint32 n);
	virtual bool SeekInternal (gint64 offset, int mode);
	virtual gint64 GetPositionInternal ();
	virtual gint64 GetSizeInternal ();

public:
	FileSource (Media *media, const char *filename);
	
	virtual MediaResult Initialize (); 
	virtual MediaSourceType GetType () { return MediaSourceTypeFile; }
	
	virtual bool Eof ();

	virtual const char *ToString () { return filename; }

	const char *GetFileName () { return filename; }
	
	virtual const char* GetTypeName () { return "FileSource"; }
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
	
	virtual gint64 GetLastAvailablePositionInternal () { return size == write_pos ? write_pos : write_pos & ~(1024*4-1); }
	virtual gint64 GetSizeInternal () { return size; }

protected:
	virtual ~ProgressiveSource ();

public:
	ProgressiveSource (Media *media);
	
	virtual MediaResult Initialize (); 
	virtual MediaSourceType GetType () { return MediaSourceTypeProgressive; }
	
	void SetTotalSize (gint64 size);
	
	virtual void Write (void *buf, gint64 offset, gint32 n);
	void NotifySize (gint64 size);
	virtual void NotifyFinished ();

	virtual const char* GetTypeName () { return "ProgressiveSource"; }
};

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

	virtual const char* GetTypeName () { return "MemorySource"; }
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

class MemoryQueueSource : public IMediaSource {
private:
	Queue *queue;
	ASFParser *parser;
	bool finished;
	guint64 write_count;

protected:
	virtual gint32 ReadInternal (void *buf, guint32 n);
	virtual gint32 PeekInternal (void *buf, guint32 n);
	virtual bool SeekInternal (gint64 offset, int mode);
	virtual gint64 GetSizeInternal ();
	virtual gint64 GetLastAvailablePositionInternal ();
	virtual ~MemoryQueueSource ();
	virtual void Dispose ();

public:
	class QueueNode : public List::Node {
	 public:
		ASFPacket *packet;
		MemorySource *source;
		QueueNode (ASFPacket *packet);
		QueueNode (MemorySource *source);
		virtual ~QueueNode ();
	};
	
	MemoryQueueSource (Media *media);
	void AddPacket (MemorySource *packet);
	ASFPacket *Pop ();
	bool Advance (); 

	virtual void NotifySize (gint64 size);
	virtual void NotifyFinished ();

	virtual MediaResult Initialize () { return MEDIA_SUCCESS; }
	virtual MediaSourceType GetType () { return MediaSourceTypeQueueMemory; }
	virtual gint64 GetPositionInternal ();
	virtual void Write (void *buf, gint64 offset, gint32 n);
	
	virtual bool CanSeek () { return true; }
	virtual bool Eof () { return finished && queue && queue->IsEmpty (); }

	virtual const char *ToString () { return "MemoryQueueSource"; }
	virtual bool CanSeekToPts () { return true; }
	virtual MediaResult SeekToPts (guint64 pts);
	
	bool IsFinished () { return finished; } // If the server sent the MMS_END packet.
	
	Queue *GetQueue ();
	void SetParser (ASFParser *parser);
	ASFParser *GetParser ();

#if DEBUG
	virtual const char* GetTypeName () { return "MemoryQueueSource"; }
#endif
};

class VideoStream : public IMediaStream {
protected:
	virtual ~VideoStream ();

public:
	IImageConverter *converter; // This stream has the ownership of the converter, it will be deleted upon destruction.
	guint32 bits_per_sample;
	guint32 msec_per_frame;
	guint64 initial_pts;
	guint32 height;
	guint32 width;
	guint32 bit_rate;
	
	VideoStream (Media *media);
	
	virtual MoonMediaType GetType () { return MediaTypeVideo; } 
	guint32 GetBitRate () { return (guint32) bit_rate; }
	
	virtual const char* GetTypeName () { return "VideoStream"; }
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
	
	AudioStream (Media *media) : IMediaStream (media) {}
	
	virtual MoonMediaType GetType () { return MediaTypeAudio; }
	guint32 GetBitRate () { return (guint32) bit_rate; }
	
	virtual const char* GetTypeName () { return "AudioStream"; }
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

public:
	ASXDemuxer (Media *media, IMediaSource *source);
	
	virtual MediaResult ReadHeader ();
	MediaResult TryReadFrame (IMediaStream *stream, MediaFrame **frame) { return MEDIA_FAIL; }

	Playlist *GetPlaylist () { return playlist; }
	virtual const char *GetName () { return "ASXDemuxer"; }
};

class ASXDemuxerInfo : public DemuxerInfo {
public:
	virtual MediaResult Supports (IMediaSource *source);
	virtual IMediaDemuxer *Create (Media *media, IMediaSource *source); 
	virtual const char *GetName () { return "ASXDemuxer"; }
};

/*
 * ASF related implementations
 */
class ASFDemuxer : public IMediaDemuxer {
private:
	gint32 *stream_to_asf_index;
	ASFReader *reader;
	ASFParser *parser;
	
	void ReadMarkers ();

protected:
	virtual ~ASFDemuxer ();
	virtual MediaResult SeekInternal (guint64 pts);

public:
	ASFDemuxer (Media *media, IMediaSource *source);
	
	virtual MediaResult ReadHeader ();
	virtual MediaResult TryReadFrame (IMediaStream *stream, MediaFrame **frame);
	virtual void UpdateSelected (IMediaStream *stream);
	
	ASFParser *GetParser () { return parser; }
	void SetParser (ASFParser *parser);
	virtual const char *GetName () { return "ASFDemuxer"; }
};

class ASFDemuxerInfo : public DemuxerInfo {
public:
	virtual MediaResult Supports (IMediaSource *source);
	virtual IMediaDemuxer *Create (Media *media, IMediaSource *source); 
	virtual const char *GetName () { return "ASFDemuxer"; }
};

class MarkerStream : public IMediaStream {
private:
	MediaClosure *closure;

protected:
	virtual ~MarkerStream ();

public:
	MarkerStream (Media *media);
	
	virtual MoonMediaType GetType () { return MediaTypeMarker; }
	
	// The marker stream will never delete the closure
	void SetCallback (MediaClosure *closure);
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
};

class ASFMarkerDecoder : public IMediaDecoder {
protected:
	virtual ~ASFMarkerDecoder () {};

public:
	ASFMarkerDecoder (Media *media, IMediaStream *stream) : IMediaDecoder (media, stream) {}
	
	virtual MediaResult DecodeFrame (MediaFrame *frame);
	virtual MediaResult Open () { return MEDIA_SUCCESS; }
	virtual const char *GetTypeName () { return "ASFMarkerDecoder"; }
}; 

class ASFMarkerDecoderInfo : public DecoderInfo {
public:
	virtual bool Supports (const char *codec) { return !strcmp (codec, "asf-marker"); };
	
	virtual IMediaDecoder *Create (Media *media, IMediaStream *stream)
	{
		return new ASFMarkerDecoder (media, stream);
	}	
	virtual const char *GetName () { return "ASFMarkerDecoder"; }
};

class NullDecoder : public IMediaDecoder {
private:
	// Video data
	guint8 *logo;
	guint32 logo_size;

	// Audio data
	guint64 prev_pts;
	
	MediaResult DecodeVideoFrame (MediaFrame *frame);
	MediaResult DecodeAudioFrame (MediaFrame *frame);
	MediaResult OpenAudio ();
	MediaResult OpenVideo ();
	
protected:
	virtual ~NullDecoder ();

public:
	NullDecoder (Media *media, IMediaStream *stream);
	virtual MediaResult DecodeFrame (MediaFrame *frame);
	virtual MediaResult Open ();
	
	virtual const char *GetTypeName () { return "NullDecoder"; }
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

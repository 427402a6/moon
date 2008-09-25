/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * pipeline.cpp: Pipeline for the media
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
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <pthread.h>
#include <sched.h>

#include <dlfcn.h>

#include "pipeline.h"
#include "pipeline-ffmpeg.h"
#include "mp3.h"
#include "uri.h"
#include "media.h"
#include "asf/asf.h"
#include "asf/asf-structures.h"
#include "yuv-converter.h"
#include "runtime.h"
#include "mms-downloader.h"
#include "pipeline-ui.h"

#define LOG_PIPELINE(...)// printf (__VA_ARGS__);
#define LOG_PIPELINE_ERROR(...) printf (__VA_ARGS__);
#define LOG_PIPELINE_ERROR_CONDITIONAL(x, ...) if (x) printf (__VA_ARGS__);
#define LOG_FRAMEREADERLOOP(...)// printf (__VA_ARGS__);

bool Media::registering_ms_codecs = false;
bool Media::registered_ms_codecs = false;

bool
Media::IsMSCodecsInstalled ()
{
	return registered_ms_codecs;
}

void
Media::RegisterMSCodecs (void)
{
	register_codec reg;
	void *dl;
	MoonlightConfiguration config;
	char *libmscodecs_path = config.GetStringValue ("Codecs", "MSCodecsPath");
	const char *functions [] = {"register_mswma", "register_mswmv", "register_msmp3"};
	registering_ms_codecs = true;

	if (libmscodecs_path == NULL || !(g_file_test (libmscodecs_path, G_FILE_TEST_EXISTS) && g_file_test (libmscodecs_path, G_FILE_TEST_IS_REGULAR)))
		libmscodecs_path = g_strdup ("libmscodecs.so");
	
	dl = dlopen (libmscodecs_path, RTLD_LAZY);
	if (dl != NULL) {
		for (int i = 0; i < 3; i++) {
			reg = (register_codec) dlsym (dl, functions [i]);
			if (reg != NULL) {
				(*reg) (MOONLIGHT_CODEC_ABI_VERSION);
			} else if (moonlight_flags & RUNTIME_INIT_CODECS_DEBUG) {
				printf ("Moonlight: Cannot find %s in %s.\n", functions [i], libmscodecs_path);
			}
		}		
		registered_ms_codecs = true;
	} else if (moonlight_flags & RUNTIME_INIT_CODECS_DEBUG) {
		printf ("Moonlight: Cannot load %s: %s\n", libmscodecs_path, dlerror ());
	}
	g_free (libmscodecs_path);

	registering_ms_codecs = false;
}


class MediaNode : public List::Node {
public:
	Media *media;
	MediaNode (Media *media)
	{
		this->media = media;
	}
};

/*
 * Media 
 */
DemuxerInfo *Media::registered_demuxers = NULL;
DecoderInfo *Media::registered_decoders = NULL;
ConverterInfo *Media::registered_converters = NULL;
Queue *Media::media_objects = NULL;

Media::Media (MediaElement *element, Downloader *dl)
{
	LOG_PIPELINE ("Media::Media (%p <id:%i>), id: %i\n", element, GET_OBJ_ID (element), GET_OBJ_ID (this));

	// Add ourselves to the global list of medias
	media_objects->Push (new MediaNode (this));

	pthread_attr_t attribs;
	
	this->element = element;
	this->SetSurface (element->GetSurface ());

	downloader = dl;
	if (downloader)
		downloader->ref ();

	queued_requests = new List ();
	
	file_or_url = NULL;
	source = NULL;
	
	demuxer = NULL;
	markers = NULL;
	
	opened = false;
	stopping = false;
	stopped = false;
	
	pthread_attr_init (&attribs);
	pthread_attr_setdetachstate (&attribs, PTHREAD_CREATE_JOINABLE);
	
	pthread_mutex_init (&queue_mutex, NULL);
	pthread_cond_init (&queue_condition, NULL);
	
	pthread_create (&queue_thread, &attribs, WorkerLoop, this); 	
	pthread_attr_destroy (&attribs);
}

Media::~Media ()
{
	MediaNode *node;

	LOG_PIPELINE ("Media::~Media (), id: %i\n", GET_OBJ_ID (this));

	pthread_mutex_lock (&queue_mutex);
	queued_requests->Clear (true);
	delete queued_requests;
	queued_requests = NULL;
	pthread_cond_signal (&queue_condition);
	pthread_mutex_unlock (&queue_mutex);
	
	if (downloader)
		downloader->unref ();
	
	if (source)
		source->Abort ();

	if (!stopped)
		pthread_join (queue_thread, NULL);
	pthread_mutex_destroy (&queue_mutex);
	pthread_cond_destroy (&queue_condition);
	pthread_detach (queue_thread);
	
	g_free (file_or_url);
	if (source)
		source->unref ();
	if (demuxer)
		demuxer->unref ();
	delete markers;

	// Remove ourselves from the global list of medias
	// media_objects might be NULL if Media::Shutdown has been called already
	if (media_objects) {
		media_objects->Lock ();
		node = (MediaNode *) media_objects->LinkedList ()->First ();
		while (node != NULL) {
			if (node->media == this) {
				media_objects->LinkedList ()->Remove (node);
				break;
			}
			node = (MediaNode *) node->next;
		}
		media_objects->Unlock ();
	}
}

void
Media::SetSource (IMediaSource *source)
{
	LOG_PIPELINE ("Media::SetSource (%p <id:%i>)\n", source, GET_OBJ_ID (source));

	if (this->source)
		this->source->unref ();
	this->source = source;
	if (this->source)
		this->source->ref ();
}

IMediaSource *
Media::GetSource ()
{
	return source;
}

void
Media::SetFileOrUrl (const char *value)
{
	LOG_PIPELINE ("Media::SetFileOrUrl ('%s')\n", value);

	if (file_or_url)
		g_free (file_or_url);
	file_or_url = g_strdup (value);
}

List * 
Media::GetMarkers ()
{
	if (markers == NULL)
		markers = new List ();
	
	return markers;
}

void
Media::RegisterDemuxer (DemuxerInfo *info)
{
	//printf ("Media::RegisterDemuxer (%p - %s)\n", info, info->GetName ());
	info->next = NULL;
	if (registered_demuxers == NULL) {
		registered_demuxers = info;
	} else {
		MediaInfo* current = registered_demuxers;
		while (current->next != NULL)
			current = current->next;
		current->next = info;
	}
}

void
Media::RegisterConverter (ConverterInfo *info)
{
	//printf ("Media::RegisterConverter (%p)\n", info);
	info->next = NULL;
	if (registered_converters == NULL) {
		registered_converters = info;
	} else {
		MediaInfo *current = registered_converters;
		while (current->next != NULL)
			current = current->next;
		current->next = info;
	}
}

void
Media::RegisterDecoder (DecoderInfo *info)
{
	MediaInfo *current;
	
	//printf ("Media::RegisterDecoder (%p)\n", info);
	info->next = NULL;
	if (registered_decoders == NULL) {
		registered_decoders = info;
	} else {
		if (registering_ms_codecs) {
			// MS codecs might get registered after all other codecs (right after installing them), 
			// which means after the null codecs so if they don't get special treatment, they won't
			// get used until the next browser restart (when they're registered normally).
			// So instead of appending them, we prepend them.
			info->next = registered_decoders;
			registered_decoders = info;
		} else {
			current = registered_decoders;
			while (current->next != NULL)
				current = current->next;
			current->next = info;
		}
	}
	if (moonlight_flags & RUNTIME_INIT_CODECS_DEBUG)
		printf ("Moonlight: Codec has been registered: %s\n", info->GetName ());
}

void
Media::Initialize ()
{
	LOG_PIPELINE ("Media::Initialize ()\n");
	
	media_objects = new Queue ();	
	
	// demuxers
	Media::RegisterDemuxer (new ASFDemuxerInfo ());
	Media::RegisterDemuxer (new Mp3DemuxerInfo ());
	Media::RegisterDemuxer (new ASXDemuxerInfo ());

	// converters
	if (!(moonlight_flags & RUNTIME_INIT_FFMPEG_YUV_CONVERTER))
		Media::RegisterConverter (new YUVConverterInfo ());

	// decoders
	Media::RegisterDecoder (new ASFMarkerDecoderInfo ());
	if (!(moonlight_flags & RUNTIME_INIT_DISABLE_MS_CODECS)) {
		RegisterMSCodecs ();
	}
#ifdef INCLUDE_FFMPEG
	if (!(moonlight_flags & RUNTIME_INIT_DISABLE_FFMPEG_CODECS)) {
		register_ffmpeg ();
	}
#endif
	
	Media::RegisterDecoder (new NullDecoderInfo ());
}

void
Media::Shutdown ()
{
	LOG_PIPELINE ("Media::Shutdown ()\n");

	MediaInfo *current;
	MediaInfo *next;
	MediaNode *node;
	
	current = registered_decoders;
	while (current != NULL) {
		next = current->next;
		delete current;
		current = next;
	}
	registered_decoders = NULL;
	
	current = registered_demuxers;
	while (current != NULL) {
		next = current->next;
		delete current;
		current = next;
	}
	registered_demuxers = NULL;
	
	current = registered_converters;
	while (current != NULL) {
		next = current->next;
		delete current;
		current = next;
	}
	registered_converters = NULL;

	// Make sure all threads are stopped
	AudioPlayer::Shutdown ();

	media_objects->Lock ();
	node = (MediaNode *) media_objects->LinkedList ()->First ();
	while (node != NULL) {
		node->media->StopThread ();
		node = (MediaNode *) node->next;
	}
	
	media_objects->Unlock ();

	delete media_objects;
	media_objects = NULL;

	LOG_PIPELINE ("Media::Shutdown () [Done]\n");
}

void
Media::AddMessage (MediaResult result, const char *msg)
{
	if (!MEDIA_SUCCEEDED (result))
		printf ("Moonlight: %s (%i)\n", msg, result);
}

void
Media::AddMessage (MediaResult result, char *msg)
{
	AddMessage (result, (const char *) msg);
	g_free (msg);
}

void
Media::AddError (MediaErrorEventArgs *args)
{
	LOG_PIPELINE ("Media::AddError (%p), message: %s, code: %i\n", args, args->error_message, args->error_code);

	//TODO: We should probably reaise MediaFailed when errors occur,
	// but it will need some testing to see what MS does (especially
	// with corrupt media during playback).
	//
	//if (element) {
	//	element->MediaFailed (args);
	//} else {
		fprintf (stderr, "Media error: %s\n", args->error_message);
	//}
}

MediaResult
Media::Seek (guint64 pts)
{
	if (demuxer)
		return demuxer->Seek (pts);
	
	return MEDIA_FAIL;
}

MediaResult
Media::SeekAsync (guint64 pts, MediaClosure *closure)
{
	LOG_PIPELINE ("Media::SeekAsync (%llu, %p), id: %i\n", pts, closure, GET_OBJ_ID (this));

	if (demuxer == NULL)
		return MEDIA_FAIL;
	
	EnqueueWork (new MediaWork (closure, pts));
	
	return MEDIA_SUCCESS;
}

MediaResult
Media::Open ()
{
	LOG_PIPELINE ("Media::Open (), id: %i\n", GET_OBJ_ID (this));

	if (source == NULL) {
		AddMessage (MEDIA_INVALID_ARGUMENT, "Media::Initialize () hasn't been called (or didn't succeed).");
		return MEDIA_INVALID_ARGUMENT;
	}
	
	if (IsOpened ()) {
		AddMessage (MEDIA_FAIL, "Media::Open () has already been called.");
		return MEDIA_FAIL;
	}
	
	return Open (source);
}

MediaResult
Media::OpenAsync (IMediaSource *source, MediaClosure *closure)
{
	LOG_PIPELINE ("Media::OpenAsync (%p <id:%i>, %p), id: %i\n", source, GET_OBJ_ID (source), closure, GET_OBJ_ID (this));

	closure->SetMedia (this);

	EnqueueWork (new MediaWork (closure, source));
	
	return MEDIA_SUCCESS;
}

MediaResult
Media::Open (IMediaSource *source)
{
	LOG_PIPELINE ("Media::Open (%p <id:%i>), id: %i, downloader: %p\n", source, GET_OBJ_ID (source), GET_OBJ_ID (this), downloader);

	MediaResult result;
	MmsDownloader *mms_dl = NULL;
	ASFParser *asf_parser = NULL;
	
	LOG_PIPELINE ("Media::Open ().\n");
	
	if (source == NULL || IsOpened ()) // Initialize wasn't called (or didn't succeed) or already open.
		return MEDIA_INVALID_ARGUMENT;
	
	if (downloader != NULL && downloader->GetInternalDownloader () != NULL && downloader->GetInternalDownloader ()->GetType () == InternalDownloader::MmsDownloader) {
		// The internal downloader doesn't get deleted until the Downloader itself is destructed, which won't happen 
		// because we have a ref to it. Which means that it's safe to access the internal dl here.
		mms_dl = (MmsDownloader *) downloader->GetInternalDownloader ();
		while ((asf_parser = mms_dl->GetASFParser ()) == NULL) {
			if (stopped || stopping)
				return MEDIA_FAIL;
				
			if (downloader->IsAborted ())
				return MEDIA_READ_ERROR;
	
			if (source->Eof ())
				return MEDIA_READ_ERROR;						
			
			LOG_PIPELINE ("Media::Open (): Waiting for asf parser...");
			g_usleep (G_USEC_PER_SEC / 100); // Sleep a bit
		}
		
		demuxer = new ASFDemuxer (this, source);
		((ASFDemuxer *) demuxer)->SetParser (asf_parser);
		asf_parser->SetSource (source);
		LOG_PIPELINE ("Media::Open (): Using parser from MmsDownloader, source: %s.\n", source->ToString ());
	}
	
	
	// Select a demuxer
	DemuxerInfo *demuxerInfo = registered_demuxers;
	while (demuxer == NULL && demuxerInfo != NULL) {
		if (demuxerInfo->Supports (source))
			break;
		
		LOG_PIPELINE ("Media::Open (): '%s' can't handle this media.\n", demuxerInfo->GetName ());
		demuxerInfo = (DemuxerInfo *) demuxerInfo->next;
	}
	
	if (demuxer == NULL && demuxerInfo == NULL) {
		const char *source_name = file_or_url;
		
		if (!source_name) {
			switch (source->GetType ()) {
			case MediaSourceTypeProgressive:
			case MediaSourceTypeFile:
				source_name = ((FileSource *) source)->GetFileName ();
				break;
			case MediaSourceTypeQueueMemory:
				source_name = "live source";
				break;
			default:
				source_name = "unknown source";
				break;
			}
		}
		
		AddMessage (MEDIA_UNKNOWN_MEDIA_TYPE,
			    g_strdup_printf ("No demuxers registered to handle the media source `%s'.",
					     source_name));
		return MEDIA_UNKNOWN_MEDIA_TYPE;
	}
	
	// Found a demuxer
	if (demuxer == NULL)
		demuxer = demuxerInfo->Create (this, source);
	result = demuxer->ReadHeader ();
	
	if (!MEDIA_SUCCEEDED (result))
		return result;
	
	LOG_PIPELINE ("Media::Open (): Found %i streams in this source.\n", demuxer->GetStreamCount ());
	
	LOG_PIPELINE ("Media::Open (): Starting to select codecs...\n");
	
	// If the demuxer has no streams (ASXDemuxer for instance)
	// then just return success.
	if (demuxer->GetStreamCount () == 0)
		return result;

	result = MEDIA_FAIL; // Only set to SUCCESS if at least 1 stream can be used
	
	// Select codecs for each stream
	for (int i = 0; i < demuxer->GetStreamCount (); i++) {
		IMediaStream *stream = demuxer->GetStream (i);
		if (stream == NULL)
			return MEDIA_INVALID_STREAM;
		
		const char *codec = stream->GetCodec ();
		IMediaDecoder *decoder = NULL;
		
		if (moonlight_flags & RUNTIME_INIT_CODECS_DEBUG)
			printf ("Moonlight: Searching registered decoders for a decoder which supports '%s'\n", codec);
		
		DecoderInfo *current_decoder = registered_decoders;
		while (current_decoder != NULL && !current_decoder->Supports (codec)) {
			if (moonlight_flags & RUNTIME_INIT_CODECS_DEBUG)
				printf ("Moonlight: Checking if registered decoder '%s' supports codec '%s': no.\n", current_decoder->GetName (), codec);
			current_decoder = (DecoderInfo*) current_decoder->next;
		}

		if (current_decoder == NULL) {
			AddMessage (MEDIA_UNKNOWN_CODEC, g_strdup_printf ("Unknown codec: '%s'.", codec));	
		} else {
			if (moonlight_flags & RUNTIME_INIT_CODECS_DEBUG)
				printf ("Moonlight: Checking if registered decoder '%s' supports codec '%s': yes.\n", current_decoder->GetName (), codec);
			decoder = current_decoder->Create (this, stream);
		}
		
		if (decoder != NULL) {
			result = decoder->Open ();
			if (!MEDIA_SUCCEEDED (result)) {
				decoder->unref ();
				decoder = NULL;
			}
		}
		
		if (decoder != NULL) {
			// Select converter for this stream
			if (stream->GetType () == MediaTypeVideo && decoder->pixel_format != MoonPixelFormatRGB32) {
				VideoStream *vs = (VideoStream *) stream;
				IImageConverter *converter = NULL;
				
				ConverterInfo* current_conv = registered_converters;
				while (current_conv != NULL && !current_conv->Supports (decoder->pixel_format, MoonPixelFormatRGB32)) {
					LOG_PIPELINE ("Checking whether '%s' supports input '%d' and output '%d': no.\n",
						current_conv->GetName (), decoder->pixel_format, MoonPixelFormatRGB32);
					current_conv = (ConverterInfo*) current_conv->next;
				}
				
				if (current_conv == NULL) {
					AddMessage (MEDIA_UNKNOWN_CONVERTER,
						    g_strdup_printf ("Can't convert from %d to %d: No converter found.",
								     decoder->pixel_format, MoonPixelFormatRGB32));	
				} else {
					LOG_PIPELINE ("Checking whether '%s' supports input '%d' and output '%d': yes.\n",
						current_conv->GetName (), decoder->pixel_format, MoonPixelFormatRGB32);
					converter = current_conv->Create (this, vs);
					converter->input_format = decoder->pixel_format;
					converter->output_format = MoonPixelFormatRGB32;
					if (!MEDIA_SUCCEEDED (converter->Open ())) {
						converter->unref ();
						converter = NULL;
					}
				}
				
				if (converter != NULL) {
					vs->converter = converter;
				} else {
					decoder->unref ();
					decoder = NULL;
				}
			}
		}
		
		if (decoder != NULL) {
			stream->SetDecoder (decoder);
			result = MEDIA_SUCCESS;
		}
	}
	
	if (result == MEDIA_SUCCESS) {
		SetSource (source);
		opened = true;
	}
	
	LOG_PIPELINE ("Media::Open (): result = %s\n", MEDIA_SUCCEEDED (result) ? "true" : "false");

	return result;
}

MediaResult
Media::GetNextFrame (MediaWork *work)
{
	MediaFrame *frame = NULL;
	guint16 states= work->data.frame.states;	
	IMediaStream *stream = work->data.frame.stream;
	MediaResult result = MEDIA_SUCCESS;
	
	//printf ("Media::GetNextFrame (%p).\n", stream);
	
	if (work == NULL) {
		AddMessage (MEDIA_INVALID_ARGUMENT, "work is NULL.");
		return MEDIA_INVALID_ARGUMENT;
	}
	
	if (stream == NULL) {
		AddMessage (MEDIA_INVALID_ARGUMENT, "work->stream is NULL.");
		return MEDIA_INVALID_ARGUMENT;
	}
	
	if ((states & FRAME_DEMUXED) != FRAME_DEMUXED)
		return result; // Nothing to do?

	do {
		if (frame) {
			LOG_PIPELINE ("Media::GetNextFrame (): delayed a frame\n");
			delete frame;
		}
		frame = new MediaFrame (stream);
		frame->AddState (states);

		// TODO: get the last frame out of delayed codecs (when the demuxer returns NO_MORE_DATA)

		result = demuxer->ReadFrame (frame);
		if (!MEDIA_SUCCEEDED (result))
			break;
		
		if ((states & FRAME_DECODED) != FRAME_DECODED) {
			// We weren't requested to decode the frame
			// This might cause some errors on delayed codecs (such as the wmv ones).
			break;
		}
		
		if (frame->event != 0)
			break;
	
		result = stream->decoder->DecodeFrame (frame);
	} while (result == MEDIA_CODEC_DELAYED);

	work->closure->frame = frame;

	//printf ("Media::GetNextFrame (%p) finished, size: %i.\n", stream, frame->buflen);
	
	return result;
}

void * 
Media::WorkerLoop (void *data)
{
	Media *media = (Media *) data;
	
	media->WorkerLoop ();
	
	return NULL;
}

void
Media::WorkerLoop ()
{
	MediaResult result;
	
	LOG_PIPELINE ("Media::WorkerLoop ().\n");

	while (queued_requests != NULL && !stopping) {
		MediaWork *node = NULL;
		
		LOG_FRAMEREADERLOOP ("Media::WorkerLoop (): entering mutex.\n");
		
		// Wait until we have something in the queue
		pthread_mutex_lock (&queue_mutex);
		while (queued_requests != NULL && !stopping && queued_requests->IsEmpty ())
			pthread_cond_wait (&queue_condition, &queue_mutex);
		
		LOG_FRAMEREADERLOOP ("Media::WorkerLoop (): got something.\n");
		
		if (queued_requests != NULL) {
			// Find the first audio node
			node = (MediaWork *) queued_requests->First ();
			
			if (node != NULL)
				queued_requests->Unlink (node);
			
			LOG_FRAMEREADERLOOP ("Media::WorkerLoop (): got a node, there are %d nodes left.\n",
					     queued_requests->Length ());
		}
		
		pthread_mutex_unlock (&queue_mutex);
		
		if (node == NULL)
			continue; // Found nothing, continue waiting.
		
		LOG_FRAMEREADERLOOP ("Media::WorkerLoop (): processing node %p with type %i.\n", node, node->type);
		
		switch (node->type) {
		case WorkTypeSeek:
			//printf ("Media::WorkerLoop (): Seeking, current count: %d\n", queued_requests->Length ());
			result = Seek (node->data.seek.seek_pts);
			break;
		case WorkTypeAudio:
		case WorkTypeVideo:
		case WorkTypeMarker:
			// Now demux and decode what we found and send it to who asked for it
			result = GetNextFrame (node);
			break;
		case WorkTypeOpen:
			result = Open (node->data.open.source);
			break;
		}
		
		node->closure->result = result;
		node->closure->Call ();

		LOG_FRAMEREADERLOOP ("Media::WorkerLoop (): processed node %p with type %i, result: %i.\n",
				     node, node->type, result);
		
		delete node;
	}
	
	stopped = true;

	LOG_PIPELINE ("Media::WorkerLoop (): exiting.\n");
}

void
Media::GetNextFrameAsync (MediaClosure *closure, IMediaStream *stream, guint16 states)
{
	MoonWorkType type;
	
	if (stream == NULL) {
		AddMessage (MEDIA_INVALID_ARGUMENT, "stream is NULL.");
		return;
	}
	
	switch (stream->GetType ()) {
	case MediaTypeAudio:
		type = WorkTypeAudio;
		break;
	case MediaTypeVideo:
		type = WorkTypeVideo;
		break;
	case MediaTypeMarker:
		type = WorkTypeMarker;
		break;
	case MediaTypeNone:
	default:
		AddMessage (MEDIA_INVALID_ARGUMENT, "The frame's stream is of an unknown type.");
		return;
	}
	
	EnqueueWork (new MediaWork (closure, stream, states));
}

void
Media::EnqueueWork (MediaWork *work)
{
	MediaWork *current;
	
	//printf ("Media::EnqueueWork (%p), type: %i.\n", work, work->type);
	
	pthread_mutex_lock (&queue_mutex);
	
	if (queued_requests->First ()) {
		switch (work->type) {
		case WorkTypeSeek:
			// Only have one seek request in the queue, and make
			// sure to have it first.
			current = (MediaWork *) queued_requests->First ();
			if (current->type == WorkTypeSeek)
				queued_requests->Remove (current);
			
			queued_requests->Prepend (work);
			break;
		case WorkTypeAudio:
		case WorkTypeVideo:
		case WorkTypeMarker:
			// Insert the work just before work with less priority.
			current = (MediaWork *) queued_requests->First ();
			while (current != NULL && work->type >= current->type)
				current = (MediaWork *) current->next;
			
			if (current)
				queued_requests->InsertBefore (work, current);
			else
				queued_requests->Append (work);
			break;
		case WorkTypeOpen:
			queued_requests->Prepend (work);
			break;
		}
	} else {
		queued_requests->Append (work);
	}
	
	//printf ("Media::EnqueueWork (%p), count: %i\n", work, queued_requests->Length ());

	pthread_cond_signal (&queue_condition);
	
	pthread_mutex_unlock (&queue_mutex);
}

void
Media::ClearQueue ()
{
	LOG_PIPELINE ("Media::ClearQueue ().\n");
	if (queued_requests != NULL) {
		pthread_mutex_lock (&queue_mutex);
		queued_requests->Clear (true);
		pthread_mutex_unlock (&queue_mutex);
	}
	
	if (source)
		source->Abort ();
}

void
Media::StopThread ()
{
	LOG_PIPELINE ("Media::ClearQueue ().\n");

	if (stopped)
		return;

	stopping = true;
	ClearQueue ();
	pthread_mutex_lock (&queue_mutex);
	pthread_cond_signal (&queue_condition);
	pthread_mutex_unlock (&queue_mutex);
	pthread_join (queue_thread, NULL);

	LOG_PIPELINE ("Media::ClearQueue () [Done]\n");
}

/*
 * ASFDemuxer
 */

ASFDemuxer::ASFDemuxer (Media *media, IMediaSource *source) : IMediaDemuxer (media, source)
{
	stream_to_asf_index = NULL;
	reader = NULL;
	parser = NULL;
}

ASFDemuxer::~ASFDemuxer ()
{
	g_free (stream_to_asf_index);
	
	delete reader;

	if (parser)
		parser->unref ();
}
guint64 
ASFDemuxer::GetLastAvailablePts ()
{
	if (reader == NULL)
		return 0;

	return reader->GetLastAvailablePts ();
}

gint64
ASFDemuxer::EstimatePtsPosition (guint64 pts)
{
	gint64 result = -1;
	
	for (int i = 0; i < GetStreamCount (); i++) {
		if (!GetStream (i)->GetSelected ())
			continue;

		result = MAX (result, reader->GetFrameReader (stream_to_asf_index [i])->EstimatePtsPosition(pts));
	}
	
	return result;
}

void
ASFDemuxer::UpdateSelected (IMediaStream *stream)
{
	if (reader)
		reader->SelectStream (stream_to_asf_index [stream->index], stream->GetSelected ());

	IMediaDemuxer::UpdateSelected (stream);
}

MediaResult
ASFDemuxer::Seek (guint64 pts)
{
	//printf ("ASFDemuxer::Seek (%llu)\n", pts);
	
	if (reader == NULL)
		return MEDIA_FAIL;
	
	return reader->Seek (pts) ? MEDIA_SUCCESS : MEDIA_FAIL;
}

void
ASFDemuxer::ReadMarkers ()
{
	/*
		We can get markers from several places:
			- The header of the file, read before starting to play
				- As a SCRIPT_COMMAND
				- As a MARKER
				They are both treated the same way, added into the timeline marker collection when the media is loaded.
			- As data in the file (a separate stream whose type is ASF_COMMAND_MEDIA)
				These markers show up while playing the file, and they don't show up in the timeline marker collection,
				they only get to raise the MarkerReached event.
				currently the demuxer will call the streamed_marker_callback when it encounters any of these.    
	*/
	
	// Hookup to the marker (ASF_COMMAND_MEDIA) stream
	MarkerStream *marker_stream;
	ASFFrameReader *reader;
	for (int i = 0; i < GetStreamCount (); i++) {
		if (GetStream (i)->GetType () == MediaTypeMarker) {
			marker_stream = (MarkerStream *) GetStream (i);
			this->reader->SelectStream (stream_to_asf_index [marker_stream->index], true);
			reader = this->reader->GetFrameReader (stream_to_asf_index [marker_stream->index]);
			reader->SetMarkerStream (marker_stream);
			printf ("ASFDemuxer::ReadMarkers (): Hooked up marker stream.\n");
		}
	}
		
	// Read the markers (if any)
	List *markers = media->GetMarkers ();
	const char *type;
	guint64 pts;
	guint64 preroll_pts = MilliSeconds_ToPts (parser->GetFileProperties ()->preroll);
	char *text;
	int i = -1;
	
	// Read the SCRIPT COMMANDs
	char **command_types = NULL;
	asf_script_command_entry **commands = NULL;
	asf_script_command *command = parser->script_command;
	
	if (command != NULL) {
		commands = command->get_commands (parser, &command_types);
		
		if (command_types == NULL) {
			//printf ("MediaElement::ReadASFMarkers (): No command types.\n");
			goto cleanup;
		}
	}
	
	if (commands != NULL) {
		for (i = 0; commands[i]; i++) {
			asf_script_command_entry *entry = commands [i];
			
			text = entry->get_name ();
			pts = MilliSeconds_ToPts (entry->pts) - preroll_pts;
			
			if (entry->type_index + 1 <= command->command_type_count)
				type = command_types [entry->type_index];
			else
				type = "";
			
			markers->Append (new MediaMarker::Node (new MediaMarker (type, text, pts)));
			
			//printf ("MediaElement::ReadMarkers () Added script command at %llu (text: %s, type: %s)\n", pts, text, type);
			
			g_free (text);
		}
	}
	
	// Read the MARKERs
	asf_marker *asf_marker;
	const asf_marker_entry* marker_entry;
	
	asf_marker = parser->marker;
	if (asf_marker != NULL) {
		for (i = 0; i < (int) asf_marker->marker_count; i++) {
			marker_entry = asf_marker->get_entry (i);
			text = marker_entry->get_marker_description ();
			
			pts = marker_entry->pts - preroll_pts;
			
			markers->Append (new MediaMarker::Node (new MediaMarker ("Name", text, pts)));
			
			//printf ("MediaElement::ReadMarkers () Added marker at %llu (text: %s, type: %s)\n", pts, text, "Name");
		
			g_free (text);
		}
	}
	
		
cleanup:
	
	g_strfreev (command_types);
	g_free (commands);
}

void
ASFDemuxer::SetParser (ASFParser *parser)
{
	this->parser = parser;
}

MediaResult
ASFDemuxer::ReadHeader ()
{
	MediaResult result = MEDIA_SUCCESS;
	ASFParser *asf_parser = NULL;
	gint32 *stream_to_asf_index = NULL;
	IMediaStream **streams = NULL;
	int current_stream = 1;
	int stream_count = 0;
	
	if (parser != NULL) {
		asf_parser = parser;
	} else {
		asf_parser = new ASFParser (source, media);
	}

	//printf ("ASFDemuxer::ReadHeader ().\n");
	
	if (!asf_parser->ReadHeader ()) {
		result = MEDIA_INVALID_MEDIA;
		GetMedia ()->AddMessage (MEDIA_INVALID_MEDIA, "asf_parser->ReadHeader () failed:");
		GetMedia ()->AddMessage (MEDIA_FAIL, asf_parser->GetLastErrorStr ());
		goto failure;
	}
	
	// Count the number of streams
	stream_count = 0;
	for (int i = 1; i <= 127; i++) {
		if (asf_parser->IsValidStream (i))
			stream_count++;
	}
	
	current_stream = 1;
	streams = (IMediaStream **) g_malloc0 (sizeof (IMediaStream *) * (stream_count + 1)); // End with a NULL element.
	stream_to_asf_index = (gint32 *) g_malloc0 (sizeof (gint32) * (stream_count + 1)); 

	// Loop through all the streams and set stream-specific data	
	for (int i = 0; i < stream_count; i++) {
		while (current_stream <= 127 && !asf_parser->IsValidStream (current_stream))
			current_stream++;
		
		if (current_stream > 127) {
			result = MEDIA_INVALID_STREAM;
			GetMedia ()->AddMessage (result, "Couldn't find all the claimed streams in the file.");
			goto failure;
		}
		
		const asf_stream_properties* stream_properties = asf_parser->GetStream (current_stream);
		IMediaStream* stream = NULL;
		
		if (stream_properties == NULL) {
			result = MEDIA_INVALID_STREAM;
			GetMedia ()->AddMessage (result, "Couldn't find all the claimed streams in the file.");
			goto failure;
		}
		
		if (stream_properties->is_audio ()) {
			AudioStream* audio = new AudioStream (GetMedia ());

			stream = audio;
			
			const WAVEFORMATEX* wave = stream_properties->get_audio_data ();
			const WAVEFORMATEXTENSIBLE* wave_ex = wave->get_wave_format_extensible ();
			int data_size = stream_properties->size - sizeof (asf_stream_properties) - sizeof (WAVEFORMATEX);
			
			audio->channels = wave->channels;
			audio->sample_rate = wave->samples_per_second;
			audio->bit_rate = wave->bytes_per_second * 8;
			audio->block_align = wave->block_alignment;
			audio->bits_per_sample = wave->bits_per_sample;
			audio->extra_data = NULL;
			audio->extra_data_size = data_size > wave->codec_specific_data_size ? wave->codec_specific_data_size : data_size;
			audio->codec_id = wave->codec_id;
			
			if (wave_ex != NULL) {
				audio->bits_per_sample = wave_ex->Samples.valid_bits_per_sample;
				audio->extra_data_size -= (sizeof (WAVEFORMATEXTENSIBLE) - sizeof (WAVEFORMATEX));
				audio->codec_id = *((guint32*) &wave_ex->sub_format);
			}

			// Fill in any extra codec data
			if (audio->extra_data_size > 0) {
				audio->extra_data = g_malloc0 (audio->extra_data_size);
				char* src = ((char*) wave) + (wave_ex ? sizeof (WAVEFORMATEX) : sizeof (WAVEFORMATEX));
				memcpy (audio->extra_data, src, audio->extra_data_size);
			}
		} else if (stream_properties->is_video ()) {
			VideoStream* video = new VideoStream (GetMedia ());
			stream = video;
			
			const asf_video_stream_data* video_data = stream_properties->get_video_data ();
			const BITMAPINFOHEADER* bmp;
			const asf_extended_stream_properties* aesp;

			if (video_data != NULL) {
				bmp = video_data->get_bitmap_info_header ();
				aesp = asf_parser->GetExtendedStream (current_stream);
				if (bmp != NULL) {
					video->width = bmp->image_width;
					video->height = bmp->image_height;
					video->bits_per_sample = bmp->bits_per_pixel;
					video->codec_id = bmp->compression_id;
					video->extra_data_size = bmp->get_extra_data_size ();
					if (video->extra_data_size > 0) {
						video->extra_data = g_malloc0 (video->extra_data_size);
						memcpy (video->extra_data, bmp->get_extra_data (), video->extra_data_size);
					} else {
						video->extra_data = NULL;
					}
				}
				if (aesp != NULL) {
					video->bit_rate = aesp->data_bitrate;
				} else {
					video->bit_rate = video->width*video->height;
				} 
			}
		} else if (stream_properties->is_command ()) {
			MarkerStream* marker = new MarkerStream (GetMedia ());
			stream = marker;
			stream->codec = g_strdup ("asf-marker");
		} else {
			// Unknown stream, ignore it.
		}
		
		if (stream != NULL) {
			if (stream_properties->is_video () || stream_properties->is_audio ()) {
				switch (stream->codec_id) {
				case CODEC_WMV1: stream->codec = g_strdup ("wmv1"); break;
				case CODEC_WMV2: stream->codec = g_strdup ("wmv2"); break;
				case CODEC_WMV3: stream->codec = g_strdup ("wmv3"); break;
				case CODEC_WMVA: stream->codec = g_strdup ("wmva"); break;
				case CODEC_WVC1: stream->codec = g_strdup ("vc1"); break;
				case CODEC_MP3: stream->codec = g_strdup ("mp3"); break;
				case CODEC_WMAV1: stream->codec = g_strdup ("wmav1"); break;
				case CODEC_WMAV2: stream->codec = g_strdup ("wmav2"); break;
				default:
					char a = ((stream->codec_id & 0x000000FF));
					char b = ((stream->codec_id & 0x0000FF00) >> 8);
					char c = ((stream->codec_id & 0x00FF0000) >> 16);
					char d = ((stream->codec_id & 0xFF000000) >> 24);
					stream->codec = g_strdup_printf ("unknown (%c%c%c%c)", a ? a : ' ', b ? b : ' ', c ? c : ' ', d ? d : ' ');
					break;
				}
			}
			streams [i] = stream;
			stream->index = i;			
			if (!asf_parser->file_properties->is_broadcast ()) {
				stream->duration = asf_parser->file_properties->play_duration - MilliSeconds_ToPts (asf_parser->file_properties->preroll);
			}
			stream_to_asf_index [i] = current_stream;
		}
		
		current_stream++;
	}
	
	
	if (!MEDIA_SUCCEEDED (result)) {
		goto failure;
	}
	
	SetStreams (streams, stream_count);
	this->stream_to_asf_index = stream_to_asf_index;
	this->parser = asf_parser;
	
	reader = new ASFReader (parser, this);
			
	ReadMarkers ();
	
	return result;
	
failure:
	asf_parser->unref ();
	asf_parser = NULL;
	
	g_free (stream_to_asf_index);
	stream_to_asf_index = NULL;
	
	if (streams != NULL) {
		for (int i = 0; i < stream_count; i++) {
			if (streams [i] != NULL) {
				streams [i]->unref ();
				streams [i] = NULL;
			}
		}
		g_free (streams);
		streams = NULL;
	}
	
	return result;
}

MediaResult
ASFDemuxer::ReadFrame (MediaFrame *frame)
{
	//printf ("ASFDemuxer::ReadFrame (%p).\n", frame);
	ASFFrameReader *reader = this->reader->GetFrameReader (stream_to_asf_index [frame->stream->index]);
	MediaResult result;
	
	result = reader->Advance ();
	if (result == MEDIA_NO_MORE_DATA) {
		//media->AddMessage (MEDIA_NO_MORE_DATA, "Reached end of data.");
		frame->event = FrameEventEOF;
		return MEDIA_NO_MORE_DATA;
	}
	
	if (!MEDIA_SUCCEEDED (result)) {
		media->AddMessage (MEDIA_DEMUXER_ERROR, g_strdup_printf ("Error while advancing to the next frame (%i)", result));
		return result;
	}
	
	frame->pts = reader->Pts ();
	//frame->duration = reader->Duration ();
	if (reader->IsKeyFrame ())
		frame->AddState (FRAME_KEYFRAME);
	frame->buflen = reader->Size ();
	frame->buffer = (guint8 *) g_try_malloc (frame->buflen + frame->stream->min_padding);
	
	if (frame->buffer == NULL) {
		media->AddMessage (MEDIA_OUT_OF_MEMORY, "Could not allocate memory for next frame.");
		return MEDIA_OUT_OF_MEMORY;
	}
	
	//printf ("ASFDemuxer::ReadFrame (%p), min_padding = %i\n", frame, frame->stream->min_padding);
	if (frame->stream->min_padding > 0)
		memset (frame->buffer + frame->buflen, 0, frame->stream->min_padding); 
	
	if (!reader->Write (frame->buffer)) {
		media->AddMessage (MEDIA_DEMUXER_ERROR, "Error while copying the next frame.");
		return MEDIA_DEMUXER_ERROR;
	}
	
	frame->AddState (FRAME_DEMUXED);
	
	return MEDIA_SUCCESS;
}

/*
 * ASFMarkerDecoder
 */

MediaResult 
ASFMarkerDecoder::DecodeFrame (MediaFrame *frame)
{
	LOG_PIPELINE ("ASFMarkerDecoder::DecodeFrame ()\n");
	
	MediaResult result;
	char *text;
	char *type;
	gunichar2 *data;
	gunichar2 *uni_type = NULL;
	gunichar2 *uni_text = NULL;
	int text_length = 0;
	int type_length = 0;
	guint32 size = 0;
	
	if (frame->buflen % 2 != 0 || frame->buflen == 0 || frame->buffer == NULL)
		return MEDIA_CORRUPTED_MEDIA;

	data = (gunichar2 *) frame->buffer;
	uni_type = data;
	size = frame->buflen;
	
	// the data is two arrays of WCHARs (type and text), null terminated.
	// loop through the data, counting characters and null characters
	// there should be at least two null characters.
	int null_count = 0;
	
	for (guint32 i = 0; i < (size / sizeof (gunichar2)); i++) {
		if (uni_text == NULL) {
			type_length++;
		} else {
			text_length++;
		}
		if (*(data + i) == 0) {
			null_count++;
			if (uni_text == NULL) {
				uni_text = data + i + 1;
			} else {
				break; // Found at least two nulls
			}
		}
	}
	
	if (null_count >= 2) {
		text = wchar_to_utf8 (uni_text, text_length);
		type = wchar_to_utf8 (uni_type, type_length);
		
		LOG_PIPELINE ("ASFMarkerDecoder::DecodeFrame (): sending script command type: '%s', text: '%s', pts: '%llu'.\n", type, text, frame->pts);

		frame->buffer = (guint8 *) new MediaMarker (type, text, frame->pts);
		frame->buflen = sizeof (MediaMarker);
		
		g_free (text);
		g_free (type);
		result = MEDIA_SUCCESS;
	} else {
		LOG_PIPELINE ("ASFMarkerDecoder::DecodeFrame (): didn't find 2 null characters in the data.\n");
		result = MEDIA_CORRUPTED_MEDIA;
	}
		
	return result;
}


/*
 * ASFDemuxerInfo
 */

bool
ASFDemuxerInfo::Supports (IMediaSource *source)
{
	guint8 buffer[16];
	
#if DEBUG
	if (!source->GetPosition () == 0) {
		fprintf (stderr, "ASFDemuxerInfo::Supports (%p): Trying to check if a media is supported, but the media isn't at position 0 (it's at position %lld)\n", source, source->GetPosition ());
	}
#endif

	if (!source->Peek (buffer, 16))
		return false;
	
	bool result = asf_guid_compare (&asf_guids_header, (asf_guid *) buffer);
	
	//printf ("ASFDemuxerInfo::Supports (%p): probing result: %s\n", source,
	//	result ? "true" : "false");
	
	return result;
}

IMediaDemuxer *
ASFDemuxerInfo::Create (Media *media, IMediaSource *source)
{
	return new ASFDemuxer (media, source);
}


/*
 * ASXDemuxer
 */

ASXDemuxer::ASXDemuxer (Media *media, IMediaSource *source) : IMediaDemuxer (media, source)
{
	playlist = NULL;
}

ASXDemuxer::~ASXDemuxer ()
{
	if (playlist)
		playlist->unref ();
}

MediaResult
ASXDemuxer::ReadHeader ()
{
	MediaResult result;

	PlaylistParser *parser = new PlaylistParser (media->GetElement (), source);

	if (parser->Parse ()) {
		result = MEDIA_SUCCESS;
		playlist = parser->GetPlaylist ();
		playlist->ref ();
	} else {
		result = MEDIA_FAIL;
	}

	delete parser;

	return result;
}

/*
 * ASXDemuxerInfo
 */

bool
ASXDemuxerInfo::Supports (IMediaSource *source)
{
	char buffer[4];
	
	if (!source->Peek ((guint8 *) buffer, 4))
		return false;
	
	return !g_ascii_strncasecmp (buffer, "<asx", 4) ||
		!g_ascii_strncasecmp (buffer, "[Ref", 4);
}

IMediaDemuxer *
ASXDemuxerInfo::Create (Media *media, IMediaSource *source)
{
	return new ASXDemuxer (media, source);
}

/*
 * ManagedStreamSource
 */

#if SL_2_0

ManagedStreamSource::ManagedStreamSource (Media *media, ManagedStreamCallbacks *stream) : IMediaSource (media)
{
	memcpy (&this->stream, stream, sizeof (this->stream));
}

ManagedStreamSource::~ManagedStreamSource ()
{
	stream.handle = NULL;
}

gint32 
ManagedStreamSource::ReadInternal (void *buf, guint32 n)
{
	return stream.Read (stream.handle, buf, 0, n);
}

gint32 
ManagedStreamSource::PeekInternal (void *buf, guint32 n, gint64 start)
{
	int read;
	gint64 position;
	
	if (start != -1) {
		position = stream.Position (stream.handle);
		read = stream.Read (stream.handle, buf, 0, n);
		stream.Seek (stream.handle, position, 0 /* SeekOrigin.Begin */);	
		return read;
	} else {
		read = stream.Read (stream.handle, buf, 0, n);
		stream.Seek (stream.handle, -read, 1 /* SeekOrigin.Current */);
		return read;
	}
}

bool 
ManagedStreamSource::SeekInternal (gint64 offset, int mode)
{
	stream.Seek (stream.handle, offset, mode /* FIXME: check if mode values matches SeekOrigin values */);
	return true;
}

gint64
ManagedStreamSource::GetPositionInternal ()
{
	return stream.Position (stream.handle);
}

gint64 
ManagedStreamSource::GetSizeInternal ()
{
	return stream.Length (stream.handle);
}
	
#endif

/*
 * FileSource
 */

FileSource::FileSource (Media *media) : IMediaSource (media)
{
	filename = g_strdup (media->GetFileOrUrl ());
	bufptr = buffer;
	buflen = 0;
	pos = -1;
	fd = -1;
	
	eof = true;
}

FileSource::FileSource (Media *media, const char *filename) : IMediaSource (media)
{
	this->filename = g_strdup (filename);
	bufptr = buffer;
	buflen = 0;
	pos = -1;
	fd = -1;
	
	eof = true;
}

FileSource::~FileSource ()
{
	g_free (filename);
	if (fd != -1)
		close (fd);
}

MediaResult 
FileSource::Initialize ()
{
	if (fd != -1)
		return MEDIA_SUCCESS;
	
	if (filename == NULL)
		return MEDIA_FILE_ERROR;
	
	if ((fd = open (filename, O_LARGEFILE | O_RDONLY)) == -1)
		return MEDIA_FILE_ERROR;
	
	eof = false;
	pos = 0;
	
	return MEDIA_SUCCESS;
}

gint64
FileSource::GetSizeInternal ()
{
	struct stat st;
	
	if (fd == -1 || fstat (fd, &st) == -1)
		return -1;
	
	return st.st_size;
}

gint64
FileSource::GetPositionInternal ()
{
	return pos;
}

bool
FileSource::SeekInternal (gint64 offset, int mode)
{
	gint64 n;
	
	if (fd == -1)
		return false;
	
	switch (mode) {
	case SEEK_CUR:
		if (offset == 0)
			return true;
		
		/* convert to SEEK_SET */
		if ((offset = pos + offset) < 0)
			return false;
		
		/* fall thru... */
	case SEEK_SET:
		if (pos == offset)
			return true;
		
		if (offset < 0)
			return false;
		
		n = (offset - pos);
		if ((n >= 0 && n <= buflen) || (n < 0 && (-n) <= (bufptr - buffer))) {
			/* the position is within our pre-buffered data */
			pos = offset;
			bufptr += n;
			buflen -= n;
			
			return true;
		}
		
		/* we are now forced to do an actual seek */
		if (lseek (fd, offset, SEEK_SET) == -1)
			return false;
		
		pos = offset;
		eof = false;
		
		bufptr = buffer;
		buflen = 0;
		
		return true;
	case SEEK_END:
		/* I doubt this code path ever gets hit, so we'll just
		 * do things the easy way... to hell with any
		 * optimizations. */
		if (lseek (fd, offset, SEEK_END) == -1)
			return false;
		
		pos = offset;
		eof = false;
		
		bufptr = buffer;
		buflen = 0;
		
		return true;
	default:
		// invalid 'whence' argument
		return false;
	}
}

/* non-interruptable read() */
static ssize_t
noint_read (int fd, char *buf, size_t n)
{
	ssize_t nread;
	
	do {
		nread = read (fd, buf, n);
	} while (nread == -1 && errno == EINTR);
	
	return nread;
}

gint32
FileSource::ReadInternal (void *buf, guint32 n)
{
	ssize_t r, nread = 0;
	
	if (fd == -1) {
		errno = EINVAL;
		LOG_PIPELINE_ERROR ("FileSource::ReadInternal (%p, %u): File not open.\n", buf, n);
		return -1;
	}
	
	while (n > 0) {
		if ((r = MIN (buflen, n)) > 0) {
			/* copy what we can from our existing buffer */
			memcpy (((char *) buf) + nread, bufptr, r);
			bufptr += r;
			buflen -= r;
			nread += r;
			pos += r;
			n -= r;
		}
		
		if (n >= sizeof (buffer)) {
			/* bypass intermediate buffer */
			bufptr = buffer;
			if ((r = noint_read (fd, ((char *) buf) + nread, n)) > 0) {
				nread += r;
				pos += r;
				n -= r;
			}
		} else if (n > 0) {
			/* buffer more data */
			if ((r = noint_read (fd, buffer, sizeof (buffer))) > 0)
				buflen = (guint32) r;
			
			bufptr = buffer;
		}
		
		if (r == -1 && nread == 0) {
			LOG_PIPELINE_ERROR ("FileSource<%d>::ReadInternal ('%s', %p, %u): Some error occured during reading, r: %d, nread: %d, errno: %d = %s.\n",
					    GET_OBJ_ID (this), filename, buf, n, r, nread, errno, strerror (errno));
			return -1;
		}
		
		if (r == 0) {
			LOG_PIPELINE ("FileSource<%d>::ReadInternal ('%s', %p, %u): Could not read all the data, eof reached. Current position: %lld\n",
					    GET_OBJ_ID (this), filename, buf, n, GetPositionInternal ());
			eof = true;
			break;
		}
	}
	
	return nread;
}

gint32
FileSource::PeekInternal (void *buf, guint32 n, gint64 start)
{
	gint32 result;
	gint64 current_pos;
	gint64 initial_pos;

	if (start == -1) {
		initial_pos = GetPosition ();
	} else {
		initial_pos = start;		
	}

	current_pos = GetPosition ();
	
	// First try to peek in the buffer.
	if (initial_pos == current_pos && PeekInBuffer (buf, n))
		return n;
	
	// We could not peek in the buffer, use a very simple fallback algorithm.
	LOG_PIPELINE ("FileSource<%i>::PeekInternal (%p, %i, %lld), initial_pos: %lld, current_pos: %lld, GetPosition (): %lld\n", GET_OBJ_ID (this), buf, n, start, initial_pos, current_pos, GetPosition ());

	if (initial_pos != current_pos)
		Seek (initial_pos, SEEK_SET);

	result = ReadSome (buf, n);
	
	Seek (current_pos, SEEK_SET);

	LOG_PIPELINE ("FileSource<%i>::PeekInternal (%p, %i, %lld), initial_pos: %lld, current_pos: %lld, GetPosition (): %lld [Done]\n", GET_OBJ_ID (this), buf, n, start, initial_pos, current_pos, GetPosition ());

	return result;
}

bool
FileSource::PeekInBuffer (void *buf, guint32 n)
{
	guint32 need, used, avail, shift;
	ssize_t r;
	
	if (fd == -1)
		return false;
	
	if (n > sizeof (buffer)) {
		/* we can't handle a peek of this size */
		return false;
	}
	
	if (buflen < n) {
		/* need to buffer more data */
		if (bufptr > buffer) {
			used = (bufptr + buflen) - buffer;
			avail = sizeof (buffer) - used;
			need = n - buflen;
			
			if (avail < need) {
				/* make room for 'need' more bytes */
				shift = need - avail;
				memmove (buffer, buffer + shift, used - shift);
				bufptr -= shift;
			} else {
				/* request 'avail' more bytes so we
				 * can hopefully fill our buffer */
				need = avail;
			}
		} else {
			/* nothing in our buffer, fill 'er up */
			need = sizeof (buffer);
		}
		
		do {
			if ((r = noint_read (fd, bufptr + buflen, need)) == 0) {
				eof = true;
				break;
			} else if (r == -1)
				break;
			
			buflen += r;
			need -= r;
		} while (buflen < n);
	}
	
	if (buflen < n) {
		/* can't peek as much data as requested... */
		return false;
	}
	
	memcpy (((char *) buf), bufptr, n);
	
	return true;
}

/*
 * ProgressiveSource
 */

ProgressiveSource::ProgressiveSource (Media *media) : FileSource (media, NULL)
{
	write_pos = 0;
	wait_pos = 0;
	size = -1;
}

ProgressiveSource::~ProgressiveSource ()
{
}

MediaResult
ProgressiveSource::Initialize ()
{
	if (filename != NULL)
		return MEDIA_SUCCESS;
	
	filename = g_build_filename (g_get_tmp_dir (), "MoonlightProgressiveStream.XXXXXX", NULL);
	if ((fd = g_mkstemp (filename)) == -1) {
		g_free (filename);
		filename = NULL;
		
		return MEDIA_FAIL;
	}
	
	// unlink the file right away so that it'll be deleted even if we crash.
	if (moonlight_flags & RUNTIME_INIT_KEEP_MEDIA) {
		printf ("Moonlight: The media file %s will not deleted.\n", filename);
	} else {
		unlink (filename);
	}

	wait_pos = 0;
	eof = false;
	pos = 0;
	
	return MEDIA_SUCCESS;
}

static ssize_t
write_all (int fd, char *buf, size_t len)
{
	size_t nwritten = 0;
	ssize_t n;
	
	do {
		do {
			n = write (fd, buf + nwritten, len - nwritten);
		} while (n == -1 && (errno == EINTR || errno == EAGAIN));
		
		if (n > 0)
			nwritten += n;
	} while (n != -1 && nwritten < len);
	
	if (n == -1)
		return -1;
	
	return nwritten;
}

void
ProgressiveSource::Write (void *buf, gint64 offset, gint32 n)
{
	ssize_t nwritten;
	bool new_pos = false;

	// printf ("ProgressiveSource::Write (%p, %lld, %i)\n", buf, offset, n);
	if (fd == -1) {
		media->AddMessage (MEDIA_FAIL, "Progressive source doesn't have a file to write the data to.");
		return;
	}
	
	Lock ();

	if (n == 0) {
		// We've got the entire file, update the size
		size = write_pos;
		goto cleanup;
	}

	 if (write_pos == -1)
		new_pos = true;

	// Seek to the write position
	if (lseek (fd, offset, SEEK_SET) != offset)
		goto cleanup;
	
	if ((nwritten = write_all (fd, (char *) buf, n)) > 0)
		write_pos = offset + nwritten;

	
	if (new_pos) {
		// Set pos to the new write position
		pos = offset;
		lseek (fd, offset, SEEK_SET);
	} else {
		// Restore the current position
		lseek (fd, pos + buflen, SEEK_SET);
	}
	
cleanup:
	if (IsWaiting ())
		Signal ();
	
	Unlock ();
}

bool
ProgressiveSource::SeekInternal (gint64 offset, int mode)
{
	return FileSource::SeekInternal (offset, mode);
}

void
ProgressiveSource::NotifySize (gint64 size)
{
	Lock ();
	this->size = size;
	Unlock ();
}

void
ProgressiveSource::NotifyFinished ()
{
	Lock ();
	this->size = write_pos;
	Unlock ();
	Signal ();
}

/*
 * MemoryQueueSource::QueueNode
 */

MemoryQueueSource::QueueNode::QueueNode (MemorySource *source)
{
	if (source)
		source->ref ();
	this->source = source;
	packet = NULL;
}

MemoryQueueSource::QueueNode::QueueNode (ASFPacket *packet)
{
	if (packet)
		packet->ref ();
	this->packet = packet;
	source = NULL;
}

MemoryQueueSource::QueueNode::~QueueNode ()
{
	if (packet)
		packet->unref ();
	if (source)
		source->unref ();
}

/*
 * MemoryNestedSource
 */

MemoryNestedSource::MemoryNestedSource (MemorySource *src) : MemorySource (src->GetMedia (), src->GetMemory (), src->GetSize (), src->GetStart ())
{
	src->ref ();
	this->src = src;
}

MemoryNestedSource::~MemoryNestedSource ()
{
	src->unref ();
}

/*
 * MemoryQueueSource
 */
 
MemorySource::MemorySource (Media *media, void *memory, gint32 size, gint64 start)
	: IMediaSource (media)
{
	this->memory = memory;
	this->size = size;
	this->start = start;
	this->pos = 0;
	this->owner = true;
}

MemorySource::~MemorySource ()
{
	if (owner)
		g_free (memory);
}

bool
MemorySource::SeekInternal (gint64 offset, int mode)
{
	gint64 real_offset;

	switch (mode) {
	case SEEK_SET:
		real_offset = offset - start;
		if (real_offset < 0 || real_offset >= size)
			return false;
		pos = real_offset;
		return true;
	case SEEK_CUR:
		if (pos + offset > size || pos + offset < 0)
			return false;
		pos += offset;
		return true;
	case SEEK_END:
		if (size - offset > size || size - offset < 0)
			return false;
		pos = size - offset;
		return true;
	default:
		return false;
	}
	return true;
}

gint32 
MemorySource::ReadInternal (void *buffer, guint32 n)
{
	guint32 k = MIN (n, size - pos);
	memcpy (buffer, ((char*) memory) + pos, k);
	pos += k;
	return k;
}

gint32
MemorySource::PeekInternal (void *buffer, guint32 n, gint64 start)
{
	if (start == -1)
		start = this->start + pos;

	if (this->start > start)
		return 0;

	if ((this->start + size) < (start + n))
		return 0;

	memcpy (buffer, ((char*) memory) + this->start - start, n);
	return n;
}

/*
 * MemoryQueueSource
 */
 
MemoryQueueSource::MemoryQueueSource (Media *media)
	: IMediaSource (media)
{
	finished = false;
	requested_pts = UINT64_MAX;
	last_requested_pts = UINT64_MAX;
	write_count = 0;
	parser = NULL;
}

MemoryQueueSource::~MemoryQueueSource ()
{
	if (this->parser)
		this->parser->unref ();
}

ASFParser *
MemoryQueueSource::GetParser ()
{
	return parser;
}

void
MemoryQueueSource::SetParser (ASFParser *parser)
{
	if (this->parser)
		this->parser->unref ();
	this->parser = parser;
	if (this->parser)
		this->parser->ref ();
}

void
MemoryQueueSource::WaitForQueue ()
{
	if (finished)
		return;

	Lock ();
	StartWaitLoop ();
	while (!finished && !Aborted () && queue.IsEmpty ()) {
		Wait ();
	}
	EndWaitLoop ();
	Unlock ();
}


gint64
MemoryQueueSource::GetPositionInternal ()
{
	g_warning ("MemoryQueueSource::GetPositionInternal (): You hit a bug in moonlight, please attach gdb, get a stack trace and file bug.");

	return -1;
}

Queue*
MemoryQueueSource::GetQueue ()
{
	QueueNode *node;
	QueueNode *next;
	
	// Make sure all nodes have asf packets.
	queue.Lock ();
	node = (QueueNode *) queue.LinkedList ()->First ();
	while (node != NULL && node->packet == NULL) {
		next = (QueueNode *) node->next;
		
		node->packet = new ASFPacket (parser, node->source);
		if (!MEDIA_SUCCEEDED (node->packet->Read ())) {
			LOG_PIPELINE_ERROR ("MemoryQueueSource::GetQueue (): Error while parsing packet, dropping packet.\n");
			queue.LinkedList ()->Remove (node);
		}
		
		node = next;
	}
	queue.Unlock ();
	
	return &queue;
}

ASFPacket *
MemoryQueueSource::Pop ()
{
	//printf ("MemoryQueueSource::Pop (), there are %i packets in the queue, of a total of %lld packets written.\n", queue.Length (), write_count);
	
	QueueNode *node;
	ASFPacket *result = NULL;
	
trynext:
	node = (QueueNode *) queue.Pop ();
	
	if (node == NULL) {
		WaitForQueue ();
		
		node = (QueueNode *) queue.Pop ();
	}
	
	if (node == NULL) {
		LOG_PIPELINE ("MemoryQueueSource::Pop (): No more packets.\n");
		return NULL; // We waited and got nothing, reached end of stream
	}
	
	if (node->packet == NULL) {
		if (parser == NULL) {
			g_warning ("MemoryQueueSource::Pop (): No parser to parse the packet.\n");
			goto cleanup;
		}
		node->packet = new ASFPacket (parser, node->source);
		if (!MEDIA_SUCCEEDED (node->packet->Read ())) {
			LOG_PIPELINE_ERROR ("MemoryQueueSource::Pop (): Error while parsing packet, getting a new packet\n");
			delete node;
			goto trynext;
		}
	}
	
	result = node->packet;
	result->ref ();

cleanup:				
	delete node;
	
	LOG_PIPELINE ("MemoryQueueSource::Pop (): popped 1 packet, there are %i packets left, of a total of %lld packets written\n", queue.Length (), write_count);
	
	return result;
}

void
MemoryQueueSource::Write (void *buf, gint64 offset, gint32 n)
{
	MemorySource *src;
	ASFPacket *packet;
	
	//printf ("MemoryQueueSource::Write (%p, %lld, %i), write_count: %lld\n", buf, offset, n, write_count + 1);
	
	write_count++;
	if (parser != NULL) {
		src = new MemorySource (NULL, buf, n, offset);
		src->SetOwner (false);
		packet = new ASFPacket (parser, src);
		if (!MEDIA_SUCCEEDED (packet->Read ())) {
			LOG_PIPELINE_ERROR ("MemoryQueueSource::Write (%p, %lld, %i): Error while parsing packet, dropping packet.\n", buf, offset, n);
		} else {
			queue.Push (new QueueNode (packet));
		}
		packet->unref ();
		src->unref ();
	} else {
		src = new MemorySource (NULL, g_memdup (buf, n), n, offset);
		queue.Push (new QueueNode (src));
		src->unref ();
	}
	
	if (IsWaiting ())
		Signal ();
}

bool
MemoryQueueSource::SeekInternal (gint64 offset, int mode)
{
	g_warning ("MemoryQueueSource::SeekInternal (%lld, %i): You hit a bug in moonlight, please attach gdb, get a stack trace and file bug.", offset, mode);
	
	return false;
}

gint32 
MemoryQueueSource::ReadInternal (void *buffer, guint32 n)
{
	g_warning ("MemoryQueueSource::ReadInternal (%p, %u): You hit a bug in moonlight, please attach gdb, get a stack trace and file bug.", buffer, n);
	
	return 0;
}

gint32
MemoryQueueSource::PeekInternal (void *buffer, guint32 n, gint64 start)
{
	g_warning ("MemoryQueueSource::PeekInternal (%p, %u, %lld): You hit a bug in moonlight, please attach gdb, get a stack trace and file bug.", buffer, n, start);
	
	return 0;
}

gint64
MemoryQueueSource::GetLastAvailablePositionInternal ()
{
	g_warning ("MemoryQueueSource::GetLastAvailablePositionInternal (): You hit a bug in moonlight, please attach gdb, get a stack trace and file bug.");
	
	return 0;
}

void
MemoryQueueSource::NotifySize (gint64 size)
{
	// We don't care.
}

void
MemoryQueueSource::NotifyFinished ()
{
	Lock ();
	this->finished = true;
	Unlock ();
	Signal ();
}

gint64
MemoryQueueSource::GetSizeInternal ()
{
	g_warning ("MemoryQueueSource::GetSizeInternal (): You hit a bug in moonlight, please attach gdb, get a stack trace and file bug.");
	
	return 0;
}

void
MemoryQueueSource::RequestPosition (gint64 *pos)
{
	Lock ();
	if (requested_pts != UINT64_MAX && requested_pts != last_requested_pts) {
		*pos = requested_pts;
		last_requested_pts = requested_pts;
		requested_pts = UINT64_MAX;
		Signal ();
	}
	Unlock ();
}

bool
MemoryQueueSource::SeekToPts (guint64 pts)
{
	LOG_PIPELINE ("MemoryQueueSource::SeekToPts (%llu)\n", pts);

	if (last_requested_pts == pts)
		return true;

	Lock ();
	queue.Clear (true);
	requested_pts = pts;
	finished = false;

	StartWaitLoop ();
	while (!Aborted () && requested_pts != UINT64_MAX)
		Wait ();
	EndWaitLoop ();

	Unlock ();

	LOG_PIPELINE ("MemoryQueueSource::SeekToPts (%llu) [Done]\n", pts);
	
	return !Aborted ();
}


/*
 * MediaClosure
 */ 

MediaClosure::MediaClosure (MediaCallback *cb)
{
	callback = cb;
	frame = NULL;
	media = NULL;
	context = NULL;
	result = 0;
	context_refcounted = true;
}

MediaClosure::~MediaClosure ()
{
	delete frame;

	if (context_refcounted && context)
		context->unref ();
	if (media)
		media->unref ();
}

MediaResult
MediaClosure::Call ()
{
	if (callback)
		return callback (this);
		
	return MEDIA_NO_CALLBACK;
}

void
MediaClosure::SetMedia (Media *media)
{
	if (this->media)
		this->media->unref ();
	this->media = media;
	if (this->media)
		this->media->ref ();
}

Media *
MediaClosure::GetMedia ()
{
	return media;
}

void
MediaClosure::SetContextUnsafe (EventObject *context)
{
	if (this->context && context_refcounted)
		this->context->unref ();
		
	this->context = context;
	context_refcounted = false;
}

void
MediaClosure::SetContext (EventObject *context)
{
	if (this->context && context_refcounted)
		this->context->unref ();
	this->context = context;
	if (this->context)
		this->context->ref ();
	context_refcounted = true;
}

EventObject *
MediaClosure::GetContext ()
{
	return context;
}

MediaClosure *
MediaClosure::Clone ()
{
	MediaClosure *closure = new MediaClosure (callback);
	closure->SetContext (context);
	closure->SetMedia (media);
	closure->frame = frame;
	closure->marker = marker;
	closure->result = result;
	return closure;
}

/*
 * IMediaStream
 */

IMediaStream::IMediaStream (Media *media) : IMediaObject (media)
{
	context = NULL;
	
	extra_data_size = 0;
	extra_data = NULL;
	
	msec_per_frame = 0;
	duration = 0;
	
	decoder = NULL;
	codec_id = 0;
	codec = NULL;
	
	min_padding = 0;
	index = -1;
	selected = false;
}

IMediaStream::~IMediaStream ()
{
	if (decoder)
		decoder->unref ();
	
	g_free (extra_data);
	g_free (codec);
}

void
IMediaStream::SetSelected (bool value)
{
	selected = value;
	if (media && media->GetDemuxer ())
		media->GetDemuxer ()->UpdateSelected (this);
}

/*
 * IMediaDemuxer
 */ 
 
IMediaDemuxer::IMediaDemuxer (Media *media, IMediaSource *source) : IMediaObject (media)
{
	this->source = source;
	this->source->ref ();
	stream_count = 0;
	streams = NULL;
}

IMediaDemuxer::~IMediaDemuxer ()
{
	if (streams != NULL) {
		for (int i = 0; i < stream_count; i++)
			streams[i]->unref ();
		
		g_free (streams);
	}
	source->unref ();
}

guint64
IMediaDemuxer::GetDuration ()
{
	guint64 result = 0;
	for (int i = 0; i < GetStreamCount (); i++)
		result = MAX (result, GetStream (i)->duration);
	return result;
}

IMediaStream*
IMediaDemuxer::GetStream (int index)
{
	return (index < 0 || index >= stream_count) ? NULL : streams [index];
}

/*
 * MediaFrame
 */ 
 
MediaFrame::MediaFrame (IMediaStream *stream)
{
	decoder_specific_data = NULL;
	this->stream = stream;
	
	duration = 0;
	pts = 0;
	
	buffer = NULL;
	buflen = 0;
	state = 0;
	event = 0;
	
	for (int i = 0; i < 4; i++) {
		data_stride[i] = 0;  
		srcStride[i] = 0;
	}
	
	srcSlideY = 0;
	srcSlideH = 0;
}
 
MediaFrame::~MediaFrame ()
{
	if (decoder_specific_data != NULL) {
		if (stream != NULL && stream->decoder != NULL)
			stream->decoder->Cleanup (this);
	}
	g_free (buffer);
}

/*
 * IMediaObject
 */
 
IMediaObject::IMediaObject (Media *media)
{
	this->media = media;
}

IMediaObject::~IMediaObject ()
{
}

/*
 * IMediaSource
 */

IMediaSource::IMediaSource (Media *media)
	: IMediaObject (media)
{
	pthread_mutexattr_t attribs;
	pthread_mutexattr_init (&attribs);
	pthread_mutexattr_settype (&attribs, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init (&mutex, &attribs);
	pthread_mutexattr_destroy (&attribs);

	pthread_cond_init (&condition, NULL);

	wait_count = 0;
	aborted = false;
}

IMediaSource::~IMediaSource ()
{
	pthread_mutex_destroy (&mutex);
	pthread_cond_destroy (&condition);	
}

void
IMediaSource::Lock ()
{
	pthread_mutex_lock (&mutex);
}

void
IMediaSource::Unlock ()
{
	pthread_mutex_unlock (&mutex);
}

void
IMediaSource::Signal ()
{
	pthread_cond_signal (&condition);
}

void
IMediaSource::Wait ()
{
	if (aborted)
		return;

	g_atomic_int_inc (&wait_count);
	pthread_cond_wait (&condition, &mutex);
	g_atomic_int_dec_and_test (&wait_count);
}

void
IMediaSource::StartWaitLoop ()
{
	g_atomic_int_inc (&wait_count);
}

void
IMediaSource::EndWaitLoop ()
{
	if (g_atomic_int_dec_and_test (&wait_count))
		aborted = false;
}

void
IMediaSource::Abort ()
{
	LOG_PIPELINE ("IMediaSource::Abort ()\n");
	//LOG_PIPELINE_ERROR_CONDITIONAL (GetLastAvailablePosition () != -1, "IMediaSource::Abort (): Any pending operations will now be aborted (probably due to a seek) and you may see a few reading errors. This is completely normal.\n")

	// There's no need to lock here, since aborted can only be set to true.
	aborted = true;
	while (IsWaiting ()) {
		Signal ();
	}
}

bool
IMediaSource::IsWaiting ()
{
	return g_atomic_int_get (&wait_count) != 0;
}

void
IMediaSource::WaitForPosition (bool block, gint64 position)
{
	LOG_PIPELINE ("IMediaSource<%i>::WaitForPosition (%i, %lld) last available pos: %lld, aborted: %i\n", GET_OBJ_ID (this), block, position, GetLastAvailablePositionInternal (), aborted);

	StartWaitLoop ();

	if (block && GetLastAvailablePositionInternal () != -1) {
		while (!aborted) {
			// Check if the entire file is available.
			if (GetSizeInternal () >= 0 && GetSizeInternal () <= GetLastAvailablePositionInternal () + 1) {
				LOG_PIPELINE ("IMediaSource<%i>::WaitForPosition (%i, %lld): GetSize (): %lld, GetLastAvailablePositionInternal (): %lld.\n", GET_OBJ_ID (this), block, position, GetSizeInternal (), GetLastAvailablePositionInternal ());	
				break;
			}

			// Check if we got the position we want
			if (GetLastAvailablePositionInternal () >= position) {
				LOG_PIPELINE ("IMediaSource<%i>::WaitForPosition (%i, %lld): GetLastAvailablePositionInternal (): %lld.\n", GET_OBJ_ID (this), block, position, GetLastAvailablePositionInternal ());	
				break;
			}
			
			Wait ();
		}
	}

	EndWaitLoop ();

	LOG_PIPELINE ("IMediaSource<%i>::WaitForPosition (%i, %lld): aborted: %i\n", GET_OBJ_ID (this), block, position, aborted);
}

gint32
IMediaSource::ReadSome (void *buf, guint32 n, bool block, gint64 start)
{
	gint32 result;

	LOG_PIPELINE ("IMediaSource<%i>::ReadSome (%p, %i, %s, %lld)\n", GET_OBJ_ID (this), buf, n, block ? "true" : "false", start);
	
	Lock ();
	
	if (start == -1)
		start = GetPositionInternal ();
	else if (start != GetPositionInternal () && !SeekInternal (start, SEEK_SET))
		return -1;
	
	WaitForPosition (block, start + n - 1);

	result = ReadInternal (buf, n);

	Unlock ();

	return result;
}

bool
IMediaSource::ReadAll (void *buf, guint32 n, bool block, gint64 start)
{
	gint32 read;
	
	LOG_PIPELINE ("IMediaSource<%d>::ReadAll (%p, %u, %s, %lld).\n",
		      GET_OBJ_ID (this), buf, n, block ? "true" : "false", start);
	
	read = ReadSome (buf, n, block, start);
	
	LOG_PIPELINE ("IMediaSource<%d>::ReadAll (%p, %u, %s, %lld), read: %d [Done].\n",
		      GET_OBJ_ID (this), buf, n, block ? "true" : "false", start, read);
	
	return (gint64) read == (gint64) n;
}

bool
IMediaSource::Peek (void *buf, guint32 n, bool block, gint64 start)
{
	bool result;

	Lock ();

	if (start == -1)
		start = GetPositionInternal ();

	WaitForPosition (block, start + n - 1);

	result = (gint64) PeekInternal (buf, n, start) == (gint64) n;

	Unlock ();

	return result;
}

bool
IMediaSource::Seek (gint64 offset, int mode)
{
	LOG_PIPELINE ("IMediaSource<%d> (%s)::Seek (%lld, %d = %s)\n",
		      GET_OBJ_ID (this), ToString (), offset, mode, mode == SEEK_SET ? "SEEK_SET"
		      : (mode == SEEK_CUR ? "SEEK_CUR" : (mode == SEEK_END ? "SEEK_END" : "<invalid value>")));
	
	bool result;
	Lock ();
	result = SeekInternal (offset, mode);
	Unlock ();
	return result;
}

gint64
IMediaSource::GetLastAvailablePosition ()
{
	gint64 result;
	Lock ();
	result = GetLastAvailablePositionInternal ();
	Unlock ();
	return result;
}

gint64
IMediaSource::GetPosition ()
{
	gint64 result;
	Lock ();
	result = GetPositionInternal ();
	Unlock ();
	return result;
}

gint64
IMediaSource::GetSize ()
{
	gint64 result;
	Lock ();
	result = GetSizeInternal ();
	Unlock ();
	return result;
}

/*
 * IMediaDemuxer
 */

void
IMediaDemuxer::SetStreams (IMediaStream** streams, int count)
{
	this->streams = streams;
	this->stream_count = count;
}

/*
 * IMediaDecoder
 */

IMediaDecoder::IMediaDecoder (Media *media, IMediaStream *stream) : IMediaObject (media)
{
	this->stream = stream;
}

/*
 * IImageConverter
 */

IImageConverter::IImageConverter (Media *media, VideoStream *stream) : IMediaObject (media)
{
	output_format = MoonPixelFormatNone;
	input_format = MoonPixelFormatNone;
	this->stream = stream;
}

/*
 * VideoStream
 */

VideoStream::VideoStream (Media *media) : IMediaStream (media)
{
	converter = NULL;
	bits_per_sample = 0;
	msec_per_frame = 0;
	initial_pts = 0;
	height = 0;
	width = 0;
}

VideoStream::~VideoStream ()
{
	if (converter)
		converter->unref ();
}

/*
 * MediaClosure
 */


/*
 * MediaMarker
 */ 

MediaMarker::MediaMarker (const char *type, const char *text, guint64 pts)
{
	this->type = g_strdup (type);
	this->text = g_strdup (text);
	this->pts = pts;
}

MediaMarker::~MediaMarker ()
{
	g_free (type);
	g_free (text);
}

/*
 * MarkerStream
 */
 
MarkerStream::MarkerStream (Media *media) : IMediaStream (media)
{
	closure = NULL;
}

MarkerStream::~MarkerStream ()
{
	closure = NULL;
}

void
MarkerStream::MarkerFound (MediaFrame *frame)
{
	MediaResult result;
	
	if (decoder == NULL) {
		LOG_PIPELINE ("MarkerStream::MarkerFound (): Got marker, but there's no decoder for the marker.\n");
		return;
	}
	
	result = decoder->DecodeFrame (frame);
	
	if (!MEDIA_SUCCEEDED (result)) {
		LOG_PIPELINE ("MarkerStream::MarkerFound (): Error while decoding marker: %i\n", result);
		return;
	}
	
	if (closure == NULL) {
		LOG_PIPELINE ("MarkerStream::MarkerFound (): Got decoded marker, but nobody is waiting ofr it.\n");
		return;
	}
	
	closure->marker = (MediaMarker *) frame->buffer;
	closure->Call ();
	
	delete closure->marker;
	closure->marker = NULL;
	
	frame->buffer = NULL;
	frame->buflen = 0;
}

void
MarkerStream::SetCallback (MediaClosure *closure)
{
	this->closure = closure;
}

/*
 * MediaWork
 */ 
MediaWork::MediaWork (MediaClosure *closure, IMediaStream *stream, guint16 states)
{
	switch (stream->GetType ()) {
	case MediaTypeVideo:
		type = WorkTypeVideo; break;
	case MediaTypeAudio:
		type = WorkTypeAudio; break;
	case MediaTypeMarker:
		type = WorkTypeMarker; break;
	default:
		fprintf (stderr, "MediaWork::MediaWork (%p, %p, %i): Invalid stream type %u\n",
			 closure, stream, (uint) states, stream->GetType ());
		break;
	}
	this->closure = closure;
	this->data.frame.states = states;
	this->data.frame.stream = stream;
	this->data.frame.stream->ref ();
}

MediaWork::MediaWork (MediaClosure *closure, guint64 seek_pts)
{
	type = WorkTypeSeek;
	this->closure = closure;
	this->data.seek.seek_pts = seek_pts;
}

MediaWork::MediaWork (MediaClosure *closure, IMediaSource *source)
{
	type = WorkTypeOpen;
	this->closure = closure;
	this->data.open.source = source;
	this->data.open.source->ref ();
}

MediaWork::~MediaWork ()
{
	switch (type) {
	case WorkTypeOpen:
		if (data.open.source)
			data.open.source->unref ();
		break;
	case WorkTypeVideo:
	case WorkTypeAudio:
	case WorkTypeMarker:
		if (data.frame.stream)
			data.frame.stream->unref ();
		break;
	case WorkTypeSeek:
		break; // Nothing to do
	}
	delete closure;

#if DEBUG
	memset (&data, 0, sizeof (data));
#endif
}

/*
 * NullDecoderInfo
 */

bool
NullDecoderInfo::Supports (const char *codec)
{
	const char *video_fourccs [] = { "wmv1", "wmv2", "wmv3", "wmva", "vc1", NULL };
	const char *audio_fourccs [] = { "wmav1","wmav2", "mp3", NULL};
	
	for (int i = 0; video_fourccs [i] != NULL; i++)
		if (!strcmp (codec, video_fourccs [i]))
			return true;

	for (int i = 0; audio_fourccs [i] != NULL; i++)
		if (!strcmp (codec, audio_fourccs [i]))
			return true;


	return false;
}

/*
 * NullDecoder
 */

NullDecoder::NullDecoder (Media *media, IMediaStream *stream) : IMediaDecoder (media, stream)
{
	logo = NULL;
	logo_size = 0;
	prev_pts = G_MAXUINT64;
}

NullDecoder::~NullDecoder ()
{
	g_free (logo);
}

MediaResult
NullDecoder::DecodeVideoFrame (MediaFrame *frame)
{
	// free encoded buffer and alloc a new one for our image
	g_free (frame->buffer);
	frame->buflen = logo_size;
	frame->buffer = (guint8*) g_malloc (frame->buflen);
	memcpy (frame->buffer, logo, frame->buflen);
	frame->AddState (FRAME_DECODED);
	
	//printf ("NullVideoDecoder::DecodeFrame () pts: %llu, w: %i, h: %i\n", frame->pts, w, h);
	
	return MEDIA_SUCCESS;
}

MediaResult
NullDecoder::DecodeAudioFrame (MediaFrame *frame)
{
	AudioStream *as = (AudioStream *) stream;
	guint32 samples;
	guint32 data_size;
	guint64 diff_pts;
	
	// discard encoded data
	g_free (frame->buffer);

	// We have no idea here how long the encoded audio data is
	// for the first frame we use 0.1 seconds, for the rest
	// we calculate the time since the last frame

	if (prev_pts == G_MAXUINT64) {
		samples = as->sample_rate / 10; // start off sending 0.1 seconds of audio
	} else {
		diff_pts = frame->pts - prev_pts;
		samples = (float) as->sample_rate / (TIMESPANTICKS_IN_SECOND_FLOAT / (float) diff_pts);
	}
	prev_pts = frame->pts;

	data_size  = samples * as->channels * 2 /* 16 bit audio */;

	frame->buflen = data_size;
	frame->buffer = (guint8 *) g_malloc0 (frame->buflen);
	
	frame->AddState (FRAME_DECODED);
	
	return MEDIA_SUCCESS;
}

MediaResult
NullDecoder::DecodeFrame (MediaFrame *frame)
{
	if (stream->GetType () == MediaTypeAudio)
		return DecodeAudioFrame (frame);
	else if (stream->GetType () == MediaTypeVideo)
		return DecodeVideoFrame (frame);
	else
		return MEDIA_FAIL;
}

MediaResult
NullDecoder::Open ()
{
	if (stream->GetType () == MediaTypeAudio)
		return OpenAudio ();
	else if (stream->GetType () == MediaTypeVideo)
		return OpenVideo ();
	else
		return MEDIA_FAIL;
}

MediaResult
NullDecoder::OpenAudio ()
{
	return MEDIA_SUCCESS;
}

MediaResult
NullDecoder::OpenVideo ()
{
	VideoStream *vs = (VideoStream *) stream;
	guint32 dest_height = vs->height;
	guint32 dest_width = vs->width;
	guint32 dest_i = 0;
	
	// We assume that the input image is a 24 bit bitmap (bmp), stored bottum up and flipped vertically.
	extern const char moonlight_logo [];
	const char *image = moonlight_logo;

	guint32 img_offset = *((guint32*)(image + 10));
	guint32 img_width  = *((guint32*)(image + 18));
	guint32 img_height = *((guint32*)(image + 22));
	guint32 img_stride = (img_width * 3 + 3) & ~3; // in bytes
	guint32 img_i, img_h, img_w;
	
	LOG_PIPELINE ("offset: %i, size: %i, width: 0x%x = %i, height: 0x%x = %i, stride: %i\n", img_offset, img_size, img_width, img_width, img_height, img_height, img_stride);
	
	// create the buffer for our image
	logo_size = dest_height * dest_width * 4;
	logo = (guint8*) g_malloc (logo_size);

	// write our image tiled into the destination rectangle, flipped horizontally
	dest_i = 4;
	for (guint32 dest_h = 0; dest_h < dest_height; dest_h++) {
		for (guint32 dest_w = 0; dest_w < dest_width; dest_w++) {
			img_h = dest_h % img_height;
			img_w = dest_w % img_width;
			img_i = img_h * img_stride + img_w * 3;
			
			logo [logo_size - dest_i + 0] = image [img_offset + img_i + 0];
			logo [logo_size - dest_i + 1] = image [img_offset + img_i + 1];
			logo [logo_size - dest_i + 2] = image [img_offset + img_i + 2];
			logo [logo_size - dest_i + 3] = 255;
			
			dest_i += 4;
		}
	}
	
	// Flip the image vertically
	for (guint32 dest_h = 0; dest_h < dest_height; dest_h++) {
		for (guint32 dest_w = 0; dest_w < dest_width / 2; dest_w++) {
			guint32 tmp;
			guint32 a = (dest_h * dest_width + dest_w) * 4;
			guint32 b = (dest_h * dest_width + dest_width - dest_w) * 4 - 4;
			for (guint32 c = 0; c < 3; c++) {
				tmp = logo [a + c];
				logo [a + c] = logo [b + c];
				logo [b + c] = tmp;
			}
		}
	}

	pixel_format = MoonPixelFormatRGB32;
	
	return MEDIA_SUCCESS;
}

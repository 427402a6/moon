/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * downloader.h: Downloader class.
 *
 * Contact:
 *   Moonlight List (moonligt-list@lists.ximian.com)
 *
 * Copyright 2008 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#ifndef __DOWNLOADER_H__
#define __DOWNLOADER_H__

#include <glib.h>

#include <stdint.h>
#include <cairo.h>

#include "dependencyobject.h"
#include "internal-downloader.h"
#include "http-streaming.h"

class FileDownloader;
class Downloader;

typedef void     (* DownloaderWriteFunc) (void *buf, gint32 offset, gint32 n, gpointer cb_data);
typedef void     (* DownloaderNotifySizeFunc) (gint64 size, gpointer cb_data);

typedef gpointer (* DownloaderCreateStateFunc) (Downloader *dl);
typedef void     (* DownloaderDestroyStateFunc) (gpointer state);
typedef void     (* DownloaderOpenFunc) (const char *verb, const char *uri, bool streaming, gpointer state);
typedef void     (* DownloaderSendFunc) (gpointer state);
typedef void     (* DownloaderAbortFunc) (gpointer state);
typedef void     (* DownloaderHeaderFunc) (gpointer state, const char *header, const char *value);
typedef void     (* DownloaderBodyFunc) (gpointer state, void *body, guint32 length);
typedef gpointer (* DownloaderCreateWebRequestFunc) (const char *method, const char *uri, gpointer context);

enum DownloaderAccessPolicy {
	DownloaderPolicy,
	MediaPolicy,
	XamlPolicy,
	StreamingPolicy,
	NoPolicy
};

/* @Namespace=None */
/* @ManagedDependencyProperties=None */
class Downloader : public DependencyObject {
	static DownloaderCreateStateFunc create_state;
	static DownloaderDestroyStateFunc destroy_state;
	static DownloaderOpenFunc open_func;
	static DownloaderSendFunc send_func;
	static DownloaderAbortFunc abort_func;
	static DownloaderHeaderFunc header_func;
	static DownloaderBodyFunc body_func;
	static DownloaderCreateWebRequestFunc request_func;

	// Set by the consumer
	DownloaderNotifySizeFunc notify_size;
	DownloaderWriteFunc writer;
	gpointer user_data;
	
	// Set by the supplier.
	gpointer downloader_state;
	
	gpointer context;
	HttpStreamingFeatures streaming_features;
	
	gint64 file_size;
	gint64 total;
	
	char *filename;
	char *buffer;
	
	char *failed_msg;
	
	int send_queued:1;
	int completed:1;
	int started:1;
	int aborted:1;
	
	InternalDownloader *internal_dl;

	DownloaderAccessPolicy access_policy;

 protected:
	virtual ~Downloader ();
	
	void SetStatusText (const char *text);
	void SetStatus (int status);
	
 public:
	/* @PropertyType=double,DefaultValue=0.0,GenerateAccessors */
	const static int DownloadProgressProperty;
	/* @PropertyType=string */
	const static int ResponseTextProperty;
	/* @PropertyType=gint32,DefaultValue=0,GenerateAccessors */
	const static int StatusProperty;
	/* @PropertyType=string,DefaultValue=\"\",GenerateAccessors */
	const static int StatusTextProperty;
	/* @PropertyType=Uri,GenerateAccessors */
	const static int UriProperty;
	
	// Events you can AddHandler to
	const static int CompletedEvent;
	const static int DownloadProgressChangedEvent;
	const static int DownloadFailedEvent;
	
	/* @GenerateCBinding */
	Downloader ();
	
	void Abort ();
	char *GetResponseText (const char *Partname, gint64 *size);
	char *GetDownloadedFilename (const char *partname);
	void Open (const char *verb, const char *uri, DownloaderAccessPolicy policy);
	void SendInternal ();
	void Send ();
	void SendNow ();
	
	// the following is stuff not exposed by C#/js, but is useful
	// when writing unmanaged code for downloader implementations
	// or data sinks.
	
	void InternalAbort ();
	void InternalWrite (void *buf, gint32 offset, gint32 n);
	void InternalOpen (const char *verb, const char *uri, bool streaming);
	void InternalSetHeader (const char *header, const char *value);
	void InternalSetBody (void *body, guint32 length);
	
	/* @GenerateCBinding,GeneratePInvoke */
	void Write (void *buf, gint32 offset, gint32 n);
	
	/* @GenerateCBinding,GeneratePInvoke */
	void NotifyFinished (const char *final_uri);
	
	/* @GenerateCBinding,GeneratePInvoke */
	void NotifyFailed (const char *msg);
	
	/* @GenerateCBinding,GeneratePInvoke */
	void NotifySize (gint64 size);
	
	void SetFilename (const char *fname);
	char *GetBuffer () { return buffer; }
	gint64 GetSize () { return total; }
	
	InternalDownloader *GetInternalDownloader () { return internal_dl; }
	
	// This is called by the consumer of the downloaded data (the
	// Image class for instance)
	void SetStreamFunctions (DownloaderWriteFunc writer,
				 DownloaderNotifySizeFunc notify_size,
				 gpointer user_data);
	
	// This is called by the supplier of the downloaded data (the
	// managed framework, the browser plugin, the demo test)
	
	/* @GenerateCBinding,GeneratePInvoke */
	static void SetFunctions (DownloaderCreateStateFunc create_state,
				  DownloaderDestroyStateFunc destroy_state,
				  DownloaderOpenFunc open,
				  DownloaderSendFunc send,
				  DownloaderAbortFunc abort,
				  DownloaderHeaderFunc header,
				  DownloaderBodyFunc body,
			          DownloaderCreateWebRequestFunc request,
				  bool only_if_not_set);
		
	bool Started ();
	bool Completed ();
	bool IsAborted () { return aborted; }
	const char *GetFailedMessage () { return failed_msg; }
	
	void     SetContext (gpointer context) { this->context = context;}
	gpointer GetContext () { return context; }
	gpointer GetDownloaderState () { return downloader_state; }
	void     SetHttpStreamingFeatures (HttpStreamingFeatures features) { streaming_features = features; }
	HttpStreamingFeatures GetHttpStreamingFeatures () { return streaming_features; }
	DownloaderCreateWebRequestFunc GetRequestFunc () {return request_func; }

	//
	// Property Accessors
	//
	void SetDownloadProgress (double progress);
	double GetDownloadProgress ();
	
	const char *GetStatusText ();
	int GetStatus ();
	
	void SetUri (Uri *uri);
	Uri *GetUri ();
};

class DownloaderResponse;
class DownloaderRequest;

typedef guint32 (* DownloaderResponseStartedHandler) (DownloaderResponse *response, gpointer context);
typedef guint32 (* DownloaderResponseDataAvailableHandler) (DownloaderResponse *response, gpointer context, char *buffer, guint32 length);
typedef guint32 (* DownloaderResponseFinishedHandler) (DownloaderResponse *response, gpointer context, bool success, gpointer data, const char *uri);
typedef void (* DownloaderResponseHeaderVisitorCallback) (const char *header, const char *value);

class IDownloader {
 private:
	Deployment *deployment;

 public:
	virtual void Abort () = 0;
	virtual const bool IsAborted () = 0;
	Deployment *GetDeployment () { return deployment; }
	void SetDeployment (Deployment *deployment) { this->deployment = deployment; }
};

class DownloaderResponse : public IDownloader {
 protected:
	DownloaderResponseStartedHandler started;
	DownloaderResponseDataAvailableHandler available;
	DownloaderResponseFinishedHandler finished;
	gpointer context;
	DownloaderRequest *request;
	bool aborted;

 public:
	DownloaderResponse ();
	DownloaderResponse (DownloaderResponseStartedHandler started, DownloaderResponseDataAvailableHandler available, DownloaderResponseFinishedHandler finished, gpointer context);
	/* @GenerateCBinding,GeneratePInvoke */
	virtual ~DownloaderResponse ();

	/* @GenerateCBinding,GeneratePInvoke */
	virtual void Abort () = 0;
	virtual const bool IsAborted () { return this->aborted; }
	virtual void SetHeaderVisitor (DownloaderResponseHeaderVisitorCallback visitor) = 0;
	/* @GenerateCBinding,GeneratePInvoke */
	virtual int GetResponseStatus () = 0;
	/* @GenerateCBinding,GeneratePInvoke */
	virtual const char * GetResponseStatusText () = 0;
	DownloaderRequest *GetDownloaderRequest () { return request; }
	void SetDownloaderRequest (DownloaderRequest *value) { request = value; }
	
	virtual void ref () = 0;
	virtual void unref () = 0;
};

class DownloaderRequest : public IDownloader {
 protected:
 	DownloaderResponse *response;
	char *uri;
	char *method;

	bool aborted;

 public:
	DownloaderRequest (const char *method, const char *uri);
	/* @GenerateCBinding,GeneratePInvoke */
	virtual ~DownloaderRequest ();

	/* @GenerateCBinding,GeneratePInvoke */
	virtual void Abort () = 0;
	virtual bool GetResponse (DownloaderResponseStartedHandler started, DownloaderResponseDataAvailableHandler available, DownloaderResponseFinishedHandler finished, gpointer context) = 0;
	virtual const bool IsAborted () { return this->aborted; }
	virtual void SetHttpHeader (const char *name, const char *value) = 0;
	virtual void SetBody (void *body, int size) = 0;
	DownloaderResponse *GetDownloaderResponse () { return response; }
	void SetDownloaderResponse (DownloaderResponse *value) { response = value; }
};

G_BEGIN_DECLS

void downloader_init (void);

//
// Used to push data to the consumer
//

void *downloader_create_webrequest (Downloader *dl, const char *method, const char *uri);

bool downloader_request_get_response (DownloaderRequest *dr, DownloaderResponseStartedHandler started, DownloaderResponseDataAvailableHandler available, DownloaderResponseFinishedHandler finished, gpointer context);
bool downloader_request_is_aborted (DownloaderRequest *dr);
void downloader_request_set_http_header (DownloaderRequest *dr, const char *name, const char *value);
void downloader_request_set_body (DownloaderRequest *dr, void *body, int size);

void downloader_response_set_header_visitor (DownloaderResponse *dr, DownloaderResponseHeaderVisitorCallback visitor);


// FIXME: get rid of this
const char *downloader_deobfuscate_font (Downloader *downloader, const char *path);

G_END_DECLS

#endif

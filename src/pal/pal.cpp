/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * pal.cpp
 *
 * Copyright 2009 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "deployment.h"
#include "downloader.h"
#include "runtime.h"
#include "uri.h"
#include "pal.h"

void
MoonWindowingSystem::SetWindowlessCtor (MoonWindowlessCtor ctor)
{
	windowless_ctor = ctor;
}

MoonWindow *
MoonWindowingSystem::CreateWindowless (int width, int height, PluginInstance *forPlugin)
{
	// call into the plugin to create the windowless
	if (windowless_ctor) {
		MoonWindow *window = windowless_ctor (width, height, forPlugin);
		return window;
	}
	else {
		g_warning ("Windowless mode only works in the plugin");
		return NULL;
	}
}


//
// MoonAppRecord
//

MoonAppRecord::MoonAppRecord ()
{
	origin = NULL;
	uid = NULL;
	mtime = 0;
}

MoonAppRecord::~MoonAppRecord ()
{
	g_free (origin);
	g_free (uid);
}

bool
MoonAppRecord::Equal (const MoonAppRecord *app) const
{
	return !strcmp (app->uid, uid);
}

bool
MoonAppRecord::Save (FILE *db) const
{
	return fprintf (db, "%s = {\n\torigin=%s\n\tmtime=%lld\n}\n", uid, origin, (long long int) mtime) > 0;
}


//
// MoonAppRecordIterator
//

MoonAppRecordIterator::MoonAppRecordIterator (FILE *db)
{
	inptr = inend = buf;
	this->db = db;
}

bool
MoonAppRecordIterator::Fill ()
{
	size_t inlen, nread, n;
	
	inlen = inend - inptr;
	
	if (inptr > buf) {
		if (inlen > 0) {
			memmove (buf, inptr, inlen);
			inend = buf + inlen;
			inptr = buf;
		} else {
			inptr = inend = buf;
		}
	}
	
	if (!(n = (sizeof (buf) - 1) - inlen))
		return false;
	
	if (!(nread = fread (inend, 1, n, db)))
		return false;
	
	inend += nread;
	
	return true;
}

bool
MoonAppRecordIterator::EatSpaces ()
{
	do {
		*inend = '\0';
		while (*inptr == ' ' || *inptr == '\t')
			inptr++;
		
		if (inptr < inend)
			return true;
	} while (Fill ());
	
	return false;
}

char
MoonAppRecordIterator::NextToken ()
{
	if (!EatSpaces ())
		return '\0';
	
	return *inptr;
}

char *
MoonAppRecordIterator::ParseUid ()
{
	char *start, *str;
	GString *uid;
	
	if (!EatSpaces ())
		return NULL;
	
	uid = g_string_new ("");
	
	do {
		start = inptr;
		while (inptr < inend && *inptr != ' ' && *inptr != '\t' && *inptr != '=')
			inptr++;
		
		g_string_append_len (uid, start, inptr - start);
		
		if (inptr < inend) {
			str = uid->str;
			g_string_free (uid, false);
			return str;
		}
	} while (Fill ());
	
	g_string_free (uid, true);
	
	return NULL;
}

char *
MoonAppRecordIterator::ParseOrigin ()
{
	char *start, *str;
	GString *origin;
	
	if (!EatSpaces ())
		return NULL;
	
	// the rest of this line is our origin value...
	origin = g_string_new ("");
	
	do {
		*inend = '\n';
		start = inptr;
		while (*inptr != '\n')
			inptr++;
		
		g_string_append_len (origin, start, inptr - start);
		
		if (inptr < inend) {
			str = origin->str;
			g_string_free (origin, false);
			
			// eat the '\n'
			inptr++;
			
			return g_strchomp (str);
		}
	} while (Fill ());
	
	g_string_free (origin, true);
	
	return NULL;
}

time_t
MoonAppRecordIterator::ParseMTime ()
{
	gint64 mtime;
	char *ptr;
	
	do {
		*inend = '\0';
		mtime = strtoll (inptr, &ptr, 10);
		if (mtime == LLONG_MAX || ptr == inptr)
			return 0;
		
		if (ptr < inend) {
			inptr = ptr;
			
			return (time_t) mtime;
		}
	} while (Fill ());
	
	return 0;
}

MoonAppRecord *
MoonAppRecordIterator::Next ()
{
	char *uid, *origin = NULL;
	MoonAppRecord *app;
	time_t mtime = 0;
	char c;
	
	if (inptr == inend && !Fill ())
		return NULL;
	
	// first, try decoding the uid
	if (!(uid = ParseUid ()))
		return NULL;
	
	// make sure the next token is an '='
	if (NextToken () != '=')
		goto error;
	
	inptr++;
	
	// make sure the next token is '{'
	if (NextToken () != '{')
		goto error;
	
	inptr++;
	
	// make sure the next token is '\n'
	if (NextToken () != '\n')
		goto error;
	
	inptr++;
	
	// now we parse the properties (like origin and mtime), 1 property per line
	while ((c = NextToken ()) != '}') {
		switch (c) {
		case 'o': /* origin? */
			if (origin) {
				// already have origin, error
				goto error;
			}
			
			// make sure we have enough data for strncmp
			while ((inend - inptr) < 6) {
				if (!Fill ())
					goto error;
			}
			
			if (strncmp (inptr, "origin", 6) != 0)
				goto error;
			
			inptr += 6;
			
			// make sure next token is '='
			if (NextToken () != '=')
				goto error;
			
			inptr++;
			
			// parse the origin
			if (!(origin = ParseOrigin ()))
				goto error;
			
			// Note: ParseOrigin() gobbles the trailing '\n' for us
			break;
		case 'm': /* mtime? */
			if (mtime != 0) {
				// already have mtime, error
				goto error;
			}
			
			// make sure we have enough data for strncmp
			while ((inend - inptr) < 5) {
				if (!Fill ())
					goto error;
			}
			
			if (strncmp (inptr, "mtime", 5) != 0)
				goto error;
			
			inptr += 5;
			
			// make sure next token is '='
			if (NextToken () != '=')
				goto error;
			
			inptr++;
			
			// parse the mtime
			if (!(mtime = ParseMTime ()))
				goto error;
			
			// make sure the next token is '\n'
			if (NextToken () != '\n')
				goto error;
			
			inptr++;
			break;
		default:
			/* error */
			goto error;
		}
	}
	
	app = new MoonAppRecord ();
	app->origin = origin;
	app->mtime = mtime;
	app->uid = uid;
	
	return app;
	
error:
	g_free (origin);
	g_free (uid);
	
	return NULL;
}


//
// MoonAppDatabase
//

MoonAppDatabase::MoonAppDatabase (const char *base_dir)
{
	this->path = g_build_filename (base_dir, "apps.db", NULL);
	this->base_dir = g_strdup (base_dir);
}

MoonAppDatabase::~MoonAppDatabase ()
{
	g_free (base_dir);
	g_free (path);
}

MoonAppRecord *
MoonAppDatabase::CreateAppRecord (const char *origin)
{
	char *install_dir, *uid;
	MoonAppRecord *app;
	const char *domain;
	Uri *uri;
	
	uri = new Uri ();
	if (!uri->Parse (origin) || !(domain = uri->GetHost ()))
		domain = "localhost";
	
	do {
		uid = g_strdup_printf ("%u.%s", g_random_int (), domain);
		install_dir = g_build_filename (base_dir, uid, NULL);
		if (g_mkdir_with_parents (install_dir, 0777) == 0)
			break;
		
		g_free (install_dir);
		g_free (uid);
		
		if (errno != EEXIST) {
			// EEXIST is the only error we can recover from...
			delete uri;
			return NULL;
		}
	} while (true);
	
	app = new MoonAppRecord ();
	app->origin = g_strdup (origin);
	app->mtime = time (NULL);
	app->uid = uid;
	delete uri;
	
	if (!AddAppRecord (app)) {
		RemoveDir (install_dir);
		delete app;
		return NULL;
	}
	
	g_free (install_dir);
	
	return app;
}

bool
MoonAppDatabase::AddAppRecord (MoonAppRecord *record)
{
	bool saved;
	FILE *db;
	
	if (!record->uid || !record->origin)
		return false;
	
	if (!(db = fopen (path, "at")))
		return false;
	
	if (record->mtime == 0)
		record->mtime = time (NULL);
	
	saved = record->Save (db);
	
	fclose (db);
	
	return saved;
}

bool
MoonAppDatabase::SyncAppRecord (const MoonAppRecord *record)
{
	MoonAppRecordIterator *iter;
	MoonAppRecord *app;
	bool error = false;
	FILE *orig, *db;
	char *tmp;
	
	if (!(orig = fopen (path, "rt")))
		return false;
	
	tmp = g_build_filename (base_dir, "apps.tmp", NULL);
	if (!(db = fopen (tmp, "wt"))) {
		fclose (orig);
		g_free (tmp);
		return false;
	}
	
	iter = new MoonAppRecordIterator (orig);
	
	while (!error && (app = iter->Next ())) {
		if (app->Equal (record))
			error = !record->Save (db);
		else
			error = !app->Save (db);
		
		delete app;
	}
	
	delete iter;
	fclose (orig);
	fclose (db);
	
	if (error || g_rename (tmp, path) == -1) {
		g_unlink (tmp);
		g_free (tmp);
		return false;
	}
	
	g_free (tmp);
	
	return true;
}

bool
MoonAppDatabase::RemoveAppRecord (const MoonAppRecord *record)
{
	MoonAppRecordIterator *iter;
	MoonAppRecord *app;
	bool error = false;
	FILE *orig, *db;
	char *tmp;
	
	if (!(orig = fopen (path, "rt")))
		return false;
	
	tmp = g_build_filename (base_dir, "apps.tmp", NULL);
	if (!(db = fopen (tmp, "wt"))) {
		fclose (orig);
		g_free (tmp);
		return NULL;
	}
	
	iter = new MoonAppRecordIterator (orig);
	
	while (!error && (app = iter->Next ())) {
		if (!app->Equal (record))
			error = !app->Save (db);
		
		delete app;
	}
	
	delete iter;
	fclose (orig);
	fclose (db);
	
	if (error || g_rename (tmp, path) == -1) {
		g_unlink (tmp);
		g_free (tmp);
		return false;
	}
	
	g_free (tmp);
	
	return true;
}

MoonAppRecord *
MoonAppDatabase::GetAppRecordByOrigin (const char *origin)
{
	MoonAppRecordIterator *iter;
	MoonAppRecord *app;
	FILE *db;
	
	if (!(db = fopen (path, "rt")))
		return NULL;
	
	iter = new MoonAppRecordIterator (db);
	
	while ((app = iter->Next ())) {
		if (!strcmp (app->origin, origin))
			break;
		
		delete app;
		app = NULL;
	}
	
	delete iter;
	fclose (db);
	
	return app;
}

MoonAppRecord *
MoonAppDatabase::GetAppRecordByUid (const char *uid)
{
	MoonAppRecordIterator *iter;
	MoonAppRecord *app;
	FILE *db;
	
	if (!(db = fopen (path, "rt")))
		return NULL;
	
	iter = new MoonAppRecordIterator (db);
	
	while ((app = iter->Next ())) {
		if (!strcmp (app->uid, uid))
			break;
		
		delete app;
		app = NULL;
	}
	
	delete iter;
	fclose (db);
	
	return app;
}


//
// MoonInstallerService
//

MoonInstallerService::MoonInstallerService ()
{
	downloader = NULL;
	completed = NULL;
	user_data = NULL;
	xap = NULL;
	app = NULL;
	db = NULL;
}

MoonInstallerService::~MoonInstallerService ()
{
	CloseDownloader (true);
	delete db;
}

bool
MoonInstallerService::InitDatabase ()
{
	const char *base_dir = GetBaseInstallDir ();
	
	if (db == NULL) {
		if (g_mkdir_with_parents (base_dir, 0777) == -1 && errno != EEXIST)
			return false;
		
		db = new MoonAppDatabase (base_dir);
	}
	
	return true;
}

void
MoonInstallerService::CloseDownloader (bool abort)
{
	if (downloader) {
		g_byte_array_free (xap, true);
		
		if (abort)
			downloader->Abort ();
		
		downloader->unref ();
		delete app;
		
		downloader = NULL;
		completed = NULL;
		user_data = NULL;
		app = NULL;
		xap = NULL;
	}
}

MoonAppRecord *
MoonInstallerService::CreateAppRecord (const char *origin)
{
	if (!InitDatabase ())
		return NULL;
	
	return db->CreateAppRecord (origin);
}

MoonAppRecord *
MoonInstallerService::GetAppRecord (Deployment *deployment)
{
	const char *xap_uri = deployment->GetXapLocation ();
	const char *base_dir = GetBaseInstallDir ();
	const char *start, *inptr;
	MoonAppRecord *app;
	char *uid;
	
	if (!InitDatabase ())
		return NULL;
	
	if (IsRunningOutOfBrowser (deployment)) {
		// we'll need to get the uid used by this app, which can be
		// extracted from the local path.
		start = xap_uri + 7 + strlen (base_dir);
		if (*start == '/')
			start++;
		
		inptr = start;
		while (*inptr != '/')
			inptr++;
		
		uid = g_strndup (start, inptr - start);
		app = db->GetAppRecordByUid (uid);
		g_free (uid);
	} else {
		app = db->GetAppRecordByOrigin (xap_uri);
	}
	
	return app;
}

void
MoonInstallerService::UpdaterNotifySize (gint64 size)
{
	g_byte_array_set_size (xap, (guint) size);
}

void
MoonInstallerService::downloader_notify_size (gint64 size, gpointer user_data)
{
	((MoonInstallerService *) user_data)->UpdaterNotifySize (size);
}

void
MoonInstallerService::UpdaterWrite (void *buf, gint32 offset, gint32 n)
{
	memcpy (((char *) xap->data) + offset, buf, n);
}

void
MoonInstallerService::downloader_write (void *buf, gint32 offset, gint32 n, gpointer user_data)
{
	((MoonInstallerService *) user_data)->UpdaterWrite (buf, offset, n);
}

void
MoonInstallerService::UpdaterCompleted ()
{
	char *content, *path, *tmp;
	int err = 0;
	gsize size;
	FILE *fp;
	
	path = g_build_filename (GetBaseInstallDir (), app->uid, "Application.xap", NULL);
	
	// check that the xap has changed...
	if (g_file_get_contents (path, &content, &size, NULL)) {
		if (xap->len == size && !memcmp (xap->data, content, size)) {
			// no change to the xap
			completed (false, NULL, user_data);
			CloseDownloader (false);
			g_free (content);
			g_free (path);
			return;
		}
		
		g_free (content);
	}
	
	tmp = g_strdup_printf ("%s.tmp", path);
	
	if ((fp = fopen (tmp, "wb"))) {
		// write to the temporary file
		if (fwrite (xap->data, 1, xap->len, fp) < xap->len)
			err = ferror (fp);
		fclose (fp);
		
		if (err == 0) {
			// rename the temp file to the actual file
			if (g_rename (tmp, path) == -1)
				err = errno;
		}
	} else {
		err = errno;
	}
	
	g_free (path);
	g_free (tmp);
	
	if (err == 0) {
		// update the app's mtime
		// FIXME: get the Last-Modified: header from the downloader?
		app->mtime = time (NULL);
		db->SyncAppRecord (app);
	}
	
	completed (err == 0, err ? g_strerror (err) : NULL, user_data);
	
	CloseDownloader (false);
}

void
MoonInstallerService::downloader_completed (EventObject *sender, EventArgs *args, gpointer user_data)
{
	((MoonInstallerService *) user_data)->UpdaterCompleted ();
}

void
MoonInstallerService::UpdaterFailed ()
{
	completed (false, downloader->GetFailedMessage (), user_data);
	
	CloseDownloader (false);
}

void
MoonInstallerService::downloader_failed (EventObject *sender, EventArgs *args, gpointer user_data)
{
	((MoonInstallerService *) user_data)->UpdaterFailed ();
}

static const char *tm_months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *tm_days[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

void
MoonInstallerService::CheckAndDownloadUpdateAsync (Deployment *deployment, UpdateCompletedCallback completed, gpointer user_data)
{
	char mtime[128];
	struct tm tm;
	
	if (downloader) {
		// already downloading an update...
		return;
	}
	
	if (!(app = GetAppRecord (deployment))) {
		// FIXME: call completed with an error??
		return;
	}
	
	gmtime_r (&app->mtime, &tm);
	
	snprintf (mtime, sizeof (mtime), "%s, %02d %s %04d %02d:%02d:%02d GMT",
		  tm_days[tm.tm_wday], tm.tm_mday, tm_months[tm.tm_mon],
		  tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
	
	downloader = deployment->GetSurface ()->CreateDownloader ();
	downloader->AddHandler (Downloader::DownloadFailedEvent, downloader_failed, this);
	downloader->AddHandler (Downloader::CompletedEvent, downloader_completed, this);
	downloader->SetStreamFunctions (downloader_write, downloader_notify_size, this);
	downloader->InternalSetHeader ("If-Modified-Since", mtime);
	downloader->Open ("GET", app->origin, XamlPolicy);
	xap = g_byte_array_new ();
	downloader->Send ();
	
	this->completed = completed;
	this->user_data = user_data;
}

bool
MoonInstallerService::IsRunningOutOfBrowser (Deployment *deployment)
{
	const char *xap_uri = deployment->GetXapLocation ();
	const char *base_dir = GetBaseInstallDir ();
	const char *xap_path;
	size_t n;
	
	// First, make sure we are dealing with a locally run app
	if (!xap_uri || strncmp (xap_uri, "file://", 7) != 0)
		return false;
	
	// Skip past the file:// for the path component
	xap_path = xap_uri + 7;
	
	// Measure the length of the base install dir for OOB apps
	n = strlen (base_dir);
	
	// If the paths match for the first n characters, then this app is
	// running out-of-browser.
	return !strncmp (xap_path, base_dir, n) && xap_path[n] == G_DIR_SEPARATOR;
}


bool
MoonInstallerService::CheckInstalled (Deployment *deployment)
{
	MoonAppRecord *app;
	
	// if the MoonAppRecord exists, then it is installed...
	if (!(app = GetAppRecord (deployment)))
		return false;
	
	delete app;
	
	return true;
}

bool
MoonInstallerService::Uninstall (Deployment *deployment)
{
	const char *base_dir = GetBaseInstallDir ();
	MoonAppRecord *app;
	char *install_dir;
	
	// get the application record
	if (!(app = GetAppRecord (deployment)))
		return false;
	
	if (!db->RemoveAppRecord (app)) {
		// oops, we failed to remove it from our database of installed apps.
		delete app;
		return false;
	}
	
	// now we can uninstall the application by removing its install directory
	install_dir = g_build_filename (base_dir, app->uid, NULL);
	RemoveDir (install_dir);
	g_free (install_dir);
	delete app;
	
	return true;
}

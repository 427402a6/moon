/*
 * playlist.cpp:
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#include <config.h>
#include <expat.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "playlist.h"
#include "downloader.h"
#include "xaml.h"
#include "runtime.h"
#include "clock.h"
#include "mediaelement.h"
#include "debug.h"


class ParserInternal {
public:
	XML_Parser parser;
	gint32 bytes_read;
	bool reparse;

	ParserInternal () 
	{
		parser = XML_ParserCreate (NULL);
		bytes_read = 0;
		reparse = false;
	}

	~ParserInternal ()
	{
		XML_ParserFree (parser);
		parser = NULL;
	}
};
	
/*
 * PlaylistNode
 */

PlaylistNode::PlaylistNode (PlaylistEntry *entry) : List::Node () 
{
	if (entry)
		entry->ref ();
	this->entry = entry;
}

PlaylistNode::~PlaylistNode ()
{
	if (entry) {
		entry->unref ();
		entry = NULL;
	}
}

/*
 * PlaylistEntry
 */

PlaylistEntry::PlaylistEntry (MediaElement *element, Playlist *parent, Media *media)
{
	LOG_PLAYLIST ("PlaylistEntry::PlaylistEntry (%p, %p, %p)\n", element, parent, media);

	this->parent = parent;
	this->element = element;
	this->media = media;
	if (this->media)
		this->media->ref ();
	source_name = NULL;
	full_source_name = NULL;
	start_time = 0;
	duration = 0;
	play_when_available = false;
	base = NULL;
	title = NULL;
	author = NULL;
	abstract = NULL;
	copyright = NULL;
	info_target = NULL;
	info_url = NULL;
	client_skip = true;
	set_values = (PlaylistNode::Kind) 0;
}


PlaylistEntry::~PlaylistEntry ()
{
	LOG_PLAYLIST ("PlaylistEntry::~PlaylistEntry ()\n");
	
	delete source_name;
	g_free (full_source_name);

	if (media)
		media->unref ();

	delete base;
	g_free (title);
	g_free (author);
	g_free (abstract);
	g_free (copyright);
	g_free (info_target);
	g_free (info_url);
}

void
PlaylistEntry::Dispose ()
{
	EventObject::Dispose ();
	if (media) {
		media->Dispose ();
		media->unref ();
		media = NULL;
	}
}

Uri *
PlaylistEntry::GetBase ()
{
	return base;
}

Uri *
PlaylistEntry::GetBaseInherited ()
{
	if (base != NULL)
		return base;
	if (parent != NULL)
		return parent->GetBaseInherited ();
	return NULL;
}

void 
PlaylistEntry::SetBase (Uri *base)
{
	// TODO: Haven't been able to make BASE work with SL,
	// which means that I haven't been able to confirm any behaviour.
	if (!(set_values & PlaylistNode::Base)) {
		this->base = base;
		set_values = (PlaylistNode::Kind) (set_values | PlaylistNode::Base);
	} else {
		delete base;
	}
}

const char *
PlaylistEntry::GetTitle ()
{
	return title;
}

void 
PlaylistEntry::SetTitle (char *title)
{
	if (!(set_values & PlaylistNode::Title)) {
		this->title = title;
		set_values = (PlaylistNode::Kind) (set_values | PlaylistNode::Title);
	}
}

const char *
PlaylistEntry::GetAuthor ()
{
	return author;
}

void PlaylistEntry::SetAuthor (char *author)
{
	if (!(set_values & PlaylistNode::Author)) {
		this->author = author;
		set_values = (PlaylistNode::Kind) (set_values | PlaylistNode::Author);
	}
}

const char *
PlaylistEntry::GetAbstract ()
{
	return abstract;
}

void 
PlaylistEntry::SetAbstract (char *abstract)
{
	if (!(set_values & PlaylistNode::Abstract)) {
		this->abstract = abstract;
		set_values = (PlaylistNode::Kind) (set_values | PlaylistNode::Abstract);
	}
}

const char *
PlaylistEntry::GetCopyright ()
{
	return copyright;
}

void 
PlaylistEntry::SetCopyright (char *copyright)
{
	if (!(set_values & PlaylistNode::Copyright)) {
		this->copyright = copyright;
		set_values = (PlaylistNode::Kind) (set_values | PlaylistNode::Copyright);
	}
}

Uri *
PlaylistEntry::GetSourceName ()
{
	return source_name;
}

void 
PlaylistEntry::SetSourceName (Uri *source_name)
{
	if (this->source_name)
		delete this->source_name;
	this->source_name = source_name;
}

TimeSpan 
PlaylistEntry::GetStartTime ()
{
	return start_time;
}

void 
PlaylistEntry::SetStartTime (TimeSpan start_time)
{
	if (!(set_values & PlaylistNode::StartTime)) {
		this->start_time = start_time;
		set_values = (PlaylistNode::Kind) (set_values | PlaylistNode::StartTime);
	}
}

TimeSpan 
PlaylistEntry::GetDuration ()
{
	return duration;
}

void 
PlaylistEntry::SetDuration (TimeSpan duration)
{
	if (!(set_values & PlaylistNode::Duration)) {
		this->duration = duration;
		set_values = (PlaylistNode::Kind) (set_values | PlaylistNode::Duration);
	}
}

const char *
PlaylistEntry::GetInfoTarget ()
{
	return info_target;
}

void
PlaylistEntry::SetInfoTarget (char *info_target)
{
	this->info_target = info_target;
}

const char *
PlaylistEntry::GetInfoURL ()
{
	return info_url;
}

void
PlaylistEntry::SetInfoURL (char *info_url)
{
	this->info_url = info_url;
}

bool
PlaylistEntry::GetClientSkip ()
{
	return client_skip;
}

void
PlaylistEntry::SetClientSkip (bool value)
{
	client_skip = value;
}

static void
add_attribute (MediaAttributeCollection *attributes, const char *name, const char *attr)
{
	if (!attr)
		return;

	MediaAttribute *attribute = new MediaAttribute ();
	attribute->SetValue (attr);
	attribute->SetName (name);
	
	attributes->Add (attribute);
	attribute->unref ();
}

void
PlaylistEntry::PopulateMediaAttributes ()
{
	LOG_PLAYLIST ("PlaylistEntry::PopulateMediaAttributes ()\n");

	const char *abstract = NULL;
	const char *author = NULL;
	const char *copyright = NULL;
	const char *title = NULL;
	const char *infotarget = NULL;
	const char *infourl = NULL;

	PlaylistEntry *current = this;
	MediaAttributeCollection *attributes;
	
	if (!(attributes = element->GetAttributes ())) {
		attributes = new MediaAttributeCollection ();
		element->SetAttributes (attributes);
	} else {
		attributes->Clear ();
	}
	
	while (current != NULL) {
		if (abstract == NULL)
			abstract = current->GetAbstract ();
		if (author == NULL)
			author = current->GetAuthor ();
		if (copyright == NULL)
			copyright = current->GetCopyright ();
		if (title == NULL)
			title = current->GetTitle ();		
		if (infotarget == NULL)
			infotarget = current->GetInfoTarget ();
		if (infourl == NULL)
			infourl = current->GetInfoURL ();

		current = current->GetParent ();
	}

	add_attribute (attributes, "Abstract", abstract);
	add_attribute (attributes, "Author", author);
	add_attribute (attributes, "Copyright", copyright);
	add_attribute (attributes, "InfoTarget", infotarget);
	add_attribute (attributes, "InfoURL", infourl);
	add_attribute (attributes, "Title", title);
}

const char *
PlaylistEntry::GetFullSourceName ()
{
	/*
	 * Now here we have some interesting semantics:
	 * - BASE has to be a complete url, with protocol and domain
	 * - BASE only matters up to the latest / (if no /, the entire BASE is used)
	 *
	 * Examples (numbered according to the test-playlist-with-base test in test/media/video)
	 *  
	 *  01 localhost/dir/ 			+ * 			= error
	 *  02 /dir/ 					+ * 			= error
	 *  03 dir						+ * 			= error
	 *  04 http://localhost/dir/	+ somefile		= http://localhost/dir/somefile
	 *  05 http://localhost/dir		+ somefile		= http://localhost/somefile
	 *  06 http://localhost			+ somefile		= http://localhost/somefile
	 *  07 http://localhost/dir/	+ /somefile		= http://localhost/somefile
	 *  08 http://localhost/dir/	+ dir2/somefile	= http://localhost/dir/dir2/somefile
	 *  09 rtsp://localhost/		+ somefile		= http://localhost/somefile
	 *  10 mms://localhost/dir/		+ somefile		= mms://localhost/dir/somefile
	 *  11 http://localhost/?huh	+ somefile		= http://localhost/somefile
	 *  12 http://localhost/#huh	+ somefile		= http://localhost/somefile
	 *  13 httP://localhost/		+ somefile		= http://localhost/somefile
	 * 
	 */
	 
	// TODO: url validation, however it should probably happen inside MediaElement when we set the source
	 
	if (full_source_name == NULL) {
		Uri *base = GetBaseInherited ();
		Uri *current = GetSourceName ();
		Uri *result = NULL;
		const char *pathsep;
		char *base_path;
		
		//printf ("PlaylistEntry::GetFullSourceName (), base: %s, current: %s\n", base ? base->ToString () : "NULL", current ? current->ToString () : "NULL");
		
		if (current->host != NULL) {
			//printf (" current host (%s) is something, protocol: %s\n", current->host, current->protocol);
			result = current;
		} else if (base != NULL) {
			result = new Uri ();
			result->protocol = g_strdup (base->protocol);
			result->user = g_strdup (base->user);
			result->passwd = g_strdup (base->passwd);
			result->host = g_strdup (base->host);
			result->port = base->port;
			// we ignore the params, query and fragment values.
			if (current->path != NULL && current->path [0] == '/') {
				//printf (" current path is relative to root dir on host\n");
				result->path = g_strdup (current->path);
			} else if (base->path == NULL) {
				//printf (" base path is root dir on host\n");
				result->path = g_strdup (current->path);
			} else {
				pathsep = strrchr (base->path, '/');
				if (pathsep != NULL) {
					if ((size_t) (pathsep - base->path + 1) == strlen (base->path)) {
						//printf (" last character of base path (%s) is /\n", base->path);
						result->path = g_strjoin (NULL, base->path, current->path, NULL);
					} else {
						//printf (" base path (%s) does not end with /, only copy path up to the last /\n", base->path);
						base_path = g_strndup (base->path, pathsep - base->path + 1);
						result->path = g_strjoin (NULL, base_path, current->path, NULL);
						g_free (base_path);
					}
				} else {
					//printf (" base path (%s) does not contain a /\n", base->path);
					result->path = g_strjoin (NULL, base->path, "/", current->path, NULL);
				}
			}
		} else {
			//printf (" there's no base\n");
			result = current;
		}
		
		full_source_name = result->ToString ();
		
		//printf (" result: %s\n", full_source_name);
		
		if (result != base && result != current)
			delete result;
	}
	return full_source_name;
}

void
PlaylistEntry::Open ()
{
	LOG_PLAYLIST ("PlaylistEntry::Open (), media = %p, FullSourceName = %s\n", media, GetFullSourceName ());

	if (media != NULL) {
		element->SetMedia (media);
		return;
	}

	Downloader *dl = element->GetSurface ()->CreateDownloader ();
	if (dl) {
		dl->Open ("GET", GetFullSourceName (), StreamingPolicy);
		element->SetSourceInternal (dl, NULL);
		dl->unref ();
	}
}

bool
PlaylistEntry::Play ()
{
	LOG_PLAYLIST ("PlaylistEntry::Play (), play_when_available: %s, media: %p, source name: %s\n", play_when_available ? "true" : "false", media, source_name ? source_name->ToString () : "NULL");

	if (media == NULL) {
		play_when_available = true;
		Open ();
		return false;
	}

	element->SetMedia (media);
	element->PlayInternal ();

	play_when_available = false;

	return true;
}

bool
PlaylistEntry::Pause ()
{
	LOG_PLAYLIST ("PlaylistEntry::Pause ()\n");
	
	play_when_available = false;
	element->GetMediaPlayer ()->Pause ();
	return true;
}

void
PlaylistEntry::Stop ()
{
	LOG_PLAYLIST ("PlaylistEntry::Stop ()\n");

	play_when_available = false;
	element->GetMediaPlayer ()->Stop ();
	
	if (media && !IsSingleFile ()) {
		media->unref ();
		media = NULL;
	}
}

Media *
PlaylistEntry::GetMedia ()
{
	return media;
}

void 	
PlaylistEntry::SetMedia (Media *media)
{
	LOG_PLAYLIST ("PlaylistEntry::SetMedia (%p), previous media: %p\n", media, this->media);

	if (this->media)
		this->media->unref ();

	this->media = media;
	this->media->ref ();

	if (play_when_available && element->GetState () != MediaElement::Buffering)
		Play ();
}

bool
PlaylistEntry::IsSingleFile ()
{
	return parent ? parent->IsSingleFile () : false;
}

/*
 * Playlist
 */

Playlist::Playlist (MediaElement *element, IMediaSource *source)
	: PlaylistEntry (element, NULL)
{
	is_single_file = false;
	autoplayed = false;
	Init (element);
	this->source = source;
}

Playlist::Playlist (MediaElement *element, Media *media)
	: PlaylistEntry (element, NULL, media)
{
	is_single_file = true;
	autoplayed = false;
	Init (element);

	AddEntry (new PlaylistEntry (element, this, media));
	current_node = (PlaylistNode *) entries->First ();
}

Playlist::~Playlist ()
{
	LOG_PLAYLIST ("Playlist::~Playlist ()\n");
	
	delete entries;
}

void
Playlist::Dispose ()
{
	PlaylistNode *node;
	PlaylistEntry *entry;
	
	PlaylistEntry::Dispose ();

	if (entries != NULL) {
		node = (PlaylistNode *) entries->First ();
		while (node != NULL) {
			entry = node->GetEntry ();
			if (entry != NULL)
				entry->Dispose ();
			node = (PlaylistNode *) node->next;
		}
	}
}

void
Playlist::Init (MediaElement *element)
{	
	LOG_PLAYLIST ("Playlist::Init (%p)\n", element);

	this->element = element;
	entries = new List ();
	current_node = NULL;
	source = NULL;
}

bool
Playlist::IsCurrentEntryLastEntry ()
{
	PlaylistEntry *entry;
	Playlist *pl;
	
	if (entries->Last () == NULL)
		return false;
		
	if (current_node != entries->Last ())
		return false;
		
	entry = GetCurrentEntry ();
	
	if (!entry->IsPlaylist ())
		return true;
		
	pl = (Playlist *) entry;
	
	return pl->IsCurrentEntryLastEntry ();
}

void
Playlist::Open ()
{
	PlaylistEntry *current_entry;

	LOG_PLAYLIST ("Playlist::Open ()\n");
	
	current_node = (PlaylistNode *) entries->First ();

	current_entry = GetCurrentEntry ();	
	
	while (current_entry && current_entry->HasDuration () && current_entry->GetDuration () == 0) {
		LOG_PLAYLIST ("Playlist::Open (), current entry (%s) has zero duration, skipping it.\n", current_entry->GetSourceName ()->ToString ());
		current_node = (PlaylistNode *) current_node->next;
		current_entry = GetCurrentEntry ();
	}
	
	if (current_entry)
		current_entry->Open ();

	LOG_PLAYLIST ("Playlist::Open (): current node: %p, current entry: %p\n", current_entry, GetCurrentEntry ());
}

void
Playlist::OnEntryEnded ()
{
	PlaylistEntry *current_entry;
	
	LOG_PLAYLIST ("Playlist::OnEntryEnded () current_node: %p\n", current_node);
	
	if (!current_node)
		return;
	
	current_node = (PlaylistNode *) current_node->next;
	
	current_entry = GetCurrentEntry ();
	if (current_entry) {
		element->SetPlayRequested ();
		current_entry->Play ();
	}
	
	LOG_PLAYLIST ("Playlist::OnEntryEnded () current_node: %p [Done]\n", current_node);
}

bool
Playlist::Play ()
{
	if (current_node == NULL)
		current_node = (PlaylistNode *) entries->First ();	

	PlaylistEntry *current_entry = GetCurrentEntry ();

	LOG_PLAYLIST ("Playlist::Play (), current entry: %p\n", current_entry);

	while (current_entry && current_entry->HasDuration () && current_entry->GetDuration () == 0) {
		LOG_PLAYLIST ("Playlist::Open (), current entry (%s) has zero duration, skipping it.\n", current_entry->GetSourceName ()->ToString ());
		OnEntryEnded ();
		current_entry = GetCurrentEntry ();
	}

	if (current_entry)
		return current_entry->Play ();

	return false;
}

bool
Playlist::Pause ()
{
	PlaylistEntry *current_entry = GetCurrentEntry ();

	LOG_PLAYLIST ("Playlist::Pause ()\n");

	if (!current_entry)
		return false;

	return current_entry->Pause ();
}

void
Playlist::Stop ()
{
	PlaylistEntry *current_entry = GetCurrentEntry ();

	LOG_PLAYLIST ("Playlist::Stop ()\n");

	if (!current_entry)
		return;

	current_entry->Stop ();
	
	current_node = NULL;

	if (GetParent () == NULL && !IsSingleFile ()) {
		element->Reinitialize (false);
		Open ();
	}
}

void
Playlist::PopulateMediaAttributes ()
{
	PlaylistEntry *current_entry = GetCurrentEntry ();

	LOG_PLAYLIST ("Playlist::PopulateMediaAttributes ()\n");

	if (!current_entry)
		return;

	current_entry->PopulateMediaAttributes ();
}

void
Playlist::AddEntry (PlaylistEntry *entry)
{
	LOG_PLAYLIST ("Playlist::AddEntry (%p)\n", entry);

	entries->Append (new PlaylistNode (entry));
	entry->unref ();
}

bool
Playlist::ReplaceCurrentEntry (Playlist *pl)
{
	PlaylistEntry *current_entry = GetCurrentEntry ();

	LOG_PLAYLIST ("Playlist::ReplaceCurrentEntry (%p)\n", pl);

	int counter = 0;
	PlaylistEntry *e = current_entry;
	while (e != NULL && e->IsPlaylist ()) {
		counter++;
		e = e->GetParent ();
		return false;
	}

	if (counter >= 5) {
		element->MediaFailed (new ErrorEventArgs (MediaError, 1001, "AG_E_UNKNOWN_ERROR"));
		return false;
	}

	if (current_entry->IsPlaylist ()) {
		return ((Playlist *) current_entry)->ReplaceCurrentEntry (pl);
	} else {
		PlaylistNode *pln = new PlaylistNode (pl);
		pl->MergeWith (current_entry);
		entries->InsertBefore (pln, current_node);
		entries->Remove (current_node);
		current_node = pln;
		return true;
	}
}

void
Playlist::MergeWith (PlaylistEntry *entry)
{
	LOG_PLAYLIST ("Playlist::MergeWith (%p)\n", entry);

	SetBase (entry->GetBase () ? entry->GetBase ()->Clone () : NULL);
	SetTitle (g_strdup (entry->GetTitle ()));
	SetAuthor (g_strdup (entry->GetAuthor ()));
	SetAbstract (g_strdup (entry->GetAbstract ()));
	SetCopyright (g_strdup (entry->GetCopyright ()));

	SetSourceName (entry->GetSourceName () ? entry->GetSourceName ()->Clone () : NULL);
	if (entry->HasDuration ()) 
		SetDuration (entry->GetDuration ());
	
	element = entry->GetElement ();
}

/*
 * PlaylistParser
 */

PlaylistParser::PlaylistParser (MediaElement *element, IMediaSource *source)
{
	this->element = element;
	this->source = source;
}

void
PlaylistParser::SetSource (IMediaSource *new_source)
{
	if (source)
		source->unref ();
	source = new_source;
}

void
PlaylistParser::Setup ()
{
	playlist = NULL;
	current_entry = NULL;
	current_text = NULL;

	was_playlist = false;

	internal = new ParserInternal ();
	kind_stack = new List ();
	PushCurrentKind (PlaylistNode::Root);

	XML_SetUserData (internal->parser, this);
	XML_SetElementHandler (internal->parser, on_start_element, on_end_element);
	XML_SetCharacterDataHandler (internal->parser, on_text);
}

void
PlaylistParser::Cleanup ()
{
	kind_stack->Clear (true);
	delete kind_stack;
	delete internal;
	if (playlist)
		playlist->unref ();
}

PlaylistParser::~PlaylistParser ()
{
	Cleanup ();
}

static bool
str_match (const char *candidate, const char *tag)
{
	return g_strcasecmp (candidate, tag) == 0;
}

void
PlaylistParser::on_start_element (gpointer user_data, const char *name, const char **attrs)
{
	((PlaylistParser *) user_data)->OnStartElement (name, attrs);
}

void
PlaylistParser::on_end_element (gpointer user_data, const char *name)
{
	((PlaylistParser *) user_data)->OnEndElement (name);
}

void
PlaylistParser::on_text (gpointer user_data, const char *data, int len)
{
	((PlaylistParser *) user_data)->OnText (data, len);
}

static bool
is_all_whitespace (const char *str)
{
	if (str == NULL)
		return true;

	for (int i = 0; str [i] != 0; i++) {
		switch (str [i]) {
		case 10:
		case 13:
		case ' ':
		case '\t':
			break;
		default:
			return false;
		}
	}
	return true;
}

// 
// To make matters more interesting, the format of the VALUE attribute in the STARTTIME tag isn't
// exactly the same as xaml's or javascript's TimeSpan format.
// 
// The time index, in hours, minutes, seconds, and hundredths of seconds.
// [[hh]:mm]:ss.fract
// 
// The parser seems to stop if it finds a second dot, returnning whatever it had parsed
// up till then
//
// At most 4 digits of fract is read, the rest is ignored (even if it's not numbers).
// 
static bool
parse_int (const char **pp, const char *end, int *result)
{
	const char *p = *pp;
	int res = 0;
	bool success = false;

	while (p <= end && isdigit (*p)) {
		res = res * 10 + *p - '0';
		p++;
	}

	success = *pp != p;
	
	*pp = p;
	*result = res;

	return success;
}

static bool
time_value_from_str (PlaylistParser *parser, const char *str, TimeSpan *res)
{
	const char *end = str + strlen (str);
	const char *p;

	int values [] = {0, 0, 0};
	int counter = 0;
	int hh = 0, mm = 0, ss = 0;
	int milliseconds = 0;
	int digits = 2;

	p = str;

	if (!isdigit (*p)) {
		parser->ParsingError (new ErrorEventArgs (MediaError, 2210, "AG_E_INVALID_ARGUMENT"));
		return false;
	}

	for (int i = 0; i < 3; i++) {
		if (!parse_int (&p, end, &values [i])) {
			parser->ParsingError (new ErrorEventArgs (MediaError, 2210, "AG_E_INVALID_ARGUMENT"));
			return false;
		}
		counter++;
		if (*p != ':') 
			break;
		p++;
	}
	
	if (*p == '.') {
		p++;
		while (digits >= 0 && isdigit (*p)) {
			milliseconds += pow (10, digits) * (*p - '0');
			p++;
			digits--;
		}
		if (counter == 3 && *p != 0 && !isdigit (*p)) {
			parser->ParsingError (new ErrorEventArgs (MediaError, 2210, "AG_E_INVALID_ARGUMENT"));
			return false;
		}
	}
	
	switch (counter) {
	case 1:
		ss = values [0];
		break;
	case 2:
		ss = values [1];
		mm = values [0];
		break;
	case 3:
		ss = values [2];
		mm = values [1];
		hh = values [0];
		break;
	default:
		parser->ParsingError (new ErrorEventArgs (MediaError, 2210, "AG_E_INVALID_ARGUMENT"));
		return false;		
	}

	gint64 ms = ((hh * 3600) + (mm * 60) + ss) * 1000 + milliseconds;
	TimeSpan result = TimeSpan_FromPts (MilliSeconds_ToPts (ms));

	*res = result;

	return true;
}

void
PlaylistParser::OnStartElement (const char *name, const char **attrs)
{
	PlaylistNode::Kind kind = StringToKind (name);
	Uri *uri;
	bool failed;

	LOG_PLAYLIST ("PlaylistParser::OnStartElement (%s, %p), kind = %d\n", name, attrs, kind);

	g_free (current_text);
	current_text = NULL;

	PushCurrentKind (kind);

	switch (kind) {
	case PlaylistNode::Abstract:
		if (attrs != NULL && attrs [0] != NULL)
			ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
		break;
	case PlaylistNode::Asx:
		// Here the kind stack should be: Root+Asx
		if (kind_stack->Length () != 2 || !AssertParentKind (PlaylistNode::Root)) {
			ParsingError (new ErrorEventArgs (MediaError, 3008, "ASX parse error"));
			return;
		}

		playlist = new Playlist (element, source);

		for (int i = 0; attrs [i] != NULL; i += 2) {
			if (str_match (attrs [i], "VERSION")) {
				if (str_match (attrs [i+1], "3")) {
					playlist_version = 3;
				} else if (str_match (attrs [i+1], "3.0")) {
					playlist_version = 3;
				} else {
					ParsingError (new ErrorEventArgs (MediaError, 3008, "ASX parse error"));
				}
			} else if (str_match (attrs [i], "BANNERBAR")) {
				ParsingError (new ErrorEventArgs (MediaError, 3007, "Unsupported ASX attribute"));
			} else if (str_match (attrs [i], "PREVIEWMODE")) {
				ParsingError (new ErrorEventArgs (MediaError, 3007, "Unsupported ASX attribute"));
			} else {
				ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
			}
		}
		break;
	case PlaylistNode::Author:
		if (attrs != NULL && attrs [0] != NULL)
			ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
		break;
	case PlaylistNode::Banner:
		if (attrs != NULL && attrs [0] != NULL)
			ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
		break;
	case PlaylistNode::Base:
		for (int i = 0; attrs [i] != NULL; i += 2) {
			if (str_match (attrs [i], "HREF")) {
				// TODO: What do we do with this value?
				if (GetCurrentContent () != NULL) {
					failed = false;
					uri = new Uri ();
					if (!uri->Parse (attrs [i+1], true)) {
						failed = true;
					} else if (uri->protocol == NULL) {
						failed = true;
					} else if (g_strcasecmp (uri->protocol, "http") && 
						   g_strcasecmp (uri->protocol, "https") && 
						   g_strcasecmp (uri->protocol, "mms") &&
						   g_strcasecmp (uri->protocol, "rtsp") && 
						   g_strcasecmp (uri->protocol, "rstpt")) {
						failed = true;
					}

					if (!failed) {
						GetCurrentContent ()->SetBase (uri);
					} else {
						delete uri;
						ParsingError (new ErrorEventArgs (MediaError, 1001, "AG_E_UNKNOWN_ERROR"));
					}
					uri = NULL;
				}
			} else {
				ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
				break;
			}
		}
		break;
	case PlaylistNode::Copyright:
		if (attrs != NULL && attrs [0] != NULL)
			ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
		break;
	case PlaylistNode::Duration: {
		TimeSpan ts;
		for (int i = 0; attrs [i] != NULL; i += 2) {
			if (str_match (attrs [i], "VALUE")) {
				if (time_value_from_str (this, attrs [i+1], &ts)) {
					if (GetCurrentEntry () != NULL && GetParentKind () != PlaylistNode::Ref) {
						LOG_PLAYLIST ("PlaylistParser::OnStartElement (%s, %p), found VALUE/timespan = %f s\n", name, attrs, TimeSpan_ToSecondsFloat (ts));
						GetCurrentEntry ()->SetDuration (ts);
					}
				}
			} else {
				ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
				break;
			}
		}
		break;
	}
	case PlaylistNode::Entry: {
		bool client_skip = true;
		for (int i = 0; attrs [i] != NULL; i += 2) {
			if (str_match (attrs [i], "CLIENTSKIP")) {
				// TODO: What do we do with this value?
				if (str_match (attrs [i+1], "YES")) {
					client_skip = true;
				} else if (str_match (attrs [i+1], "NO")) {
					client_skip = false;
				} else {
					ParsingError (new ErrorEventArgs (MediaError, 3008, "ASX parse error"));
					break;
				}
			} else if (str_match (attrs [i], "SKIPIFREF")) {
				ParsingError (new ErrorEventArgs (MediaError, 3007, "Unsupported ASX attribute"));
				break;
			} else {
				ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
				break;
			}
		}
		PlaylistEntry *entry = new PlaylistEntry (element, playlist);
		entry->SetClientSkip (client_skip);
		playlist->AddEntry (entry);
		current_entry = entry;
		break;
	}
	case PlaylistNode::EntryRef: {
		char *href = NULL;
		for (int i = 0; attrs [i] != NULL; i += 2) {
			if (str_match (attrs [i], "HREF")) {
				if (href == NULL)
					href = g_strdup (attrs [i+1]);
			// Docs says this attribute isn't unsupported, but an error is emitted.
			//} else if (str_match (attrs [i], "CLIENTBIND")) {
			//	// TODO: What do we do with this value?
			} else {
				ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
				break;
			}
		}

		if (href) {
			uri = new Uri ();
			if (!uri->Parse (href)) {
				delete uri;
				uri = NULL;
				ParsingError (new ErrorEventArgs (MediaError, 1001, "AG_E_UNKNOWN_ERROR"));
			}
		}

		PlaylistEntry *entry = new PlaylistEntry (element, playlist);
		if (uri)
			entry->SetSourceName (uri);
		uri = NULL;
		playlist->AddEntry (entry);
		current_entry = entry;
		break;
	}
	case PlaylistNode::LogUrl:
		if (attrs != NULL && attrs [0] != NULL)
			ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
		break;
	case PlaylistNode::MoreInfo:
		for (int i = 0; attrs [i] != NULL; i += 2) {
			if (str_match (attrs [i], "HREF")) {
				if (GetCurrentEntry () != NULL)
					GetCurrentEntry ()->SetInfoURL (g_strdup (attrs [i+1]));
			} else if (str_match (attrs [i], "TARGET")) {
				if (GetCurrentEntry () != NULL)
					GetCurrentEntry ()->SetInfoTarget (g_strdup (attrs [i+1]));
			} else {
				ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
				break;
			}
		}
		break;
	case PlaylistNode::StartTime: {
		TimeSpan ts;
		for (int i = 0; attrs [i] != NULL; i += 2) {
			if (str_match (attrs [i], "VALUE")) {
				if (time_value_from_str (this, attrs [i+1], &ts)) {
					if (GetCurrentEntry () != NULL && GetParentKind () != PlaylistNode::Ref) {
						LOG_PLAYLIST ("PlaylistParser::OnStartElement (%s, %p), found VALUE/timespan = %f s\n", name, attrs, TimeSpan_ToSecondsFloat (ts));
						GetCurrentEntry ()->SetStartTime (ts);
					}
				}
			} else {
				ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
				break;
			}
		}

		break;
	}
	case PlaylistNode::Ref: {
		for (int i = 0; attrs [i] != NULL; i += 2) {
			if (str_match (attrs [i], "HREF")) {
				if (GetCurrentEntry () != NULL && GetCurrentEntry ()->GetSourceName () == NULL) {
					uri = new Uri ();
					if (uri->Parse (attrs [i+1])) {
						GetCurrentEntry ()->SetSourceName (uri);
					} else {
						delete uri;
						ParsingError (new ErrorEventArgs (MediaError, 1001, "AG_E_UNKNOWN_ERROR"));
					}
					uri = NULL;
				}
			} else {
				ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
				break;
			}
		}
		break;
	}
	case PlaylistNode::Title:
		if (attrs != NULL && attrs [0] != NULL)
			ParsingError (new ErrorEventArgs (MediaError, 3005, "Invalid ASX attribute"));
		break;
	case PlaylistNode::StartMarker:
	case PlaylistNode::EndMarker:
	case PlaylistNode::Repeat:
	case PlaylistNode::Param:
	case PlaylistNode::Event:
		ParsingError (new ErrorEventArgs (MediaError, 3006, "Unsupported ASX element"));
		break;
	case PlaylistNode::Root:
	case PlaylistNode::Unknown:
	default:
		LOG_PLAYLIST ("PlaylistParser::OnStartElement ('%s', %p): Unknown kind: %d\n", name, attrs, kind);
		ParsingError (new ErrorEventArgs (MediaError, 3004, "Invalid ASX element"));
		break;
	}
}

void
PlaylistParser::OnEndElement (const char *name)
{
	PlaylistNode::Kind kind = GetCurrentKind ();
	TimeSpan ts; 

	LOG_PLAYLIST ("PlaylistParser::OnEndElement (%s), GetCurrentKind (): %d, GetCurrentKind () to string: %s\n", name, kind, KindToString (kind));

	switch (kind) {
	case PlaylistNode::Abstract:
		if (!AssertParentKind (PlaylistNode::Asx | PlaylistNode::Entry))
			break;
		if (GetCurrentContent () != NULL)
			GetCurrentContent ()->SetAbstract (current_text);
		current_text = NULL;
		break;
	case PlaylistNode::Author:
		if (!AssertParentKind (PlaylistNode::Asx | PlaylistNode::Entry))
			break;
		if (GetCurrentContent () != NULL)
			GetCurrentContent ()->SetAuthor (current_text);
		current_text = NULL;
		break;
	case PlaylistNode::Base:
		if (!AssertParentKind (PlaylistNode::Asx | PlaylistNode::Entry))
			break;
		break;
	case PlaylistNode::Copyright:
		if (!AssertParentKind (PlaylistNode::Asx | PlaylistNode::Entry))
			break;
		if (GetCurrentContent () != NULL)
			GetCurrentContent ()->SetCopyright (current_text);
		current_text = NULL;
		break;
	case PlaylistNode::Duration:
		if (!AssertParentKind (PlaylistNode::Entry | PlaylistNode::Ref))
			break;
		if (current_text == NULL)
			break;
		time_span_from_str (current_text, &ts);
		if (GetCurrentEntry () != NULL)
			GetCurrentEntry ()->SetDuration (ts);
		break;
	case PlaylistNode::Entry:
		if (!AssertParentKind (PlaylistNode::Asx))
			break;
		break;
	case PlaylistNode::EntryRef:
		if (!AssertParentKind (PlaylistNode::Asx))
			break;
		break;
	case PlaylistNode::StartTime:
		if (!AssertParentKind (PlaylistNode::Entry | PlaylistNode::Ref))
			break;
		if (!is_all_whitespace (current_text)) {
			ParsingError (new ErrorEventArgs (MediaError, 3008, "ASX parse error"));
		}
		break;
	case PlaylistNode::Title:
		if (!AssertParentKind (PlaylistNode::Asx | PlaylistNode::Entry))
			break;
		if (GetCurrentContent () != NULL)
			GetCurrentContent ()->SetTitle (current_text);
		current_text = NULL;
		break;
	case PlaylistNode::Asx:
		if (playlist_version == 3)
			was_playlist = true;
		if (!AssertParentKind (PlaylistNode::Root))
			break;
		break;
	case PlaylistNode::Ref:
		if (!AssertParentKind (PlaylistNode::Entry))
			break;
		if (!is_all_whitespace (current_text)) {
			ParsingError (new ErrorEventArgs (MediaError, 3008, "ASX parse error"));
		}
		break;
	case PlaylistNode::MoreInfo:
		if (!AssertParentKind (PlaylistNode::Asx | PlaylistNode::Entry))
			break;
		if (!is_all_whitespace (current_text)) {
			ParsingError (new ErrorEventArgs (MediaError, 3008, "ASX parse error"));
		}
		break;
	default:
		LOG_PLAYLIST ("PlaylistParser::OnEndElement ('%s'): Unknown kind %d.\n", name, kind);
		ParsingError (new ErrorEventArgs (MediaError, 3008, "ASX parse error"));
		break;
	}
	
	if (current_text != NULL) {	
		g_free (current_text);
		current_text = NULL;
	}

	switch (GetCurrentKind ()) {
	case PlaylistNode::Entry:
		EndEntry ();
		break;
	default:
		break;
	}
	PopCurrentKind ();
}

void
PlaylistParser::OnText (const char *text, int len)
{
	char *a = g_strndup (text, len);

#if DEBUG
	char *p = g_strndup (text, len);
	for (int i = 0; p [i] != 0; i++)
		if (p [i] == 10 || p [i] == 13)
			p [i] = ' ';

	LOG_PLAYLIST ("PlaylistParser::OnText (%s, %d)\n", p, len);
	g_free (p);
#endif

	if (current_text == NULL) {
		current_text = a;
	} else {
		char *b = g_strconcat (current_text, a, NULL);
		g_free (current_text);
		current_text = b;
	}
}

bool
PlaylistParser::IsASX3 (IMediaSource *source)
{
	static const char *asx_header = "<ASX";
	int asx_header_length = strlen (asx_header);
	char buffer [20];
	
	if (!source->Peek (buffer, asx_header_length))
		return false;
		
	return strncmp (asx_header, buffer, asx_header_length) == 0;
}

bool
PlaylistParser::IsASX2 (IMediaSource *source)
{
	static const char *asx2_header = "[Reference]";
	int asx2_header_length = strlen (asx2_header);
	char buffer [20];
	
	if (!source->Peek (buffer, asx2_header_length))
		return false;
		
	return strncmp (asx2_header, buffer, asx2_header_length) == 0;
}

bool
PlaylistParser::ParseASX2 ()
{
	const int BUFFER_SIZE = 1024;
	int bytes_read;
	char buffer[BUFFER_SIZE];
	char *ref;
	char *mms_uri;
	GKeyFile *key_file;
	Uri *uri;
	
	playlist_version = 2;

	bytes_read = source->ReadSome (buffer, BUFFER_SIZE);
	if (bytes_read < 0) {
		LOG_PLAYLIST_WARN ("Could not read asx document for parsing.\n");
		return false;
	}

	key_file = g_key_file_new ();
	if (!g_key_file_load_from_data (key_file, buffer, bytes_read,
					G_KEY_FILE_NONE, NULL)) {
		LOG_PLAYLIST_WARN ("Invalid asx2 document.\n");
		g_key_file_free (key_file);
		return false;
	}

	ref = g_key_file_get_value (key_file, "Reference", "Ref1", NULL);
	if (ref == NULL) {
		LOG_PLAYLIST_WARN ("Could not find Ref1 entry in asx2 document.\n");
		g_key_file_free (key_file);
		return false;
	}

	if (!g_str_has_prefix (ref, "http://") || !g_str_has_suffix (ref, "MSWMExt=.asf")) {
		LOG_PLAYLIST_WARN ("Could not find a valid uri within Ref1 entry in asx2 document.\n");
		g_free (ref);
		g_key_file_free (key_file);
		return false;
	}

	mms_uri = g_strdup_printf ("mms://%s", strstr (ref, "http://") + strlen ("http://"));
	g_free (ref);
	g_key_file_free (key_file);


	playlist = new Playlist (element, source);

	PlaylistEntry *entry = new PlaylistEntry (element, playlist);
	uri = new Uri ();
	if (uri->Parse (mms_uri)) {
		entry->SetSourceName (uri);
	} else {
		delete uri;
	}
	playlist->AddEntry (entry);
	current_entry = entry;

	return true;
}

bool
PlaylistParser::TryFixError (gint8 *current_buffer, int bytes_read)
{
	if (XML_GetErrorCode (internal->parser) != XML_ERROR_INVALID_TOKEN)
		return false;

	int index = XML_GetErrorByteIndex (internal->parser);

	if (index > bytes_read)
		return false;
	
	LOG_PLAYLIST ("Attempting to fix invalid token error  %d.\n", index);

	// OK, so we are going to guess that we are in an attribute here and walk back
	// until we hit a control char that should be escaped.
	char * escape = NULL;
	while (index >= 0) {
		switch (current_buffer [index]) {
		case '&':
			escape = g_strdup ("&amp;");
			break;
		case '<':
			escape = g_strdup ("&lt;");
			break;
		case '>':
			escape = g_strdup ("&gt;");
			break;
		case '\"':
			break;
		}
		if (escape)
			break;
		index--;
	}

	if (!escape) {
		LOG_PLAYLIST_WARN ("Unable to find an invalid escape character to fix in ASX: %s.\n", current_buffer);
		g_free (escape);
		return false;
	}

	int escape_len = strlen (escape);
	int new_size = source->GetSize () + escape_len - 1;
	int patched_size = internal->bytes_read + bytes_read + escape_len - 1;
	gint8 * new_buffer = (gint8 *) g_malloc (new_size);

	source->Seek (0, SEEK_SET);
	source->ReadSome (new_buffer, internal->bytes_read);

	memcpy (new_buffer + internal->bytes_read, current_buffer, index);
	memcpy (new_buffer + internal->bytes_read + index, escape, escape_len);
	memcpy (new_buffer + internal->bytes_read + index + escape_len, current_buffer + index + 1, bytes_read - index - 1); 
	
	source->Seek (internal->bytes_read + bytes_read, SEEK_SET);
	source->ReadSome (new_buffer + patched_size, new_size - patched_size);

	MemorySource *reparse_source = new MemorySource (source->GetMedia (), new_buffer, new_size);
	SetSource (reparse_source);

	internal->reparse = true;

	g_free (escape);

	return true;
}

MediaResult
PlaylistParser::Parse ()
{
	bool result;
	gint64 last_available_pos;
	gint64 size;

	LOG_PLAYLIST ("PlaylistParser::Parse ()\n");

	do {
		Setup ();

		// Don't try to parse anything until we have all the data.
		size = source->GetSize ();
		last_available_pos = source->GetLastAvailablePosition ();
		if (size != -1 && last_available_pos != -1 && size != last_available_pos)
			return MEDIA_NOT_ENOUGH_DATA; 

		if (!this->IsASX3 (source) && this->IsASX2 (source)) {
			/* Parse as a asx2 mms file */
			result = this->ParseASX2 ();
		} else {
			result = this->ParseASX3 ();
		}

		if (internal->reparse)
			Cleanup ();
	} while (internal->reparse);

	return result ? MEDIA_SUCCESS : MEDIA_FAIL;
}

bool
PlaylistParser::ParseASX3 ()
{
	int bytes_read;
	void *buffer;

// asx documents don't tend to be very big, so there's no need for a big buffer
	const int BUFFER_SIZE = 1024;

	for (;;) {
		buffer = XML_GetBuffer(internal->parser, BUFFER_SIZE);
		if (buffer == NULL) {
			fprintf (stderr, "Could not allocate memory for asx document parsing.\n");
			return false;
		}
		
		bytes_read = source->ReadSome (buffer, BUFFER_SIZE);
		if (bytes_read < 0) {
			fprintf (stderr, "Could not read asx document for parsing.\n");
			return false;
		}

		if (!XML_ParseBuffer (internal->parser, bytes_read, bytes_read == 0)) {
			if (!TryFixError ((gint8 *) buffer, bytes_read))
				ParsingError (new ErrorEventArgs (MediaError, 3000, g_strdup_printf ("%s  (%d, %d)", XML_ErrorString (XML_GetErrorCode (internal->parser)),
												     (int) XML_GetCurrentLineNumber (internal->parser),
												     (int) XML_GetCurrentColumnNumber (internal->parser))));
			return false;
		}

		if (bytes_read == 0)
			break;

		internal->bytes_read += bytes_read;
	}

	return playlist != NULL;
}

PlaylistEntry *
PlaylistParser::GetCurrentContent ()
{
	if (current_entry != NULL)
		return current_entry;

	return playlist;
}

PlaylistEntry *
PlaylistParser::GetCurrentEntry ()
{
	return current_entry;
}

void 
PlaylistParser::EndEntry ()
{
	this->current_entry = NULL;
}

void
PlaylistParser::PushCurrentKind (PlaylistNode::Kind kind)
{
	kind_stack->Append (new KindNode (kind));
	LOG_PLAYLIST ("PlaylistParser::Push (%d)\n", kind);
}

void
PlaylistParser::PopCurrentKind ()
{
	LOG_PLAYLIST ("PlaylistParser::PopCurrentKind (), current: %d\n", ((KindNode *)kind_stack->Last ())->kind);
	kind_stack->Remove (kind_stack->Last ());
}

PlaylistNode::Kind
PlaylistParser::GetCurrentKind ()
{
	KindNode *node = (KindNode *) kind_stack->Last ();
	return node->kind;
}

PlaylistNode::Kind
PlaylistParser::GetParentKind ()
{
	KindNode *node = (KindNode *) kind_stack->Last ()->prev;
	return node->kind;
}

bool
PlaylistParser::AssertParentKind (int kind)
{
	LOG_PLAYLIST ("PlaylistParser::AssertParentKind (%d), GetParentKind: %d, result: %d\n", kind, GetParentKind (), GetParentKind () & kind);

	if (GetParentKind () & kind)
		return true;

	ParsingError (new ErrorEventArgs (MediaError, 3008, "ASX parse error"));
	
	return false;
}

void
PlaylistParser::ParsingError (ErrorEventArgs *args)
{
	LOG_PLAYLIST ("PlaylistParser::ParsingError (%s)\n", args->error_message);
	
	XML_StopParser (internal->parser, false);
	element->MediaFailed (args);
}


PlaylistParser::PlaylistKind PlaylistParser::playlist_kinds [] = {
	PlaylistParser::PlaylistKind ("ABSTRACT", PlaylistNode::Abstract), 
	PlaylistParser::PlaylistKind ("ASX", PlaylistNode::Asx),
	PlaylistParser::PlaylistKind ("ROOT", PlaylistNode::Root),
	PlaylistParser::PlaylistKind ("AUTHOR", PlaylistNode::Author),
	PlaylistParser::PlaylistKind ("BANNER", PlaylistNode::Banner),
	PlaylistParser::PlaylistKind ("BASE", PlaylistNode::Base),
	PlaylistParser::PlaylistKind ("COPYRIGHT", PlaylistNode::Copyright),
	PlaylistParser::PlaylistKind ("DURATION", PlaylistNode::Duration),
	PlaylistParser::PlaylistKind ("ENTRY", PlaylistNode::Entry),
	PlaylistParser::PlaylistKind ("ENTRYREF", PlaylistNode::EntryRef),
	PlaylistParser::PlaylistKind ("LOGURL", PlaylistNode::LogUrl),
	PlaylistParser::PlaylistKind ("MOREINFO", PlaylistNode::MoreInfo),
	PlaylistParser::PlaylistKind ("REF", PlaylistNode::Ref),
	PlaylistParser::PlaylistKind ("STARTTIME", PlaylistNode::StartTime),
	PlaylistParser::PlaylistKind ("TITLE", PlaylistNode::Title),
	PlaylistParser::PlaylistKind ("STARTMARKER", PlaylistNode::StartMarker),
	PlaylistParser::PlaylistKind ("REPEAT", PlaylistNode::Repeat),
	PlaylistParser::PlaylistKind ("ENDMARKER", PlaylistNode::EndMarker),
	PlaylistParser::PlaylistKind ("PARAM", PlaylistNode::Param),
	PlaylistParser::PlaylistKind ("EVENT", PlaylistNode::Event),
	PlaylistParser::PlaylistKind (NULL, PlaylistNode::Unknown)
};

PlaylistNode::Kind
PlaylistParser::StringToKind (const char *str)
{
	PlaylistNode::Kind kind = PlaylistNode::Unknown;

	for (int i = 0; playlist_kinds [i].str != NULL; i++) {
		if (str_match (str, playlist_kinds [i].str)) {
			kind = playlist_kinds [i].kind;
			break;
		}
	}

	LOG_PLAYLIST ("PlaylistParser::StringToKind ('%s') = %d\n", str, kind);

	return kind;
}

const char *
PlaylistParser::KindToString (PlaylistNode::Kind kind)
{
	const char *result = NULL;

	for (int i = 0; playlist_kinds [i].str != NULL; i++) {
		if (playlist_kinds [i].kind == kind) {
			result = playlist_kinds [i].str;
			break;
		}
	}

	LOG_PLAYLIST ("PlaylistParser::KindToString (%d) = '%s'\n", kind, result);

	return result;
}

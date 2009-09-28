/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#ifndef MOON_CLIPBOARD_GTK_H
#define MOON_CLIPBOARD_GTK_H

#include <glib.h>
#include <gtk/gtk.h>

#include "window-gtk.h"
#include "pal.h"

class MoonClipboardGtk : public MoonClipboard {
public:
	MoonClipboardGtk (MoonWindowGtk *win);

	void SetSelection (const char *text, int length);

	virtual void SetText (const char *text, int length);
	virtual void AsyncGetText (MoonClipboardGetTextCallback cb, gpointer data);
	virtual char* GetText ();

private:
	static void async_get_text (GtkClipboard *clipboard, const char *text, gpointer data);
	GtkClipboard *clipboard;
};

#endif

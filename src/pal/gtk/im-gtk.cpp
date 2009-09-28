/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#include "im-gtk.h"

#include <gtk/gtkimmulticontext.h>

MoonIMContextGtk::MoonIMContextGtk ()
{
	im = gtk_im_multicontext_new ();
}

MoonIMContextGtk::~MoonIMContextGtk ()
{
	g_object_unref (im);
	im = NULL;
}


void
MoonIMContextGtk::SetUsePreedit (bool flag)
{
	gtk_im_context_set_use_preedit (im, flag);
}

void
MoonIMContextGtk::SetClientWindow (MoonWindow* window)
{
	gtk_im_context_set_client_window (im, GTK_WIDGET(window->GetPlatformWindow ())->window);
}

bool
MoonIMContextGtk::FilterKeyPress (MoonKeyEvent* event)
{
	return gtk_im_context_filter_keypress (im, (GdkEventKey*)event->GetPlatformEvent());
}

void
MoonIMContextGtk::SetSurroundingText (const char *text, int offset, int length)
{
	gtk_im_context_set_surrounding (im, text, offset, length);
}

void
MoonIMContextGtk::Reset ()
{
	gtk_im_context_reset (im);
}


void
MoonIMContextGtk::FocusIn ()
{
	gtk_im_context_focus_in (im);
}

void
MoonIMContextGtk::FocusOut ()
{
	gtk_im_context_focus_out (im);
}

void
MoonIMContextGtk::SetCursorLocation (Rect r)
{
	GdkRectangle area;
	Rect rect;

	area = r.ToGdkRectangle ();

	gtk_im_context_set_cursor_location (im, &area);
}

void
MoonIMContextGtk::SetRetrieveSurroundingCallback (MoonCallback cb, gpointer data)
{
	g_signal_connect (im, "retrieve-surrounding", G_CALLBACK (cb), data);
}

void
MoonIMContextGtk::SetDeleteSurroundingCallback (MoonCallback cb, gpointer data)
{
	g_signal_connect (im, "delete-surrounding", G_CALLBACK (cb), data);
}

void
MoonIMContextGtk::SetCommitCallback (MoonCallback cb, gpointer data)
{
	g_signal_connect (im, "commit", G_CALLBACK (cb), data);
}

gpointer
MoonIMContextGtk::GetPlatformIMContext ()
{
	return im;
}


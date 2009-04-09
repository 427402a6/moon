/*
 * openfile.cpp: File open / save interfaces
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007, 2009 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */
#include <config.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include "openfile.h"

// FIXME: Silverlight 1.1 (alpha, refresh) doesn't allow the selection(*) of
// symlinks on Mac OSX. (*) actually it de-selects them on the fly. We should
// be able to duplicate this with lstat and the selection-changed event.

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

char **
open_file_dialog_show (const char *title, bool multsel, const char *filter, int idx)
{
	GtkWidget *widget = gtk_file_chooser_dialog_new (title, NULL, 
					    GTK_FILE_CHOOSER_ACTION_OPEN, 
					    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	GtkFileChooser *chooser = GTK_FILE_CHOOSER (widget);
	set_filters (chooser, filter, idx);
	gtk_file_chooser_set_select_multiple (chooser, multsel ? TRUE : FALSE);

	char **ret = NULL;
	if (gtk_dialog_run (GTK_DIALOG (widget)) == GTK_RESPONSE_ACCEPT){
		GSList *k, *l = gtk_file_chooser_get_filenames (chooser);
		int i, count = g_slist_length (l);

		ret = g_new (char *, count + 1);
		ret [count] = NULL;
		
		for (i = 0, k = l; k; k = k->next)
			ret [i++] = (char *) k->data;

		g_slist_free (l);
	}

	gtk_widget_destroy (widget);

	return ret;
}

char *
save_file_dialog_show (const char *title, const char *filter, int idx)
{
	GtkWidget *widget = gtk_file_chooser_dialog_new (title, NULL, 
					    GTK_FILE_CHOOSER_ACTION_SAVE, 
					    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	GtkFileChooser *chooser = GTK_FILE_CHOOSER (widget);
	set_filters (chooser, filter, idx);
	gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

	char *ret = NULL;
	if (gtk_dialog_run (GTK_DIALOG (widget)) == GTK_RESPONSE_ACCEPT)
		ret = gtk_file_chooser_get_filename (chooser);

	gtk_widget_destroy (widget);

	return ret;
}


/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * debug-ui.h: debugging/inspection support for gtk+
 *
 * Copyright 2008 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 *
 */

#ifndef __MOON_DEBUG_UI_H__
#define __MOON_DEBUG_UI_H__

#include "window-gtk.h"

G_BEGIN_DECLS

void show_debug (MoonWindowGtk *window);
void show_sources (MoonWindowGtk *window);

G_END_DECLS

#endif /* __MOON_DEBUG_UI_H__ */

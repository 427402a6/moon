/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * security.h: 
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2009 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#ifndef __SECURITY_H__
#define __SECURITY_H__

#include <string.h>
#include <glib.h>
#include <sys/stat.h>

#include "dependencyobject.h"

/* @Namespace=System.Windows */
class SecuritySettings : public DependencyObject {
protected:
	virtual ~SecuritySettings () {}

public:
	/* @GeneratePInvoke,GenerateCBinding */
	SecuritySettings () {}

	/* @PropertyType=gint32,ManagedPropertyType=ElevatedPermissions,GenerateAccessors */
	const static int ElevatedPermissionsProperty;

	gint32 GetElevatedPermissions ();
	void SetElevatedPermissions (gint32 value);
};

G_BEGIN_DECLS

G_GNUC_INTERNAL void security_enable_coreclr (const char *platform_dir);

G_END_DECLS

#endif


AC_DEFUN([MOONLIGHT_CHECK_BUILD_CONFIGURATION],
[
	dnl
	dnl debug checks
	dnl
	
	AC_ARG_WITH(debug, AC_HELP_STRING([--with-debug=yes|no],
		[If you want to enable debug support (default=yes)]),
		[], [with_debug=yes])

	if test x$with_debug = xyes; then
		DEBUG_OPTIONS="-fno-inline -g -fno-inline-functions"
		CFLAGS=`echo $CFLAGS | sed 's/-O2//'`
		CFLAGS="$DEBUG_OPTIONS $CFLAGS"
		CXXFLAGS=`echo $CXXFLAGS | sed 's/-O2//'`
		CXXFLAGS="$DEBUG_OPTIONS $CXXFLAGS"
		
		if test x$managed_code = xyes; then
			MOON_LIBS="$MOON_LIBS $MONO_LIBS"
		fi
		
		AC_DEFINE([DEBUG],[1],[Include debugging support])
	fi
	AM_CONDITIONAL(DEBUG,test x$with_debug = xyes)

	dnl
	dnl sanity checks
	dnl

	AC_ARG_WITH(sanity-checks, AC_HELP_STRING([--with-sanity-checks=yes|no],
		[If you want to enable sanity checks (default=no)]),
		[], [with_sanity_checks=no])
	if test x$with_sanity_checks = xyes; then
  		AC_DEFINE([SANITY], [1], [Include sanity checks])
	fi
	AM_CONDITIONAL(SANITY,test x$with_sanity_checks = xyes)

	dnl 
	dnl object tracking
	dnl

	AC_ARG_WITH(object-tracking, AC_HELP_STRING([--with-object-tracking=yes|no],
		[If you want to enable object tracking (default=no)]),
		[], [with_object_tracking=no])

	if test x$with_object_tracking = xyes; then
		AC_DEFINE([OBJECT_TRACKING], [1], [Include object tracking])
	fi

	dnl
	dnl logging
	dnl

	AC_ARG_WITH(logging, AC_HELP_STRING([--with-logging=yes|no],
		[If you want to enable support for logging with MOONLIGHT_DEBUG (default=yes)]),
		[], [with_logging=yes])

	if test x$with_logging = xyes; then
		AC_DEFINE([LOGGING], [1], [Include support for logging with MOONLIGHT_DEBUG])
	fi

	dnl
	dnl build mono with moon
	dnl

	AC_ARG_WITH(manual-mono, AC_HELP_STRING([--with-manual-mono=yes|no],
		[If you want to build mono automatically (default=yes)]),
		[], [with_manual_mono=no])

	AM_CONDITIONAL(BUILD_MONO,test x$with_manual_mono = xno)
])


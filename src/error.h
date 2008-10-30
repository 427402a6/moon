/*
 * error.h: ErrorEventArgs and its subclasses
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#ifndef __MOON_ERROR_H__
#define __MOON_ERROR_H__

class ErrorEventArgs;


#include "enums.h"
#include "eventargs.h"

/* @Namespace=None,ManagedDependencyProperties=None */
class ErrorEventArgs : public EventArgs  {
protected:
	virtual ~ErrorEventArgs ()
	{
		g_free (error_message);
	}


public:
	ErrorEventArgs (ErrorType type, int code, const char *msg)
	{
		error_type = type;
		error_code = code;
		error_message = g_strdup (msg);
	}
	
	virtual Type::Kind GetObjectType () { return Type::ERROREVENTARGS; };

	int error_code;
	char *error_message;
	ErrorType error_type;
};

/* @Namespace=None,ManagedDependencyProperties=None */
class ImageErrorEventArgs : public ErrorEventArgs {
protected:
	virtual ~ImageErrorEventArgs () {}

public:
	ImageErrorEventArgs (const char *msg)
		: ErrorEventArgs (ImageError, 0, msg)
	{
	}
	virtual Type::Kind GetObjectType () { return Type::IMAGEERROREVENTARGS; };
};

/* @Namespace=None,ManagedDependencyProperties=None */
class ParserErrorEventArgs : public ErrorEventArgs {
protected:
	virtual ~ParserErrorEventArgs ()
	{
		g_free (xaml_file);
		g_free (xml_element);
		g_free (xml_attribute);
	}


public:
	ParserErrorEventArgs (const char *msg, const char *file,
			      int line, int column, int error_code, 
			      const char *element, const char *attribute)
		: ErrorEventArgs (ParserError, error_code, msg)
	{
		xml_attribute = g_strdup (attribute);
		xml_element = g_strdup (element);
		xaml_file = g_strdup (file);
		char_position = column;
		line_number = line;
	}
	
	virtual Type::Kind GetObjectType () { return Type::PARSERERROREVENTARGS; };
	
	int char_position;
	int line_number;
	char *xaml_file;
	char *xml_element;
	char *xml_attribute;
};

class MoonError {
public:
	enum ErrorType {
		NO_ERROR = 0,
		EXCEPTION = 1,
		ARGUMENT = 2,
		ARGUMENT_NULL = 3,
		ARGUMENT_OUT_OF_RANGE = 4,
		INVALID_OPERATION = 5,
		XAML_PARSE_EXCEPTION = 6
	};

	// non-zero if an error occurred.
	ErrorType number;

	// the silverlight error code
	int code;

	// the caller of the method which returned the error must call Dispose to free this value
	// (only necessary if there were any errors)
	char *message;
	
	MoonError () : number ((ErrorType)0), code (0), message (0) {}
	~MoonError ();
	
	void Clear () { number = NO_ERROR; code = 0; g_free (message); message = NULL; }
	
	static void FillIn (MoonError *error, ErrorType type, int code, char *message /* this message must be allocated using glib methods */);
  	static void FillIn (MoonError *error, ErrorType type, int code, const char *message);

	static void FillIn (MoonError *error, ErrorType type, char *message /* this message must be allocated using glib methods */);
  	static void FillIn (MoonError *error, ErrorType type, const char *message);
};

#endif /* __MOON_ERROR_H__ */

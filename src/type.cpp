/*
 * type.cpp: Our type system
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#include <config.h>
#include <string.h>
#include <stdlib.h>

#include "type.h"
#include "runtime.h"
#include "deployment.h"
#include "dependencyproperty.h"

/*
 * Type implementation
 */
Type::Type (Type::Kind type, Type::Kind parent, bool value_type, const char *name, 
		const char *kindname, int event_count, int total_event_count, const char **events, 
		create_inst_func *create_inst, const char *content_property)
{
	this->type = type;
	this->parent = parent;
	this->value_type = value_type;
	this->name = name;
	this->kindname = kindname;
	this->event_count = event_count;
	this->total_event_count = total_event_count;
	this->events = events;
	this->create_inst = create_inst;
	this->content_property = content_property;
	this->properties = NULL;
}
		
Type::~Type ()
{
	if (properties) {
		g_hash_table_destroy (properties);
		properties = NULL;
	}
}

const char *
Type::LookupEventName (int id)
{
	Type *parent_type = Type::Find (parent);
	int parent_event_count = (parent_type == NULL ? 0 : parent_type->total_event_count);
	int current_id;
	const char *result;
	
	if (id < 0)
		return "";
		
	if (events != NULL) {
		for (int i = 0; events [i] != NULL; i++) {
			current_id = i + parent_event_count;
			if (current_id == id)
				return events [i];
		}
	}
	
	if (parent == Type::INVALID || parent_type == NULL) {
		printf ("Event lookup of event id %i in type '%s' failed.\n", id, name);
		return NULL;
	}
	
	result = parent_type->LookupEventName (id);

	if (result == NULL)
		printf ("Event lookup of event %i in (more exactly) type '%s' failed.\n", id, name);

	return result;
}

int
Type::LookupEvent (const char *event_name)
{
	Type *parent_type = Type::Find (parent);
	int result;

	if (events != NULL) {
		for (int i = 0; events [i] != NULL; i++) {
			if (!g_ascii_strcasecmp (events [i], event_name))
				return i + (parent_type == NULL ? 0 : parent_type->total_event_count);
		}
	}
	
	if (parent == Type::INVALID || parent_type == NULL) {
		printf ("Event lookup of event '%s' in type '%s' failed.\n", event_name, name);
		return -1;
	}

	result = parent_type->LookupEvent (event_name);

	if (result == -1)
		printf ("Event lookup of event '%s' in (more exactly) type '%s' failed.\n", event_name, name);

	return result;
}

bool
Type::IsSubclassOf (Type::Kind type, Type::Kind super)
{
	return Deployment::GetCurrent ()->GetTypes ()->IsSubclassOf (type, super);
}

bool 
Type::IsSubclassOf (Type::Kind super)
{
	return Deployment::GetCurrent ()->GetTypes ()->IsSubclassOf (type, super);
}

bool
Types::IsSubclassOf (Type::Kind type, Type::Kind super)
{
	Type *t;
	Type::Kind parent;
	
	if (type == Type::INVALID)
		return false;
	
	if (type == super)
		return true;
	
	t = Find (type);
	
	g_return_val_if_fail (t != NULL, false);
	
	do {
		parent = t->GetParent ();
		
		if (parent == super)
			return true;
			
		if (parent == Type::INVALID)
			return false;
		
		t = Find (parent);
		
		if (t == NULL)
			return false;		
	} while (true);
	
	return false;
}

Type *
Type::Find (const char *name)
{
	return Deployment::GetCurrent ()->GetTypes ()->Find (name);
}

Type *
Type::Find (const char *name, bool ignore_case)
{
	return Deployment::GetCurrent ()->GetTypes ()->Find (name, ignore_case);
}

Type *
Type::Find (Type::Kind type)
{
	if (type < Type::INVALID || type == Type::LASTTYPE)
		return NULL;
	
	return Deployment::GetCurrent ()->GetTypes ()->Find (type);
}

DependencyObject *
Type::CreateInstance ()
{
	if (!create_inst) {
		g_warning ("Unable to create an instance of type: %s\n", name);
		return NULL;
	}
	
	return create_inst ();
}

const char *
Type::GetContentPropertyName ()
{
	Type *parent_type;

	if (type == INVALID)
		return NULL;

	if (content_property)
		return content_property;

	parent_type = Find (parent);

	if (parent_type == NULL)
		return NULL;

	return parent_type->GetContentPropertyName ();
}

DependencyProperty *
Type::LookupProperty (const char *name)
{
	DependencyProperty *property = NULL;
	
	g_return_val_if_fail (name != NULL, NULL);
	
	if (properties != NULL) {
		char *key = g_ascii_strdown (name, -1);
		property = (DependencyProperty*) g_hash_table_lookup (properties, key);
		g_free (key);
		
		if (property)
			return property;
	}
	
	return NULL;
}

void
Type::AddProperty (DependencyProperty *property)
{
	DependencyProperty *existing = NULL;

	g_return_if_fail (property != NULL);

	if (properties == NULL) {
		properties = g_hash_table_new (g_str_hash, g_str_equal);
	} else {
		existing = (DependencyProperty *) g_hash_table_lookup (properties, property->GetHashKey ());
	}

	if (existing == NULL || existing->IsCustom ()) {
		// Allow overwriting of custom properties
		g_hash_table_insert (properties, (gpointer) property->GetHashKey (), property);
	} else {
		g_warning ("Type::AddProperty (): Trying to register the property '%s' (of type %s) in the owner type '%s', and there already is a property registered on that type with the same name.",
			   property->GetName (), Type::Find (property->GetPropertyType ())->GetName(), GetName());	
	}
}

bool
type_get_value_type (Type::Kind type)
{
	Type *t = Type::Find (type);
	
	if (t == NULL)
		return false;
	
	return t->IsValueType ();
}

bool
type_is_dependency_object (Type::Kind type)
{
	return Type::IsSubclassOf (type, Type::DEPENDENCY_OBJECT);
}

DependencyObject *
type_create_instance (Type *type)
{
	if (!type) {
		g_warning ("Unable to create instance of type %p.", type);
		return NULL;
	}

	return type->CreateInstance ();
}

DependencyObject *
type_create_instance_from_kind (Type::Kind kind)
{
	Type *t = Type::Find (kind);
	
	if (t == NULL) {
		g_warning ("Unable to create instance of type %d. Type not found.", kind);
		return NULL;
	}
	
	return t->CreateInstance ();
}

/*
 * Types
 */

Types::Types ()
{
	//printf ("Types::Types (). this: %p\n", this);
	types.SetCount ((int) Type::LASTTYPE + 1);
	RegisterNativeTypes ();
}

void
Types::Initialize ()
{
	RegisterNativeProperties ();
}

Types::~Types ()
{
	//printf ("Types::~Types (). this: %p\n", this);
	for (int i = 0; i < properties.GetCount (); i++)
		delete (DependencyProperty *) properties [i];
	
	for (int i = 0; i < types.GetCount (); i++)
		delete (Type *) types [i];
}

void
Types::AddProperty (DependencyProperty *property)
{
	Type *type;
	
	g_return_if_fail (property != NULL);
	
	type = Find (property->GetOwnerType ());
	
	g_return_if_fail (type != NULL);
	
	property->SetId (properties.Add (property));
	type->AddProperty (property);
}

DependencyProperty *
Types::GetProperty (int id)
{
	g_return_val_if_fail (properties.GetCount () > id, NULL);
	return (DependencyProperty *) properties [id];
}

Type *
Types::Find (Type::Kind type)
{
	if ((int) type + 1 > types.GetCount ())
		return NULL;
	
	return (Type *) types [(int) type];
}

Type *
Types::Find (const char *name)
{
	return Types::Find (name, true);
}

Type *
Types::Find (const char *name, bool ignore_case)
{
	Type *t;
	
	for (int i = 1; i < types.GetCount (); i++) { // 0 = INVALID, shouldn't compare against that
		if (i == Type::LASTTYPE)
			continue;
	
		t = (Type *) types [i];
		if ((ignore_case && !g_ascii_strcasecmp (t->GetName (), name)) || !strcmp (t->GetName (), name))
			return t;
	}

	return NULL;
}

Type::Kind
Types::RegisterType (const char *name, void *gc_handle, Type::Kind parent)
{
	Type *type = new Type (Type::INVALID, parent, false, g_strdup (name), NULL, 0, Find (parent)->GetEventCount (), NULL, NULL, NULL);
	
	// printf ("Types::RegisterType (%s, %p, %i (%s)). this: %p, size: %i, count: %i\n", name, gc_handle, parent, Type::Find (this, parent) ? Type::Find (this, parent)->name : NULL, this, size, count);
	
	type->SetKind ((Type::Kind) types.Add (type));
	
	return type->GetKind ();
}

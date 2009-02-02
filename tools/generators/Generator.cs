/*
 * Generator.cs
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2008 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */
using System;
using System.Linq;
using System.Collections.Generic;
using System.IO;
using System.Text;


class Generator {
	delegate void output_native_type_delegate (string t, string k);

	public void Generate ()
	{
		GlobalInfo info = GetTypes2 ();
		//info.Dump (0);
		
		GenerateValueH (info);	
		GenerateTypeH (info);
		GenerateKindCs ();
		
		GenerateTypeStaticCpp (info);
		GenerateCBindings (info);
		GeneratePInvokes (info);
		GenerateTypes_G (info);
		
		GenerateDPs (info);
		GenerateManagedDPs (info);
		GenerateManagedDOs (info);
	}

	static void GenerateManagedDOs (GlobalInfo all)
	{
		string base_dir = Environment.CurrentDirectory;
		string class_dir = Path.Combine (base_dir, "class");
		string sys_win_dir = Path.Combine (Path.Combine (class_dir, "System.Windows"), "System.Windows");
		string filename = Path.Combine (sys_win_dir, "DependencyObject.g.cs");
		string previous_namespace = "";
		StringBuilder text = new StringBuilder ();
		List<TypeInfo> types = all.GetDependencyObjects (all);
				
		Helper.WriteWarningGenerated (text);
		text.AppendLine ("using Mono;");
		text.AppendLine ("using System;");
		text.AppendLine ("using System.Collections.Generic;");
		text.AppendLine ("using System.Windows;");
		text.AppendLine ("using System.Windows.Controls;");
		text.AppendLine ("using System.Windows.Documents;");
		text.AppendLine ("using System.Windows.Ink;");
		text.AppendLine ("using System.Windows.Input;");
		text.AppendLine ("using System.Windows.Markup;");
		text.AppendLine ("using System.Windows.Media;");
		text.AppendLine ("using System.Windows.Media.Animation;");
		text.AppendLine ("using System.Windows.Shapes;");
		text.AppendLine ();
		
		for (int i = 0; i < types.Count; i++) {
			TypeInfo type = types [i];
			bool call_initialize = type.Annotations.ContainsKey ("CallInitialize");
			string ns;
			
			ns = type.Namespace;
			
			if (string.IsNullOrEmpty (ns)) {
				Console.WriteLine ("The type '{0}' in {1} does not have a namespace annotation.", type.FullName, Path.GetFileName (type.Header));
				continue;
			}
			
			if (ns == "None") {
				//Console.WriteLine ("The type '{0}''s Namespace annotation is 'None'.", type.FullName);
				continue;
			}
			
			string check_ns = Path.Combine (Path.Combine (Path.Combine (class_dir, "System.Windows"), ns), type.ManagedName.Replace ("`1", "") + ".cs");
			if (!File.Exists (check_ns)) {
				Console.WriteLine ("The file {0} does not exist, did you annotate the class with the wrong namespace?", check_ns);
				continue;
			}
			
			if (previous_namespace != ns) {
				if (previous_namespace != string.Empty) {
					text.AppendLine ("}");
					text.AppendLine ();
				}
				text.Append ("namespace ");
				text.Append (ns);
				text.AppendLine (" {");
				previous_namespace = ns;
			} else {
				text.AppendLine ();
			}
			text.Append ("\tpartial class ");
			text.Append (type.ManagedName.Replace ("`1", "<T>"));
			text.AppendLine (" {");
			
			// Public ctor
			if (!string.IsNullOrEmpty (type.C_Constructor)) {
				string access = "Public";
				foreach (MemberInfo member in type.Children.Values) {
					MethodInfo method = member as MethodInfo;
					
					if (method == null || !method.IsConstructor || method.IsStatic)
						continue;
					
					if (method.Parameters.Count != 0)
						continue;
					
					if (method.Annotations.ContainsKey ("ManagedAccess"))
						access = method.Annotations.GetValue ("ManagedAccess");
					break;
				}
				
				
				text.Append ("\t\t");
				Helper.WriteAccess (text, access);
				text.Append (" ");
				text.Append (type.ManagedName.Replace ("`1", ""));
				text.Append (" () : base (NativeMethods.");
				text.Append (type.C_Constructor);
				text.Append (" ())");
				if (call_initialize) {
					text.AppendLine ();
					text.AppendLine ("\t\t{");
					text.AppendLine ("\t\t\tInitialize ();");
					text.AppendLine ("\t\t}");
				} else {
					text.AppendLine (" {}");
				}
			}
			
			// Internal ctor
			text.Append ("\t\tinternal ");
			text.Append (type.ManagedName.Replace ("`1", ""));
			text.Append (" (IntPtr raw) : base (raw)");
			if (call_initialize) {
				text.AppendLine ();
				text.AppendLine ("\t\t{");
				text.AppendLine ("\t\t\tInitialize ();");
				text.AppendLine ("\t\t}");
			} else {
				text.AppendLine (" {}");
			}

			text.AppendLine ("\t}");
		}
		text.AppendLine ("}");		
		
		Helper.WriteAllText (filename, text.ToString ());
	}
	
	static void GenerateManagedDPs (GlobalInfo all)
	{
		string base_dir = Environment.CurrentDirectory;
		string class_dir = Path.Combine (base_dir, "class");
		string sys_win_dir = Path.Combine (Path.Combine (class_dir, "System.Windows"), "System.Windows");
		string filename = Path.Combine (sys_win_dir, "DependencyProperty.g.cs");
		string previous_namespace = "";
		List<TypeInfo> sorted_types = new List<TypeInfo>  ();
		StringBuilder text = new StringBuilder ();
		Dictionary <TypeInfo, List<FieldInfo>> types = new Dictionary<TypeInfo,List<FieldInfo>> ();
		
		foreach (FieldInfo field in all.DependencyProperties) {
			TypeInfo parent = field.Parent as TypeInfo;
			List <FieldInfo> fields;
			string managed_parent = field.Annotations.GetValue ("ManagedDeclaringType");
			
			if (field.Annotations.GetValue ("GenerateManagedDP") == "false")
				continue;
			
			if (managed_parent != null) {
				parent = all.Children [managed_parent] as TypeInfo;
				
				if (parent == null)
					throw new Exception (string.Format ("Could not find the type '{0}' set as ManagedDeclaringType of '{1}'", managed_parent, field.FullName));
			}
			
			if (parent == null)
				throw new Exception (string.Format ("The field '{0}' does not have its parent set.", field.FullName));
			
			if (!types.TryGetValue (parent, out fields)) {
				fields = new List<FieldInfo> ();
				types.Add (parent, fields);
				sorted_types.Add (parent);
			}
			fields.Add (field);
		}
		
		Helper.WriteWarningGenerated (text);
		text.AppendLine ("using Mono;");
		text.AppendLine ("using System;");
		text.AppendLine ("using System.Collections.Generic;");
		text.AppendLine ("using System.Windows;");
		text.AppendLine ("using System.Windows.Controls;");
		text.AppendLine ("using System.Windows.Documents;");
		text.AppendLine ("using System.Windows.Ink;");
		text.AppendLine ("using System.Windows.Input;");
		text.AppendLine ("using System.Windows.Markup;");
		text.AppendLine ("using System.Windows.Media;");
		text.AppendLine ("using System.Windows.Media.Animation;");
		text.AppendLine ("using System.Windows.Shapes;");
		text.AppendLine ();
		
		sorted_types.Sort (new Members.MembersSortedByManagedFullName <TypeInfo> ());
		for (int i = 0; i < sorted_types.Count; i++) {
			TypeInfo type = sorted_types [i];
			List<FieldInfo> fields = types [type];
			TypeInfo parent = type;
			string ns;
			
			ns = parent.Namespace;
			
			if (string.IsNullOrEmpty (ns)) {
				Console.WriteLine ("The type '{0}' in {1} does not have a namespace annotation.", parent.FullName, parent.Header);
				continue;
			}

			if (type.Annotations.ContainsKey ("ManagedDependencyProperties")) {
				string dp_mode = type.Annotations.GetValue ("ManagedDependencyProperties");
				switch (dp_mode) {
				case "None":
				case "Manual":
					continue;
				case "Generate":
					break;
				default:
					throw new Exception (string.Format ("Invalid value '{0}' for ManagedDependencyProperties in '{1}'", dp_mode, type.FullName));
				}
			}

			if (ns == "None") {
				Console.WriteLine ("'{0}''s Namespace = 'None', this type should have set @ManagedDependencyProperties=Manual to not create DPs.", type.FullName);
				continue;
			}
			
			string check_ns = Path.Combine (Path.Combine (Path.Combine (class_dir, "System.Windows"), ns), parent.Name + ".cs");
			if (!File.Exists (check_ns))
				Console.WriteLine ("The file {0} does not exist, did you annotate the class with the wrong namespace?", check_ns);
			
			if (previous_namespace != ns) {
				if (previous_namespace != string.Empty) {
					text.AppendLine ("}");
					text.AppendLine ();
				}
				text.Append ("namespace ");
				text.Append (ns);
				text.AppendLine (" {");
				previous_namespace = ns;
			} else {
				text.AppendLine ();
			}
			text.Append ("\tpartial class ");
			text.Append (parent.ManagedName);
			text.AppendLine (" {");
			
			fields.Sort (new Members.MembersSortedByName <FieldInfo> ());
			
			// The DP registration
			foreach (FieldInfo field in fields) {
				bool conv_int_to_double = field.GetDPManagedPropertyType (all) == "int" && field.GetDPPropertyType (all).Name == "double";
				
				text.Append ("\t\t");
				Helper.WriteAccess (text, field.GetManagedFieldAccess ());
				text.Append (" static readonly DependencyProperty ");
				text.Append (field.Name);
				text.Append (" = DependencyProperty.Lookup (Kind.");
				text.Append (field.ParentType.KindName);
				text.Append (", \"");
				text.Append (field.GetDependencyPropertyName ());
				text.Append ("\", typeof (");
				if (conv_int_to_double)
					text.Append ("double");
				else
					text.Append (field.GetDPManagedPropertyType (all));
				text.AppendLine ("));");
			}
			
			foreach (FieldInfo field in fields) {
				bool conv_int_to_double = field.GetDPManagedPropertyType (all) == "int" && field.GetDPPropertyType (all).Name == "double";
				
				if (field.IsDPAttached)
					continue;
				
				text.AppendLine ();
				text.Append ("\t\t");
				Helper.WriteAccess (text, field.GetManagedAccessorAccess ());
				text.Append (" ");
				text.Append (field.GetDPManagedPropertyType (all));
				text.Append (" ");
				text.Append (field.GetDependencyPropertyName ());
				text.AppendLine (" {");
				
				text.Append ("\t\t\t");
				if (field.GetManagedAccessorAccess () != field.GetManagedGetterAccess ()) {
					Helper.WriteAccess (text, field.GetManagedGetterAccess ());
					text.Append (" ");
				}
				text.Append ("get { return (");
				text.Append (field.GetDPManagedPropertyType (all));
				if (conv_int_to_double)
					text.Append (") (double");
				text.Append (") GetValue (");
				text.Append (field.Name);
				text.AppendLine ("); }");
				if (!field.IsDPReadOnly) {
					text.Append ("\t\t\t");
					if (field.GetManagedAccessorAccess () != field.GetManagedSetterAccess ()) {
						Helper.WriteAccess (text, field.GetManagedSetterAccess ());
						text.Append (" ");
					}
					text.Append ("set { SetValue (");
					text.Append (field.Name);
					text.AppendLine (", value); }");
				}
				text.AppendLine ("\t\t}");
			}
			
			text.AppendLine ("\t}");
		}
		text.AppendLine ("}");		
		
		Helper.WriteAllText (filename, text.ToString ());
	}

	static void GenerateDPs (GlobalInfo all)
	{	
		string base_dir = Environment.CurrentDirectory;
		string moon_dir = Path.Combine (base_dir, "src");
// 		int version_previous = 0;
		StringBuilder text = new StringBuilder ();
		List<FieldInfo> fields = all.DependencyProperties;
		List<string> headers = new List<string> ();
		
		headers.Add ("dependencyproperty.h");
		headers.Add ("validators.h");
		headers.Add ("color.h");
		foreach (FieldInfo field in fields) {
			string h;
			if (string.IsNullOrEmpty (field.Header))
				continue;
			h = Path.GetFileName (field.Header);
			
			if (!headers.Contains (h))
				headers.Add (h);
		}
		
		Helper.WriteWarningGenerated (text);
		text.AppendLine ();
		text.AppendLine ("#ifdef HAVE_CONFIG_H");
		text.AppendLine ("#include <config.h>");
		text.AppendLine ("#endif");
		text.AppendLine ();
		headers.Sort ();
		foreach (string h in headers) {
			text.Append ("#include \"");
			text.Append (h);
			text.AppendLine ("\"");
		}
		text.AppendLine ();
		text.AppendLine ("bool dependency_properties_initialized = false;");
		text.AppendLine ("void");
		text.AppendLine ("dependency_property_g_init (void)");
		text.AppendLine ("{");
		text.AppendLine ("\tif (dependency_properties_initialized)");
		text.AppendLine ("\t\treturn;");
		text.AppendLine ("\tdependency_properties_initialized = true;");
		
		for (int i = 0; i < fields.Count; i++) {
			FieldInfo field = fields [i];
			TypeInfo type = field.ParentType;
			TypeInfo propertyType = null;
			string default_value = field.DPDefaultValue;
			bool has_default_value = !string.IsNullOrEmpty (default_value);
			bool is_nullable = field.IsDPNullable;
			bool is_attached = field.IsDPAttached;
			bool is_readonly = field.IsDPReadOnly;
			bool is_always_change = field.IsDPAlwaysChange;
			string validator = field.DPValidator;
			bool is_full = is_attached || is_readonly || is_always_change || validator != null;

			propertyType = field.GetDPPropertyType (all);
			
			text.Append ("\t");
			
			if (propertyType == null) {
				text.Append ("// (no PropertyType was found for this DependencyProperty) ");
			} else {
				headers.Add (propertyType.Header);
			}
			
			text.Append (type.Name);
			text.Append ("::");
			text.Append (field.Name);
			text.Append (" = DependencyProperty::Register");
			if (is_nullable)
				text.Append ("Nullable");
			else if (is_full)
				text.Append ("Full");
			text.Append (" (Type::");
			text.Append (type.KindName);
			text.Append (", \"");
			
			text.Append (field.GetDependencyPropertyName ());
			text.Append ("\"");
			text.Append (", ");
			if (is_nullable) {
				if (propertyType != null) {
					if (propertyType.IsEnum) {
						text.Append ("Type::INT32");
					} else {
						text.Append ("Type::");
						text.Append (propertyType.KindName);
					}
				} else {
					text.Append ("Type::INVALID");
					//Console.WriteLine ("{0} does not define its property type.", field.FullName);
				}
				text.Append (", ");
				text.Append (validator != null ? ("Validators::" + validator) : "NULL");
			}
			else {
				if (is_full) {
					if (has_default_value) {
						text.Append ("new Value (");
						text.Append (default_value);
						text.Append (")");
					} else {
						text.Append ("NULL");
					}
				} else {
					if (has_default_value) {
						text.Append ("new Value (");
						text.Append (default_value);
						text.Append (")");
					}
				}

				if ((has_default_value || is_full))
					text.Append (", ");
				if (propertyType != null) {
					if (propertyType.IsEnum) {
						text.Append ("Type::INT32");
					} else {
						text.Append ("Type::");
						text.Append (propertyType.KindName);
					}
				} else if (!has_default_value) {
					text.Append ("Type::INVALID");
					//Console.WriteLine ("{0} does not define its property type.", field.FullName);
				}
				
				if (is_full) {
					text.Append (", ");
					text.Append (is_attached ? "true" : "false");
					text.Append (", ");
					text.Append (is_readonly ? "true" : "false");
					text.Append (", ");
					text.Append (is_always_change ? "true" : "false");
					text.Append (", ");
					text.Append ("NULL");
					text.Append (", ");
					text.Append (validator != null ? ("Validators::" + validator) : "NULL");
				}
			}

			text.AppendLine (");");
		}
		text.AppendLine ("}");
		text.AppendLine ();
			
		// Static initializers
		for (int i = 0; i < fields.Count; i++) {
			FieldInfo field = fields [i];
			text.Append ("DependencyProperty *");
			text.Append (field.Parent.Name);
			text.Append ("::");
			text.Append (field.Name);
			text.AppendLine (" = NULL;");
		}
		text.AppendLine ();
		
		// C++ Accessors
		for (int i = 0; i < fields.Count; i++) {
			FieldInfo field = fields [i];
			TypeInfo prop_type;
			string prop_type_str;
			string value_str;
			string prop_default = null;
			bool both = field.Annotations.ContainsKey ("GenerateAccessors");
			bool setter = both || field.Annotations.ContainsKey ("GenerateSetter");
			bool getter = both || field.Annotations.ContainsKey ("GenerateGetter");
			bool is_attached = field.IsDPAttached;
			bool nullable_setter = setter && field.IsDPNullable;
			bool doing_nullable_setter = false;
			
			if (!setter && !getter)
				continue;
			
			prop_type = field.GetDPPropertyType (all);
			
			switch (prop_type.Name) {
			case "char*": 
				prop_type_str = "const char *"; 
				value_str = "String";
				break;
			case "int":
			case "gint32":
				value_str = "Int32";
				prop_type_str = prop_type.Name;
				prop_default = "0";
				break;
			case "double":
				value_str = "Double";
				prop_type_str = prop_type.Name;
				prop_default = "0.0";
				break;
			case "bool":
				prop_type_str = prop_type.Name;
				value_str = "Bool";
				prop_default = "false";
				break;
			case "char":
				prop_type_str = "gint32";
				value_str = "Int32";
				prop_default = "0";
				break;
			case "object":
				prop_type_str = "Value *";
				prop_default = "NULL";
				value_str = null;
				break;
			default:
				prop_type_str = prop_type.Name; 
				value_str = prop_type.Name;
				break;
			}

			string GetterName = string.Format ("{0}::Get{1}", field.ParentType.Name, field.GetDependencyPropertyName());
			string SetterName = string.Format ("{0}::Set{1}", field.ParentType.Name, field.GetDependencyPropertyName());

			if (getter) {
				text.Append (prop_type_str);
				if (field.IsDPNullable || (prop_type.IsClass || prop_type.IsStruct))
					text.Append (" *");
				text.AppendLine ();
				text.Append (GetterName);
				if (is_attached)
					text.AppendLine (" (DependencyObject *obj)");
				else
					text.AppendLine (" ()");
				text.AppendLine ("{");


				if (value_str == null) {
					text.Append ("\treturn ");
				} else if (is_attached) {
					text.Append ("\tValue *value = (!obj) ? NULL : ");
				} else {
					text.Append ("\tValue *value = ");
				}

				text.AppendFormat ("{0}{1}GetValue ({2}::{3});\n",
						   is_attached ? "obj->" : "",
						   field.ParentType.NeedsQualifiedGetValue(all) ? "DependencyObject::" : "",
						   field.ParentType.Name, field.Name);

				if (is_attached) {
					text.AppendFormat ("\tif (!value) value = {0}::{1}->GetDefaultValue();\n",
							   field.ParentType.Name, field.Name);
				}

				if (value_str == null) {
					// Skip this
				} else if (field.IsDPNullable || (prop_type.IsClass || prop_type.IsStruct || prop_type.Name == "char*")) {
					text.Append ("\treturn value ? ");
					if (prop_type.IsEnum) {
						text.AppendFormat ("({0}) value->AsInt32() : ({0}) 0", prop_type.Name);
					} else {
						if (!field.IsDPNullable && (/*prop_type.IsStruct || */prop_default != null))
						    if (string.IsNullOrEmpty (prop_default))
							throw new NotImplementedException (
								string.Format ("Generation of DependencyProperties with struct values ({0}.{1})",
									       field.ParentType.Name, field.Name));

						text.AppendFormat ("value->As{0}{1} () : {2}",
								   field.IsDPNullable && !(prop_type.IsStruct || prop_type.IsClass) ? "Nullable" : "",
								   value_str,
								   !field.IsDPNullable && prop_default != null ? prop_default : "NULL");
					}
				} else {
					// Value cannot be null, so don't need to check for it
					text.Append ("\treturn ");
					if (prop_type.IsEnum) {
						text.AppendFormat ("({0}) value->AsInt32 ()", prop_type.Name);
					} else {
						text.AppendFormat ("value->As{0} ()", value_str);
					}
				}

				if (value_str != null)
					text.AppendLine (";");
				text.AppendLine ("}");
				text.AppendLine ();
			}
			
		 do_nullable_setter:
			if (setter) {		
				text.AppendLine ("void");
				text.Append (SetterName);
				text.Append (" (");
				if (is_attached)
					text.Append ("DependencyObject *obj, ");
				text.Append (prop_type_str);
				if (prop_type.Name != "char*")
					text.Append (' ');
				if (!nullable_setter && (prop_type.IsClass || prop_type.IsStruct))
					text.Append ('*');
				text.AppendLine ("value)");
				
				text.AppendLine ("{");
				if (is_attached)
					text.AppendLine ("\tif (!obj) return;");
				if (doing_nullable_setter) {
					text.AppendLine ("\tif (!value)");
					text.Append ("\t\t");
					text.AppendFormat ("{0}{1}SetValue ({2}::{3}, NULL);\n",
							   is_attached ? "obj->" : "",
							   field.ParentType.NeedsQualifiedGetValue(all) ? "DependencyObject::" : "",
							   field.ParentType.Name, field.Name);
					text.AppendLine ("\telse");
					text.Append ("\t\t");
					text.AppendFormat ("{0}{1}SetValue ({2}::{3}, Value (*value));\n",
							   is_attached ? "obj->" : "",
							   field.ParentType.NeedsQualifiedGetValue(all) ? "DependencyObject::" : "",
							   field.ParentType.Name, field.Name);
				} else {
					if (!nullable_setter && prop_type.IsStruct)
						text.AppendLine ("\tif (!value) return;");
					text.Append ("\t");
					text.AppendFormat ("{0}{1}SetValue ({2}::{3}, ",
							   is_attached ? "obj->" : "",
							   field.ParentType.NeedsQualifiedGetValue(all) ? "DependencyObject::" : "",
							   field.ParentType.Name, field.Name);

					if (prop_type.Name == "guint64" || prop_type.Name == "TimeSpan") {
						text.AppendFormat ("Value (value, Type::{0}));\n",
								   prop_type.KindName);
					}
					else if ((value_str == null) || (!nullable_setter && prop_type.IsStruct)) {
						text.AppendLine ("Value (*value));");
					}
					else {
						text.AppendLine ("Value (value));");
					}
				}
				text.AppendLine ("}");
				text.AppendLine ();
			}
			
			if (nullable_setter) {
				if (!prop_type.IsStruct)
					prop_type_str += " *";
				nullable_setter = false;
				doing_nullable_setter = true;
				goto do_nullable_setter;
			}
		}
		
		Helper.WriteAllText (Path.Combine (moon_dir, "dependencyproperty.g.cpp"), text.ToString ());
		
	}
	
	static GlobalInfo GetTypes2 ()
	{
		string srcdir = Path.Combine (Environment.CurrentDirectory, "src");
		string asfdir = Path.Combine (srcdir, "asf");
		string plugindir = Path.Combine (Environment.CurrentDirectory, "plugin");
		List<string> all_files = new List<string> ();

		all_files.AddRange (Directory.GetFiles (srcdir, "*.h"));
		all_files.AddRange (Directory.GetFiles (asfdir, "*.h"));
		all_files.AddRange (Directory.GetFiles (plugindir, "*.h"));
		
		Tokenizer tokenizer = new Tokenizer (all_files.ToArray ());
		GlobalInfo all = new GlobalInfo ();
		
		tokenizer.Advance (false);
		
		try {
			while (ParseMembers (all, tokenizer)) {
			}
		} catch (Exception ex) {
			throw new Exception (string.Format ("{0}({1}): {2}", tokenizer.CurrentFile, tokenizer.CurrentLine, ex.Message), ex);
		}
		
		// Add all the manual types
		all.Children.Add (new TypeInfo ("object", "OBJECT", "INVALID", true, true));
		all.Children.Add (new TypeInfo ("bool", "BOOL", "OBJECT", true, true, true));
		all.Children.Add (new TypeInfo ("double", "DOUBLE", "OBJECT", true, true, true));
		all.Children.Add (new TypeInfo ("guint64", "UINT64", "OBJECT", true, true, true));
		all.Children.Add (new TypeInfo ("gint64", "INT64", "OBJECT", true, true, true));
		all.Children.Add (new TypeInfo ("guint32", "UINT32", "OBJECT", true, true, true));
		all.Children.Add (new TypeInfo ("gint32", "INT32", "OBJECT", true, true, true));
		all.Children.Add (new TypeInfo ("char*", "STRING", "OBJECT", true, true, true));
		all.Children.Add (new TypeInfo ("NPObj", "NPOBJ", "OBJECT", true, true, true));
		all.Children.Add (new TypeInfo ("Managed", "MANAGED", "OBJECT", true, 2, true));
		all.Children.Add (new TypeInfo ("TimeSpan", "TIMESPAN", "OBJECT", true, true, true));
		all.Children.Add (new TypeInfo ("char", "CHAR", "OBJECT", true, true, true));
		all.Children.Add (new TypeInfo ("System.Windows.FontStretch", "FONTSTRETCH", "OBJECT", true, true));
		all.Children.Add (new TypeInfo ("System.Windows.Markup.XmlLanguage", "XMLLANGUAGE", "OBJECT", true, true));

		// Set IncludeInKinds for all types which inherit from EventObject
		foreach (MemberInfo member in all.Children.Values) {
			TypeInfo type = member as TypeInfo;
			if (type == null)
				continue;
			if (type.Name == "EventObject")
				type.Annotations ["IncludeInKinds"] = null;
			
			TypeReference bR = type.Base;
			MemberInfo m;
			TypeInfo b;
			while (bR != null) {
				if (bR.Value == "EventObject") {
					member.Annotations ["IncludeInKinds"] = null;
				}

				if (!all.Children.TryGetValue (bR.Value, out m))
					break;
				
				b = m as TypeInfo;
				if (b != null)
					bR = b.Base;
				else
					bR = null;
			}
		}
		
		return all;
	}
	
	// Returns false if there are no more tokens (reached end of code)
	static bool ParseClassOrStruct (Annotations annotations, MemberInfo parent, Tokenizer tokenizer)
	{
		TypeInfo type = new TypeInfo ();
		
		type.Annotations = annotations;
		type.Header = tokenizer.CurrentFile;
		type.Parent = parent;
		
		type.IsPublic = tokenizer.Accept (Token2Type.Identifier, "public");

		if (tokenizer.Accept (Token2Type.Identifier, "class")) {
			type.IsClass = true;
		} else if (tokenizer.Accept (Token2Type.Identifier, "struct")) {
			type.IsStruct = true;
			type.IsValueType = true;
		} else if (tokenizer.Accept (Token2Type.Identifier, "union")) {
			type.IsStruct = true; // Not entirely correct, but a union can be parsed as a struct
			type.IsValueType = true;
		} else {
			throw new Exception (string.Format ("Expected 'class' or 'struct', not '{0}'", tokenizer.CurrentToken.value));
		}

		if (tokenizer.CurrentToken.type == Token2Type.Identifier) {
			type.Name = tokenizer.GetIdentifier ();
		} else {
			type.Name = "<anonymous>";
		}

		if (tokenizer.Accept (Token2Type.Punctuation, ";")) {
			// A forward declaration.
			//Console.WriteLine ("ParseType: Found a forward declaration to {0}", type.Name);
			return true;
		}
		
		if (tokenizer.Accept (Token2Type.Punctuation, ":")) {
			if (!tokenizer.Accept (Token2Type.Identifier, "public") && type.IsClass)
				throw new Exception (string.Format ("The base class of {0} is not public.", type.Name));
			
			type.Base = ParseTypeReference (tokenizer);
			
			// accept multiple inheritance the easy way
			while (tokenizer.CurrentToken.value == ",") {
				tokenizer.Accept (Token2Type.Punctuation, ",");
				
				while (tokenizer.CurrentToken.value != "," &&
				       tokenizer.CurrentToken.value != "{")
					tokenizer.GetIdentifier ();
			}
			
			//Console.WriteLine ("ParseType: Found {0}'s base class: {1}", type.Name, type.Base);
		}
		
		tokenizer.AcceptOrThrow (Token2Type.Punctuation, "{");
		
		//Console.WriteLine ("ParseType: Found a type: {0} in {1}", type.Name, type.Header);
		parent.Children.Add (type);
		ParseMembers (type, tokenizer);
		
		tokenizer.AcceptOrThrow (Token2Type.Punctuation, "}");
		
		if (tokenizer.CurrentToken.type == Token2Type.Identifier)
			tokenizer.Advance (true);
		
		if (tokenizer.CurrentToken.value != ";")
			throw new Exception (string.Format ("Expected ';', not '{0}'", tokenizer.CurrentToken.value));
		
		return tokenizer.Advance (false);
	}
	
	static bool ParseMembers (MemberInfo parent, Tokenizer tokenizer)
	{
		Annotations properties = new Annotations ();
		TypeInfo parent_type = parent as TypeInfo;
		string accessibility;
		TypeReference returntype;
		bool is_dtor;
		bool is_ctor;
		bool is_virtual;
		bool is_static;
		bool is_const;
		bool is_extern;
		string name;
		
		//Console.WriteLine ("ParseMembers ({0})", type.Name);
		
	 	do {
			returntype = null;
			is_dtor = is_ctor = is_virtual = is_static = false;
			is_extern = is_const = false;
			name = null;
			properties = new Annotations ();
			
			if (parent_type != null)
				accessibility = parent_type.IsStruct ? "public" : "private";
			else
				accessibility = "public";
			
			if (tokenizer.Accept (Token2Type.Punctuation, ";"))
				continue;
			
			if (tokenizer.CurrentToken.value == "}")
				return true;
			
			while (tokenizer.CurrentToken.type == Token2Type.CommentProperty) {
				properties.Add (tokenizer.CurrentToken.value);
				tokenizer.Advance (true);
			}
			
			//Console.WriteLine ("ParseMembers: Current token: {0}", tokenizer.CurrentToken);
			
			if (tokenizer.CurrentToken.type == Token2Type.Identifier) {
				string v = tokenizer.CurrentToken.value;
				switch (v) {
				case "public":
				case "protected":
				case "private":
					accessibility = v;
					tokenizer.Advance (true);
					tokenizer.Accept (Token2Type.Punctuation, ":");
					continue;
				case "enum":
					ParseEnum (parent, tokenizer);
					continue;
				case "friend":
					tokenizer.Advance (true);
					tokenizer.Advance (true);
					tokenizer.Advance (true);
					continue;
				case "struct":
				case "class":
				case "union":
					if (!ParseClassOrStruct (properties, parent, tokenizer))
						return false;
					continue;
				case "typedef":
					tokenizer.Advance (true);
					while (!tokenizer.Accept (Token2Type.Punctuation, ";")) {
						if (tokenizer.CurrentToken.value == "{")
							tokenizer.SyncWithEndBrace ();
						tokenizer.Advance (true);
					}
					continue;
				}
			}
			
			do {
				if (tokenizer.Accept (Token2Type.Identifier, "virtual")) {
					is_virtual = true;
					continue;
				}
				
				if (tokenizer.Accept (Token2Type.Identifier, "static")) {
					is_static = true;
					continue;
				}
			   
				if (tokenizer.Accept (Token2Type.Identifier, "const")) {
					is_const = true;
					continue;
				}
				
				if (tokenizer.Accept (Token2Type.Identifier, "extern")) {
					is_extern = true;
					continue;
				}

				if (tokenizer.Accept (Token2Type.Identifier, "volatile")) {
					continue;
				}
				
			    break;
			} while (true);

			if (is_extern && tokenizer.Accept (Token2Type.Literal, "C")) {
				tokenizer.SyncWithEndBrace ();
				continue;
			}
			
			if (tokenizer.Accept (Token2Type.Punctuation, "~")) {
				is_dtor = true;
				if (!is_virtual) {
					TypeInfo ti = parent as TypeInfo;
					if (ti != null && ti.Base != null)
						Console.WriteLine ("The class {0} has a non-virtual destructor, and it's base class is {2} ({1}).", parent.Name, parent.Header, ti != null && ti.Base != null ? ti.Base.Value : "<none>");
				}
			}
					
			if (is_dtor) {
				name = "~" + tokenizer.GetIdentifier ();
				returntype = new TypeReference ("void");
			} else {
				returntype = ParseTypeReference (tokenizer);
				
				if (returntype.Value == parent.Name && tokenizer.CurrentToken.value == "(") {
					is_ctor = true;
					name = returntype.Value;
					returntype.Value += "*";
				} else {
					name = tokenizer.GetIdentifier ();
				}
			}
			returntype.IsConst = is_const;
			returntype.IsReturnType = true;

			//Console.WriteLine ("ParseMembers: found member '{0}' is_ctor: {1}", name, is_ctor);
			
			if (tokenizer.Accept (Token2Type.Punctuation, "(")) {
				// Method
				MethodInfo method = new MethodInfo ();
				method.Header = tokenizer.CurrentFile;
				method.Parent = parent;
				method.Annotations = properties;
				method.Name = name;
				method.IsConstructor = is_ctor;
				method.IsDestructor = is_dtor;
				method.IsVirtual = is_virtual;
				method.IsStatic = is_static;
				method.IsPublic = accessibility == "public";
				method.IsPrivate = accessibility == "private";
				method.IsProtected = accessibility == "protected";
				method.ReturnType = returntype;
				
				//Console.WriteLine ("ParseMembers: found method '{0}' is_ctor: {1}", name, is_ctor);
				
				if (!tokenizer.Accept (Token2Type.Punctuation, ")")) {
					string param_value = null;
					do {
						ParameterInfo parameter = new ParameterInfo ();
						if (tokenizer.Accept (Token2Type.Punctuation, ".") && tokenizer.Accept (Token2Type.Punctuation, ".") && tokenizer.Accept (Token2Type.Punctuation, ".")) {
							// ... variable argument declaration
							parameter.ParameterType = new TypeReference ("...");
						} else {
							parameter.ParameterType = ParseTypeReference (tokenizer);
						}
						if (tokenizer.CurrentToken.value != "," && tokenizer.CurrentToken.value != ")") {
							parameter.Name = tokenizer.GetIdentifier ();
							if (tokenizer.Accept (Token2Type.Punctuation, "["))
								tokenizer.AcceptOrThrow (Token2Type.Punctuation, "]");
							if (tokenizer.Accept (Token2Type.Punctuation, "=")) {
								param_value = string.Empty;
								if (tokenizer.Accept (Token2Type.Punctuation, "-"))
									param_value = "-";
								param_value += tokenizer.GetIdentifier ();
							}
						}
						method.Parameters.Add (parameter);
						//Console.WriteLine ("ParseMember: got parameter, type: '{0}' name: '{1}' value: '{2}'", parameter.ParameterType.Value, parameter.Name, param_value);
					} while (tokenizer.Accept (Token2Type.Punctuation, ","));
					tokenizer.AcceptOrThrow (Token2Type.Punctuation, ")");
				}
				
				parent.Children.Add (method);

				//Allow const member functions, ignore the const keyword
				tokenizer.Accept (Token2Type.Identifier, "const");
				
				if (tokenizer.CurrentToken.value == "{") {
					//Console.WriteLine ("ParseMember: member has body, skipping it");
					tokenizer.SyncWithEndBrace ();
				} else if (is_ctor && tokenizer.Accept (Token2Type.Punctuation, ":")) {
					// ctor method implemented in header with field initializers and/or base class ctor call
					tokenizer.FindStartBrace ();
					tokenizer.SyncWithEndBrace ();
					//Console.WriteLine ("ParseMember: skipped ctor method implementation");				
				} else if (tokenizer.Accept (Token2Type.Punctuation, "=")) {
					// pure virtual method
					tokenizer.AcceptOrThrow (Token2Type.Identifier, "0");
					tokenizer.AcceptOrThrow (Token2Type.Punctuation, ";");
					method.IsAbstract = true;
				} else {
					tokenizer.AcceptOrThrow (Token2Type.Punctuation, ";");
				}
			} else {
				if (is_ctor || is_dtor)
					throw new Exception (string.Format ("Expected '(', not '{0}'", tokenizer.CurrentToken.value));
				
				if (name == "operator") {
					while (true) {
						if (tokenizer.CurrentToken.value == ";") {
							// End of operator
							break;
						} else if (tokenizer.CurrentToken.value == "{") {
							// In-line implementation
							tokenizer.SyncWithEndBrace ();
							break;
						}
						tokenizer.Advance (true);
					}
					//Console.WriteLine ("ParseMembers: skipped operator");
				} else {
					FieldInfo field = new FieldInfo ();
					field.IsConst = is_const;
					field.IsStatic = is_static;
					field.IsExtern = is_extern;
					field.Name = name;
					field.FieldType = returntype;
					field.IsPublic = accessibility == "public";
					field.IsPrivate = accessibility == "private";
					field.IsProtected = accessibility == "protected";
					field.Annotations = properties;
					
					// Field
					do {
						//Console.WriteLine ("ParseMembers: found field '{0}'", name);
						field.Parent = parent;
						parent.Children.Add (field);
						
						if (tokenizer.Accept (Token2Type.Punctuation, "[")) {
							while (!tokenizer.Accept (Token2Type.Punctuation, "]")) {
								tokenizer.Advance (true);
							}
						}
						if (tokenizer.Accept (Token2Type.Punctuation, ":")) {
							field.BitField = tokenizer.GetIdentifier ();
						}
						if (tokenizer.Accept (Token2Type.Punctuation, ",")) {
							field = new FieldInfo ();
							if (tokenizer.Accept (Token2Type.Punctuation, "*")) {
								// ok
							}
							field.Name = tokenizer.GetIdentifier ();
							field.FieldType = returntype;
							continue;
						}
						break;
					} while (true);
					
					tokenizer.Accept (Token2Type.Punctuation, ";");
				}
			}
		} while (true);
	}
	
	static void ParseEnum (MemberInfo parent, Tokenizer tokenizer)
	{
		FieldInfo field;
		StringBuilder value = new StringBuilder ();
		TypeInfo type = new TypeInfo ();
		
		type.IsEnum = true;
		
		tokenizer.AcceptOrThrow (Token2Type.Identifier, "enum");
		if (tokenizer.CurrentToken.type == Token2Type.Identifier) {
			type.Name = tokenizer.GetIdentifier ();
		} else {
			type.Name = "<anonymous>";
		}
		parent.Children.Add (type);
		
		tokenizer.AcceptOrThrow (Token2Type.Punctuation, "{");
		
		//Console.WriteLine ("ParseEnum: {0}", name);
		
		while (tokenizer.CurrentToken.type == Token2Type.Identifier) {
			field = new FieldInfo ();
			field.Name = tokenizer.GetIdentifier ();
			value.Length = 0;
			if (tokenizer.Accept (Token2Type.Punctuation, "=")) {
				while (tokenizer.CurrentToken.value != "," && tokenizer.CurrentToken.value != "}") {
					value.Append (" ");
					value.Append (tokenizer.CurrentToken.value);
					tokenizer.Advance (true);
				}
			}
			type.Children.Add (field);
			//Console.WriteLine ("ParseEnum: {0}: {1} {2} {3}", name, field, value.Length != 0 != null ? "=" : "", value);
						
			if (!tokenizer.Accept (Token2Type.Punctuation, ","))
				break;
		}
		
		tokenizer.AcceptOrThrow (Token2Type.Punctuation, "}");
		tokenizer.AcceptOrThrow (Token2Type.Punctuation, ";");
	}
	
	public static TypeReference ParseTypeReference (Tokenizer tokenizer)
	{
		TypeReference tr = new TypeReference ();
		StringBuilder result = new StringBuilder ();
		bool unsigned;

		if (tokenizer.Accept (Token2Type.Identifier, "const"))
			tr.IsConst = true;

		if (tokenizer.Accept (Token2Type.Identifier, "unsigned"))
			result.Append ("unsigned");
		
		if (tokenizer.Accept (Token2Type.Identifier, "const"))
			tr.IsConst = true;
		
		result.Append (tokenizer.GetIdentifier ());
		
		if (tokenizer.Accept (Token2Type.Punctuation, ":")) {
			tokenizer.AcceptOrThrow (Token2Type.Punctuation, ":");
			result.Append ("::");
			result.Append (tokenizer.GetIdentifier ());
		}
		
		if (tokenizer.Accept (Token2Type.Identifier, "const"))
			tr.IsConst = true;
		
		while (tokenizer.Accept (Token2Type.Punctuation, "*"))
			result.Append ("*");
		
		if (tokenizer.Accept (Token2Type.Identifier, "const"))
			tr.IsConst = true;
		
		if (tokenizer.Accept (Token2Type.Punctuation, "&"))
			result.Append ("&");
		
		if (tokenizer.Accept (Token2Type.Identifier, "const"))
			tr.IsConst = true;
		
		//Console.WriteLine ("ParseTypeReference: parsed '{0}'", result.ToString ());
		
		tr.Value = result.ToString ();

		return tr;
	}

	public static string getU (string v)
	{
		if (v.Contains ("::"))
			v = v.Substring (v.IndexOf ("::") + 2);

		v = v.ToUpper ();
		v = v.Replace ("DEPENDENCYOBJECT", "DEPENDENCY_OBJECT");
		if (v.Length > "COLLECTION".Length && !v.StartsWith ("COLLECTION"))
			v = v.Replace ("COLLECTION", "_COLLECTION");
		if (v.Length > "DICTIONARY".Length)
			v = v.Replace ("DICTIONARY", "_DICTIONARY");
		return v;
	}
	
	public void GenerateTypes_G (GlobalInfo all)
	{
		string base_dir = Environment.CurrentDirectory;
		string class_dir = Path.Combine (base_dir, "class");
		string moon_moonlight_dir = Path.Combine (class_dir, "System.Windows");
		List<TypeInfo> types = new List<TypeInfo> (all.GetDependencyObjects (all));
		
		StringBuilder text = new StringBuilder ();
		
		Helper.WriteWarningGenerated (text);
					
		text.AppendLine ("using Mono;");
		text.AppendLine ("using System;");
		text.AppendLine ("using System.Reflection;");
		text.AppendLine ("using System.Collections.Generic;");
		text.AppendLine ("");
		text.AppendLine ("namespace Mono {");
		text.AppendLine ("\tpartial class Types {");
		text.AppendLine ("\t\tprivate void CreateNativeTypes ()");
		text.AppendLine ("\t\t{");
		text.AppendLine ("\t\t\tType t;");
		text.AppendLine ("\t\t\ttry {");

		foreach (MemberInfo m in all.Children.Values) {
			TypeInfo t = m as TypeInfo;
			if (t == null)
				continue;
			if (types.Contains (t))
				continue;
			types.Add (t);
		}

		types.Sort (new Members.MembersSortedByManagedFullName <TypeInfo> ());
		
		for (int i = 0; i < types.Count; i++) {
			TypeInfo t = types [i];
			string type = t.ManagedName;
			
			if (String.IsNullOrEmpty (t.Namespace) || t.Namespace == "None" || t.Name.StartsWith ("MoonWindow"))
				continue;
			
			if (type == "PresentationFrameworkCollection`1")
				type = "PresentationFrameworkCollection<>";
				
			//Log.WriteLine ("Found Kind.{0} in {1} which result in type: {2}.{3}", kind, file, ns, type);
			
			text.Append ("\t\t\t\tt = typeof (");
			text.Append (t.Namespace);
			text.Append (".");
			text.Append (type);
			text.AppendLine ("); ");
			
			text.Append ("\t\t\t\ttypes.Add (t, new ManagedType (t, Kind.");
			text.Append (t.KindName);
			text.AppendLine ("));");
		}

		// now handle the primitive types
		output_native_type_delegate f = delegate (string t, string k) {
			text.Append ("\t\t\t\tt = typeof (");
			text.Append (t);
			text.AppendLine (");");
				
				
			text.Append ("\t\t\t\ttypes.Add (t, new ManagedType (t, Kind.");
			text.Append (k);
			text.AppendLine ("));");
		};

		f ("char", "INT32");
		f ("object", "OBJECT");
		f ("bool", "BOOL");
		f ("double", "DOUBLE");
		f ("ulong", "UINT64");
		f ("long", "INT64");
		f ("uint", "UINT32");
		f ("int", "INT32");
		f ("string", "STRING");
		f ("TimeSpan", "TIMESPAN");
		f ("System.Windows.Application", "APPLICATION");
		f ("System.Windows.Thickness", "THICKNESS");
		f ("System.Windows.CornerRadius", "CORNERRADIUS");
		f ("System.Windows.Point", "POINT");
		f ("System.Windows.FontStretch", "FONTSTRETCH");
		f ("System.Windows.Media.FontFamily", "FONTFAMILY");
		f ("System.Windows.Markup.XmlLanguage", "XMLLANGUAGE");

		text.AppendLine ("\t\t\t} catch (Exception ex) {");
		text.AppendLine ("\t\t\t\tConsole.WriteLine (\"There was an error while loading native types: \" + ex.ToString ());");
		text.AppendLine ("\t\t\t}");
		text.AppendLine ("\t\t}");
		text.AppendLine ("\t}");
		text.AppendLine ("}");
		
	 	Log.WriteLine ("typeandkidngen done");
		
		Helper.WriteAllText (Path.Combine (Path.Combine (moon_moonlight_dir, "Mono"), "Types.g.cs"), text.ToString ());
	}
	
	public static void GenerateCBindings (GlobalInfo info)
	{
		string base_dir = Environment.CurrentDirectory;
		string plugin_dir = Path.Combine (base_dir, "plugin");
		string moon_dir = Path.Combine (base_dir, "src");
		List<MethodInfo> methods;
		StringBuilder header = new StringBuilder ();
		StringBuilder impl = new StringBuilder ();
		List <string> headers = new List<string> ();
		string last_type = string.Empty;
		
		if (!Directory.Exists (plugin_dir))
			throw new ArgumentException (string.Format ("cgen must be executed from the base directory of the moon module ({0} does not exist).", plugin_dir));
		
		if (!Directory.Exists (moon_dir))
			throw new ArgumentException (string.Format ("methodgen must be executed from the base directory of the moon module ({0} does not exist).", moon_dir));
		
		methods = info.CPPMethodsToBind;
		
		Helper.WriteWarningGenerated (header);;
		Helper.WriteWarningGenerated (impl);

		header.AppendLine ("#ifndef __MOONLIGHT_C_BINDING_H__");
		header.AppendLine ("#define __MOONLIGHT_C_BINDING_H__");
		header.AppendLine ();
		header.AppendLine ("#include <glib.h>");
		header.AppendLine ("// This should probably be changed to somehow not include c++ headers.");
		foreach (MemberInfo method in methods) {
			string h;
			if (string.IsNullOrEmpty (method.Header))
				continue;
			h = Path.GetFileName (method.Header);
			
			if (!headers.Contains (h))
				headers.Add (h);
		}
		headers.Sort ();
		foreach (string h in headers) {
			header.Append ("#include \"");
			header.Append (h);
			header.AppendLine ("\"");
		}
		
		header.AppendLine ();
		header.AppendLine ("G_BEGIN_DECLS");
		header.AppendLine ();
		
		impl.AppendLine ("#ifdef HAVE_CONFIG_H");
		impl.AppendLine ("#include <config.h>");
		impl.AppendLine ("#endif");
		impl.AppendLine ();
		impl.AppendLine ("#include <stdio.h>");
		impl.AppendLine ("#include <stdlib.h>");
		impl.AppendLine ();
		impl.AppendLine ("#include \"cbinding.h\"");
		impl.AppendLine ();
		
		foreach (MemberInfo member in methods) {
			MethodInfo method = (MethodInfo) member;			
			
			if (last_type != method.Parent.Name) {
				last_type = method.Parent.Name;
				foreach (StringBuilder text in new StringBuilder [] {header, impl}) {
					text.AppendLine ("/**");
					text.Append (" * ");
					text.AppendLine (last_type);
					text.AppendLine (" **/");
				}
			}
			
			WriteHeaderMethod (method.CMethod, method, header);
			header.AppendLine ();
			
			WriteImplMethod (method.CMethod, method, impl);
			impl.AppendLine ();
			impl.AppendLine ();
		}
		
		header.AppendLine ();
		header.AppendLine ("G_END_DECLS");
		header.AppendLine ();
		header.AppendLine ("#endif");
		
		Helper.WriteAllText (Path.Combine (moon_dir, "cbinding.h"), header.ToString ());
		Helper.WriteAllText (Path.Combine (moon_dir, "cbinding.cpp"), impl.ToString ());
	}
	
	static void WriteHeaderMethod (MethodInfo cmethod, MethodInfo cppmethod, StringBuilder text)	
	{
		Log.WriteLine ("Writing header: {0}::{1} (Version: '{2}', GenerateManaged: {3})", 
		               cmethod.Parent.Name, cmethod.Name, 
		               cmethod.Annotations.GetValue ("Version"),
		               cmethod.Annotations.ContainsKey ("GenerateManaged"));
		
		if (cmethod.Annotations.ContainsKey ("GeneratePInvoke"))
			text.AppendLine ("/* @GeneratePInvoke */");
		cmethod.ReturnType.Write (text, SignatureType.Native);
		if (!cmethod.ReturnType.IsPointer)
			text.Append (" ");
		text.Append (cmethod.Name);
		cmethod.Parameters.Write (text, SignatureType.Native, false);
		text.AppendLine (";");
	}
	
	static void WriteImplMethod (MethodInfo cmethod, MethodInfo cppmethod, StringBuilder text)
	{
		bool is_void = cmethod.ReturnType.Value == "void";
		bool is_ctor = cmethod.IsConstructor;
		bool is_static = cmethod.IsStatic;
		bool is_dtor = cmethod.IsDestructor;
		bool check_instance = !is_static && !is_ctor;
		bool check_error = false;
		
		foreach (ParameterInfo parameter in cmethod.Parameters) {
			if (parameter.ParameterType.Value == "MoonError*") {
				check_error = true;
				break;
			}
		}
		
		cmethod.ReturnType.Write (text, SignatureType.Native);
		text.AppendLine ();
		text.Append (cmethod.Name);
		cmethod.Parameters.Write (text, SignatureType.Native, false);
		text.AppendLine ("");
		text.AppendLine ("{");
		
		if (is_ctor) {
			text.Append ("\treturn new ");
			text.Append (cmethod.Parent.Name);
			cmethod.Parameters.Write (text, SignatureType.Native, true);
			text.AppendLine (";");
		} else if (is_dtor) {
			text.AppendLine ("\tdelete instance;");
		} else {
			if (check_instance) {
				text.AppendLine ("\tif (instance == NULL)");
				
				if (cmethod.ReturnType.Value == "void") {
					text.Append ("\t\treturn");
				} else if (cmethod.ReturnType.Value.Contains ("*")) {	
					text.Append ("\t\treturn NULL");
				} else if (cmethod.ReturnType.Value == "Type::Kind") {
					text.Append ("\t\treturn Type::INVALID");
				} else if (cmethod.ReturnType.Value == "bool") {
					text.Append ("\t\treturn false");
				} else if (cmethod.ReturnType.Value == "Point") {
					text.Append ("\t\treturn Point (0, 0)");
				} else {
					text.AppendLine ("\t\t// Need to find a proper way to get the default value for the specified type and return that if instance is NULL.");
					text.Append ("\t\treturn");
					text.Append (" (");
					text.Append (cmethod.ReturnType.Value);
					text.Append (") 0");
				}
				text.AppendLine (";");
				
				text.AppendLine ("\t");
			}
			
			if (check_error) {
				text.AppendLine ("\tif (error == NULL)");
				text.Append ("\t\tg_warning (\"Moonlight: Called ");
				text.Append (cmethod.Name);
				text.AppendLine (" () with error == NULL.\");");
			}
			
			text.Append ("\t");
			if (!is_void)
				text.Append ("return ");
			
			if (is_static) {
				text.Append (cmethod.Parent.Name);
				text.Append ("::");
			} else {
				text.Append ("instance->");
				cmethod.Parameters [0].DisableWriteOnce = true;
			}
			text.Append (cppmethod.Name);
			cmethod.Parameters.Write (text, SignatureType.Native, true);
			text.AppendLine (";");
		}
		
		text.AppendLine ("}");
	}
	
	static void GenerateTypeStaticCpp (GlobalInfo all)
	{
		string header;
		List<string> headers = new List<string> ();
		
		StringBuilder text = new StringBuilder ();
		
		Helper.WriteWarningGenerated (text);
					
		text.AppendLine ("#ifdef HAVE_CONFIG_H");
		text.AppendLine ("#include <config.h>");
		text.AppendLine ("#endif");
		text.AppendLine ();
		text.AppendLine ("#include <stdlib.h>");

		headers.Add ("cbinding.h");
		foreach (TypeInfo t in all.Children.SortedTypesByKind) {
			if (t.IsPluginMember)
				continue;
			
			if (t.C_Constructor == string.Empty || t.C_Constructor == null || !t.GenerateCBindingCtor) {
				//Console.WriteLine ("{0} does not have a C ctor", t.FullName);
				if (t.GetTotalEventCount () == 0)
					continue;
			}
	
			if (string.IsNullOrEmpty (t.Header)) {
			//	Console.WriteLine ("{0} does not have a header", t.FullName);
				continue;
			}
			
			//Console.WriteLine ("{0}'s header is {1}", t.FullName, t.Header);
			
			header = Path.GetFileName (t.Header);
			if (!headers.Contains (header))
				headers.Add (header);
		}
		
		// Loop through all the classes and check which headers
		// are needed for the c constructors
		text.AppendLine ("");
		headers.Sort ();
		foreach (string h in headers) {
			text.Append ("#include \"");
			text.Append (h);
			text.AppendLine ("\"");
		}
		text.AppendLine ();
			
		foreach (TypeInfo t in all.Children.SortedTypesByKind) {
			if (t.GetEventCount () == 0)
				continue;
				
				
			foreach (FieldInfo field in t.Events) {
				text.Append ("const int ");
				text.Append (t.Name);
				text.Append ("::");
				text.Append (field.EventName);
				text.Append ("Event = ");
				text.Append (t.GetEventId (field));
				text.AppendLine (";");
			}
		}
	
		// Create the arrays of event names for the classes which have events
		text.AppendLine ("");
		foreach (TypeInfo t in all.Children.SortedTypesByKind) {
			if (t.Events == null || t.Events.Count == 0)
				continue;
				
			text.Append ("const char *");
			text.Append (t.Name);
			text.Append ("_Events [] = { ");
				
			foreach (FieldInfo field in t.Events) {
				text.Append ("\"");
				text.Append (field.EventName);
				text.Append ("\", ");
			}
				
			text.AppendLine ("NULL };");
		}
	
		// Create the array of type data
		text.AppendLine ("");
		text.AppendLine ("Type type_infos [] = {");
		text.AppendLine ("\t{ Type::INVALID, Type::INVALID, false, \"INVALID\", NULL, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL },");
		foreach (TypeInfo type in all.Children.SortedTypesByKind) {
			MemberInfo member;
			TypeInfo parent = null;
			string events = "NULL";
				
			if (!type.Annotations.ContainsKey ("IncludeInKinds"))
				continue;
				
			if (type.Base != null && type.Base.Value != null && all.Children.TryGetValue (type.Base.Value, out member))
				parent = (TypeInfo) member;
				
			if (type.Events != null && type.Events.Count != 0)
				events = type.Name + "_Events";
	
			text.AppendLine (string.Format (@"	{{ {0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}, NULL, NULL, NULL }}, ",
							"Type::" + type.KindName, 
							type.KindName == "OBJECT" ? "Type::INVALID" : ("Type::" + (parent != null ? parent.KindName : "OBJECT")),
							type.IsValueType ? "true" : "false",
							"\"" + type.Name + "\"", 
							"\"" + type.KindName + "\"", 
							type.GetEventCount (),
							type.GetTotalEventCount (),
							events,
							(type.C_Constructor != null && type.GenerateCBindingCtor) ? string.Concat ("(create_inst_func *) ", type.C_Constructor) : "NULL", 
							type.ContentProperty != null ? string.Concat ("\"", type.ContentProperty, "\"") : "NULL"
							)
					 );
		}

		text.AppendLine ("\t{ Type::LASTTYPE, Type::INVALID, false, NULL, NULL, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL }");
		text.AppendLine ("};");

		text.AppendLine ();
				
		Helper.WriteAllText ("src/type-generated.cpp", text.ToString ());
	}
	
	static void GenerateTypeH (GlobalInfo all)
	{
		const string file = "src/type.h";
		StringBuilder text;
		string contents = File.ReadAllText (file + ".in");
		
		contents = contents.Replace ("/*DO_KINDS*/", all.Children.GetKindsForEnum ().ToString ());

		text = new StringBuilder ();
					
		Helper.WriteWarningGenerated (text);
		
		contents = text.ToString () + contents;

		Helper.WriteAllText (file, contents);
	}

	static void GenerateKindCs ()
	{
		const string file = "src/type.h";
		StringBuilder text = new StringBuilder ();
		string contents = File.ReadAllText (file);
		int a = contents.IndexOf ("// START_MANAGED_MAPPING") + "// START_MANAGED_MAPPING".Length;
		int b = contents.IndexOf ("// END_MANAGED_MAPPING");
		string values = contents.Substring (a, b - a);		
		
		Helper.WriteWarningGenerated (text);
					
		text.AppendLine ("namespace Mono {");
		text.AppendLine ("#if NET_2_1");
		text.AppendLine ("\tinternal enum Kind {");
		text.AppendLine ("#else");
		text.AppendLine ("\tpublic enum Kind {");
		text.AppendLine ("#endif");
		text.AppendLine (values);
		text.AppendLine ("\t}");
		text.AppendLine ("}");
		
		string realfile = "class/System.Windows/Mono/Kind.cs".Replace ('/', Path.DirectorySeparatorChar);
		Helper.WriteAllText (realfile, text.ToString ());
	}
	
	static void GenerateValueH (GlobalInfo all)
	{
		const string file = "src/value.h";
		StringBuilder result = new StringBuilder ();	

		Helper.WriteWarningGenerated (result);

		using (StreamReader reader = new StreamReader (file + ".in")) {
			string line;
			line = reader.ReadLine ();
			while (line != null) {
				if (line.Contains ("/*DO_FWD_DECLS*/")) {
					foreach (TypeInfo type in all.Children.SortedTypesByKind) {
						if (!type.Annotations.ContainsKey("IncludeInKinds") ||
						    type.Annotations.ContainsKey("SkipValue") ||
						    type.IsNested ||
						    type.IsStruct)
							continue;

						if (type.IsStruct) {
							result.Append ("struct ");
						} else {
							result.Append ("class ");
						}
						result.Append (type.Name);
						result.AppendLine (";");
					}
					result.AppendLine ();
				} else if (line.Contains ("/*DO_AS*/")) {
					foreach (TypeInfo type in all.Children.SortedTypesByKind) {
						if (!type.Annotations.ContainsKey("IncludeInKinds") ||
						    type.Annotations.ContainsKey("SkipValue") ||
						    type.IsNested ||
						    type.IsStruct)
							continue;
			
						//do_as.AppendLine (string.Format ("	{1,-30} As{0} () {{ checked_get_subclass (Type::{2}, {0}) }}", type.Name, type.Name + "*", type.KindName));
						
						result.Append ('\t');
						result.Append (type.Name);
						result.Append ("*");
						result.Append (' ', 30 - type.Name.Length);
						result.Append ("As");
						result.Append (type.Name);
						result.Append (" () { checked_get_subclass (Type::");
						result.Append (type.KindName);
						result.Append (", ");
						result.Append (type.Name);
						result.Append (") }");
						result.AppendLine ();
					}
					result.AppendLine ();
				} else {
					result.AppendLine (line);
				}
				line = reader.ReadLine ();
			}
		}
		
		Helper.WriteAllText (file, result.ToString ());
	}
	
	static bool IsManuallyDefined (string NativeMethods_cs, string method)
	{
		if (NativeMethods_cs.Contains (" " + method + " "))
			return true;
		else if (NativeMethods_cs.Contains (" " + method + "("))
			return true;
		else if (NativeMethods_cs.Contains ("\"" + method + "\""))
			return true;
		else
			return false;
	}
	
	static void GeneratePInvokes (GlobalInfo all)
	{
		string base_dir = Environment.CurrentDirectory;
		List <MethodInfo> methods = new List<MethodInfo> ();
		StringBuilder text = new StringBuilder ();
		string NativeMethods_cs;
		
		NativeMethods_cs = File.ReadAllText (Path.Combine (base_dir, "class/System.Windows/Mono/NativeMethods.cs".Replace ('/', Path.DirectorySeparatorChar)));

		methods = all.CPPMethodsToBind;		

		foreach (MemberInfo info in all.Children.Values) {
			MethodInfo minfo = info as MethodInfo;
			if (minfo == null)
				continue;
			if (!minfo.Annotations.ContainsKey ("GeneratePInvoke"))
				continue;
			foreach (MethodInfo mi in methods) {
				if (mi.CMethod.Name == minfo.Name) {
					minfo = null;
					break;
				}
			}
			if (minfo == null)
				continue;
			//Console.WriteLine ("Added: {0} IsSrc: {1} IsPlugin: {2} Header: {3}", minfo.Name, minfo.IsSrcMember, minfo.IsPluginMember, minfo.Header);
			methods.Add (minfo);
		}
		
		Helper.WriteWarningGenerated (text);
		text.AppendLine ("using System;");
		text.AppendLine ("using System.Windows;");
		text.AppendLine ("using System.Runtime.InteropServices;");
		text.AppendLine ("");
		text.AppendLine ("namespace Mono {");
		text.AppendLine ("\tinternal static partial class NativeMethods");
		text.AppendLine ("\t{");
		text.AppendLine ("\t\t/* moonplugin methods */");
		text.AppendLine ("\t");
		foreach (MethodInfo method in methods) {
			if (!method.IsPluginMember || !method.Annotations.ContainsKey ("GeneratePInvoke"))
				continue;
			WritePInvokeMethod (NativeMethods_cs, method, text, "moonplugin");
			text.AppendLine ();
		}
		
		text.AppendLine ("\t");
		text.AppendLine ("\t\t/* libmoon methods */");
		text.AppendLine ("\t");
		foreach (MethodInfo method in methods) {
			if (!method.IsSrcMember || !method.Annotations.ContainsKey ("GeneratePInvoke"))
				continue;
			WritePInvokeMethod (NativeMethods_cs, method, text, "moon");
			text.AppendLine ();
		}
		text.AppendLine ("\t}");
		text.AppendLine ("}");
		
		Helper.WriteAllText (Path.Combine (base_dir, "class/System.Windows/Mono/GeneratedPInvokes.cs".Replace ('/', Path.DirectorySeparatorChar)), text.ToString ());
	}
	
	static void WritePInvokeMethod (string NativeMethods_cs, MethodInfo method, StringBuilder text, string library)
	{
		bool marshal_string_returntype = false;
		bool marshal_moonerror = false;
		bool generate_wrapper = false;
//		bool is_manually_defined;
		bool comment_out;
		bool is_void = false;
		bool contains_unknown_types;
		string name;
		string managed_name;
		string tabs;
		TypeReference returntype;
		MethodInfo cmethod = method.CMethod;
		ParameterInfo error_parameter = null;
		
		if (method.ReturnType == null)
			throw new Exception (string.Format ("Method {0} in type {1} does not have a return type.", method.Name, method.Parent.Name));
		
		if (method.ReturnType.Value == "char*") {
			marshal_string_returntype = true;
			generate_wrapper = true;
		} else if (method.ReturnType.Value == "void") {
			is_void = true;
		}
		
		// Check for parameters we can automatically generate code for.
		foreach (ParameterInfo parameter in cmethod.Parameters) {
			if (parameter.Name == "surface" && parameter.ParameterType.Value == "Surface*") {
				parameter.ManagedWrapperCode = "Mono.Xaml.XamlLoader.SurfaceInDomain";
				generate_wrapper = true;
			} else if (parameter.Name == "error" && parameter.ParameterType.Value == "MoonError*") {
				marshal_moonerror = true;
				generate_wrapper = true;
				error_parameter = parameter;
			}
		}
		
		name = method.CMethod.Name;
		managed_name = name;
		if (marshal_moonerror)
			managed_name = managed_name.Replace ("_with_error", "");
		
		returntype = method.ReturnType;
//		is_manually_defined = IsManuallyDefined (NativeMethods_cs, managed_name);
		contains_unknown_types = method.ContainsUnknownTypes;
		comment_out = contains_unknown_types;
		tabs = comment_out ? "\t\t// " : "\t\t";
				
//		if (is_manually_defined)
//			text.AppendLine ("\t\t// NOTE: There is a method in NativeMethod.cs with the same name.");

		if (contains_unknown_types)
			text.AppendLine ("\t\t// This method contains types the generator didn't know about. Fix the generator (find the method 'GetManagedType' in TypeReference.cs and add the missing case) and try again.");
			
		text.Append (tabs);
		text.Append ("[DllImport (\"");
		text.Append (library);
		if (generate_wrapper) {
			text.Append ("\", EntryPoint=\"");
			text.Append (name);
		}
		text.AppendLine ("\")]");

		if (method.ReturnType.Value == "bool") {
			text.Append (tabs);
			text.AppendLine ("[return: MarshalAs (UnmanagedType.U1)]");
		}
		
		// Always output the native signature too, makes it easier to check if the generation is wrong.
		text.Append ("\t\t// ");
		cmethod.WriteFormatted (text);
		text.AppendLine ();
		
		text.Append (tabs);
		text.Append (generate_wrapper ? "private " : "public ");
		text.Append ("extern static ");
		if (marshal_string_returntype)
			text.Append ("IntPtr");
		else
			returntype.Write (text, SignatureType.PInvoke);
		text.Append (" ");
		text.Append (name);
		if (generate_wrapper)
			text.Append ("_");
		cmethod.Parameters.Write (text, SignatureType.PInvoke, false);
		text.AppendLine (";");
		
		if (generate_wrapper) {
			text.Append (tabs);
			text.Append ("public static ");
			returntype.Write (text, SignatureType.Managed);
			text.Append (" ");
			text.Append (managed_name);
			
			foreach (ParameterInfo parameter in cmethod.Parameters)
				parameter.DisableWriteOnce = parameter.ManagedWrapperCode != null;

			if (error_parameter != null)
				error_parameter.DisableWriteOnce = true;
			
			cmethod.Parameters.Write (text, SignatureType.Managed, false);
			text.AppendLine ();
			
			text.Append (tabs);
			text.Append ("{");
			text.AppendLine ();
			
			text.Append (tabs);
			
			if (marshal_string_returntype) {
				text.AppendLine ("\tIntPtr result;");
			} else if (!is_void) {
				text.Append ("\t");
				returntype.Write (text, SignatureType.Managed);
				text.AppendLine (" result;");
			}
			
			if (marshal_moonerror) {
				text.Append (tabs);
				text.AppendLine ("\tMoonError error;");
			}

			text.Append (tabs);
			text.Append ("\t");
			if (!is_void)
				text.Append ("result = ");
				
			text.Append (cmethod.Name);
			text.Append ("_");
			cmethod.Parameters.Write (text, SignatureType.Managed, true);
			
			text.AppendLine (";");
			
			if (marshal_moonerror) {
				text.Append (tabs);
				text.AppendLine ("\tif (error.Number != 0)");
				
				text.Append (tabs);
				text.AppendLine ("\t\tthrow CreateManagedException (error);");
			}
			
			if (marshal_string_returntype) {
				text.Append (tabs);
				text.AppendLine ("\tif (result == IntPtr.Zero)");
				text.Append (tabs);
				text.AppendLine ("\t\treturn null;");
				text.Append (tabs);
				text.AppendLine ("\tstring s = Marshal.PtrToStringAnsi (result);\t// *copy* unmanaged string");
				text.Append (tabs);
				text.AppendLine ("\tMarshal.FreeHGlobal (result);\t\t\t// g_free the unmanaged string");
				text.Append (tabs);
				text.AppendLine ("\treturn s;");
			} else if (!is_void) {
				text.Append (tabs);
				text.AppendLine ("\treturn result;");
			}
		
			text.Append (tabs);
			text.Append ("}");
			text.AppendLine ();
		}
	}
}

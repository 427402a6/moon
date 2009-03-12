// Contact:
//   Moonlight List (moonlight-list@lists.ximian.com)
//
// Copyright (C) 2008 Novell, Inc (http://www.novell.com)
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Browser;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Globalization;
using Mono;

namespace System.Windows.Browser
{

	delegate void InvokeDelegate (IntPtr obj_handle, IntPtr method_handle,
								[MarshalAs (UnmanagedType.LPStr)] string name,
								[MarshalAs (UnmanagedType.LPArray, SizeParamIndex = 4)]
								IntPtr[] args,
								int arg_count,
								ref Value return_value);

	delegate void SetPropertyDelegate (IntPtr obj_handle, string name, ref Value value);
	delegate void GetPropertyDelegate (IntPtr obj_handle, string name, ref Value value);
	delegate void EventHandlerDelegate (IntPtr obj_handle, IntPtr event_handle, IntPtr scriptable_obj, IntPtr closure);
	
	internal class ScriptableObjectWrapper : ScriptObject {

		static InvokeDelegate invoke = new InvokeDelegate (InvokeFromUnmanagedSafe);
		static SetPropertyDelegate set_prop = new SetPropertyDelegate (SetPropertyFromUnmanagedSafe);
		static GetPropertyDelegate get_prop = new GetPropertyDelegate (GetPropertyFromUnmanagedSafe);
		static EventHandlerDelegate add_event = new EventHandlerDelegate (AddEventFromUnmanagedSafe);
		static EventHandlerDelegate remove_event = new EventHandlerDelegate (RemoveEventFromUnmanagedSafe);

		Dictionary<IntPtr, Delegate> events;
		Dictionary<string, List<MethodInfo>> methods;
		Dictionary<string, PropertyInfo> properties;

		GCHandle obj_handle;
		IntPtr moon_handle;
		public IntPtr MoonHandle {
			get { return moon_handle; }
		}

		public bool HasTypes { get; set; }

		static ScriptableObjectWrapper ()
		{
		}

		public ScriptableObjectWrapper () : this(null)
		{
		}

		public ScriptableObjectWrapper (object obj) : this(obj, IntPtr.Zero)
		{
		}

		public ScriptableObjectWrapper (object obj, IntPtr parent) : base (obj)
		{
			this.events = new Dictionary<IntPtr, Delegate> ();
			this.methods = new Dictionary<string, List<MethodInfo>> ();
			this.properties = new Dictionary<string, PropertyInfo> ();

			obj_handle = GCHandle.Alloc (this);
			if (parent == IntPtr.Zero) {
				moon_handle = ScriptableNativeMethods.wrapper_create_root (
							WebApplication.Current.PluginHandle,
							(IntPtr) obj_handle,
							invoke,
							set_prop,
							get_prop,
							add_event,
							remove_event);
			} else {
				moon_handle = ScriptableNativeMethods.wrapper_create (parent,
							(IntPtr) obj_handle,
							invoke,
							set_prop,
							get_prop,
							add_event,
							remove_event);
			}
			handle = ScriptableNativeMethods.moonlight_object_to_npobject (moon_handle);
			WebApplication.CachedObjects [handle] = new WeakReference (this);
		}

		public void Register (string scriptKey)
		{
			ScriptableNativeMethods.register (WebApplication.Current.PluginHandle, scriptKey, moon_handle);
		}

		public void AddProperty (PropertyInfo pi)
		{
			TypeCode tc = Type.GetTypeCode (pi.PropertyType);

			string name = pi.Name;
			if (pi.IsDefined (typeof(ScriptableMemberAttribute), false)) {
			    ScriptableMemberAttribute att = (ScriptableMemberAttribute) pi.GetCustomAttributes (typeof (ScriptableMemberAttribute), false)[0];
				name = (att.ScriptAlias ?? name);
			}

			properties[name] = pi;
			ScriptableNativeMethods.add_property (WebApplication.Current.PluginHandle,
									moon_handle,
									IntPtr.Zero,
									name,
									tc,
									pi.CanRead,
									pi.CanWrite);
		}

		public void AddEvent (EventInfo ei)
		{
			GCHandle event_handle = GCHandle.Alloc (ei);

			string name = ei.Name;
			if (ei.IsDefined (typeof(ScriptableMemberAttribute), false)) {
			    ScriptableMemberAttribute att = (ScriptableMemberAttribute) ei.GetCustomAttributes (typeof (ScriptableMemberAttribute), false)[0];
				name = (att.ScriptAlias ?? name);
			}
			ScriptableNativeMethods.add_event (WebApplication.Current.PluginHandle,
									moon_handle,
									(IntPtr)event_handle,
									name);
		}

		public void AddMethod (string name, TypeCode[] args, TypeCode ret_type)
		{
			ScriptableNativeMethods.add_method (
								WebApplication.Current.PluginHandle,
								moon_handle,
								IntPtr.Zero,
								name,
								ret_type,
								args,
								args.Length);
		}

		public void AddMethod (MethodInfo mi)
		{
			ParameterInfo[] ps = mi.GetParameters();
			TypeCode[] tcs = new TypeCode [ps.Length];

			foreach (ParameterInfo p in ps) {
				TypeCode pc = Type.GetTypeCode (p.ParameterType);
				tcs[p.Position] = pc;
			}

			string name = mi.Name;
			if (mi.IsDefined (typeof(ScriptableMemberAttribute), false)) {
			    ScriptableMemberAttribute att = (ScriptableMemberAttribute) mi.GetCustomAttributes (typeof (ScriptableMemberAttribute), false)[0];
				name = (att.ScriptAlias ?? name);
			}

			if (!methods.ContainsKey (name)) {
				methods[name] = new List<MethodInfo>();
			}
			methods[name].Add (mi);

			ScriptableNativeMethods.add_method (WebApplication.Current.PluginHandle,
								moon_handle,
								IntPtr.Zero,
								name,
								mi.ReturnType == typeof(void) ? 0 : Type.GetTypeCode (mi.ReturnType),
								tcs,
								tcs.Length);
		}

		[ScriptableMember(ScriptAlias="createObject")]
		public ScriptableObjectWrapper CreateObject (string name)
		{
			if (!WebApplication.ScriptableTypes.ContainsKey (name))
				return null;

			object o = Activator.CreateInstance (WebApplication.ScriptableTypes[name]);
			return ScriptableObjectGenerator.Generate (o, false);
		}

		internal static T CreateInstance<T> (IntPtr ptr)
		{
			ConstructorInfo i = typeof(T).GetConstructor (BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance,
			                          null, new Type[]{typeof(IntPtr)}, null);

			object o = i.Invoke (new object[]{ptr});
			WebApplication.CachedObjects[ptr] = o;
			return (T) o;
		}

		internal static object ObjectFromValue<T> (Value v)
		{
			switch (v.k) {
			case Kind.BOOL:
				return v.u.i32 != 0;
			case Kind.UINT64:
				return v.u.ui64;
			case Kind.INT32:
				return v.u.i32;
			case Kind.INT64:
				return v.u.i64;
			case Kind.DOUBLE:
				return v.u.d;
			case Kind.STRING:
				return Marshal.PtrToStringAnsi (v.u.p);
			case Kind.NPOBJ:
				// FIXME: Move all of this one caller up
				Type type = typeof (T);
				if (type.Equals (typeof(IntPtr)))
				    return v.u.p;

				object reference;
				if (WebApplication.CachedObjects.TryGetValue (v.u.p, out reference)) {
					if (reference is WeakReference) {
						if (((WeakReference)reference).IsAlive)
							return (T) ((WeakReference)reference).Target;
						else
							WebApplication.CachedObjects.Remove (v.u.p);
					} else {
						return (T) reference;
					}
				}

				if (!type.Equals (typeof(object)) && typeof (ScriptObject).IsAssignableFrom (type)) {
					return CreateInstance<T> (v.u.p);
				} else if (type.Equals (typeof(object))) {
					if (NativeMethods.html_object_has_property (WebApplication.Current.PluginHandle, v.u.p, "nodeType")) {
						Value val;
						NativeMethods.html_object_get_property (WebApplication.Current.PluginHandle, v.u.p, "nodeType", out val);

						if (v.u.i32 == 9) // HtmlDocument
							return CreateInstance<HtmlDocument> (v.u.p);
						else if (v.u.i32 == 1) //HtmlElement
							return CreateInstance<HtmlElement> (v.u.p);
					}
					return CreateInstance<ScriptObject> (v.u.p);
				} else
					return v.u.p;
			default:
				Console.WriteLine ("unsupported Kind.{0}", v.k);
				throw new NotSupportedException ();
			}
		}

		internal static void ValueFromObject (ref Value v, object o)
		{
			if (o == null) {
				v.k = Kind.NPOBJ;
				v.u.p = IntPtr.Zero;
				return;
			}

			switch (Type.GetTypeCode (o.GetType())) {
			case TypeCode.Boolean:
				v.k = Kind.BOOL;
				v.u.i32 = ((bool) o) ? 1 : 0;
				break;
			case TypeCode.Double:
				v.k = Kind.DOUBLE;
				v.u.d = (double)o;
				break;
			case TypeCode.Int32:
				v.k = Kind.INT32;
				v.u.i32 = (int)o;
				break;
			case TypeCode.UInt32:
				v.k = Kind.UINT32;
				v.u.ui32 = (uint)o;
				break;
			case TypeCode.Int64:
				v.k = Kind.INT64;
				v.u.i64 = (long)o;
				break;
			case TypeCode.UInt64:
				v.k = Kind.UINT64;
				v.u.ui64 = (ulong)o;
				break;
			case TypeCode.String:
				v.k = Kind.STRING;
				byte[] bytes = System.Text.Encoding.UTF8.GetBytes (o as string);
				IntPtr result = Helper.AllocHGlobal (bytes.Length + 1);
				Marshal.Copy (bytes, 0, result, bytes.Length);
				Marshal.WriteByte (result, bytes.Length, 0);
				v.u.p = result;
				break;
			case TypeCode.Object:
				//Console.Write ("Trying to marshal managed object {0}...", o.GetType ().FullName);
				ScriptObject so = o as ScriptObject;
				if (so != null) {
					v.u.p = so.Handle;
					v.k = Kind.NPOBJ;
				} else if (ScriptableObjectGenerator.ValidateType (o.GetType())) {
					ScriptableObjectWrapper wrapper = ScriptableObjectGenerator.Generate (o, false);
					v.u.p = wrapper.Handle;
					v.k = Kind.NPOBJ;
				} else {
					GCHandle handle = new GCHandle ();
					handle.Target = o;
					v.u.p = Helper.GCHandleToIntPtr (handle);
					v.k = Kind.MANAGED;
				}
				//Console.WriteLine ("  Marshalled as {0}", v.k);
				break;
			default:
				Console.WriteLine ("unsupported TypeCode.{0} = {1}", Type.GetTypeCode(o.GetType()), o.GetType ().FullName);
				throw new NotSupportedException ();
			}
		}

#region Methods

		bool ValidateArguments (MethodInfo mi, object[] args)
		{
			if (mi.GetParameters().Length != args.Length)
				return false;

			// TODO: refactor this, the js binder is doing this work already
			ParameterInfo[] parms = mi.GetParameters ();
			for (int i = 0; i < parms.Length; i++) {
				Type t1 = args[i].GetType();
				Type t2 = parms[i].ParameterType;
				if (t1 != t2 && Type.GetTypeCode (t1) != Type.GetTypeCode (t2)) {
					switch (Type.GetTypeCode (t2)) {
						case TypeCode.Int32:
							if (t1.IsPrimitive || (t1 == typeof(object)))
								continue;
							break;
						case TypeCode.String:
							if (t1 == typeof(char) || t1 == typeof(object) || t1 == typeof(Guid))
								continue;
							break;
					}
					return false;
				}
			}
			return true;
		}

		void Invoke (string name, object[] args, ref Value ret)
		{
			if (methods.ContainsKey (name)) {
				foreach (MethodInfo mi in methods[name]) {
					if (ValidateArguments (mi, args)) {
						Invoke (mi, args, ref ret);
						return;
					}
				}
			}

			switch (name.ToLower ()) {
				case "createmanagedobject":
					if (args.Length == 1) {
						ScriptableObjectWrapper wrapper = CreateObject ((string)args[0]);
						ValueFromObject (ref ret, wrapper);
					}
				break;
			}
		}

		void Invoke (MethodInfo mi, object[] args, ref Value ret)
		{
			object rv = mi.Invoke (this.ManagedObject, BindingFlags.Default, new JSFriendlyMethodBinder (), args, null);
			if (mi.ReturnType != typeof (void))
				ValueFromObject (ref ret, rv);
		}

		static void InvokeFromUnmanagedSafe (IntPtr obj_handle, IntPtr method_handle, string name, IntPtr[] uargs, int arg_count, ref Value return_value)
		{
			try {
				InvokeFromUnmanaged (obj_handle, method_handle, name, uargs, arg_count, ref return_value);
			} catch (Exception ex) {
				try {
					Console.WriteLine ("Moonlight: Unhandled exception in ScriptableObjectWrapper.InvokeFromUnmanagedSafe: {0}", ex);
				} catch {
				}
			}
		}
		
		static void InvokeFromUnmanaged (IntPtr obj_handle, IntPtr method_handle, string name, IntPtr[] uargs, int arg_count, ref Value return_value)
		{
			//Console.WriteLine ("Invoke " + name);

			ScriptableObjectWrapper obj = (ScriptableObjectWrapper) ((GCHandle)obj_handle).Target;
			object[] args = new object[arg_count];
			for (int i = 0; i < arg_count; i++) {
				Value v = (Value)Marshal.PtrToStructure (uargs[i], typeof (Value));
				args[i] = ObjectFromValue<object> (v);
			}

			if (method_handle == IntPtr.Zero) {
				obj.Invoke (name, args, ref return_value);
			} else {
				throw new Exception ("Invalid method invoke");
/*
				MethodInfo mi = (MethodInfo)((GCHandle)method_handle).Target;

				object[] margs = new object[arg_count];
				ParameterInfo[] pis = mi.GetParameters ();

				//Console.WriteLine ("arg_count = {0}", arg_count);
				for (int i = 0; i < arg_count; i ++) {
					Value v = (Value)Marshal.PtrToStructure (uargs[i], typeof (Value));

					object o = ObjectFromValue<object> (v);
					//Console.WriteLine ("margs[{1}] = {2} ({0})", o.GetType(), i, o);

					margs[i] = o; //Convert.ChangeType (o, pis[i].ParameterType);
				}

				obj.Invoke (mi, margs, ref return_value);
*/
			}
		}

#endregion

#region Properties

		public override void SetProperty (string name, object value)
		{
			PropertyInfo pi = properties[name];
			pi.SetValue (this.ManagedObject, value, null);
		}

		static void SetPropertyFromUnmanagedSafe (IntPtr obj_handle, string name, ref Value value)
		{
			try {
				SetPropertyFromUnmanaged (obj_handle, name, ref value);
			} catch (Exception ex) {
				try {
					Console.WriteLine ("Moonlight: Unhandled exception in ScriptableObjectWrapper.SetPropertyFromUnmanagedSafe: {0}", ex);
				} catch {
				}
			}
		}
		
		static void SetPropertyFromUnmanaged (IntPtr obj_handle, string name, ref Value value)
		{
			ScriptableObjectWrapper obj = (ScriptableObjectWrapper) ((GCHandle)obj_handle).Target;
			object v = ObjectFromValue<object> (value);
			obj.SetProperty (name, v);
		}

		public override object GetProperty (string name)
		{
			PropertyInfo pi = properties[name];
			return pi.GetValue (this.ManagedObject, null);
		}

		static void GetPropertyFromUnmanagedSafe (IntPtr obj_handle, string name, ref Value value)
		{
			try {
				GetPropertyFromUnmanaged (obj_handle, name, ref value);
			} catch (Exception ex) {
				try {
					Console.WriteLine ("Moonlight: Unhandled exception in ScriptableObjectWrapper.GetPropertyFromUnmanagedSafe: {0}", ex);
				} catch {
				}
			}
		}
		
		static void GetPropertyFromUnmanaged (IntPtr obj_handle, string name, ref Value value)
		{
			ScriptableObjectWrapper obj = (ScriptableObjectWrapper) ((GCHandle)obj_handle).Target;
			object v = obj.GetProperty (name);

			if (Type.GetTypeCode (v.GetType ()) == TypeCode.Object) {
				v = ScriptableObjectGenerator.Generate (v, false); // the type has already been validated
			}

			ValueFromObject (ref value, v);
		}

#endregion

#region Events
		void AddEvent (EventInfo ei, IntPtr scriptable_handle, IntPtr closure)
		{
			Delegate d = new EventDelegate (ei.EventHandlerType, scriptable_handle, closure).Delegate;
			ei.AddEventHandler (this.ManagedObject, d);
			if (!this.events.ContainsKey (closure))
				this.events[closure] = d;
		}

		static void AddEventFromUnmanagedSafe (IntPtr obj_handle, IntPtr event_handle, IntPtr scriptable_obj, IntPtr closure)
		{
			try {
				AddEventFromUnmanaged (obj_handle, event_handle, scriptable_obj, closure);
			} catch (Exception ex) {
				try {
					Console.WriteLine ("Moonlight: Unhandled exception in ScriptableObjectWrapper.AddEventFromUnmanagedSafe: {0}", ex);
				} catch {
				}
			}
		}
		
		static void AddEventFromUnmanaged (IntPtr obj_handle, IntPtr event_handle, IntPtr scriptable_obj, IntPtr closure)
		{
			ScriptableObjectWrapper obj = (ScriptableObjectWrapper) ((GCHandle)obj_handle).Target;
			EventInfo ei = (EventInfo)((GCHandle)event_handle).Target;
			obj.AddEvent (ei, scriptable_obj, closure);
		}

		void RemoveEvent (EventInfo ei, IntPtr closure)
		{
			Delegate d = this.events[closure];
			ei.RemoveEventHandler (this.ManagedObject, d);
			events.Remove (closure);
		}

		static void RemoveEventFromUnmanagedSafe (IntPtr obj_handle, IntPtr event_handle, IntPtr scriptable_obj, IntPtr closure)
		{
			try {
				RemoveEventFromUnmanaged (obj_handle, event_handle, scriptable_obj, closure);
			} catch (Exception ex) {
				try {
					Console.WriteLine ("Moonlight: Unhandled exception in ScriptableObjectWrapper.RemoveEventFromUnmanagedSafe: {0}", ex);
				} catch {
				}
			}
		}
		
		static void RemoveEventFromUnmanaged (IntPtr obj_handle, IntPtr event_handle, IntPtr scriptable_obj, IntPtr closure)
		{
			ScriptableObjectWrapper obj = (ScriptableObjectWrapper) ((GCHandle)obj_handle).Target;
			EventInfo ei = (EventInfo)((GCHandle)event_handle).Target;

			obj.RemoveEvent (ei, closure);
		}

		class EventDelegate {
			public EventDelegate (Type event_handler_type, IntPtr scriptable_handle, IntPtr closure)
			{
				this.event_handler_type = event_handler_type;
				this.scriptable_handle = scriptable_handle;
				this.closure = closure;
			}

			Type event_handler_type;
			IntPtr scriptable_handle;
			IntPtr closure;

			public Delegate Delegate {
				get {
					return Delegate.CreateDelegate (event_handler_type, this, GetType().GetMethod ("del"));
				}
			}

			public void del (object sender, object args)
			{
				// don't need to validate the type
				// again, this was done when the class
				// containing the event was validated.
				ScriptableObjectWrapper event_wrapper = ScriptableObjectGenerator.Generate (args, false);

				//Console.WriteLine ("emitting scriptable event!");

				ScriptableNativeMethods.emit_event (WebApplication.Current.PluginHandle,
								    scriptable_handle,
								    event_wrapper.MoonHandle,
								    closure);
			}
		}

#endregion

		class ScriptableNativeMethods {
			[DllImport ("moonplugin", EntryPoint = "moonlight_scriptable_object_wrapper_create_root")]
			public static extern IntPtr wrapper_create_root (IntPtr plugin, IntPtr obj_handle,
									InvokeDelegate invoke,
									SetPropertyDelegate set_prop,
									GetPropertyDelegate get_prop,
									EventHandlerDelegate add_event,
									EventHandlerDelegate remove_event);

			[DllImport ("moonplugin", EntryPoint = "moonlight_scriptable_object_wrapper_create")]
			public static extern IntPtr wrapper_create (IntPtr parent_handle, IntPtr obj_handle,
									InvokeDelegate invoke,
									SetPropertyDelegate set_prop,
									GetPropertyDelegate get_prop,
									EventHandlerDelegate add_event,
									EventHandlerDelegate remove_event);

			[DllImport ("moonplugin", EntryPoint = "moonlight_scriptable_object_add_property")]
			public static extern void add_property (IntPtr plugin_handle,
								IntPtr wrapper,
								IntPtr property_handle,
								string property_name,
								TypeCode property_type,
								bool readable,
								bool writable);

			[DllImport ("moonplugin", EntryPoint = "moonlight_scriptable_object_add_event")]
			public static extern void add_event (IntPtr plugin_handle,
								IntPtr wrapper,
								IntPtr event_handle,
								string event_name);

			[DllImport ("moonplugin", EntryPoint = "moonlight_scriptable_object_add_method")]
			public static extern void add_method (IntPtr plugin_handle,
								IntPtr wrapper,
								IntPtr method_handle,
								string method_name,
								TypeCode method_return_type,
								TypeCode[] method_parameter_types,
								int parameter_count);

			[DllImport ("moonplugin", EntryPoint = "moonlight_scriptable_object_emit_event")]
			public static extern void emit_event (IntPtr plugin_handle,
								IntPtr scriptable_obj,
								IntPtr event_wrapper,
								IntPtr closure);

			[DllImport ("moonplugin", EntryPoint = "moonlight_scriptable_object_register")]
			public static extern void register (IntPtr plugin_handle,
								string name,
								IntPtr wrapper);

			[DllImport ("moonplugin", EntryPoint = "moonlight_object_to_npobject")]
			public static extern IntPtr moonlight_object_to_npobject (IntPtr obj);

			[DllImport ("moonplugin", EntryPoint = "npobject_to_moonlight_object")]
			public static extern IntPtr npobject_to_moonlight_object (IntPtr obj);	
		}
	}
}

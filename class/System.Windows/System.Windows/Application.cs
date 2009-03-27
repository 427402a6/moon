//
// Application.cs
//
// Contact:
//   Moonlight List (moonlight-list@lists.ximian.com)
//
// Copyright 2008 Novell, Inc.
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

using Mono;
using Mono.Xaml;
using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Windows.Controls;
using System.Windows.Resources;
using System.Windows.Interop;
using System.Collections;
using System.Collections.Generic;
using System.Resources;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Markup;

namespace System.Windows {

	public partial class Application : INativeDependencyObjectWrapper {
		//
		// Application instance fields
		//
		UIElement root_visual;
		SilverlightHost host;

		ApplyDefaultStyleCallback apply_default_style;
		ApplyStyleCallback apply_style;
		GetResourceCallback get_resource;

		static Application ()
		{
			ImportXamlNamespace ("clr-namespace:System.Windows;assembly:System.Windows.dll");
			ImportXamlNamespace ("clr-namespace:System.Windows.Controls;assembly:System.Windows.dll");
		}

		internal Application (IntPtr raw)
		{
			NativeHandle = raw;

			apply_default_style = new ApplyDefaultStyleCallback (apply_default_style_cb_safe);
			apply_style = new ApplyStyleCallback (apply_style_cb_safe);
			get_resource = new GetResourceCallback (get_resource_cb_safe);

			NativeMethods.application_register_callbacks (NativeHandle, apply_default_style, apply_style, get_resource);

			if (Current == null) {
				Current = this;

				if (Host.Source != null) {
					// IsolatedStorage (inside mscorlib.dll) needs some information about the XAP file
					// to initialize it's application and site directory storage.
					AppDomain ad = AppDomain.CurrentDomain;
					ad.SetData ("xap_uri", Host.Source.AbsoluteUri);
					ad.SetData ("xap_host", Host.Source.Host);
				}

				SynchronizationContext context = new System.Windows.Threading.DispatcherSynchronizationContext ();
				SynchronizationContext.SetSynchronizationContext (context);
			} else {
				root_visual = Current.root_visual;
			}
		}

		public Application () : this (NativeMethods.application_new ())
		{
		}

		internal void Terminate ()
		{
			if (Deployment.Current.XapDir == null)
				return;

			if (Exit != null)
				Exit (this, EventArgs.Empty);
			
			try {
				Helper.DeleteDirectory (Deployment.Current.XapDir);
				Deployment.Current.XapDir = null;
			} catch {
			}

			root_visual = null;
			Application.Current = null;

			// XXX free the application?
		}				


		Dictionary<Assembly, ResourceDictionary> assemblyToGenericXaml = new Dictionary<Assembly, ResourceDictionary>();

		void apply_default_style_cb_safe (IntPtr fwe_ptr, IntPtr type_info_ptr)
		{
			try {
				apply_default_style_cb (fwe_ptr, type_info_ptr);
			} catch (Exception ex) {
				try {
					Console.WriteLine ("Moonlight: Unhandled exception in Application.apply_default_style_cb_safe: {0}", ex);
				} catch {
				}
			}
		}
		
		void apply_default_style_cb (IntPtr fwe_ptr, IntPtr type_info_ptr)
		{
			ManagedTypeInfo type_info = (ManagedTypeInfo)Marshal.PtrToStructure (type_info_ptr, typeof (ManagedTypeInfo));
			Type type = null;

			string assembly_name = Helper.PtrToStringAuto (type_info.assembly_name);
			string full_name = Helper.PtrToStringAuto (type_info.full_name);

			Assembly asm = Application.GetAssembly (assembly_name);
			if (asm == null) {
				Console.Error.WriteLine ("failed to lookup assembly_name {0} while applying style", assembly_name);
				return;
			}

			type = asm.GetType (full_name);

			if (type == null) {
				Console.Error.WriteLine ("failed to lookup type {0} in assembly {1} while applying style", full_name, assembly_name);
				return;
			}

			Style s = GetGenericXamlStyleFor (type);
			if (s == null)
				return;

			NativeMethods.framework_element_set_default_style (fwe_ptr, s.native);
		}

		void apply_style_cb_safe (IntPtr fwe_ptr, IntPtr style_ptr)
		{
			try {
				apply_style_cb (fwe_ptr, style_ptr);
			} catch (Exception ex) {
				try {
					Console.WriteLine ("Moonlight: Unhandled exception in Application.apply_style_cb_safe: {0}", ex);
				} catch {
				}
			}
		}
		
		void apply_style_cb (IntPtr fwe_ptr, IntPtr style_ptr)
		{
#if not_needed
			FrameworkElement fwe = NativeDependencyObjectHelper.FromIntPtr(fwe_ptr) as FrameworkElement;
			if (fwe == null)
				return;
#endif

			Style style = NativeDependencyObjectHelper.FromIntPtr(style_ptr) as Style;
			if (style == null)
				return;

			style.ConvertSetterValues ();
		}

		internal Style GetGenericXamlStyleFor (Type type)
		{
			ResourceDictionary rd = null;

			if (assemblyToGenericXaml.ContainsKey (type.Assembly)) {
				rd = assemblyToGenericXaml[type.Assembly];
			}
			else {
				Console.WriteLine ("trying to load: /{0};component/themes/generic.xaml",
						   type.Assembly.GetName().Name);

				StreamResourceInfo info = null;

				try {
					info = GetResourceStream (new Uri (string.Format ("/{0};component/themes/generic.xaml",
											  type.Assembly.GetName().Name), UriKind.Relative));
				}
				catch (Exception e) {
					Console.WriteLine ("no generic.xaml for assembly {0}", type.Assembly.GetName().Name);
				}
				
				if (info != null) {
					using (StreamReader sr = new StreamReader (info.Stream)) {
						string generic_xaml = sr.ReadToEnd();

						ManagedXamlLoader loader = new ManagedXamlLoader (type.Assembly, Deployment.Current.Surface.Native, PluginHost.Handle);

						try {
							rd = loader.CreateDependencyObjectFromString (generic_xaml, false) as ResourceDictionary;
						}
						catch (Exception e) {
							Console.WriteLine ("failed generic.xaml parsing:");
							Console.WriteLine (e);
						}
					}
				}

				if (rd == null)
					// create an empty one so we don't fall into this block again for this assembly
					rd = new ResourceDictionary();

				assemblyToGenericXaml[type.Assembly] = rd;
			}

			return rd[type.FullName] as Style;
		}

		public static void LoadComponent (object component, Uri resourceLocator)
		{
			INativeDependencyObjectWrapper wrapper = component as INativeDependencyObjectWrapper;

			// XXX still needed for the app.surface reference when creating the ManagedXamlLoader
			Application app = wrapper as Application;
			
			if (wrapper == null)
				throw new ArgumentNullException ("component");

			if (resourceLocator == null)
				throw new ArgumentNullException ("resourceLocator");

			StreamResourceInfo sr = GetResourceStream (resourceLocator);

			// Does not seem to throw.
			if (sr == null)
				return;

			string xaml = new StreamReader (sr.Stream).ReadToEnd ();
			Assembly loading_asm = component.GetType ().Assembly;
			ManagedXamlLoader loader = new ManagedXamlLoader (loading_asm, Deployment.Current.Surface.Native, PluginHost.Handle);

			loader.Hydrate (wrapper.NativeHandle, xaml);
		}

		/*
		 * Resources take the following format:
		 * 	[/[AssemblyName;component/]]resourcename
		 * They will always be resolved in the following order:
		 * 	1. Application manifest resources
		 * 	2. XAP content
		 */
		public static StreamResourceInfo GetResourceStream (Uri uriResource)
		{
			if (uriResource == null)
				throw new ArgumentNullException ("uriResource");

			if (uriResource.IsAbsoluteUri && uriResource.Scheme != Uri.UriSchemeFile) {
				throw new ArgumentException ("Absolute uriResource");
			}

			// FIXME: URI must point to
			// - the application assembly (embedded resources)
			// - an assembly part of the application package (embedded resources)
			// - something included in the package

			Assembly assembly;
			string assembly_name;
			string resource;
			string loc = uriResource.ToString ();
			int p = loc.IndexOf (';');

			/* We have a resource of the format /assembly;component/resourcename */
			/* It looks like the / is optional tho.  *SIGH* */
			if (p > 0) {
				int l = loc [0] == '/' ? 1 : 0;
				assembly_name = loc.Substring (l, p - l);
				assembly = GetAssembly (assembly_name);
				if (assembly == null)
					return null;

				resource = loc.Substring (p + 11);
			} else {
				assembly = Deployment.Current.EntryAssembly;
				assembly_name = Deployment.Current.EntryPointAssembly;
				resource = loc [0] == '/' ? loc.Substring (1) : loc;	
			}

			try {
				var manager = new ResourceManager (assembly_name + ".g", assembly) { IgnoreCase = true };
				var stream = manager.GetStream (resource);
				if (stream != null)
					return new StreamResourceInfo (stream, string.Empty);
			} catch {}

			string res_file = Path.Combine (Deployment.Current.XapDir, resource);
			if (File.Exists (res_file))
				return StreamResourceInfo.FromFile (res_file);

			return null;
		}

		internal static IntPtr get_resource_cb_safe (string name, out int size)
		{
			try {
				return get_resource_cb (name, out size);
			} catch (Exception ex) {
				try {
					Console.WriteLine ("Moonlight: Unhandled exception in Application.get_resource_cb: {0}", ex);
				} catch {
				}
			}
			size = 0;
			return new IntPtr ();
		}
		
		internal static IntPtr get_resource_cb (string name, out int size)
		{
			size = 0;
			try {
				StreamResourceInfo info = GetResourceStream (new Uri (name, UriKind.Relative));

				if (info == null)
					return IntPtr.Zero;
				
				size = (int) info.Stream.Length;
				return Helper.StreamToIntPtr (info.Stream);
			} catch {
				return IntPtr.Zero;
			}
		}

		internal static Assembly GetAssembly (string name)
		{
			foreach (var assembly in Deployment.Current.Assemblies)
				if (assembly.GetName ().Name == name)
					return assembly;

			return null;
		}

		public static StreamResourceInfo GetResourceStream (StreamResourceInfo zipPackageStreamResourceInfo, Uri uriResource)
		{
			if (zipPackageStreamResourceInfo == null)
				throw new ArgumentNullException ("zipPackageStreamResourceInfo");
			if (uriResource == null)
				throw new ArgumentNullException ("resourceUri");
			
			MemoryStream ms = new MemoryStream ();
			ManagedStreamCallbacks source;
			ManagedStreamCallbacks dest;
			StreamWrapper source_wrapper;
			StreamWrapper dest_wrapper;

			source_wrapper = new StreamWrapper (zipPackageStreamResourceInfo.Stream);
			dest_wrapper = new StreamWrapper (ms);

			source = source_wrapper.GetCallbacks ();
			dest = dest_wrapper.GetCallbacks ();

			if (NativeMethods.managed_unzip_stream_to_stream (ref source, ref dest, uriResource.ToString ())) {
				ms.Seek (0, SeekOrigin.Begin);
				return new StreamResourceInfo (ms, null);
			}

			return null;
		}

		public static Application Current {
			get {
				IntPtr app = NativeMethods.application_get_current ();
				return NativeDependencyObjectHelper.Lookup (Kind.APPLICATION, app) as Application;
			}

			private set {
				NativeMethods.application_set_current (value == null ? IntPtr.Zero : value.NativeHandle);
			}
		}

		public ResourceDictionary Resources {
			get {
				return (ResourceDictionary) ((INativeDependencyObjectWrapper)this).GetValue (ResourcesProperty);
			}
		}

		public UIElement RootVisual {
			get {
				return root_visual;
			}

			set {
				if (value == null)
					throw new InvalidOperationException ();

				// Can only be set once according to the docs.
				if (root_visual != null)
					return;
				
				root_visual = value;

				NativeMethods.surface_attach (Deployment.Current.Surface.Native, root_visual.native);
			}
		}

		public SilverlightHost Host {
			get { return host ?? (host = new SilverlightHost ()); }
		}

		public event EventHandler Exit;
		public event StartupEventHandler Startup;
		public event EventHandler<ApplicationUnhandledExceptionEventArgs> UnhandledException;

		internal void OnStartup () {
			if (Startup != null){
				Startup (this, new StartupEventArgs ());
			}	
		}

		internal static Dictionary<XmlnsDefinitionAttribute,Assembly> xmlns_definitions = new Dictionary<XmlnsDefinitionAttribute, Assembly> ();
		internal static List<string> imported_namespaces = new List<string> ();
		
		internal static void LoadXmlnsDefinitionMappings (Assembly a)
		{
			object [] xmlns_defs = a.GetCustomAttributes (typeof (XmlnsDefinitionAttribute), false);

			foreach (XmlnsDefinitionAttribute ns_mapping in xmlns_defs){
				xmlns_definitions [ns_mapping] = a;
			}
		}

		internal static void ImportXamlNamespace (string xmlns)
		{
			imported_namespaces.Add (xmlns);
		}

		static Type GetType (Assembly assembly, string ns, string name)
		{
			var fullname = string.IsNullOrEmpty (ns) ? name : ns + "." + name;
			return assembly.GetType (fullname);
		}

		internal static Type GetComponentTypeFromName (string name)
		{
			foreach (var pair in xmlns_definitions) {
				if (!imported_namespaces.Contains (pair.Key.XmlNamespace))
					continue;

				var type = GetType (pair.Value, pair.Key.ClrNamespace, name);
				if (type != null)
					return type;
			}

			return null;
		}

		//
		// Creates the proper component by looking the namespace and name
		// in the various assemblies loaded
		//
		internal static object CreateComponentFromName (string name)
		{
			Type t = GetComponentTypeFromName (name);

			if (t == null) {
				Console.Error.WriteLine ("Application.CreateComponentFromName - could not find type: {0}", name);
				return null;
			}

			return Activator.CreateInstance (t);
		}

		internal static void OnUnhandledException (object sender, Exception ex)
		{
			if (Application.Current != null && Application.Current.UnhandledException != null) {
				ApplicationUnhandledExceptionEventArgs args = new ApplicationUnhandledExceptionEventArgs (ex, false);
				try {
					Application.Current.UnhandledException (Application.Current, args);
				} catch (Exception ex2) {
					Console.WriteLine ("Exception caught in Application UnhandledException handler: " + ex2);
				}
			} else {
				Console.WriteLine ("Unhandled Exception: " + ex);
			}
		}

		private static readonly DependencyProperty ResourcesProperty =
			DependencyProperty.Lookup (Kind.APPLICATION, "Resources", typeof (ResourceDictionary));

#region "INativeDependencyObjectWrapper interface"
		IntPtr _native;

		internal IntPtr NativeHandle {
			get { return _native; }
			set {
				if (_native != IntPtr.Zero) {
					throw new InvalidOperationException ("Application.native is already set");
				}

				NativeDependencyObjectHelper.AddNativeMapping (value, this);

				_native = value;
			}
		}

		IntPtr INativeDependencyObjectWrapper.NativeHandle {
			get { return NativeHandle; }
			set { NativeHandle = value; }
		}

		object INativeDependencyObjectWrapper.GetValue (DependencyProperty dp)
		{
			return NativeDependencyObjectHelper.GetValue (this, dp);
		}

		void INativeDependencyObjectWrapper.SetValue (DependencyProperty dp, object value)
		{
			NativeDependencyObjectHelper.SetValue (this, dp, value);
		}

		object INativeDependencyObjectWrapper.GetAnimationBaseValue (DependencyProperty dp)
		{
			return NativeDependencyObjectHelper.GetAnimationBaseValue (this, dp);
		}

		object INativeDependencyObjectWrapper.ReadLocalValue (DependencyProperty dp)
		{
			return NativeDependencyObjectHelper.ReadLocalValue (this, dp);
		}

		void INativeDependencyObjectWrapper.ClearValue (DependencyProperty dp)
		{
			NativeDependencyObjectHelper.ClearValue (this, dp);
		}

		Kind INativeDependencyObjectWrapper.GetKind ()
		{
			return Kind.APPLICATION;
		}

		bool INativeDependencyObjectWrapper.CheckAccess ()
		{
			return Thread.CurrentThread == DependencyObject.moonlight_thread;
		}
#endregion
	}
}

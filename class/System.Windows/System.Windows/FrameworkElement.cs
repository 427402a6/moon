//
// FrameworkElement.cs
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
using System.Security;

namespace System.Windows {
	public abstract class FrameworkElement : UIElement {
		public static readonly DependencyProperty HeightProperty =
			DependencyProperty.Lookup (Kind.FRAMEWORKELEMENT, "Height", typeof (double));

		public static readonly DependencyProperty WidthProperty =
			DependencyProperty.Lookup (Kind.FRAMEWORKELEMENT, "Width", typeof (double));
		
		internal FrameworkElement () : base (NativeMethods.framework_element_new ())
		{
			Console.WriteLine ("*** Created a {0} (frameworkelement) with {1}", this.GetType (), native);
		}
		
		internal FrameworkElement (IntPtr raw) : base (raw)
		{
		}
		
		internal override Kind GetKind ()
		{
			return Kind.FRAMEWORKELEMENT;
		}
		
		public double Height {
			get {
				return (double) GetValue (HeightProperty);
			}

			set {
				SetValue (HeightProperty, value);
			}
		}

		public DependencyObject Parent {
#if NET_2_1
			[SecuritySafeCritical]
#endif
			get {
				IntPtr parent_handle = NativeMethods.uielement_get_parent (native);
				if (parent_handle == IntPtr.Zero)
					return null;

				Kind k = NativeMethods.dependency_object_get_object_type (parent_handle);
				return DependencyObject.Lookup (k, parent_handle);
			}
		}

		public double Width {
			get {
				return (double) GetValue (WidthProperty);
			}

			set {
				SetValue (WidthProperty, value);
			}
		}
		
#if NET_2_1
		[SecuritySafeCritical]
#endif
		public object FindName (string name)
		{
			return DepObjectFindName (name);
		}
		
		static object LoadedEvent = new object ();
		
		public event RoutedEventHandler Loaded {
			add {
				if (events[LoadedEvent] == null)
					Events.AddHandler (this, "Loaded", Events.loaded);
				events.AddHandler (LoadedEvent, value);
			}
			remove {
				events.RemoveHandler (LoadedEvent, value);
				if (events[LoadedEvent] == null)
					Events.RemoveHandler (this, "Loaded", Events.loaded);
			}
		}
	}
}

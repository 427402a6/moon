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
using System;
using System.Windows;
using System.Windows.Media;
using System.Windows.Documents;
using Mono;

namespace System.Windows.Controls
{
	[TemplatePartAttribute(Name = "ContentElement", Type = typeof(FrameworkElement))]
	[TemplateVisualStateAttribute(Name = "Disabled", GroupName = "CommonStates")]
	[TemplatePartAttribute(Name = "DisabledVisualElement", Type = typeof(FrameworkElement))]
	[TemplateVisualStateAttribute(Name = "Focused", GroupName = "FocusStates")]
	[TemplatePartAttribute(Name = "FocusVisualElement", Type = typeof(FrameworkElement))]
	[TemplateVisualStateAttribute(Name = "MouseOver", GroupName = "CommonStates")]
	[TemplateVisualStateAttribute(Name = "Normal", GroupName = "CommonStates")]
	[TemplatePartAttribute(Name = "RootElement", Type = typeof(FrameworkElement))]
	[TemplateVisualStateAttribute(Name = "Unfocused", GroupName = "FocusStates")]
	public sealed partial class PasswordBox : Control
	{
		public static readonly DependencyProperty SelectionBackgroundProperty = DependencyProperty.Lookup (Kind.TEXTBOX, "SelectionBackground", typeof (Brush));
		public static readonly DependencyProperty SelectionForegroundProperty = DependencyProperty.Lookup (Kind.TEXTBOX, "SelectionForeground", typeof (Brush));
		
		public PasswordBox (string s)
			: base (NativeMethods.password_box_new ())
		{
			
		}
		
		public Brush SelectionBackground {
			get { return (Brush) GetValue (SelectionBackgroundProperty); }
			set { SetValue (SelectionBackgroundProperty, value); }
		}
		
		public Brush SelectionForeground {
			get { return (Brush) GetValue (SelectionForegroundProperty); }
			set { SetValue (SelectionForegroundProperty, value); }
		}
		
		public FontSource FontSource {
			get { throw new NotImplementedException (); }
			set { throw new NotImplementedException (); }
		}
		
		public void SelectAll ()
		{
			NativeMethods.text_box_select_all (native);
		}
		
		static UnmanagedEventHandler password_changed = Events.CreateSafeHandler (password_changed_cb);
		static object PasswordChangedEvent = new object ();
		
		void InvokePasswordChanged (RoutedEventArgs args)
		{
			RoutedEventHandler h = (RoutedEventHandler) EventList [PasswordChangedEvent];
			
			if (h != null)
				h (this, args);
		}
		
		static void password_changed_cb (IntPtr target, IntPtr calldata, IntPtr closure)
		{
			PasswordBox passwordbox = (PasswordBox) Helper.ObjectFromIntPtr (closure);
			RoutedEventArgs args = new RoutedEventArgs (calldata);
			
			passwordbox.InvokePasswordChanged (args);
		}
		
		public event RoutedEventHandler PasswordChanged {
			add {
				RegisterEvent (PasswordChangedEvent, "TextChanged", password_changed, value);
			}
			remove {
				UnregisterEvent (PasswordChangedEvent, "TextChanged", password_changed, value);
			}
		}
	}
}

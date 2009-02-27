//
// Selector.cs
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

using System.Collections.Specialized;

namespace System.Windows.Controls.Primitives {
	public abstract class Selector : ItemsControl {
		
		public static readonly DependencyProperty SelectedIndexProperty =
			DependencyProperty.Register ("SelectedIndex", typeof(int), typeof(Selector),
						     new PropertyMetadata(-1, new PropertyChangedCallback(OnSelectedIndexChanged)));

		static void OnSelectedIndexChanged (DependencyObject o, DependencyPropertyChangedEventArgs e)
		{
			((Selector) o).SelectedIndexChanged (o, e);
		}

		public static readonly DependencyProperty SelectedItemProperty =
			DependencyProperty.Register ("SelectedItem", typeof(object), typeof(Selector),
						     new PropertyMetadata(new PropertyChangedCallback(OnSelectedItemChanged)));

		
		static void OnSelectedItemChanged (DependencyObject o, DependencyPropertyChangedEventArgs e)
		{
			((Selector) o).SelectedItemChanged (o, e);
		}

		internal Selector ()
		{
		}

		public int SelectedIndex {
			get { return (int)GetValue(SelectedIndexProperty); }
			set { SetValue (SelectedIndexProperty, value); }
		}

		
		public object SelectedItem {
			get { return GetValue (SelectedItemProperty); }
			set { SetValue (SelectedItemProperty, value); }
		}

		bool changing;
		public event SelectionChangedEventHandler SelectionChanged;

		void SelectedIndexChanged (DependencyObject o, DependencyPropertyChangedEventArgs e)
		{
			int newVal = (int) e.NewValue;
			if (newVal == (int) e.OldValue || changing) {
				SelectedIndex = newVal;
				return;
			}

			SelectedIndex = newVal;
			changing = true;
			try {
				if (newVal < 0)
					ClearValue (SelectedItemProperty);
				else if (newVal < Items.Count)
					SelectedItem = Items [newVal];

			} finally {
				changing = false;
			}
			RaiseSelectionChanged (o, new SelectionChangedEventArgs (new object[] { e.OldValue }, new object [] { e.NewValue }));
		}
		
		void SelectedItemChanged (DependencyObject o, DependencyPropertyChangedEventArgs e)
		{
			if (e.NewValue == e.OldValue || changing) {
				SelectedItem = e.NewValue;
				return;
			}
			
			changing = true;
			try {
				int index = e.NewValue == null ? -1 : Items.IndexOf (e.NewValue);
				if (index == -1) {
					SelectedIndex = e.OldValue == null ? -1 : Items.IndexOf (e.OldValue);
					if (e.OldValue == null)
						ClearValue (SelectedItemProperty);
					else
						SelectedItem = e.OldValue;
				}
				else {
					SelectedItem = e.NewValue;
					SelectedIndex = index;
					RaiseSelectionChanged (o, new SelectionChangedEventArgs (new object[] { e.OldValue }, new object [] { e.NewValue }));
				}
			} finally {
				changing = false;
			}
		}

		void RaiseSelectionChanged (object o, SelectionChangedEventArgs e)
		{
			SelectionChangedEventHandler h = SelectionChanged;
			if (h != null)
				h (o, e);
		}
		

		[MonoTODO]
		public static bool GetIsSelectionActive (DependencyObject element)
		{
			if (element == null)
				throw new ArgumentNullException ("element");

			Selector s = (element as Selector);
			if (s == null)
				return false;

			// FIXME: return true if focused (but there's no public IsFocused available)
			return false;
		}

		protected virtual void ClearContainerForItemOverride (DependencyObject element, object item)
		{
		}
		
		protected virtual void OnItemsChanged (NotifyCollectionChangedEventArgs e)
		{
		}
	}
}

//
// System.Windows.Documents.Inline.cs
//
// Contact:
//   Moonlight List (moonlight-list@lists.ximian.com)
//
// Copyright (C) 2007 Novell, Inc (http://www.novell.com)
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
using System.Windows;
using System.Windows.Media;
using System.Windows.Markup;

namespace System.Windows.Documents {
	public abstract partial class Inline : TextElement {

		public static DependencyProperty FontFamilyProperty = TextElement.FontFamilyProperty;
		public static DependencyProperty FontSizeProperty = TextElement.FontSizeProperty;
		public static DependencyProperty FontStretchProperty = TextElement.FontStretchProperty;
		public static DependencyProperty FontStyleProperty = TextElement.FontStyleProperty;
		public static DependencyProperty FontWeightProperty = TextElement.FontWeightProperty;
		public static DependencyProperty ForegroundProperty = TextElement.ForegroundProperty;
		public static DependencyProperty TextDecorationsProperty = TextElement.TextDecorationsProperty;
		public static DependencyProperty LanguageProperty = TextElement.LanguageProperty;

		public object FindName (string name)
		{
			return DepObjectFindName (name);
		}

		public string Name {
			get { return (string) GetValue (FrameworkElement.NameProperty); }
		}
	}
}

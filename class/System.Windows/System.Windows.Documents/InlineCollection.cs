//
// System.Windows.Documents.InlineCollection.cs
//
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

using System.Windows;
using Mono;

namespace System.Windows.Documents {
	public sealed class InlineCollection : PresentationFrameworkCollection<Inline> {
		public InlineCollection () : base (NativeMethods.inlines_new ())
		{
		}
		
		internal InlineCollection (IntPtr raw) : base (raw)
		{
		}
		
		internal override Kind GetKind ()
		{
			return Kind.INLINES;
		}

		public override bool Contains (Inline inline)
		{
			throw new NotImplementedException ();
		}

		public override bool Remove (Inline inline)
		{
			throw new NotImplementedException ();
		}

		public override int IndexOf (Inline inline)
		{
			throw new NotImplementedException ();
		}

		public override void Add (Inline inline)
		{
			throw new NotImplementedException ();
		}

		public override void Insert (int index, Inline inline)
		{
			throw new NotImplementedException ();
		}

		public override Inline this[int index] {
			get { throw new NotImplementedException (); }
			set { throw new NotImplementedException (); }
		}
	}
}

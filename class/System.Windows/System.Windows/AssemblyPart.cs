//
// AssemblyPart.cs
//
// Author:
//   Miguel de Icaza (miguel@novell.com)
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
using System.IO;
using System.Windows;
using System.Security;
using System.Reflection;
using Mono;

namespace System.Windows {

	public sealed class AssemblyPart : DependencyObject {

		static AssemblyPart ()
		{
			SourceProperty = DependencyProperty.Lookup (Kind.ASSEMBLYPART, "Source", typeof (string));
		}

		public AssemblyPart () : base (NativeMethods.assembly_part_new ())
		{
		}

		internal AssemblyPart (IntPtr raw) : base (raw)
		{
		}

		public string Source {
			get {
				return (string) GetValue (SourceProperty);
			}

			set {
				SetValue (SourceProperty, value);
			}
		}

#if NET_2_1
		[SecuritySafeCritical]
#endif
		public Assembly Load (Stream assemblyStream)
		{
			//
			// Eventually, we will be using temporary files for now to simplify debugging
			// so this method does nothing, and Application does all the work
			//
			return null;
		}
		
		public static readonly DependencyProperty SourceProperty;
	}
}

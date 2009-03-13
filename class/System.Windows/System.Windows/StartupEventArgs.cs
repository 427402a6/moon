//
// System.Windows.StartupEventArgs delegate
//
// <mono@novell.com>
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
using System.Collections.Generic;
using System.Windows.Interop;
using System.Runtime.InteropServices;
using Mono;

namespace System.Windows {
	public sealed class StartupEventArgs : EventArgs {
		private Dictionary<string,string> init_params;

		internal StartupEventArgs () {}

		public IDictionary<string,string> InitParams {
			get {
				if (init_params == null) {
					char [] param_separator = new char [] { ',' };
					char [] value_separator = new char [] { '=' };
					
					IntPtr raw = NativeMethods.plugin_instance_get_init_params (PluginHost.Handle);
					string param_string = Marshal.PtrToStringAnsi (raw);
					init_params = new Dictionary<string,string> ();
					
					// Console.WriteLine ("params = {0}", param_string);
					if (param_string != null) {
						foreach (string val in param_string.Split (param_separator)) {
							string trimmed = val.Trim ();
							int split = trimmed.IndexOf ('=');
							if (split >= 0)
								init_params.Add (trimmed.Substring (0, split), trimmed.Substring (split + 1));
						}
					}
				}

				return init_params;
			}
		}
	}
}

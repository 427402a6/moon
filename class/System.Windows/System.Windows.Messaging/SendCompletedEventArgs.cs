//
// Contact:
//   Moonlight List (moonlight-list@lists.ximian.com)
//
// Copyright 2009 Novell, Inc.
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
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace System.Windows.Messaging {


	public sealed class SendCompletedEventArgs : AsyncCompletedEventArgs, INativeEventObjectWrapper
	{
		internal SendCompletedEventArgs (IntPtr raw, Exception exc, bool dropref)
			: base (exc,
				false,
				NativeMethods.send_completed_event_args_get_managed_user_state (raw) == IntPtr.Zero
				? null
				: GCHandle.FromIntPtr (NativeMethods.send_completed_event_args_get_managed_user_state (raw)).Target)
		{
			NativeHandle = raw;
			if (dropref)
				NativeMethods.event_object_unref (raw);
		}

		~SendCompletedEventArgs ()
		{
			Free ();
		}

		void Free ()
		{
			if (free_mapping) {
				free_mapping = false;
				NativeDependencyObjectHelper.FreeNativeMapping (this);
			}
		}

		public string Message {
			get { return NativeMethods.send_completed_event_args_get_message (NativeHandle); }
		}

		public string ReceiverDomain {
			get { return NativeMethods.send_completed_event_args_get_receiver_domain (NativeHandle); }
		}

		public string ReceiverName {
			get { return NativeMethods.send_completed_event_args_get_receiver_name (NativeHandle); }
		}

		public string Response {
			get { return NativeMethods.send_completed_event_args_get_response (NativeHandle); }
		}

		bool free_mapping;

#region "INativeEventObjectWrapper interface"
		IntPtr _native;

		internal IntPtr NativeHandle {
			get { return _native; }
			set {
				if (_native != IntPtr.Zero) {
					throw new InvalidOperationException ("native handle is already set");
				}

				_native = value;

				free_mapping = NativeDependencyObjectHelper.AddNativeMapping (value, this);
			}
		}

		IntPtr INativeEventObjectWrapper.NativeHandle {
			get { return NativeHandle; }
			set { NativeHandle = value; }
		}

		Kind INativeEventObjectWrapper.GetKind ()
		{
			return Kind.SENDCOMPLETEDEVENTARGS;
		}
#endregion

	}

}


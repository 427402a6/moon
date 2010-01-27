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
using System.Collections.Generic;


namespace System.Windows.Messaging {

	public sealed class MessageReceivedEventArgs : EventArgs, INativeEventObjectWrapper
	{
		internal MessageReceivedEventArgs (IntPtr raw, bool dropref)
		{
			NativeHandle = raw;
			if (dropref)
				NativeMethods.event_object_unref (raw);
		}

		~MessageReceivedEventArgs ()
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
			get { return NativeMethods.message_received_event_args_get_message (NativeHandle); }
		}

		public ReceiverNameScope NameScope {
			get { return (ReceiverNameScope)NativeMethods.message_received_event_args_get_namescope (NativeHandle); }
		}

		public string ReceiverName {
			get { return NativeMethods.message_received_event_args_get_receiver_name (NativeHandle); }
		}

		public string Response {
			get { return NativeMethods.message_received_event_args_get_response (NativeHandle); }
			set { NativeMethods.message_received_event_args_set_response (NativeHandle, value); }
		}

		public string SenderDomain {
			get { return NativeMethods.message_received_event_args_get_sender_domain (NativeHandle); }
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
			return Kind.MESSAGERECEIVEDEVENTARGS;
		}
#endregion
	}

}


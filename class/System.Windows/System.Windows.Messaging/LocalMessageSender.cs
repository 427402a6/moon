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
using System.Threading;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace System.Windows.Messaging {

	public sealed class LocalMessageSender : INativeDependencyObjectWrapper
	{
		public const string Global = "*";

		internal LocalMessageSender (IntPtr raw, bool dropref)
		{
			NativeHandle = raw;
			if (dropref)
				NativeMethods.event_object_unref (raw);
		}

		public LocalMessageSender (string receiverName)
			: this (receiverName, Global)
		{
		}

		public LocalMessageSender (string receiverName,
					   string receiverDomain)
			: this (NativeMethods.local_message_sender_new (receiverName, receiverDomain), true)
		{
			this.receiverName = receiverName;
			this.receiverDomain = receiverDomain;
		}

		~LocalMessageSender ()
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

		public void SendAsync (string message)
		{
			NativeMethods.local_message_sender_send_async (NativeHandle, message, (IntPtr)GCHandle.Alloc (null));
		}

		public void SendAsync (string message,
				       object userState)
		{
			NativeMethods.local_message_sender_send_async (NativeHandle, message, (IntPtr)GCHandle.Alloc (userState));
		}

		public string ReceiverDomain {
			get { return receiverDomain; }
		}
		public string ReceiverName {
			get { return receiverName; }
		}

		public event EventHandler<SendCompletedEventArgs> SendCompleted {
			add { RegisterEvent (EventIds.LocalMessageSender_SendCompletedEvent, value, Events.CreateSendCompletedEventArgsEventHandlerDispatcher (value)); }
			remove { UnregisterEvent (EventIds.LocalMessageSender_SendCompletedEvent, value); }
		}

		string receiverDomain;
		string receiverName;

		bool free_mapping;

#region "INativeDependencyObjectWrapper interface"
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

		Kind INativeEventObjectWrapper.GetKind ()
		{
			return Kind.LOCALMESSAGESENDER;
		}

		bool INativeDependencyObjectWrapper.CheckAccess ()
		{
			return Thread.CurrentThread == DependencyObject.moonlight_thread;
		}
#endregion

		private EventHandlerList EventList = new EventHandlerList ();

		private void RegisterEvent (int eventId, Delegate managedHandler, UnmanagedEventHandler nativeHandler)
		{
			if (managedHandler == null)
				return;

			int token = Events.AddHandler (this, eventId, nativeHandler);
			EventList.AddHandler (eventId, token, managedHandler, nativeHandler);
		}

		private void UnregisterEvent (int eventId, Delegate managedHandler)
		{
			UnmanagedEventHandler nativeHandler = EventList.RemoveHandler (eventId, managedHandler);

			if (nativeHandler == null)
				return;

			Events.RemoveHandler (this, eventId, nativeHandler);
		}
	}

}


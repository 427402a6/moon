//
// System.Windows.Controls.MultiScaleImage.cs
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

using Mono;
using System.Collections.ObjectModel;
using System.Security;

namespace System.Windows.Controls {

	public partial class MultiScaleImage : FrameworkElement {

		[SecuritySafeCritical]
		public void ZoomAboutLogicalPoint (double zoomIncrementFactor, double zoomCenterLogicalX, double zoomCenterLogicalY)
		{
			NativeMethods.multi_scale_image_zoom_about_logical_point (this.native, zoomIncrementFactor, zoomCenterLogicalX, zoomCenterLogicalY);
		}

		public Point ElementToLogicalPoint (Point elementPoint)
		{
			return NativeMethods.multi_scale_image_element_to_logical_point (this.native, elementPoint);
		}

		public Point LogicalToElementPoint (Point logicalPoint)
		{
			throw new NotImplementedException ();
		}

		public ReadOnlyCollection<MultiScaleSubImage> SubImages {
			get { return new ReadOnlyCollection<MultiScaleSubImage> (new System.Collections.Generic.List<MultiScaleSubImage> ()); }
//			get { throw new NotImplementedException (); }
		}

		static object ImageFailedEvent = new object ();
		static object ImageOpenFailedEvent = new object ();
		static object ImageOpenSucceededEvent = new object ();
		static object MotionFinishedEvent = new object ();
		static object ViewportChangedEvent = new object ();

		static UnmanagedEventHandler image_failed = Events.CreateSafeHandler (image_failed_cb);
		static UnmanagedEventHandler image_open_failed = Events.CreateSafeHandler (image_open_failed_cb);
		static UnmanagedEventHandler image_open_succeeded = Events.CreateSafeHandler (image_open_succeeded_cb);
		static UnmanagedEventHandler motion_finished = Events.CreateSafeHandler (motion_finished_cb);
		static UnmanagedEventHandler viewport_changed = Events.CreateSafeHandler (viewport_changed_cb);

		public event RoutedEventHandler ImageFailed {
			add {
				RegisterEvent (ImageFailedEvent, "ImageFailed", image_failed, value);
			}
			remove {
				UnregisterEvent (ImageFailedEvent, "ImageFailed", image_failed, value);
			}
		}

		public event EventHandler<ExceptionRoutedEventArgs> ImageOpenFailed {
			add {
				RegisterEvent (ImageOpenFailedEvent, "ImageOpenFailed", image_open_failed, value);
			}
			remove {
				UnregisterEvent (ImageOpenFailedEvent, "ImageOpenFailed", image_open_failed, value);
			}
		}

		public event RoutedEventHandler ImageOpenSucceeded {
			add {
				RegisterEvent (ImageOpenSucceededEvent, "ImageOpenSucceeded", image_open_succeeded, value);
			}
			remove {
				UnregisterEvent (ImageOpenSucceededEvent, "ImageOpenSucceeded", image_open_succeeded, value);
			}
		}

		public event RoutedEventHandler MotionFinished {
			add {
				RegisterEvent (MotionFinishedEvent, "MotionFinished", motion_finished, value);
			}
			remove {
				UnregisterEvent (MotionFinishedEvent, "MotionFinished", motion_finished, value);
			}
		}

		public event RoutedEventHandler ViewportChanged {
			add {
				RegisterEvent (ViewportChangedEvent, "ViewportChanged", viewport_changed, value);
			}
			remove {
				UnregisterEvent (ViewportChangedEvent, "ViewportChanged", viewport_changed, value);
			}
		}

		static void image_failed_cb (IntPtr target, IntPtr calldata, IntPtr closure)
		{
			((MultiScaleImage) Helper.GCHandleFromIntPtr (closure).Target).InvokeImageFailed ();
		}


		static void image_open_failed_cb (IntPtr target, IntPtr calldata, IntPtr closure) {
			((MultiScaleImage) Helper.GCHandleFromIntPtr (closure).Target).InvokeImageOpenFailed ();
		}

		static void image_open_succeeded_cb (IntPtr target, IntPtr calldata, IntPtr closure)
		{
			((MultiScaleImage) Helper.GCHandleFromIntPtr (closure).Target).InvokeImageOpenSucceeded ();
		}

		static void motion_finished_cb (IntPtr target, IntPtr calldata, IntPtr closure)
		{
			((MultiScaleImage) Helper.GCHandleFromIntPtr (closure).Target).InvokeMotionFinished ();
		}

		static void viewport_changed_cb (IntPtr target, IntPtr calldata, IntPtr closure)
		{
			((MultiScaleImage) Helper.GCHandleFromIntPtr (closure).Target).InvokeViewportChanged ();
		}

		void InvokeImageFailed ()
		{
			EventHandler h = (EventHandler) EventList [ImageFailedEvent];
			if (h != null)
				h (this, EventArgs.Empty);
		}

		private void InvokeImageOpenFailed ()
		{
			EventHandler<ExceptionRoutedEventArgs> h = (EventHandler<ExceptionRoutedEventArgs>) EventList [ImageOpenFailedEvent];
			if (h != null)
				h (this, null);
		}

		void InvokeImageOpenSucceeded ()
		{
			EventHandler h = (EventHandler) EventList [ImageOpenSucceededEvent];
			if (h != null)
				h (this, EventArgs.Empty);
		}

		void InvokeMotionFinished ()
		{
			EventHandler h = (EventHandler) EventList [MotionFinishedEvent];
			if (h != null)
				h (this, EventArgs.Empty);
		}

		void InvokeViewportChanged ()
		{
			EventHandler h = (EventHandler) EventList [ViewportChangedEvent];
			if (h != null)
				h (this, EventArgs.Empty);
		}
	}
}

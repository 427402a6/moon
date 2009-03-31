/*
 * TypeReference.cs.
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2008 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

using System;
using System.Collections.Generic;
using System.Text;

class TypeReference {
	public string Value;
	public bool IsConst;
	public bool IsRef;
	public bool IsOut;
	public bool IsReturnType;

	private string managed_type;
	private Nullable <bool> is_known;
	
	public TypeReference () {}
	public TypeReference (string value)
	{
		this.Value = value;
	}
	
	public void WriteFormatted (StringBuilder text)
	{
		if (IsConst)
			text.Append ("const ");
		text.Append (GetPrettyType ());
	}
	
	public void Write (StringBuilder text, SignatureType type)
	{
		if (IsConst && type == SignatureType.Native)
			text.Append ("const ");
		
		if (type != SignatureType.Native) {
			if (IsRef && !IsReturnType)
				text.Append ("ref ");
			if (IsOut && !IsReturnType)
				text.Append ("out ");
		}
		
		if (type == SignatureType.Native) {
			text.Append (GetPrettyType ());
		} else {
			text.Append (GetManagedType ());
		}
	}
	
	public bool IsKnown {
		get {
			if (!is_known.HasValue) {
				if (string.IsNullOrEmpty (GetManagedType ()))
					is_known = new Nullable<bool> (false);
				else if (GetManagedType ().Contains ("Unknown"))
					is_known = new Nullable<bool> (false);
				else
					is_known = new Nullable<bool> (true);
			}
			return is_known.Value;
		}
	}
	
	public bool IsPointer {
		get { return Value[Value.Length - 1] == '*'; }
	}
	
	public string GetPrettyType ()
	{
		if (IsPointer)
			return Value.Substring (0, Value.Length - 1) + " *";
		
		return Value;
	}
	
	public string GetManagedType ()
	{
		if (managed_type == null) {
			switch (Value) {
			case "int":
			case "bool":
			case "void":
			case "long":
			case "double":
				managed_type = Value;
				break;
			case "guint32":
			case "uint32_t":
				managed_type = "uint";
				break;
			case "gint32":
			case "int32_t":
				managed_type = "int";
				break;
			case "int64_t":
			case "gint64":
				managed_type = "long";
				break;
			case "uint64_t":
			case "guint64":
				managed_type = "ulong";
				break;
			case "ManagedStreamCallbacks*":
				IsRef = true;
				managed_type = "ManagedStreamCallbacks";
				break;
			case "MoonError*":
				IsOut = true;
				managed_type = "MoonError";
				break;
			case "Value*":
				if (IsReturnType) {
					managed_type = "IntPtr";
				} else {
					IsRef = true;
					managed_type = "Value";
				}
				break;
			case "ApplyDefaultStyleCallback":
				managed_type = "Mono.ApplyDefaultStyleCallback";
				break;
			case "ApplyStyleCallback":
				managed_type = "Mono.ApplyStyleCallback";
				break;
			case "NativePropertyChangedHandler*":
				managed_type = "Mono.NativePropertyChangedHandler";
				break;
			case "MeasureOverrideCallback":
				managed_type = "Mono.MeasureOverrideCallback";
				break;
			case "ArrangeOverrideCallback":
				managed_type = "Mono.ArrangeOverrideCallback";
				break;
			case "GetResourceCallback":
				managed_type = "Mono.GetResourceCallback";
				break;
			case "GetValueCallback":
				managed_type = "Mono.GetValueCallback";
				break;
			case "SetValueCallback":
				managed_type = "Mono.SetValueCallback";
				break;
			case "CloseDemuxerCallback":
			case "GetDiagnosticAsyncCallback":
			case "GetFrameAsyncCallback":
			case "OpenDemuxerAsyncCallback":
			case "SeekAsyncCallback":
			case "SwitchMediaStreamAsyncCallback":
				managed_type = "System.Windows.Media.MediaStreamSource." + Value.Replace ("Callback", "Delegate");
				break;
			case "GDestroyNotify":
				managed_type = "IntPtr"; // hack, because we never pass this from managed code
				                         // (it's used for EventObject::AddHandler).
				break;
			case "char*":
				managed_type = "string";
				break;
			case "double*":
				managed_type = "double";
				IsOut = true;
				break;
			case "EventHandler":
				managed_type = "UnmanagedEventHandler";
				break;
			case "Size":
				managed_type = "Size";
				break;
			case "Rect":
				managed_type = "Rect";
				break;
			case "Point":
				managed_type = "Point";
				break;
			case "Thickness":
				managed_type = "Thickness";
				break;
			case "CornerRadius":
				managed_type = "CornerRadius";
				break;
			case "TimeSpan":
				managed_type = "long";
				break;
			case "bool*":
				if (IsReturnType) {
					managed_type = "IntPtr";
				} else {
					IsOut = true;
					managed_type = "bool";
				}
				break;
			case "int*":
				if (IsReturnType) {
					managed_type = "IntPtr";
				} else {
					IsOut = true;
					managed_type = "int";
				}
				break;
			case "Type::Kind*":
				if (IsReturnType) {
					managed_type = "IntPtr";
				} else {
					IsOut = true;
					managed_type = "Kind";
				}
				break;
			case "Type::Kind":
				managed_type = "Kind";
				break;
			case "gpointer":
				managed_type = "IntPtr";
				break;
			case "ModifierKeys":
				managed_type = "int";
				break;
			case "BindingMode":
				managed_type = "int";
				break;
			case "ToggleNotifyHandler":
				managed_type = "Mono.ToggleRef.ToggleNotifyHandler";
				break;

			case "downloader_create_state_func":
				managed_type = "Mono.DownloaderCreateStateFunc";
				break;
			case "downloader_destroy_state_func":
				managed_type = "Mono.DownloaderDestroyStateFunc";
				break;
			case "downloader_open_func":
				managed_type = "Mono.DownloaderOpenFunc";
				break;
			case "downloader_send_func":
				managed_type = "Mono.DownloaderSendFunc";
				break;
			case "downloader_abort_func":
				managed_type = "Mono.DownloaderAbortFunc";
				break;
			case "downloader_header_func":
				managed_type = "Mono.DownloaderHeaderFunc";
				break;
			case "downloader_body_func":
				managed_type = "Mono.DownloaderBodyFunc";
				break;
			case "downloader_create_webrequest_func":
				managed_type = "Mono.DownloaderCreateWebRequestFunc";
				break;
			default:
				if (Value.Contains ("*"))
					managed_type = "IntPtr";
				else
					managed_type = "/* Unknown: '" + Value + "' */";
				break;
			}
		}
		
		return managed_type;
	}
}

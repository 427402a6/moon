# [SecurityCritical] needed to execute code inside 'mscorlib, Version=2.0.5.0, Culture=neutral, PublicKeyToken=7cec85d7bea7798e'.
# 476 methods needs to be decorated.

# internal call
+SC-M: System.AppDomain System.AppDomain::getCurDomain()

# internal call
+SC-M: System.AppDomain System.AppDomain::getRootDomain()

# internal call
+SC-M: System.AppDomain System.AppDomain::InternalSetDomain(System.AppDomain)

# internal call
+SC-M: System.AppDomain System.AppDomain::InternalSetDomainByID(System.Int32)

# internal call
+SC-M: System.AppDomainSetup System.AppDomain::getSetup()

# internal call
+SC-M: System.Array System.Array::CreateInstanceImpl(System.Type,System.Int32[],System.Int32[])

# [VISIBLE] overrides 'System.Boolean System.Runtime.InteropServices.CriticalHandle::get_IsInvalid()'.
+SC-M: System.Boolean Microsoft.Win32.SafeHandles.CriticalHandleMinusOneIsInvalid::get_IsInvalid()

# [VISIBLE] overrides 'System.Boolean System.Runtime.InteropServices.SafeHandle::get_IsInvalid()'.
+SC-M: System.Boolean Microsoft.Win32.SafeHandles.SafeHandleMinusOneIsInvalid::get_IsInvalid()

# [VISIBLE] overrides 'System.Boolean System.Runtime.InteropServices.SafeHandle::get_IsInvalid()'.
+SC-M: System.Boolean Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid::get_IsInvalid()

# localloc
+SC-M: System.Boolean Mono.Globalization.Unicode.SimpleCollator::IsPrefix(System.String,System.String,System.Int32,System.Int32,System.Globalization.CompareOptions)

# using 'System.Byte*' as a parameter type
+SC-M: System.Boolean Mono.Globalization.Unicode.SimpleCollator::MatchesBackward(System.String,System.Int32&,System.Int32,System.Int32,System.Int32,System.Byte*,System.Boolean,Mono.Globalization.Unicode.SimpleCollator/Context&)

# using 'System.Byte*' as a parameter type
+SC-M: System.Boolean Mono.Globalization.Unicode.SimpleCollator::MatchesBackwardCore(System.String,System.Int32&,System.Int32,System.Int32,System.Int32,System.Byte*,System.Boolean,Mono.Globalization.Unicode.SimpleCollator/ExtenderType,Mono.Globalization.Unicode.Contraction&,Mono.Globalization.Unicode.SimpleCollator/Context&)

# using 'System.Byte*' as a parameter type
+SC-M: System.Boolean Mono.Globalization.Unicode.SimpleCollator::MatchesForward(System.String,System.Int32&,System.Int32,System.Int32,System.Byte*,System.Boolean,Mono.Globalization.Unicode.SimpleCollator/Context&)

# using 'System.Byte*' as a parameter type
+SC-M: System.Boolean Mono.Globalization.Unicode.SimpleCollator::MatchesForwardCore(System.String,System.Int32&,System.Int32,System.Int32,System.Byte*,System.Boolean,Mono.Globalization.Unicode.SimpleCollator/ExtenderType,Mono.Globalization.Unicode.Contraction&,Mono.Globalization.Unicode.SimpleCollator/Context&)

# using 'System.Byte*' as a parameter type
+SC-M: System.Boolean Mono.Globalization.Unicode.SimpleCollator::MatchesPrimitive(System.Globalization.CompareOptions,System.Byte*,System.Int32,Mono.Globalization.Unicode.SimpleCollator/ExtenderType,System.Byte*,System.Int32,System.Boolean)

# internal call
+SC-M: System.Boolean System.AppDomain::InternalIsFinalizingForUnload(System.Int32)

# internal call
+SC-M: System.Boolean System.Array::FastCopy(System.Array,System.Int32,System.Array,System.Int32,System.Int32)

# internal call
+SC-M: System.Boolean System.Buffer::BlockCopyInternal(System.Array,System.Int32,System.Array,System.Int32,System.Int32)

# internal call
+SC-M: System.Boolean System.CurrentSystemTimeZone::GetTimeZoneData(System.Int32,System.Int64[]&,System.String[]&)

# internal call
+SC-M: System.Boolean System.Diagnostics.Debugger::IsAttached_internal()

# internal call
+SC-M: System.Boolean System.Diagnostics.StackFrame::get_frame_info(System.Int32,System.Boolean,System.Reflection.MethodBase&,System.Int32&,System.Int32&,System.String&,System.Int32&,System.Int32&)

# internal call
+SC-M: System.Boolean System.Double::ParseImpl(System.Byte*,System.Double&)

# internal call
+SC-M: System.Boolean System.Globalization.CultureInfo::construct_internal_locale_from_current_locale(System.Globalization.CultureInfo)

# internal call
+SC-M: System.Boolean System.Globalization.CultureInfo::construct_internal_locale_from_lcid(System.Int32)

# internal call
+SC-M: System.Boolean System.Globalization.CultureInfo::construct_internal_locale_from_name(System.String)

# internal call
+SC-M: System.Boolean System.Globalization.CultureInfo::construct_internal_locale_from_specific_name(System.Globalization.CultureInfo,System.String)

# internal call
+SC-M: System.Boolean System.Globalization.CultureInfo::internal_is_lcid_neutral(System.Int32,System.Boolean&)

# internal call
+SC-M: System.Boolean System.Globalization.RegionInfo::construct_internal_region_from_name(System.String)

# p/invoke declaration
+SC-M: System.Boolean System.IO.IsolatedStorage.IsolatedStorageFile::isolated_storage_increase_quota_to(System.String,System.String)

# internal call
+SC-M: System.Boolean System.IO.MonoIO::Close(System.IntPtr,System.IO.MonoIOError&)

# internal call
+SC-M: System.Boolean System.IO.MonoIO::CopyFile(System.String,System.String,System.Boolean,System.IO.MonoIOError&)

# internal call
+SC-M: System.Boolean System.IO.MonoIO::CreateDirectory(System.String,System.IO.MonoIOError&)

# internal call
+SC-M: System.Boolean System.IO.MonoIO::DeleteFile(System.String,System.IO.MonoIOError&)

# internal call
+SC-M: System.Boolean System.IO.MonoIO::GetFileStat(System.String,System.IO.MonoIOStat&,System.IO.MonoIOError&)

# internal call
+SC-M: System.Boolean System.IO.MonoIO::MoveFile(System.String,System.String,System.IO.MonoIOError&)

# internal call
+SC-M: System.Boolean System.IO.MonoIO::RemoveDirectory(System.String,System.IO.MonoIOError&)

# internal call
+SC-M: System.Boolean System.IO.MonoIO::SetCurrentDirectory(System.String,System.IO.MonoIOError&)

# internal call
+SC-M: System.Boolean System.IO.MonoIO::SetFileAttributes(System.String,System.IO.FileAttributes,System.IO.MonoIOError&)

# internal call
+SC-M: System.Boolean System.IO.MonoIO::SetLength(System.IntPtr,System.Int64,System.IO.MonoIOError&)

# internal call
+SC-M: System.Boolean System.MonoCustomAttrs::IsDefinedInternal(System.Reflection.ICustomAttributeProvider,System.Type)

# internal call
+SC-M: System.Boolean System.Reflection.Assembly::GetManifestResourceInfoInternal(System.String,System.Reflection.ManifestResourceInfo)

# internal call
+SC-M: System.Boolean System.Reflection.AssemblyName::ParseName(System.Reflection.AssemblyName,System.String)

# internal call
+SC-M: System.Boolean System.Runtime.InteropServices.GCHandle::CheckCurrentDomain(System.Int32)

# internal call
+SC-M: System.Boolean System.Runtime.Remoting.RemotingServices::IsTransparentProxy(System.Object)

# internal call
+SC-M: System.Boolean System.Security.Cryptography.RNGCryptoServiceProvider::RngOpen()

# using 'System.Security.RuntimeDeclSecurityActions*' as a parameter type
+SC-M: System.Boolean System.Security.SecurityManager::InheritanceDemand(System.AppDomain,System.Reflection.Assembly,System.Security.RuntimeDeclSecurityActions*)

# using 'System.Security.RuntimeDeclSecurityActions*' as a parameter type
+SC-M: System.Boolean System.Security.SecurityManager::LinkDemand(System.Reflection.Assembly,System.Security.RuntimeDeclSecurityActions*,System.Security.RuntimeDeclSecurityActions*)

# internal call
+SC-M: System.Boolean System.Threading.Monitor::Monitor_test_synchronised(System.Object)

# internal call
+SC-M: System.Boolean System.Threading.Monitor::Monitor_try_enter(System.Object,System.Int32)

# internal call
+SC-M: System.Boolean System.Threading.Monitor::Monitor_wait(System.Object,System.Int32)

# internal call
+SC-M: System.Boolean System.Threading.Mutex::ReleaseMutex_internal(System.IntPtr)

# internal call
+SC-M: System.Boolean System.Threading.NativeEventCalls::ResetEvent_internal(System.IntPtr)

# internal call
+SC-M: System.Boolean System.Threading.NativeEventCalls::SetEvent_internal(System.IntPtr)

# internal call
+SC-M: System.Boolean System.Threading.Thread::Join_internal(System.Int32,System.IntPtr)

# internal call
+SC-M: System.Boolean System.Threading.WaitHandle::SignalAndWait_Internal(System.IntPtr,System.IntPtr,System.Int32,System.Boolean)

# internal call
+SC-M: System.Boolean System.Threading.WaitHandle::WaitAll_internal(System.Threading.WaitHandle[],System.Int32,System.Boolean)

# internal call
+SC-M: System.Boolean System.Threading.WaitHandle::WaitOne_internal(System.IntPtr,System.Int32,System.Boolean)

# internal call
+SC-M: System.Boolean System.Type::EqualsInternal(System.Type)

# internal call
+SC-M: System.Boolean System.Type::IsArrayImpl(System.Type)

# internal call
+SC-M: System.Boolean System.Type::type_is_assignable_from(System.Type,System.Type)

# internal call
+SC-M: System.Boolean System.Type::type_is_subtype_of(System.Type,System.Type,System.Boolean)

# internal call
+SC-M: System.Boolean System.ValueType::InternalEquals(System.Object,System.Object,System.Object[]&)

# internal call
+SC-M: System.Byte System.Buffer::GetByteInternal(System.Array,System.Int32)

# internal call
+SC-M: System.Byte System.Threading.Thread::VolatileRead(System.Byte&)

# using 'System.Byte*' as a parameter type
+SC-M: System.Byte[] Mono.Security.BitConverterLE::GetUIntBytes(System.Byte*)

# using 'System.Byte*' as a parameter type
+SC-M: System.Byte[] Mono.Security.BitConverterLE::GetULongBytes(System.Byte*)

# using 'System.Byte*' as a parameter type
+SC-M: System.Byte[] Mono.Security.BitConverterLE::GetUShortBytes(System.Byte*)

# using 'System.Byte*' as a parameter type
+SC-M: System.Byte[] System.BitConverter::GetBytes(System.Byte*,System.Int32)

# internal call
+SC-M: System.Byte[] System.Convert::InternalFromBase64CharArray(System.Char[],System.Int32,System.Int32)

# internal call
+SC-M: System.Byte[] System.Convert::InternalFromBase64String(System.String,System.Boolean)

# internal call
+SC-M: System.Byte[] System.Reflection.Emit.CustomAttributeBuilder::GetBlob(System.Reflection.Assembly,System.Reflection.ConstructorInfo,System.Object[],System.Reflection.PropertyInfo[],System.Object[],System.Reflection.FieldInfo[],System.Object[])

# internal call
+SC-M: System.Byte[] System.Reflection.Emit.SignatureHelper::get_signature_field()

# internal call
+SC-M: System.Byte[] System.Reflection.Emit.SignatureHelper::get_signature_local()

# internal call
+SC-M: System.Byte[] System.Reflection.Module::ResolveSignature(System.IntPtr,System.Int32,System.Reflection.ResolveTokenError&)

# internal call
+SC-M: System.Byte[] System.Threading.Thread::GetSerializedCurrentCulture()

# internal call
+SC-M: System.Byte[] System.Threading.Thread::GetSerializedCurrentUICulture()

# internal call
+SC-M: System.Char System.IO.MonoIO::get_AltDirectorySeparatorChar()

# internal call
+SC-M: System.Char System.IO.MonoIO::get_DirectorySeparatorChar()

# internal call
+SC-M: System.Char System.IO.MonoIO::get_PathSeparator()

# internal call
+SC-M: System.Char System.IO.MonoIO::get_VolumeSeparatorChar()

# internal call
+SC-M: System.Delegate System.Delegate::CreateDelegate_internal(System.Type,System.Object,System.Reflection.MethodInfo,System.Boolean)

# internal call
+SC-M: System.Delegate System.Runtime.InteropServices.Marshal::GetDelegateForFunctionPointerInternal(System.IntPtr,System.Type)

# internal call
+SC-M: System.Diagnostics.StackFrame[] System.Diagnostics.StackTrace::get_trace(System.Exception,System.Int32,System.Boolean)

# internal call
+SC-M: System.Double System.Decimal::decimal2double(System.Decimal&)

# internal call
+SC-M: System.Double System.Math::Round2(System.Double,System.Int32,System.Boolean)

# internal call
+SC-M: System.Double System.Threading.Interlocked::CompareExchange(System.Double&,System.Double,System.Double)

# internal call
+SC-M: System.Double System.Threading.Interlocked::Exchange(System.Double&,System.Double)

# internal call
+SC-M: System.Double System.Threading.Thread::VolatileRead(System.Double&)

# internal call
+SC-M: System.Globalization.CultureInfo System.Threading.Thread::GetCachedCurrentCulture()

# internal call
+SC-M: System.Globalization.CultureInfo System.Threading.Thread::GetCachedCurrentUICulture()

# internal call
+SC-M: System.Globalization.CultureInfo[] System.Globalization.CultureInfo::internal_get_cultures(System.Boolean,System.Boolean,System.Boolean)

# internal call
+SC-M: System.Int16 System.Threading.Thread::VolatileRead(System.Int16&)

# localloc
+SC-M: System.Int32 Mono.Globalization.Unicode.SimpleCollator::Compare(System.String,System.Int32,System.Int32,System.String,System.Int32,System.Int32,System.Globalization.CompareOptions)

# localloc
+SC-M: System.Int32 Mono.Globalization.Unicode.SimpleCollator::IndexOf(System.String,System.Char,System.Int32,System.Int32,System.Globalization.CompareOptions)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 Mono.Globalization.Unicode.SimpleCollator::IndexOf(System.String,System.String,System.Int32,System.Int32,System.Byte*,Mono.Globalization.Unicode.SimpleCollator/Context&)

# localloc
+SC-M: System.Int32 Mono.Globalization.Unicode.SimpleCollator::IndexOf(System.String,System.String,System.Int32,System.Int32,System.Globalization.CompareOptions)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 Mono.Globalization.Unicode.SimpleCollator::IndexOfSortKey(System.String,System.Int32,System.Int32,System.Byte*,System.Char,System.Int32,System.Boolean,Mono.Globalization.Unicode.SimpleCollator/Context&)

# localloc
+SC-M: System.Int32 Mono.Globalization.Unicode.SimpleCollator::LastIndexOf(System.String,System.Char,System.Int32,System.Int32,System.Globalization.CompareOptions)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 Mono.Globalization.Unicode.SimpleCollator::LastIndexOf(System.String,System.String,System.Int32,System.Int32,System.Byte*,Mono.Globalization.Unicode.SimpleCollator/Context&)

# localloc
+SC-M: System.Int32 Mono.Globalization.Unicode.SimpleCollator::LastIndexOf(System.String,System.String,System.Int32,System.Int32,System.Globalization.CompareOptions)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 Mono.Globalization.Unicode.SimpleCollator::LastIndexOfSortKey(System.String,System.Int32,System.Int32,System.Int32,System.Byte*,System.Int32,System.Boolean,Mono.Globalization.Unicode.SimpleCollator/Context&)

# internal call
+SC-M: System.Int32 System.AppDomain::ExecuteAssembly(System.Reflection.Assembly,System.String[])

# internal call
+SC-M: System.Int32 System.Array::GetRank()

# internal call
+SC-M: System.Int32 System.Buffer::ByteLengthInternal(System.Array)

# internal call
+SC-M: System.Int32 System.Decimal::decimal2Int64(System.Decimal&,System.Int64&)

# internal call
+SC-M: System.Int32 System.Decimal::decimal2string(System.Decimal&,System.Int32,System.Int32,System.Char[],System.Int32,System.Int32&,System.Int32&)

# internal call
+SC-M: System.Int32 System.Decimal::decimal2UInt64(System.Decimal&,System.UInt64&)

# internal call
+SC-M: System.Int32 System.Decimal::decimalCompare(System.Decimal&,System.Decimal&)

# internal call
+SC-M: System.Int32 System.Decimal::decimalDiv(System.Decimal&,System.Decimal&,System.Decimal&)

# internal call
+SC-M: System.Int32 System.Decimal::decimalIncr(System.Decimal&,System.Decimal&)

# internal call
+SC-M: System.Int32 System.Decimal::decimalIntDiv(System.Decimal&,System.Decimal&,System.Decimal&)

# internal call
+SC-M: System.Int32 System.Decimal::decimalMult(System.Decimal&,System.Decimal&)

# internal call
+SC-M: System.Int32 System.Decimal::decimalSetExponent(System.Decimal&,System.Int32)

# internal call
+SC-M: System.Int32 System.Decimal::string2decimal(System.Decimal&,System.String,System.UInt32,System.Int32)

# internal call
+SC-M: System.Int32 System.Enum::compare_value_to(System.Object)

# internal call
+SC-M: System.Int32 System.Enum::get_hashcode()

# internal call
+SC-M: System.Int32 System.GC::CollectionCount(System.Int32)

# internal call
+SC-M: System.Int32 System.GC::GetGeneration(System.Object)

# internal call
+SC-M: System.Int32 System.Globalization.CompareInfo::internal_compare(System.String,System.Int32,System.Int32,System.String,System.Int32,System.Int32,System.Globalization.CompareOptions)

# internal call
+SC-M: System.Int32 System.Globalization.CompareInfo::internal_index(System.String,System.Int32,System.Int32,System.Char,System.Globalization.CompareOptions,System.Boolean)

# internal call
+SC-M: System.Int32 System.Globalization.CompareInfo::internal_index(System.String,System.Int32,System.Int32,System.String,System.Globalization.CompareOptions,System.Boolean)

# internal call
+SC-M: System.Int32 System.IO.MonoIO::Read(System.IntPtr,System.Byte[],System.Int32,System.Int32,System.IO.MonoIOError&)

# internal call
+SC-M: System.Int32 System.IO.MonoIO::Write(System.IntPtr,System.Byte[],System.Int32,System.Int32,System.IO.MonoIOError&)

# internal call
+SC-M: System.Int32 System.Object::InternalGetHashCode(System.Object)

# internal call
+SC-M: System.Int32 System.Reflection.Assembly::MonoDebugger_GetMethodToken(System.Reflection.MethodBase)

# internal call
+SC-M: System.Int32 System.Reflection.Emit.ModuleBuilder::getMethodToken(System.Reflection.Emit.ModuleBuilder,System.Reflection.MethodInfo,System.Type[])

# internal call
+SC-M: System.Int32 System.Reflection.Emit.ModuleBuilder::getToken(System.Reflection.Emit.ModuleBuilder,System.Object)

# internal call
+SC-M: System.Int32 System.Reflection.Emit.ModuleBuilder::getUSIndex(System.Reflection.Emit.ModuleBuilder,System.String)

# internal call
+SC-M: System.Int32 System.Reflection.Module::GetMDStreamVersion(System.IntPtr)

# internal call
+SC-M: System.Int32 System.Runtime.InteropServices.GCHandle::GetTargetHandle(System.Object,System.Int32,System.Runtime.InteropServices.GCHandleType)

# internal call
+SC-M: System.Int32 System.Runtime.InteropServices.Marshal::AddRefInternal(System.IntPtr)

# internal call
+SC-M: System.Int32 System.Runtime.InteropServices.Marshal::QueryInterfaceInternal(System.IntPtr,System.Guid&,System.IntPtr&)

# internal call
+SC-M: System.Int32 System.Runtime.InteropServices.Marshal::ReleaseInternal(System.IntPtr)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.ASCIIEncoding::GetByteCount(System.Char*,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.ASCIIEncoding::GetBytes(System.Char*,System.Int32,System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.ASCIIEncoding::GetCharCount(System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.ASCIIEncoding::GetChars(System.Byte*,System.Int32,System.Char*,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.Encoding::GetByteCount(System.Char*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.Encoding::GetCharCount(System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.Encoding::GetChars(System.Byte*,System.Int32,System.Char*,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.UnicodeEncoding::GetByteCount(System.Char*,System.Int32)

# [VISIBLE] overrides 'System.Int32 System.Text.Encoding::GetBytes(System.Char*,System.Int32,System.Byte*,System.Int32)'.
+SC-M: System.Int32 System.Text.UnicodeEncoding::GetBytes(System.Char*,System.Int32,System.Byte*,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.UnicodeEncoding::GetBytesInternal(System.Char*,System.Int32,System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.UnicodeEncoding::GetCharCount(System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.UnicodeEncoding::GetChars(System.Byte*,System.Int32,System.Char*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.UnicodeEncoding::GetCharsInternal(System.Byte*,System.Int32,System.Char*,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.UTF7Encoding::GetByteCount(System.Char*,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.UTF7Encoding::GetBytes(System.Char*,System.Int32,System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.UTF7Encoding::GetCharCount(System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.UTF7Encoding::GetChars(System.Byte*,System.Int32,System.Char*,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.UTF8Encoding/UTF8Encoder::GetByteCount(System.Char*,System.Int32,System.Boolean)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.UTF8Encoding/UTF8Encoder::GetBytes(System.Char*,System.Int32,System.Byte*,System.Int32,System.Boolean)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.UTF8Encoding::Fallback(System.Object,System.Text.DecoderFallbackBuffer&,System.Byte[]&,System.Byte*,System.Int64,System.UInt32)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.UTF8Encoding::GetByteCount(System.Char*,System.Int32)

# [VISIBLE] overrides 'System.Int32 System.Text.Encoding::GetBytes(System.Char*,System.Int32,System.Byte*,System.Int32)'.
+SC-M: System.Int32 System.Text.UTF8Encoding::GetBytes(System.Char*,System.Int32,System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.UTF8Encoding::GetCharCount(System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.UTF8Encoding::GetChars(System.Byte*,System.Int32,System.Char*,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.UTF8Encoding::InternalGetByteCount(System.Char*,System.Int32,System.Char&,System.Boolean)

# using 'System.Char*' as a parameter type
+SC-M: System.Int32 System.Text.UTF8Encoding::InternalGetBytes(System.Char*,System.Int32,System.Byte*,System.Int32,System.Char&,System.Boolean)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.UTF8Encoding::InternalGetCharCount(System.Byte*,System.Int32,System.UInt32,System.UInt32,System.Object,System.Text.DecoderFallbackBuffer&,System.Byte[]&,System.Boolean)

# using 'System.Byte*' as a parameter type
+SC-M: System.Int32 System.Text.UTF8Encoding::InternalGetChars(System.Byte*,System.Int32,System.Char*,System.Int32,System.UInt32&,System.UInt32&,System.Object,System.Text.DecoderFallbackBuffer&,System.Byte[]&,System.Boolean)

# internal call
+SC-M: System.Int32 System.Threading.Thread::GetDomainID()

# internal call
+SC-M: System.Int32 System.Threading.Thread::VolatileRead(System.Int32&)

# internal call
+SC-M: System.Int32 System.Threading.WaitHandle::WaitAny_internal(System.Threading.WaitHandle[],System.Int32,System.Boolean)

# internal call
+SC-M: System.Int32 System.Type::GetGenericParameterPosition()

# internal call
+SC-M: System.Int32 System.ValueType::InternalGetHashCode(System.Object,System.Object[]&)

# internal call
+SC-M: System.Int64 System.DateTime::GetNow()

# internal call
+SC-M: System.Int64 System.DateTime::GetTimeMonotonic()

# p/invoke declaration
+SC-M: System.Int64 System.IO.IsolatedStorage.IsolatedStorage::isolated_storage_get_current_usage(System.String)

# internal call
+SC-M: System.Int64 System.IO.MonoIO::GetLength(System.IntPtr,System.IO.MonoIOError&)

# internal call
+SC-M: System.Int64 System.IO.MonoIO::Seek(System.IntPtr,System.Int64,System.IO.SeekOrigin,System.IO.MonoIOError&)

# internal call
+SC-M: System.Int64 System.Threading.Interlocked::Read(System.Int64&)

# internal call
+SC-M: System.Int64 System.Threading.Thread::VolatileRead(System.Int64&)

# internal call
+SC-M: System.IntPtr System.ArgIterator::IntGetNextArgType()

# internal call
+SC-M: System.IntPtr System.IO.MonoIO::get_ConsoleError()

# internal call
+SC-M: System.IntPtr System.IO.MonoIO::get_ConsoleInput()

# internal call
+SC-M: System.IntPtr System.IO.MonoIO::get_ConsoleOutput()

# internal call
+SC-M: System.IntPtr System.IO.MonoIO::Open(System.String,System.IO.FileMode,System.IO.FileAccess,System.IO.FileShare,System.IO.FileOptions,System.IO.MonoIOError&)

# internal call
+SC-M: System.IntPtr System.Object::obj_address()

# internal call
+SC-M: System.IntPtr System.Reflection.Assembly::GetManifestResourceInternal(System.String,System.Int32&,System.Reflection.Module&)

# internal call
+SC-M: System.IntPtr System.Reflection.Module::GetHINSTANCE()

# internal call
+SC-M: System.IntPtr System.Reflection.Module::ResolveFieldToken(System.IntPtr,System.Int32,System.IntPtr[],System.IntPtr[],System.Reflection.ResolveTokenError&)

# internal call
+SC-M: System.IntPtr System.Reflection.Module::ResolveMethodToken(System.IntPtr,System.Int32,System.IntPtr[],System.IntPtr[],System.Reflection.ResolveTokenError&)

# internal call
+SC-M: System.IntPtr System.Reflection.Module::ResolveTypeToken(System.IntPtr,System.Int32,System.IntPtr[],System.IntPtr[],System.Reflection.ResolveTokenError&)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.GCHandle::GetAddrOfPinnedObject(System.Int32)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::AllocCoTaskMem(System.Int32)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::AllocHGlobal(System.IntPtr)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::GetFunctionPointerForDelegateInternal(System.Delegate)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::OffsetOf(System.Type,System.String)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::ReadIntPtr(System.IntPtr,System.Int32)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::ReAllocCoTaskMem(System.IntPtr,System.Int32)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::ReAllocHGlobal(System.IntPtr,System.IntPtr)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::StringToBSTR(System.String)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::StringToHGlobalAnsi(System.String)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::StringToHGlobalUni(System.String)

# internal call
+SC-M: System.IntPtr System.Runtime.InteropServices.Marshal::UnsafeAddrOfPinnedArrayElement(System.Array,System.Int32)

# internal call
+SC-M: System.IntPtr System.RuntimeMethodHandle::GetFunctionPointer(System.IntPtr)

# internal call
+SC-M: System.IntPtr System.Security.Cryptography.RNGCryptoServiceProvider::RngGetBytes(System.IntPtr,System.Byte[])

# internal call
+SC-M: System.IntPtr System.Security.Cryptography.RNGCryptoServiceProvider::RngInitialize(System.Byte[])

# internal call
+SC-M: System.IntPtr System.Threading.Interlocked::CompareExchange(System.IntPtr&,System.IntPtr,System.IntPtr)

# internal call
+SC-M: System.IntPtr System.Threading.Interlocked::Exchange(System.IntPtr&,System.IntPtr)

# internal call
+SC-M: System.IntPtr System.Threading.Mutex::CreateMutex_internal(System.Boolean,System.String,System.Boolean&)

# internal call
+SC-M: System.IntPtr System.Threading.NativeEventCalls::CreateEvent_internal(System.Boolean,System.Boolean,System.String,System.Boolean&)

# internal call
+SC-M: System.IntPtr System.Threading.Thread::Thread_internal(System.MulticastDelegate)

# internal call
+SC-M: System.IntPtr System.Threading.Thread::VolatileRead(System.IntPtr&)

# internal call
+SC-M: System.IO.FileAttributes System.IO.MonoIO::GetFileAttributes(System.String,System.IO.MonoIOError&)

# [VISIBLE] implements 'System.IO.FileStream System.Runtime.InteropServices._Assembly::GetFile(System.String)'.
+SC-M: System.IO.FileStream System.Reflection.Assembly::GetFile(System.String)

# [VISIBLE] overrides 'System.IO.FileStream System.Reflection.Assembly::GetFile(System.String)'.
+SC-M: System.IO.FileStream System.Reflection.Emit.AssemblyBuilder::GetFile(System.String)

# Promoting interface member to [SecurityCritical] because of 'System.IO.FileStream System.Reflection.Assembly::GetFile(System.String)'.
+SC-M: System.IO.FileStream System.Runtime.InteropServices._Assembly::GetFile(System.String)

# implements 'System.IO.FileStream[] System.Runtime.InteropServices._Assembly::GetFiles()'.
+SC-M: System.IO.FileStream[] System.Reflection.Assembly::GetFiles()

# [VISIBLE] implements 'System.IO.FileStream[] System.Runtime.InteropServices._Assembly::GetFiles(System.Boolean)'.
+SC-M: System.IO.FileStream[] System.Reflection.Assembly::GetFiles(System.Boolean)

# [VISIBLE] overrides 'System.IO.FileStream[] System.Reflection.Assembly::GetFiles(System.Boolean)'.
+SC-M: System.IO.FileStream[] System.Reflection.Emit.AssemblyBuilder::GetFiles(System.Boolean)

# Promoting interface member to [SecurityCritical] because of 'System.IO.FileStream[] System.Reflection.Assembly::GetFiles()'.
+SC-M: System.IO.FileStream[] System.Runtime.InteropServices._Assembly::GetFiles()

# Promoting interface member to [SecurityCritical] because of 'System.IO.FileStream[] System.Reflection.Assembly::GetFiles(System.Boolean)'.
+SC-M: System.IO.FileStream[] System.Runtime.InteropServices._Assembly::GetFiles(System.Boolean)

# internal call
+SC-M: System.IO.MonoFileType System.IO.MonoIO::GetFileType(System.IntPtr,System.IO.MonoIOError&)

# internal call
+SC-M: System.Object System.Activator::CreateInstanceInternal(System.Type)

# internal call
+SC-M: System.Object System.Array::GetValueImpl(System.Int32)

# internal call
+SC-M: System.Object System.Enum::get_value()

# [VISIBLE] overrides 'System.Object System.Delegate::DynamicInvokeImpl(System.Object[])'.
+SC-M: System.Object System.MulticastDelegate::DynamicInvokeImpl(System.Object[])

# internal call
+SC-M: System.Object System.Reflection.Assembly::GetFilesInternal(System.String,System.Boolean)

# internal call
+SC-M: System.Object System.Reflection.MonoCMethod::InternalInvoke(System.Object,System.Object[],System.Exception&)

# internal call
+SC-M: System.Object System.Reflection.MonoField::GetValueInternal(System.Object)

# internal call
+SC-M: System.Object System.Reflection.MonoMethod::InternalInvoke(System.Object,System.Object[],System.Exception&)

# internal call
+SC-M: System.Object System.Runtime.InteropServices.GCHandle::GetTarget(System.Int32)

# internal call
+SC-M: System.Object System.Runtime.Remoting.Activation.ActivationServices::AllocateUninitializedClassInstance(System.Type)

# internal call
+SC-M: System.Object System.Runtime.Remoting.RemotingServices::InternalExecute(System.Reflection.MethodBase,System.Object,System.Object[],System.Object[]&)

# internal call
+SC-M: System.Object System.Threading.Interlocked::CompareExchange(System.Object&,System.Object,System.Object)

# internal call
+SC-M: System.Object System.Threading.Interlocked::Exchange(System.Object&,System.Object)

# internal call
+SC-M: System.Object System.Threading.Thread::VolatileRead(System.Object&)

# internal call
+SC-M: System.Object System.TypedReference::ToObject(System.TypedReference)

# internal call
+SC-M: System.Object[] System.MonoCustomAttrs::GetCustomAttributesInternal(System.Reflection.ICustomAttributeProvider,System.Type,System.Boolean)

# internal call
+SC-M: System.PlatformID System.Environment::get_Platform()

# internal call
+SC-M: System.Reflection.Assembly System.AppDomain::LoadAssembly(System.String,System.Security.Policy.Evidence,System.Boolean)

# internal call
+SC-M: System.Reflection.Assembly System.AppDomain::LoadAssemblyRaw(System.Byte[],System.Byte[],System.Security.Policy.Evidence,System.Boolean)

# internal call
+SC-M: System.Reflection.Assembly System.Reflection.Assembly::GetEntryAssembly()

# internal call
+SC-M: System.Reflection.Assembly System.Reflection.Assembly::load_with_partial_name(System.String,System.Security.Policy.Evidence)

# internal call
+SC-M: System.Reflection.Assembly System.Reflection.Assembly::LoadFrom(System.String,System.Boolean)

# internal call
+SC-M: System.Reflection.Assembly[] System.AppDomain::GetAssemblies(System.Boolean)

# [VISIBLE] implements 'System.Reflection.AssemblyName System.Runtime.InteropServices._Assembly::GetName()'.
+SC-M: System.Reflection.AssemblyName System.Reflection.Assembly::GetName()

# [VISIBLE] implements 'System.Reflection.AssemblyName System.Runtime.InteropServices._Assembly::GetName(System.Boolean)'.
+SC-M: System.Reflection.AssemblyName System.Reflection.Assembly::GetName(System.Boolean)

# Promoting interface member to [SecurityCritical] because of 'System.Reflection.AssemblyName System.Reflection.Assembly::GetName()'.
+SC-M: System.Reflection.AssemblyName System.Runtime.InteropServices._Assembly::GetName()

# Promoting interface member to [SecurityCritical] because of 'System.Reflection.AssemblyName System.Reflection.Assembly::GetName(System.Boolean)'.
+SC-M: System.Reflection.AssemblyName System.Runtime.InteropServices._Assembly::GetName(System.Boolean)

# internal call
+SC-M: System.Reflection.ConstructorInfo System.MonoType::GetCorrespondingInflatedConstructor(System.Reflection.ConstructorInfo)

# internal call
+SC-M: System.Reflection.ConstructorInfo[] System.MonoType::GetConstructors_internal(System.Reflection.BindingFlags,System.Type)

# internal call
+SC-M: System.Reflection.Emit.UnmanagedMarshal System.Reflection.FieldInfo::GetUnmanagedMarshal()

# internal call
+SC-M: System.Reflection.Emit.UnmanagedMarshal System.Reflection.MonoMethodInfo::get_retval_marshal(System.IntPtr)

# internal call
+SC-M: System.Reflection.EventInfo System.MonoType::InternalGetEvent(System.String,System.Reflection.BindingFlags)

# internal call
+SC-M: System.Reflection.EventInfo System.Reflection.Emit.TypeBuilder::get_event_info(System.Reflection.Emit.EventBuilder)

# internal call
+SC-M: System.Reflection.EventInfo[] System.MonoType::GetEvents_internal(System.Reflection.BindingFlags,System.Type)

# internal call
+SC-M: System.Reflection.FieldInfo System.Reflection.FieldInfo::internal_from_handle_type(System.IntPtr,System.IntPtr)

# internal call
+SC-M: System.Reflection.FieldInfo[] System.MonoType::GetFields_internal(System.Reflection.BindingFlags,System.Type)

# internal call
+SC-M: System.Reflection.GenericParameterAttributes System.Type::GetGenericParameterAttributes()

# internal call
+SC-M: System.Reflection.MemberInfo System.Reflection.Module::ResolveMemberToken(System.IntPtr,System.Int32,System.IntPtr[],System.IntPtr[],System.Reflection.ResolveTokenError&)

# internal call
+SC-M: System.Reflection.MethodBase System.Reflection.MethodBase::GetMethodFromHandleInternalType(System.IntPtr,System.IntPtr)

# internal call
+SC-M: System.Reflection.MethodBase System.Runtime.Remoting.RemotingServices::GetVirtualMethod(System.Type,System.Reflection.MethodBase)

# internal call
+SC-M: System.Reflection.MethodBody System.Reflection.MethodBase::GetMethodBodyInternal(System.IntPtr)

# internal call
+SC-M: System.Reflection.MethodInfo System.MonoType::GetCorrespondingInflatedMethod(System.Reflection.MethodInfo)

# internal call
+SC-M: System.Reflection.MethodInfo System.Reflection.MonoMethod::GetGenericMethodDefinition_impl()

# internal call
+SC-M: System.Reflection.MethodInfo System.Reflection.MonoMethod::MakeGenericMethod_impl(System.Type[])

# internal call
+SC-M: System.Reflection.MethodInfo[] System.MonoType::GetMethodsByName(System.String,System.Reflection.BindingFlags,System.Boolean,System.Type)

# internal call
+SC-M: System.Reflection.Module System.Reflection.Assembly::GetManifestModuleInternal()

# internal call
+SC-M: System.Reflection.Module System.Reflection.Emit.AssemblyBuilder::InternalAddModule(System.String)

# internal call
+SC-M: System.Reflection.MonoMethod System.Reflection.MonoMethod::get_base_definition(System.Reflection.MonoMethod)

# internal call
+SC-M: System.Reflection.ParameterInfo[] System.Reflection.MonoMethodInfo::get_parameter_info(System.IntPtr,System.Reflection.MemberInfo)

# internal call
+SC-M: System.Reflection.PropertyInfo[] System.MonoType::GetPropertiesByName(System.String,System.Reflection.BindingFlags,System.Boolean,System.Type)

# internal call
+SC-M: System.Reflection.TypeAttributes System.MonoType::get_attributes(System.Type)

# internal call
+SC-M: System.Runtime.InteropServices.DllImportAttribute System.Reflection.MonoMethod::GetDllImportAttribute(System.IntPtr)

# internal call
+SC-M: System.Runtime.Remoting.Contexts.Context System.AppDomain::InternalGetContext()

# internal call
+SC-M: System.Runtime.Remoting.Contexts.Context System.AppDomain::InternalGetDefaultContext()

# internal call
+SC-M: System.Runtime.Remoting.Contexts.Context System.AppDomain::InternalSetContext(System.Runtime.Remoting.Contexts.Context)

# internal call
+SC-M: System.SByte System.Threading.Thread::VolatileRead(System.SByte&)

# internal call
+SC-M: System.Single System.Threading.Interlocked::CompareExchange(System.Single&,System.Single,System.Single)

# internal call
+SC-M: System.Single System.Threading.Interlocked::Exchange(System.Single&,System.Single)

# internal call
+SC-M: System.Single System.Threading.Thread::VolatileRead(System.Single&)

# internal call
+SC-M: System.String System.AppDomain::getFriendlyName()

# internal call
+SC-M: System.String System.AppDomain::InternalGetProcessGuid(System.String)

# internal call
+SC-M: System.String System.Environment::get_MachineName()

# internal call
+SC-M: System.String System.Environment::get_UserName()

# internal call
+SC-M: System.String System.Environment::GetMachineConfigPath()

# internal call
+SC-M: System.String System.Environment::GetOSVersionString()

# internal call
+SC-M: System.String System.Environment::GetWindowsFolderPath(System.Int32)

# internal call
+SC-M: System.String System.Environment::internalGetEnvironmentVariable(System.String)

# internal call
+SC-M: System.String System.Environment::internalGetHome()

# internal call
+SC-M: System.String System.IO.MonoIO::GetCurrentDirectory(System.IO.MonoIOError&)

# internal call
+SC-M: System.String System.IO.Path::get_temp_path()

# internal call
+SC-M: System.String System.MonoType::getFullName(System.Boolean,System.Boolean)

# internal call
+SC-M: System.String System.Reflection.Assembly::get_code_base(System.Boolean)

# [VISIBLE] implements 'System.String System.Runtime.InteropServices._Assembly::get_CodeBase()'.
+SC-M: System.String System.Reflection.Assembly::get_CodeBase()

# internal call
+SC-M: System.String System.Reflection.Assembly::get_fullname()

# internal call
+SC-M: System.String System.Reflection.Assembly::get_location()

# [VISIBLE] implements 'System.String System.Runtime.InteropServices._Assembly::get_Location()'.
+SC-M: System.String System.Reflection.Assembly::get_Location()

# internal call
+SC-M: System.String System.Reflection.Assembly::InternalImageRuntimeVersion()

# [VISIBLE] overrides 'System.String System.Reflection.Assembly::get_CodeBase()'.
+SC-M: System.String System.Reflection.Emit.AssemblyBuilder::get_CodeBase()

# [VISIBLE] overrides 'System.String System.Reflection.Assembly::get_Location()'.
+SC-M: System.String System.Reflection.Emit.AssemblyBuilder::get_Location()

# [VISIBLE] overrides 'System.String System.Reflection.Module::get_FullyQualifiedName()'.
+SC-M: System.String System.Reflection.Emit.ModuleBuilder::get_FullyQualifiedName()

# internal call
+SC-M: System.String System.Reflection.Module::GetGuidInternal()

# internal call
+SC-M: System.String System.Reflection.Module::ResolveStringToken(System.IntPtr,System.Int32,System.Reflection.ResolveTokenError&)

# internal call
+SC-M: System.String System.Reflection.MonoMethod::get_name(System.Reflection.MethodBase)

# Promoting interface member to [SecurityCritical] because of 'System.String System.Reflection.Assembly::get_CodeBase()'.
+SC-M: System.String System.Runtime.InteropServices._Assembly::get_CodeBase()

# Promoting interface member to [SecurityCritical] because of 'System.String System.Reflection.Assembly::get_Location()'.
+SC-M: System.String System.Runtime.InteropServices._Assembly::get_Location()

# internal call
+SC-M: System.String System.Runtime.InteropServices.Marshal::PtrToStringBSTR(System.IntPtr)

# arglist
+SC-M: System.String System.String::Concat(System.Object,System.Object,System.Object,System.Object)

# using 'System.Char*' as a parameter type
+SC-M: System.String System.String::CreateString(System.Char*)

# using 'System.Char*' as a parameter type
+SC-M: System.String System.String::CreateString(System.Char*,System.Int32,System.Int32)

# using 'System.SByte*' as a parameter type
+SC-M: System.String System.String::CreateString(System.SByte*)

# using 'System.SByte*' as a parameter type
+SC-M: System.String System.String::CreateString(System.SByte*,System.Int32,System.Int32)

# using 'System.SByte*' as a parameter type
+SC-M: System.String System.String::CreateString(System.SByte*,System.Int32,System.Int32,System.Text.Encoding)

# internal call
+SC-M: System.String System.String::InternalAllocateStr(System.Int32)

# internal call
+SC-M: System.String System.String::InternalIntern(System.String)

# internal call
+SC-M: System.String System.String::InternalIsInterned(System.String)

# localloc
+SC-M: System.String System.String::ReplaceUnchecked(System.String,System.String)

# internal call
+SC-M: System.String System.Text.Encoding::InternalCodePage(System.Int32&)

# internal call
+SC-M: System.String System.Threading.Thread::GetName_internal()

# internal call
+SC-M: System.String[] System.Environment::GetCommandLineArgs()

# internal call
+SC-M: System.String[] System.Environment::GetEnvironmentVariableNames()

# internal call
+SC-M: System.String[] System.Environment::GetLogicalDrivesInternal()

# internal call
+SC-M: System.String[] System.IO.MonoIO::GetFileSystemEntries(System.String,System.String,System.Int32,System.Int32,System.IO.MonoIOError&)

# internal call
+SC-M: System.String[] System.Reflection.Assembly::GetNamespaces()

# internal call
+SC-M: System.String[] System.String::InternalSplit(System.Char[],System.Int32,System.Int32)

# internal call
+SC-M: System.Threading.Thread System.Threading.Thread::CurrentThread_internal()

# internal call
+SC-M: System.Threading.ThreadState System.Threading.Thread::GetState()

# internal call
+SC-M: System.Type System.Enum::get_underlying_type(System.Type)

# internal call
+SC-M: System.Type System.Reflection.Assembly::InternalGetType(System.Reflection.Module,System.String,System.Boolean,System.Boolean)

# internal call
+SC-M: System.Type System.Reflection.Emit.ModuleBuilder::create_modified_type(System.Reflection.Emit.TypeBuilder,System.String)

# internal call
+SC-M: System.Type System.Reflection.Emit.TypeBuilder::create_runtime_class(System.Reflection.Emit.TypeBuilder)

# internal call
+SC-M: System.Type System.Reflection.Module::GetGlobalType()

# internal call
+SC-M: System.Type System.Reflection.MonoField::GetParentType(System.Boolean)

# internal call
+SC-M: System.Type System.Runtime.Remoting.Proxies.RealProxy::InternalGetProxyType(System.Object)

# internal call
+SC-M: System.Type System.Type::GetGenericTypeDefinition_impl()

# internal call
+SC-M: System.Type System.Type::internal_from_handle(System.IntPtr)

# internal call
+SC-M: System.Type System.Type::internal_from_name(System.String,System.Boolean,System.Boolean)

# internal call
+SC-M: System.Type System.Type::make_array_type(System.Int32)

# internal call
+SC-M: System.Type System.Type::make_byref_type()

# internal call
+SC-M: System.Type System.Type::MakeGenericType(System.Type,System.Type[])

# internal call
+SC-M: System.Type[] System.Reflection.FieldInfo::GetTypeModifiers(System.Boolean)

# internal call
+SC-M: System.Type[] System.Reflection.Module::InternalGetTypes()

# internal call
+SC-M: System.Type[] System.Reflection.MonoPropertyInfo::GetTypeModifiers(System.Reflection.MonoProperty,System.Boolean)

# internal call
+SC-M: System.Type[] System.Reflection.ParameterInfo::GetTypeModifiers(System.Boolean)

# internal call
+SC-M: System.Type[] System.Type::GetGenericParameterConstraints_impl()

# internal call
+SC-M: System.TypeCode System.Type::GetTypeCodeInternal(System.Type)

# internal call
+SC-M: System.TypedReference System.ArgIterator::IntGetNextArg()

# internal call
+SC-M: System.TypedReference System.ArgIterator::IntGetNextArg(System.IntPtr)

# internal call
+SC-M: System.UInt16 System.Threading.Thread::VolatileRead(System.UInt16&)

# using 'System.Byte*' as a parameter type
+SC-M: System.UInt32 Mono.Globalization.Unicode.MSCompatUnicodeTable::UInt32FromBytePtr(System.Byte*,System.UInt32)

# internal call
+SC-M: System.UInt32 System.Threading.Thread::VolatileRead(System.UInt32&)

# internal call
+SC-M: System.UInt64 System.Threading.Thread::VolatileRead(System.UInt64&)

# internal call
+SC-M: System.UIntPtr System.Threading.Thread::VolatileRead(System.UIntPtr&)

# internal call
+SC-M: System.Void Mono.Globalization.Unicode.Normalization::load_normalization_resource(System.IntPtr&,System.IntPtr&,System.IntPtr&,System.IntPtr&,System.IntPtr&,System.IntPtr&)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void Mono.Globalization.Unicode.SimpleCollator/Context::.ctor(System.Globalization.CompareOptions,System.Byte*,System.Byte*,System.Byte*,System.Byte*,System.Byte*,System.Boolean)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void Mono.Globalization.Unicode.SimpleCollator::ClearBuffer(System.Byte*,System.Int32)

# localloc
+SC-M: System.Void Mono.Globalization.Unicode.SimpleCollator::GetSortKey(System.String,System.Int32,System.Int32,Mono.Globalization.Unicode.SortKeyBuffer,System.Globalization.CompareOptions)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void Mono.Security.BitConverterLE::UIntFromBytes(System.Byte*,System.Byte[],System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void Mono.Security.BitConverterLE::ULongFromBytes(System.Byte*,System.Byte[],System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void Mono.Security.BitConverterLE::UShortFromBytes(System.Byte*,System.Byte[],System.Int32)

# internal call
+SC-M: System.Void System.AppDomain::InternalPopDomainRef()

# internal call
+SC-M: System.Void System.AppDomain::InternalPushDomainRef(System.AppDomain)

# internal call
+SC-M: System.Void System.AppDomain::InternalPushDomainRefByID(System.Int32)

# internal call
+SC-M: System.Void System.AppDomain::InternalUnload(System.Int32)

# using 'System.Void*' as a parameter type
+SC-M: System.Void System.ArgIterator::.ctor(System.RuntimeArgumentHandle,System.Void*)

# internal call
+SC-M: System.Void System.ArgIterator::Setup(System.IntPtr,System.IntPtr)

# internal call
+SC-M: System.Void System.Array::ClearInternal(System.Array,System.Int32,System.Int32)

# internal call
+SC-M: System.Void System.Array::GetGenericValueImpl<T>(System.Int32,T&)

# internal call
+SC-M: System.Void System.Array::SetGenericValueImpl<T>(System.Int32,T&)

# internal call
+SC-M: System.Void System.Array::SetValueImpl(System.Object,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void System.BitConverter::PutBytes(System.Byte*,System.Byte[],System.Int32,System.Int32)

# internal call
+SC-M: System.Void System.Buffer::SetByteInternal(System.Array,System.Int32,System.Int32)

# internal call
+SC-M: System.Void System.Char::GetDataTablePointers(System.Byte*&,System.Byte*&,System.Double*&,System.UInt16*&,System.UInt16*&,System.UInt16*&,System.UInt16*&)

# arglist
+SC-M: System.Void System.Console::Write(System.String,System.Object,System.Object,System.Object,System.Object)

# arglist
+SC-M: System.Void System.Console::WriteLine(System.String,System.Object,System.Object,System.Object,System.Object)

# localloc
+SC-M: System.Void System.DateTimeUtils::ZeroPad(System.Text.StringBuilder,System.Int32,System.Int32)

# internal call
+SC-M: System.Void System.Decimal::decimalFloorAndTrunc(System.Decimal&,System.Int32)

# internal call
+SC-M: System.Void System.Delegate::SetMulticastInvoke()

# internal call
+SC-M: System.Void System.Environment::Exit(System.Int32)

# internal call
+SC-M: System.Void System.GC::InternalCollect(System.Int32)

# internal call
+SC-M: System.Void System.GC::RecordPressure(System.Int64)

# internal call
+SC-M: System.Void System.Globalization.CompareInfo::assign_sortkey(System.Object,System.String,System.Globalization.CompareOptions)

# internal call
+SC-M: System.Void System.Globalization.CompareInfo::construct_compareinfo(System.String)

# internal call
+SC-M: System.Void System.Globalization.CompareInfo::free_internal_collator()

# internal call
+SC-M: System.Void System.Globalization.CultureInfo::construct_datetime_format()

# internal call
+SC-M: System.Void System.Globalization.CultureInfo::construct_number_format()

# using 'System.Void*' as a parameter type
+SC-M: System.Void System.Globalization.TextInfo::.ctor(System.Globalization.CultureInfo,System.Int32,System.Void*,System.Boolean)

# [VISIBLE] overrides 'System.Void System.IO.FileSystemInfo::Delete()'.
+SC-M: System.Void System.IO.DirectoryInfo::Delete()

# [VISIBLE] overrides 'System.Void System.IO.FileSystemInfo::Delete()'.
+SC-M: System.Void System.IO.FileInfo::Delete()

# internal call
+SC-M: System.Void System.IO.MonoIO::Lock(System.IntPtr,System.Int64,System.Int64,System.IO.MonoIOError&)

# internal call
+SC-M: System.Void System.IO.MonoIO::Unlock(System.IntPtr,System.Int64,System.Int64,System.IO.MonoIOError&)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void System.IO.UnmanagedMemoryStream::.ctor(System.Byte*,System.Int64)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void System.IO.UnmanagedMemoryStream::.ctor(System.Byte*,System.Int64,System.Int64,System.IO.FileAccess)

# internal call
+SC-M: System.Void System.MonoEnumInfo::get_enum_info(System.Type,System.MonoEnumInfo&)

# internal call
+SC-M: System.Void System.NumberFormatter::GetFormatterTables(System.UInt64*&,System.Int32*&,System.Char*&,System.Char*&,System.Int64*&,System.Int32*&)

# internal call
+SC-M: System.Void System.Reflection.Assembly::FillName(System.Reflection.Assembly,System.Reflection.AssemblyName)

# internal call
+SC-M: System.Void System.Reflection.Assembly::InternalGetAssemblyName(System.String,System.Reflection.AssemblyName)

# internal call
+SC-M: System.Void System.Reflection.Emit.AssemblyBuilder::basic_init(System.Reflection.Emit.AssemblyBuilder)

# internal call
+SC-M: System.Void System.Reflection.Emit.DerivedType::create_unmanaged_type(System.Type)

# internal call
+SC-M: System.Void System.Reflection.Emit.DynamicMethod::create_dynamic_method(System.Reflection.Emit.DynamicMethod)

# internal call
+SC-M: System.Void System.Reflection.Emit.DynamicMethod::destroy_dynamic_method(System.Reflection.Emit.DynamicMethod)

# internal call
+SC-M: System.Void System.Reflection.Emit.EnumBuilder::setup_enum_type(System.Type)

# internal call
+SC-M: System.Void System.Reflection.Emit.GenericTypeParameterBuilder::initialize()

# internal call
+SC-M: System.Void System.Reflection.Emit.ModuleBuilder::basic_init(System.Reflection.Emit.ModuleBuilder)

# internal call
+SC-M: System.Void System.Reflection.Emit.ModuleBuilder::build_metadata(System.Reflection.Emit.ModuleBuilder)

# internal call
+SC-M: System.Void System.Reflection.Emit.ModuleBuilder::RegisterToken(System.Object,System.Int32)

# internal call
+SC-M: System.Void System.Reflection.Emit.ModuleBuilder::set_wrappers_type(System.Reflection.Emit.ModuleBuilder,System.Type)

# internal call
+SC-M: System.Void System.Reflection.Emit.ModuleBuilder::WriteToFile(System.IntPtr)

# internal call
+SC-M: System.Void System.Reflection.Emit.TypeBuilder::create_generic_class()

# internal call
+SC-M: System.Void System.Reflection.Emit.TypeBuilder::create_internal_class(System.Reflection.Emit.TypeBuilder)

# internal call
+SC-M: System.Void System.Reflection.Emit.TypeBuilder::setup_generic_class()

# internal call
+SC-M: System.Void System.Reflection.Emit.TypeBuilder::setup_internal_class(System.Reflection.Emit.TypeBuilder)

# internal call
+SC-M: System.Void System.Reflection.Module::GetPEKind(System.IntPtr,System.Reflection.PortableExecutableKinds&,System.Reflection.ImageFileMachine&)

# internal call
+SC-M: System.Void System.Reflection.MonoEventInfo::get_event_info(System.Reflection.MonoEvent,System.Reflection.MonoEventInfo&)

# internal call
+SC-M: System.Void System.Reflection.MonoField::SetValueInternal(System.Reflection.FieldInfo,System.Object,System.Object)

# internal call
+SC-M: System.Void System.Reflection.MonoGenericClass::initialize(System.Reflection.MethodInfo[],System.Reflection.ConstructorInfo[],System.Reflection.FieldInfo[],System.Reflection.PropertyInfo[],System.Reflection.EventInfo[])

# internal call
+SC-M: System.Void System.Reflection.MonoMethodInfo::get_method_info(System.IntPtr,System.Reflection.MonoMethodInfo&)

# internal call
+SC-M: System.Void System.Reflection.MonoPropertyInfo::get_property_info(System.Reflection.MonoProperty,System.Reflection.MonoPropertyInfo&,System.Reflection.PInfo)

# internal call
+SC-M: System.Void System.Runtime.CompilerServices.RuntimeHelpers::InitializeArray(System.Array,System.IntPtr)

# internal call
+SC-M: System.Void System.Runtime.CompilerServices.RuntimeHelpers::RunClassConstructor(System.IntPtr)

# internal call
+SC-M: System.Void System.Runtime.CompilerServices.RuntimeHelpers::RunModuleConstructor(System.IntPtr)

# internal call
+SC-M: System.Void System.Runtime.InteropServices.GCHandle::FreeHandle(System.Int32)

# internal call
+SC-M: System.Void System.Runtime.InteropServices.Marshal::copy_from_unmanaged(System.IntPtr,System.Int32,System.Array,System.Int32)

# internal call
+SC-M: System.Void System.Runtime.InteropServices.Marshal::copy_to_unmanaged(System.Array,System.Int32,System.IntPtr,System.Int32)

# internal call
+SC-M: System.Void System.Runtime.InteropServices.Marshal::DestroyStructure(System.IntPtr,System.Type)

# internal call
+SC-M: System.Void System.Runtime.InteropServices.Marshal::FreeBSTR(System.IntPtr)

# internal call
+SC-M: System.Void System.Runtime.InteropServices.Marshal::FreeCoTaskMem(System.IntPtr)

# internal call
+SC-M: System.Void System.Runtime.InteropServices.Marshal::FreeHGlobal(System.IntPtr)

# internal call
+SC-M: System.Void System.Runtime.InteropServices.Marshal::WriteInt16(System.IntPtr,System.Int32,System.Char)

# internal call
+SC-M: System.Void System.Runtime.InteropServices.Marshal::WriteIntPtr(System.IntPtr,System.Int32,System.IntPtr)

# internal call
+SC-M: System.Void System.Runtime.Remoting.Activation.ActivationServices::EnableProxyActivation(System.Type,System.Boolean)

# internal call
+SC-M: System.Void System.Runtime.Remoting.Messaging.MonoMethodMessage::InitMessage(System.Reflection.MonoMethod,System.Object[])

# internal call
+SC-M: System.Void System.Security.Cryptography.RNGCryptoServiceProvider::RngClose(System.IntPtr)

# internal call
+SC-M: System.Void System.String::.ctor(System.SByte*,System.Int32,System.Int32)

# internal call
+SC-M: System.Void System.String::.ctor(System.SByte*,System.Int32,System.Int32,System.Text.Encoding)

# using 'System.Char*' as a parameter type
+SC-M: System.Void System.String::CharCopy(System.Char*,System.Char*,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Void System.String::CharCopyReverse(System.Char*,System.Char*,System.Int32)

# internal call
+SC-M: System.Void System.String::InternalStrcpy(System.String,System.Int32,System.Char[])

# internal call
+SC-M: System.Void System.String::InternalStrcpy(System.String,System.Int32,System.Char[],System.Int32,System.Int32)

# internal call
+SC-M: System.Void System.String::InternalStrcpy(System.String,System.Int32,System.String)

# internal call
+SC-M: System.Void System.String::InternalStrcpy(System.String,System.Int32,System.String,System.Int32,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void System.String::memcpy(System.Byte*,System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void System.String::memcpy1(System.Byte*,System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void System.String::memcpy2(System.Byte*,System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void System.String::memcpy4(System.Byte*,System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void System.String::memset(System.Byte*,System.Int32,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Void System.Text.Decoder::CheckArguments(System.Char*,System.Int32,System.Byte*,System.Int32)

# using 'System.Char*' as a parameter type
+SC-M: System.Void System.Text.Encoder::CheckArguments(System.Char*,System.Int32,System.Byte*,System.Int32)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void System.Text.UnicodeEncoding::CopyChars(System.Byte*,System.Byte*,System.Int32,System.Boolean)

# using 'System.Byte*' as a parameter type
+SC-M: System.Void System.Text.UTF8Encoding::Fallback(System.Object,System.Text.DecoderFallbackBuffer&,System.Byte[]&,System.Byte*,System.Int64,System.UInt32,System.Char*,System.Int32&)

# internal call
+SC-M: System.Void System.Threading.Monitor::Monitor_pulse(System.Object)

# internal call
+SC-M: System.Void System.Threading.Monitor::Monitor_pulse_all(System.Object)

# internal call
+SC-M: System.Void System.Threading.NativeEventCalls::CloseEvent_internal(System.IntPtr)

# internal call
+SC-M: System.Void System.Threading.Thread::Abort_internal(System.Object)

# internal call
+SC-M: System.Void System.Threading.Thread::ClrState(System.Threading.ThreadState)

# internal call
+SC-M: System.Void System.Threading.Thread::FreeLocalSlotValues(System.Int32,System.Boolean)

# internal call
+SC-M: System.Void System.Threading.Thread::ResetAbort_internal()

# internal call
+SC-M: System.Void System.Threading.Thread::SetCachedCurrentCulture(System.Globalization.CultureInfo)

# internal call
+SC-M: System.Void System.Threading.Thread::SetCachedCurrentUICulture(System.Globalization.CultureInfo)

# internal call
+SC-M: System.Void System.Threading.Thread::SetName_internal(System.String)

# internal call
+SC-M: System.Void System.Threading.Thread::SetSerializedCurrentCulture(System.Byte[])

# internal call
+SC-M: System.Void System.Threading.Thread::SetSerializedCurrentUICulture(System.Byte[])

# internal call
+SC-M: System.Void System.Threading.Thread::SetState(System.Threading.ThreadState)

# internal call
+SC-M: System.Void System.Threading.Thread::Sleep_internal(System.Int32)

# internal call
+SC-M: System.Void System.Threading.Thread::SpinWait_nop()

# internal call
+SC-M: System.Void System.Threading.Thread::Thread_free_internal(System.IntPtr)

# internal call
+SC-M: System.Void System.Threading.Thread::Thread_init()

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.Byte&,System.Byte)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.Double&,System.Double)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.Int16&,System.Int16)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.Int32&,System.Int32)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.Int64&,System.Int64)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.IntPtr&,System.IntPtr)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.Object&,System.Object)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.SByte&,System.SByte)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.Single&,System.Single)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.UInt16&,System.UInt16)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.UInt32&,System.UInt32)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.UInt64&,System.UInt64)

# internal call
+SC-M: System.Void System.Threading.Thread::VolatileWrite(System.UIntPtr&,System.UIntPtr)

# internal call
+SC-M: System.Void System.Type::GetInterfaceMapData(System.Type,System.Type,System.Reflection.MethodInfo[]&,System.Reflection.MethodInfo[]&)

# internal call
+SC-M: System.Void System.Type::GetPacking(System.Int32&,System.Int32&)


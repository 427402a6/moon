//
// PresentationFrameworkCollection.cs: provides a wrapper to the unmanaged collection class
//
// Author:
//   Miguel de Icaza (miguel@novell.com)
//
// Copyright 2007, 2008 Novell, Inc.
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
using System.Windows;
using System.Collections;
using System.Collections.Generic;

namespace System.Windows {
	//
	// It seems that this only works for DependencyObjects, but its not in 
	// the public contract, I added the `where' so I can cast DependencyObjects
	// into T's but it might just be that I do not know how to do this.
	//
	public abstract class PresentationFrameworkCollection<T> : DependencyObject,
		IEnumerable, IList,
		IList<T>, ICollection<T> {

		public static readonly System.Windows.DependencyProperty CountProperty =
			DependencyProperty.Lookup (Kind.COLLECTION, "Count", typeof (int));
		
		internal PresentationFrameworkCollection () : base (NativeMethods.dependency_object_collection_new ())
		{
			//
			// We really need to revisit native collections, should
			// they all be Collection *, instead of Collection fields?
			//
			// If we keep them as Collection fields, when we "new" one,
			// the "new" versions should be immediately dispossed once
			// we do a 'set' operation (which should be a bitwise copy).
			//
		}
		
		internal PresentationFrameworkCollection (IntPtr raw) : base (raw)
		{
		}
		
		int IList.Add (object value)
		{
			DependencyObject dob = value as DependencyObject;
			if (dob == null)
				throw new Exception ("The collection only supports DependencyObjects");
			
			return NativeMethods.collection_add (native, dob.native);
		}
		
		public abstract void Add (T value);
		
		void IList.Remove (object value)
		{
			DependencyObject dob = value as DependencyObject;
			if (dob == null)
				throw new Exception ("The collection only supports DependencyObjects");
			
			NativeMethods.collection_remove (native, dob.native);
		}
		
		public abstract bool Remove (T value);
		
		public void Clear ()
		{
			NativeMethods.collection_clear (native);
		}
		
		void IList.Insert (int index, object value)
		{
			DependencyObject dob = value as DependencyObject;
			if (dob == null)
				throw new Exception ("The collection only supports DependencyObjects");
			
			NativeMethods.collection_insert (native, index, dob.native);
		}
		
		public abstract void Insert (int index, T value);

		public void RemoveAt (int index)
		{
			NativeMethods.collection_remove_at (native, index);
		}
		
		//
		// ICollection members
		//
		public int Count {
			get {
				return NativeMethods.collection_get_count (native);
			}
		}
		
		public void CopyTo (Array array, int index)
		{
			if (array == null)
				throw new ArgumentNullException ("array");
			
			if (index < 0)
				throw new ArgumentOutOfRangeException ("index");
			
			int n = Count;
			
			for (int i = 0; i < n; i++)
				array.SetValue (((IList) this)[i], index + i);
		}
		
		public void CopyTo (T [] array, int index)
		{
			if (array == null)
				throw new ArgumentNullException ("array");
			
			if (index < 0)
				throw new ArgumentOutOfRangeException ("index");
			
			int n = Count;
			
			for (int i = 0; i < n; i++)
				array[index + i] = this[i];
		}
		
		object IList.this [int index] {
			get {
				IntPtr o = NativeMethods.collection_get_value_at (native, index);
				
				if (o == IntPtr.Zero)
					throw new ArgumentOutOfRangeException ("index");
				
				Kind k = NativeMethods.dependency_object_get_object_type (o);
				return DependencyObject.Lookup (k, o) as object;
			}
			
			set {
				DependencyObject dob = value as DependencyObject;
				
				if (dob == null)
					throw new Exception ("The collection only supports DependencyObjects");
				
				NativeMethods.collection_set_value_at (native, index, dob.native);
			}
		}
		
		public abstract T this [int index] {
			get;
			set;
		}
		
		public object SyncRoot {
			get {
				return this;
			}
		}

		public bool IsSynchronized {
			get {
				return false;
			}
		}
		
		internal override Kind GetKind ()
		{
			return Kind.COLLECTION;
		}

		static internal Exception GetInvalid ()
		{
			return new InvalidOperationException ("The underlying collection has mutated");
		}
		
		internal class CollectionIterator : System.Collections.IEnumerator {
			IntPtr native_iter;
			
			public CollectionIterator(IntPtr native_iter)
			{
				this.native_iter = native_iter;
			}
			
			public bool MoveNext ()
			{
				int r = NativeMethods.collection_iterator_move_next (native_iter);

				if (r == -1)
					throw GetInvalid ();
				
				return r == 1;
			}
			
			public void Reset ()
			{
				if (NativeMethods.collection_iterator_reset (native_iter))
					return;

				throw GetInvalid ();
			}

			public object Current {
				get {
					throw new NotImplementedException ();
#if false
					int error;
					IntPtr o = NativeMethods.collection_iterator_get_current (native_iter, out error);
					Kind k;

					if (error == 1)
						throw GetInvalid ();
					
					if (o == IntPtr.Zero)
						return null;
					
					k = NativeMethods.dependency_object_get_object_type (o);
					
					return DependencyObject.Lookup (k, o);
#endif
				}
			}

			~CollectionIterator ()
			{
				// This is safe, as it only does a "delete" in the C++ side
				NativeMethods.collection_iterator_destroy (native_iter);
			}
		}
		
		internal class GenericCollectionIterator : IEnumerator<T> {
			IntPtr native_iter;
			
			public GenericCollectionIterator(IntPtr native_iter)
			{
				this.native_iter = native_iter;
			}
			
			public bool MoveNext ()
			{
				int r = NativeMethods.collection_iterator_move_next (native_iter);

				if (r == -1)
					throw GetInvalid ();
				
				return r == 1;
			}
			
			public void Reset ()
			{
				if (NativeMethods.collection_iterator_reset (native_iter))
					return;

				throw GetInvalid ();
			}

			T GetCurrent ()
			{
				throw new NotImplementedException ();
#if false
				int error;
				IntPtr o = NativeMethods.collection_iterator_get_current (native_iter, out error);
				Kind k;

				if (error == 1)
					throw GetInvalid ();
				
				if (o == IntPtr.Zero)
					return null;
				
				k = NativeMethods.dependency_object_get_object_type (o);
				
				return (T) DependencyObject.Lookup (k, o);
#endif
			}
			
			public T Current {
				get {
					return GetCurrent ();
				}
			}

			object System.Collections.IEnumerator.Current {
				get {
					return GetCurrent ();
				}
			}

			public void Dispose ()
			{
				if (native_iter != IntPtr.Zero){
					// This is safe, as it only does a "delete" in the C++ side
					NativeMethods.collection_iterator_destroy (native_iter);
					native_iter = IntPtr.Zero;
				}
			}
			
			~GenericCollectionIterator ()
			{
				if (native_iter != IntPtr.Zero){
					// This is safe, as it only does a "delete" in the C++ side
					NativeMethods.collection_iterator_destroy (native_iter);
					native_iter = IntPtr.Zero;
				}
			}
		}
		
		public IEnumerator<T> GetEnumerator ()
		{
			return new GenericCollectionIterator (NativeMethods.collection_get_iterator (native));
		}
		
		IEnumerator IEnumerable.GetEnumerator ()
		{
			return new CollectionIterator (NativeMethods.collection_get_iterator (native));
		}
		
		protected internal bool ContainsDependencyObject (DependencyObject value)
		{
			if (value == null)
				throw new ArgumentNullException ("value");
			
			return NativeMethods.collection_index_of (native, value.native) != -1;
		}
		
		protected internal bool ContainsDouble (double value)
		{
			// FIXME: implement me
			return false;
		}
		
		protected internal bool ContainsPoint (Point value)
		{
			// FIXME: implement me
			return false;
		}
		
		protected internal virtual bool ContainsImpl (object value)
		{
			if (value == null)
				throw new ArgumentNullException ("value");
			
			// FIXME: implement me
			return false;
		}
		
		bool IList.Contains (object value)
		{
			return ((IList) this).IndexOf (value) != -1;
		}
		
		public abstract bool Contains (T value);
		
		int IList.IndexOf (object value)
		{
			if (value == null)
				throw new ArgumentNullException ("value");
			
			DependencyObject dob = value as DependencyObject;
			if (dob == null)
				throw new Exception ("The collection only supports DependencyObjects");
			
			return NativeMethods.collection_index_of (native, dob.native);
		}
		
		public abstract int IndexOf (T value);

		public bool IsFixedSize {
			get {
				return false;
			}
		}

		public bool IsReadOnly {
			get {
				return false;
			}
		}

	}
}

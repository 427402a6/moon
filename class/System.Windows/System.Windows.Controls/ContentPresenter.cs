//
// Contact:
//   Moonlight List (moonlight-list@lists.ximian.com)
//
// Copyright (C) 2009 Novell, Inc (http://www.novell.com)
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


// Copyright © Microsoft Corporation. 
// This source is subject to the Microsoft Source License for Silverlight Controls (March 2008 Release).
// Please see http://go.microsoft.com/fwlink/?LinkID=111693 for details.
// All other rights reserved. 

using Mono;
using System;
using System.ComponentModel; 
using System.Diagnostics; 
using System.Windows.Documents;
using System.Windows.Markup; 
using System.Windows.Media;
using System.Windows.Controls;

namespace System.Windows.Controls
{
	/// <summary>
	/// Displays the content of a ContentControl. 
	/// </summary> 
	/// <remarks>
	/// Typically, you the ContentPresenter directly within the ControlTemplate 
	/// of a ContentControl to mark where the content should be displayed.
	/// Every type derived from ContentControl should have a ContentPrenter in
	/// its ControlTemplate (although it may not necessarily be a TemplatePart). 
	/// The ContentPresenter in the ControlTemplate should use a TemplateBinding
	/// to associate ContentControl.Content with ContentPresenter.Content (and
	/// an other relevant properties like FontSize, VeriticalAlignment, etc.). 
	/// </remarks> 
	[ContentProperty ("Content")]
	public class ContentPresenter : FrameworkElement
	{ 
		internal UIElement _contentRoot;

#region Content
		/// <summary> 
		/// Gets or sets the data used to generate the contentPresenter elements of a 
		/// ContentPresenter.
		/// </summary> 
		public object Content
		{
			get { return GetValue(ContentProperty); } 
			set { SetValue(ContentProperty, value); }
		}
 
		/// <summary> 
		/// Identifies the Content dependency property.
		/// </summary> 
		public static readonly DependencyProperty ContentProperty =
			DependencyProperty.Register(
						    "Content", 
						    typeof(object),
						    typeof(ContentPresenter),
						    new PropertyMetadata(OnContentPropertyChanged)); 
 
		/// <summary>
		/// ContentProperty property changed handler. 
		/// </summary>
		/// <param name="d">ContentPresenter that changed its Content.</param>
		/// <param name="e">DependencyPropertyChangedEventArgs.</param> 
		private static void OnContentPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
		{
			ContentPresenter source = d as ContentPresenter; 
			Debug.Assert(source != null, 
				     "The source is not an instance of ContentPresenter!");
 
			// Use the Content as the DataContext to enable bindings in
			// ContentTemplate
			if (source.ContentTemplate != null) {
				source.DataContext = e.NewValue;
			} 
 
			// Display the Content
			source.PrepareContentPresenter(); 
		}
#endregion Content
 
#region ContentTemplate
		/// <summary>
		/// Gets or sets the data template used to display the content of the 
		/// ContentPresenter. 
		/// </summary>
		public DataTemplate ContentTemplate 
		{
			get { return GetValue(ContentTemplateProperty) as DataTemplate; }
			set { SetValue(ContentTemplateProperty, value); } 
		}

		/// <summary> 
		/// Identifies the ContentTemplate dependency property. 
		/// </summary>
		public static readonly DependencyProperty ContentTemplateProperty = 
			DependencyProperty.Register(
						    "ContentTemplate",
						    typeof(DataTemplate), 
						    typeof(ContentPresenter),
						    new PropertyMetadata(OnContentTemplatePropertyChanged));
 
		/// <summary> 
		/// ContentTemplateProperty property changed handler.
		/// </summary> 
		/// <param name="d">ContentPresenter that changed its ContentTemplate.</param>
		/// <param name="e">DependencyPropertyChangedEventArgs.</param>
		private static void OnContentTemplatePropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e) 
		{
			ContentPresenter source = d as ContentPresenter;
			Debug.Assert(source != null, 
				     "The source is not an instance of ContentPresenter!"); 

			// Use the Content as the DataContext to enable bindings in 
			// ContentTemplate or clear it if we removed our template (NOTE:
			// this should use ClearValue instead when it's available).
			source.DataContext = e.NewValue != null ? source.Content : null; 

			// Display the Content
			source.PrepareContentPresenter(); 
		} 
#endregion ContentTemplate

		/// <summary>
		/// Initializes a new instance of the ContentPresenter class.
		/// </summary> 
		public ContentPresenter() 
		{
		}

		internal override void InvokeLoaded ()
		{
			PrepareContentPresenter ();

			base.InvokeLoaded ();
		}

		/// <summary> 
		/// Update the ContentPresenter's logical tree with the appropriate
		/// visual elements when its Content is changed.
		/// </summary> 
		private void PrepareContentPresenter() 
		{
			// Expand the ContentTemplate if it exists
			DataTemplate template = ContentTemplate; 
			object content = (template != null) ? 
				template.LoadContent() :
				Content; 

			UIElement newContentRoot = null;

			// Add the new content
			UIElement element = content as UIElement; 
			if (element != null) {
				newContentRoot = element;
			}
			else if (content != null) {
				TextBlock elementText = new TextBlock();
				elementText.Text = content.ToString();

				Grid grid = new Grid();
				grid.Children.Add (elementText);

				newContentRoot = grid;
			}

			if (newContentRoot == _contentRoot)
				return;

			if (_contentRoot != null) {
				// clear the old content
				NativeMethods.uielement_element_removed (native, _contentRoot.native);
				NativeMethods.uielement_set_subtree_object (native, IntPtr.Zero);
			}

			_contentRoot = newContentRoot;

			if (_contentRoot != null) {
				// set the new content
				NativeMethods.uielement_element_added (native, _contentRoot.native);
				NativeMethods.uielement_set_subtree_object (native, _contentRoot.native);
			}
		}
	}
} 

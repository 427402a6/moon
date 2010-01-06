//
// Contact:
//   Moonlight List (moonlight-list@lists.ximian.com)
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
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Media;

using Mono.Moonlight.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting;

using System.Windows.Shapes;
using Microsoft.Silverlight.Testing;
using MoonTest.System.Windows.Controls.Primitives;

namespace MoonTest.System.Windows.Controls {
	public class ListBoxPoker : ListBox, IPoker
	{
		public DependencyObject ContainerItem { get; set; }

		public DependencyObject LastClearedContainer {
			get; set;
		}

		public DependencyObject LastCreatedContainer {
			get;set;
		}

		public DependencyObject LastPreparedContainer {
			get; set;
		}

		public object LastPreparedItem {
			get; set;
		}

		public bool TemplateApplied
		{
			get;
			private set;
		}

		public void ClearContainerForItemOverride_ (DependencyObject element, object item)
		{
			ClearContainerForItemOverride (element, item);
		}

		protected override void ClearContainerForItemOverride (DependencyObject element, object item)
		{
			LastClearedContainer = element;
			base.ClearContainerForItemOverride (element, item);
		}

		public DependencyObject GetContainerForItemOverride_ ()
		{
			return GetContainerForItemOverride ();
		}

		protected override DependencyObject GetContainerForItemOverride ()
		{
			LastCreatedContainer = ContainerItem ?? base.GetContainerForItemOverride ();
			return LastCreatedContainer;
		}

		public DependencyObject GetTemplateChild (string name)
		{
			return base.GetTemplateChild (name);
		}

		public bool IsItemItsOwnContainerOverride_ (object item)
		{
			return IsItemItsOwnContainerOverride (item);
		}

		public override void OnApplyTemplate ()
		{
			TemplateApplied = true;
			base.OnApplyTemplate ();
		}

		public void PrepareContainerForItemOverride_ (DependencyObject element, object item)
		{
			PrepareContainerForItemOverride (element, item);
		}

		protected override void PrepareContainerForItemOverride (DependencyObject element, object item)
		{
			LastPreparedContainer = element;
			LastPreparedItem = item;
			base.PrepareContainerForItemOverride (element, item);
		}
	}

	[TestClass]
	public partial class ListBoxTest : SelectorTest
	{
		protected override IPoker CreateControl ()
		{
			return new ListBoxPoker ();
		}
		protected override object CreateContainer ()
		{
			return new ListBoxItem ();
		}

		[Asynchronous]
		public override void ChangeContainerStyle ()
		{
			base.ChangeContainerStyle ();
			Enqueue (() => Assert.AreSame (CurrentControl.ItemContainerStyle, ((ListBoxItem)CurrentControl.Items[0]).Style, "#non-null style"));
			EnqueueTestComplete ();
		}

		[Asynchronous]
		public override void ContainerItemTest2 ()
		{
			base.ContainerItemTest2 ();
			IPoker c = CurrentControl;
			Enqueue (() => {
				Assert.IsInstanceOfType<ListBoxItem> (c.LastCreatedContainer, "#1");
				ListBoxItem lbi = (ListBoxItem) c.LastCreatedContainer;
				Assert.AreEqual (lbi.Content, c.LastPreparedItem, "#2");
				Assert.AreEqual (lbi.DataContext, c.LastPreparedItem, "#3");
			});
			EnqueueTestComplete ();
		}

		[Asynchronous]
		public override void DisableControlTest ()
		{
			ItemsControl c = (ItemsControl) CurrentControl;
			base.DisableControlTest ();
			Enqueue (() => {
				foreach (Control item in c.Items) {
					Assert.IsFalse (item.IsEnabled, "#1");
					Assert.IsFalse ((bool) item.GetValue (Control.IsEnabledProperty), "#2");
				}
			});
			EnqueueTestComplete ();
		}

		[Asynchronous]
		public override void DisplayMemberPathTest ()
		{
			base.DisplayMemberPathTest ();
			ItemsControl c = (ItemsControl) CurrentControl;
			Enqueue (() => {
				ListBoxItem item = (ListBoxItem) CurrentControl.LastCreatedContainer;
				Assert.IsNull (item.ContentTemplate, "#template");
			});
			EnqueueTestComplete ();
		}

		public override void GetContainerForItemOverride2 ()
		{
			base.GetContainerForItemOverride2 ();
			Assert.IsInstanceOfType<ListBoxItem> (CurrentControl.LastCreatedContainer, "#1");
			ListBoxItem c = (ListBoxItem) CurrentControl.LastCreatedContainer;
			Assert.IsNull (c.Style, "null style");
			Assert.IsFalse (c.IsSelected, "Selected");
			Assert.IsNull (c.Content, "content is null");
		}

		[TestMethod]
		public override void GetContainerForItemOverride3 ()
		{
			base.GetContainerForItemOverride3 ();
			Assert.IsNotNull (((ListBoxItem) CurrentControl.LastCreatedContainer).Style, "#1");
		}

		[Asynchronous]
		public override void GetContainerForItemOverride10 ()
		{
			base.GetContainerForItemOverride10 ();
			Enqueue (() => CurrentControl.Items.Add ("Test"));
			EnqueueTestComplete ();
		}

		[Asynchronous]
		[Ignore ("This should throw an InvalidOperationException but SL throws a WrappedException containing the InvalidOperationException")]
		public override void GetInvalidContainerItemTest ()
		{
			base.GetInvalidContainerItemTest ();

			Enqueue (() => {
				try {
					CurrentControl.Items.Add ("New Item");
					Assert.Fail ("An exception should be thrown");
				} catch (Exception ex) {
					// Have to return a UIElement subclass, should throw InvalidOperationException i think
					Assert.AreEqual ("WrappedException", ex.GetType ().Name, "Exception type");
				}
			});
			EnqueueTestComplete ();
		}

		public override void IsItemItsOwnContainerTest ()
		{
			base.IsItemItsOwnContainerTest ();
			Assert.IsTrue (CurrentControl.IsItemItsOwnContainerOverride_ (new ComboBoxItem ()));
			Assert.IsTrue (CurrentControl.IsItemItsOwnContainerOverride_ (new ListBoxItem ()));
		}

		class ListBoxItemSubclass : ListBoxItem { }

		[TestMethod]
		[Asynchronous]
		[Ignore ("Loaded Corner Case - The template has already been applied under SL even though the rules say it shouldn't be")]
		public void TemplateChild ()
		{
			ListBoxPoker box = new ListBoxPoker ();

			CreateAsyncTest (box, () => {
				DependencyObject o = box.GetTemplateChild ("ScrollViewer");
				Assert.IsNotNull (o);
			});
		}

		[TestMethod]
		[Asynchronous]
		[MoonlightBug ("The default template changed in SL3")]
		public void AfterRender ()
		{
			// Fails in Silverlight 3
			ListBox c = new ListBox ();
			ListBoxItem item = new ListBoxItem {
				Content = new Rectangle { Fill = new SolidColorBrush (Colors.Black), Width = 20, Height = 20 }
			};

			TestPanel.Children.Add (c);
			Enqueue (() => c.Items.Add (item));
			Enqueue (() => {
				Assert.VisualChildren (c,
					new VisualNode<Grid>("#0",
						new VisualNode<Border> ("#1",
							new VisualNode<ScrollViewer> ("#2",
								new VisualNode<Border> ("#3",
									new VisualNode<Grid> ("#4",
										new VisualNode<ScrollContentPresenter> ("#5",
											new VisualNode<ItemsPresenter> ("#6",
												new VisualNode<VirtualizingStackPanel> ("#7", (VisualNode[])null)
											)
										),
										new VisualNode<Rectangle> ("#7"),
										new VisualNode<ScrollBar> ("#8"),
										new VisualNode<ScrollBar> ("#9")
									)
								)
							)
						),
						new VisualNode<Border>("#a", (VisualNode[])null)
					)
				);
			});
			EnqueueTestComplete ();
		}

		[TestMethod]
		[Asynchronous]
		[MoonlightBug ("The default template changed in SL3")]
		public void AfterRender2 ()
		{
			ListBox c = new ListBox ();
			ListBoxItem item = new ListBoxItem {
				Content = new Rectangle { Fill = new SolidColorBrush (Colors.Black), Width = 20, Height = 20 }
			};
			TestPanel.Children.Add (c);
			Enqueue (() => c.Items.Add (item));
			Enqueue (() => {
				Assert.VisualParent ((Rectangle) item.Content,
					new VisualNode<ContentPresenter> ("#1"),
					new VisualNode<Grid> ("#2"),
					new VisualNode<ListBoxItem> ("#3"),
					new VisualNode<VirtualizingStackPanel> ("#4"),
					new VisualNode<ItemsPresenter> ("#5"),
					new VisualNode<ScrollContentPresenter> ("#6"),
					new VisualNode <Grid>("#7"),
					new VisualNode<Border>("#8"),
					new VisualNode<ScrollViewer>("#9"),
					new VisualNode<Border>("#10"),
					new VisualNode <Grid>("#11"),
					new VisualNode<ListBox>("#12"),
					null
				);
			});
			EnqueueTestComplete ();
		}

		[TestMethod]
		public void DefaultValues ()
		{
			ListBox lb = new ListBox ();
			Assert.IsNull (lb.ItemContainerStyle, "ItemContainerStyle = null");
			Assert.AreEqual (lb.ReadLocalValue (ListBox.ItemContainerStyleProperty), DependencyProperty.UnsetValue, "ItemContainerStyle = Unset");

			Assert.IsNull (lb.ItemTemplate, "ItemTemplate == null");
			Assert.AreEqual (ScrollViewer.GetHorizontalScrollBarVisibility (lb), ScrollBarVisibility.Auto, "Horizontal Scroll Vis"); // Fails in Silverlight 3
			Assert.AreEqual (ScrollViewer.GetVerticalScrollBarVisibility (lb), ScrollBarVisibility.Auto, "Vertical Scroll Vis");
		}

		[TestMethod]
		[Asynchronous]
		public void Focusable ()
		{
			bool loaded = false;
			ListBoxItem item = new ListBoxItem { Content = "Hello World" };
			item.Loaded += delegate { loaded = true; };

			ListBox box = new ListBox ();
			box.Items.Add (item);
			TestPanel.Children.Add (box);

			EnqueueConditional (() => loaded);
			Enqueue (() => {
				Assert.IsTrue (item.Focus (), "#1");
				box.SelectedItem = item;
				box.SelectedItem = null;
				Assert.IsTrue (item.Focus (), "#2");
			});
			EnqueueTestComplete ();
		}

		[TestMethod]
		public void ParentTest ()
		{
			ListBox box = new ListBox ();
			Rectangle r = new Rectangle ();
			ListBoxItem item = new ListBoxItem { Content = r };
			Assert.IsNull (item.Parent, "#1");
			Assert.AreEqual (item, r.Parent, "#2");
			box.Items.Add (item);
			Assert.AreEqual (item, r.Parent, "#3");
			Assert.IsNotNull (item.Parent, "#4");
			box.SelectedItem = item;
			Assert.AreEqual (item, r.Parent, "#5");
			Assert.IsNotNull (item.Parent, "#6");
			box.SelectedItem = null;
			Assert.AreEqual (item, r.Parent, "#7");
			Assert.IsNotNull (item.Parent, "#8");
		}

		[TestMethod]
		[MoonlightBug]
		public void ReferenceDoesNotChangeTest ()
		{
			object str = "Hello";
			ListBox box = new ListBox ();
			box.Items.Add (str);
			box.SelectedItem = str;
			Assert.AreSame (str, box.SelectedItem, "#1");
			Assert.AreSame (str, box.Items [0], "#2");
		}

		[TestMethod]
		[Asynchronous]
		[MoonlightBug ("The default template changed in SL3")]
		public void VisualTree ()
		{
			ListBoxPoker box = new ListBoxPoker ();
			box.ItemsSource = new int [] { 1, 2, 3 };

			Assert.VisualChildren (box, "#2"); // No VisualChildren

			// The presenter is attached after we measure
			Assert.IsFalse (box.TemplateApplied, "#1");
			box.Measure (new Size (100, 100));
			Assert.IsTrue (box.TemplateApplied, "#2");

			// A standard ItemsPresenter attaches itself during Measure
			Assert.VisualChildren (box, "#3",
				new VisualNode<ItemsPresenter> ("#a",
					new VisualNode<VirtualizingStackPanel> ("#b",
						new VisualNode<ListBoxItem> ("#c1", (VisualNode []) null),
						new VisualNode<ListBoxItem> ("#c2", (VisualNode []) null),
						new VisualNode<ListBoxItem> ("#c3", (VisualNode []) null)
					)
				)
			);

			// The template from the attached Style has attached itself here.
			CreateAsyncTest (box, () => {
				Assert.VisualChildren (box, "#4",
					new VisualNode<Grid> ("#0",
						new VisualNode<Border> ("#d",
							new VisualNode<ScrollViewer> ("#e",
								new VisualNode<Border> ("#f",
									new VisualNode<Grid> ("#g", (VisualNode []) null)
								)
							)
						),
						new VisualNode <Border>("#h", (VisualNode[])null)
					)
				);
			});
		}

		[TestMethod]
		[MoonlightBug ("Arrange should not be calling measure, this causes the template to be incorrectly applied")]
		public void VisualTree2 ()
		{
			ListBoxPoker box = new ListBoxPoker ();
			box.ItemsSource = new int [] { 1, 2, 3 };

			Assert.IsFalse (box.TemplateApplied, "#1");
			Assert.VisualChildren (box, "#2"); // No VisualChildren

			// The presenter is attached after we measure
			box.Arrange (new Rect (0, 0, 100, 100));
			Assert.VisualChildren (box, "#3");
			Assert.IsFalse (box.TemplateApplied, "#4");
		}
	}
}


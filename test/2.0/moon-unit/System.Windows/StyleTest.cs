using System;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;
using System.Collections.Generic;
using Mono.Moonlight.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace MoonTest.System.Windows
{
	public class StyledControl : Control
	{
		public static readonly DependencyProperty PropProperty = DependencyProperty.Register("Prop", typeof(int), typeof(StyledControl), null);
		public int Prop {
			get { throw new InvalidOperationException(); }
			set { throw new InvalidOperationException(); }
		}
	}

	public class StyledPanel : Panel
	{
		public static readonly DependencyProperty AttachedPropProperty = DependencyProperty.RegisterAttached("AttachedProp", typeof(int), typeof(StyledPanel), null);
		public int AttachedProp {
			get { throw new InvalidOperationException(); }
			set { throw new InvalidOperationException(); }
		}

		public static void SetAttachedProp (DependencyObject obj, int value)
		{
			obj.SetValue (AttachedPropProperty, value);
		}

		public static int GetAttachedProp (DependencyObject obj)
		{
			return (int)obj.GetValue (AttachedPropProperty);
		}
	}
		
	[TestClass]
	public class StyleTest
	{
		[TestMethod]
		public void Sealed ()
		{
			Style style = new Style (typeof (UIElement));

			style.Seal ();
			
			// This should throw, no?
			/*Assert.Throws (delegate {*/ style.TargetType = typeof (FrameworkElement);/* }, typeof (Exception));*/

			// This too?
			/*Assert.Throws (delegate {*/ style.TargetType = typeof (SolidColorBrush);/* }, typeof (Exception));*/
		}



		[TestMethod]
		public void ApplyStyleToManagedDP()
		{
			StyledControl t = new StyledControl();
			Style s = new Style(typeof(StyledControl));
			s.Setters.Add(new Setter(StyledControl.PropProperty, 100));
			t.Style = s;
			Assert.AreEqual(100, t.GetValue(StyledControl.PropProperty));
		}
		
		[TestMethod]
		public void SetTwiceOnElement ()
		{
			Style style = new Style (typeof (Rectangle));
			Rectangle r = new Rectangle ();

			// FIXME: This should pass, but commenting it out so i can test the setting an element twice
			//Assert.IsTrue (double.IsNaN (r.Width));

			r.Style = style;
			Assert.Throws (delegate { r.Style = style; }, typeof (Exception));
		}

		[TestMethod]
		[MoonlightBug ("The XamlLoader should not call the managed properties when setting the value of 'Setter.Value'")]
		public void ManagedAccessAfterParsing ()
		{
			Style s = (Style)XamlReader.Load (@"<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""Button""><Setter Property=""Width"" Value=""10""/></Style>");
			Button b = new Button ();

			b.Style = s;
			Assert.AreEqual(typeof(Button), s.TargetType, "#0");
			Setter setter = (Setter)s.Setters[0];
			Assert.IsNull(setter.Property, "#1");
			Assert.AreEqual(null, setter.Value, "#2");

			Assert.AreEqual (10, b.Width);
		}

		[TestMethod]
		public void ModifyAfterBinding()
		{
			Button b = new Button();
			Assert.AreEqual(ClickMode.Release, b.ClickMode, "#a");

			Style style = new Style(typeof(Button));

			b.Style = style;
			Assert.Throws<Exception>(delegate {
				style.Setters.Add(new Setter());
			}, "#b");

			Assert.AreEqual(0, style.Setters.Count, "#c");
			Assert.AreEqual(ClickMode.Release, b.ClickMode, "#d");

			style.Setters.Add(new Setter(Button.ClickModeProperty, ClickMode.Press));
			Assert.AreEqual(1, style.Setters.Count, "#e");
			Assert.AreEqual(ClickMode.Release, b.ClickMode, "#f");

			b.ClearValue(Button.ClickModeProperty);
			Assert.AreEqual(ClickMode.Release, b.ClickMode, "#f2");

			b.ClickMode = ClickMode.Hover;
			b.ClearValue(Button.ClickModeProperty);
			Assert.AreEqual(ClickMode.Press, b.ClickMode, "#g");

			style.Setters.Clear();
			Assert.AreEqual(0, style.Setters.Count, "#h");
			Assert.AreEqual(ClickMode.Press, b.ClickMode, "#i");

			style = new Style(typeof(Button));
			style.Setters.Add(new Setter(Button.ClickModeProperty, ClickMode.Press));
			b = new Button();
			b.Style = style;
			b.ClearValue(Button.ClickModeProperty);
			Assert.AreEqual(b.ClickMode, ClickMode.Press, "#j");

			style.Setters.Clear();
			Assert.AreEqual(0, style.Setters.Count, "#k");
			Assert.AreEqual(b.ClickMode, ClickMode.Press, "#l");

			Button c = new Button ();
			c.Style = style;

			b.ClearValue(Button.ClickModeProperty);
			Assert.AreEqual(b.ClickMode, ClickMode.Press, "#m");

			style = new Style (typeof (Button));
			b = new Button ();
			b.Style = style;

			style.Setters.Add(new Setter(Button.ClickModeProperty, ClickMode.Press));
			Assert.AreEqual(1, style.Setters.Count, "#n");
			Assert.AreEqual(ClickMode.Release, b.ClickMode, "#o");
		}

		[TestMethod]
		public void InvalidValueProgrammatically()
		{
			Button b = new Button();
			Style style = new Style(typeof(Button));
			Setter setter = new Setter(Button.WidthProperty, "this is a string");
			b.Style = style;
			Assert.IsTrue(double.IsNaN(b.Width));
		}

		[TestMethod]
		[MoonlightBug ("DP lookup isn't working")]
		public void InvalidValueParsed ()
		{
			XamlReader.Load (@"<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""Button""><Setter Property=""Width"" Value=""this is a string""/></Style>");
		}

		[TestMethod]
		public void NullLocalValue ()
		{
			Brush blue = new SolidColorBrush(Colors.Blue);
			Brush red = new SolidColorBrush(Colors.Red);
			Style style = new Style(typeof(Canvas));
			style.Setters.Add(new Setter(Canvas.BackgroundProperty, red));
			Canvas c = new Canvas();
			c.Background = blue;
			c.Style = style;
			Assert.AreEqual(blue, c.Background, "#1");
			c.Background = null;
			Assert.AreEqual(null, c.Background, "#2");
			c.ClearValue(Canvas.BackgroundProperty);
			Assert.AreEqual(red, c.Background, "#3");
		}
		
		[TestMethod]
		[MoonlightBug ("DP lookup isn't working")]
		public void MismatchTargetType ()
		{
			Button b = new Button ();
			Style s = new Style (typeof (CheckBox));
			Assert.Throws<XamlParseException> (delegate { b.Style = s; }, "#1");
			
			s = (Style)XamlReader.Load (@"<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""CheckBox""><Setter Property=""Width"" Value=""10""/></Style>");
			b = new Button ();
			Assert.Throws<XamlParseException> (delegate { b.Style = s; }, "#2");
		}

		[TestMethod]
		public void InvalidPropertyNameInSetter ()
		{
			Assert.Throws<XamlParseException> (delegate { XamlReader.Load (@"<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""Button""><Setter Property=""WidthOrHeight"" Value=""10""/></Style>"); });

			Assert.Throws<XamlParseException> (delegate { XamlReader.Load (@"<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""Button""><Setter Property=""TargetType"" Value=""10""/></Style>"); });
		}

		[TestMethod]
		[Ignore("On silverlight this seems to throw an uncatchable exception")]
		public void MissingTargetType ()
		{
			Assert.Throws<ExecutionEngineException>(delegate {
				XamlReader.Load(@"<Style xmlns=""http://schemas.microsoft.com/client/2007""><Setter Property=""Width"" Value=""10""/></Style>");
			});

			Assert.Throws<ExecutionEngineException>(delegate {
				XamlReader.Load(@"<Style xmlns=""http://schemas.microsoft.com/client/2007""><Setter Property=""WidthOrHeight"" Value=""10""/></Style>");
			});
		}

		[TestMethod]
		public void UseSetterTwice()
		{
			Style s1 = new Style(typeof(Rectangle));
			Style s2 = new Style(typeof(Rectangle));
			Setter setter = new Setter(Rectangle.WidthProperty, 5);
			s1.Setters.Add(setter);
			Assert.Throws<InvalidOperationException>(delegate {
				s2.Setters.Add(setter);
			});
			s1.Setters.Clear();
			s2.Setters.Add(setter);
		}

		public void LoadFromXaml ()
		{
			Button b = (Button)XamlReader.Load(@"
<Button xmlns=""http://schemas.microsoft.com/client/2007"" >
	<Button.Style>
		<Style TargetType=""Button"">
			<Setter Property=""Width"" Value=""10"" />
		</Style>
	</Button.Style>
</Button>");

			Assert.IsTrue(b.Style.IsSealed);
		}

		[TestMethod]
		[MoonlightBug ("DP lookup isn't working")]
		public void StyleXaml ()
		{
			Thumb t = (Thumb) XamlReader.Load (@"
<Thumb xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"">
	<Thumb.Style>
		<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""Control"">
			<Setter Property=""Tag"" Value=""Test""/>
		</Style>
	</Thumb.Style>
</Thumb>");
			Assert.IsNotNull (t, "Thumb");
			Assert.IsTrue (t.Style.IsSealed, "IsSealed");
			Assert.AreEqual (1, t.Style.Setters.Count, "Setters");
			Assert.AreEqual (typeof (Control), t.Style.TargetType, "TargetType");
		}

		[TestMethod]
		[MoonlightBug ("DP lookup isn't working")]
		public void StyleCustomProperty ()
		{
			// a valid value
			XamlReader.Load(@"
		<Style xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:T=""clr-namespace:MoonTest.System.Windows;assembly=moon-unit"" TargetType=""T:StyledControl"">
			<Setter Property=""Prop"" Value=""5""/>
		</Style>");

			// an invalid value
			XamlReader.Load(@"
		<Style xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:T=""clr-namespace:MoonTest.System.Windows;assembly=moon-unit"" TargetType=""T:StyledControl"">
			<Setter Property=""Prop"" Value=""this is a string""/>
		</Style>");
		}

		[TestMethod]
		[MoonlightBug ("Attached DP lookup isn't working")]
		public void StyleAttachedProperty()
		{
			Assert.Throws<XamlParseException> (delegate { XamlReader.Load(@"
<Rectangle xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"">
	<Rectangle.Style>
		<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""Rectangle"">
			<Setter Property=""Left"" Value=""5""/>
		</Style>
	</Rectangle.Style>
</Rectangle>"); });

			// try using the (property.path) syntax
			Assert.Throws<XamlParseException> (delegate { XamlReader.Load(@"
<Rectangle xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"">
	<Rectangle.Style>
		<Style xmlns=""http://schemas.microsoft.com/client/2007"" x:Key=""Foo"" TargetType=""Rectangle"">
			<Setter Property=""(Canvas.Left)"" Value=""5""/>
		</Style>
	</Rectangle.Style>
</Rectangle>"); });

			// try using the Type.Property syntax. this one works.
			XamlReader.Load(@"
<Rectangle xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"">
	<Rectangle.Style>
		<Style xmlns=""http://schemas.microsoft.com/client/2007"" x:Key=""Foo"" TargetType=""Rectangle"">
			<Setter Property=""Canvas.Left"" Value=""5""/>
		</Style>
	</Rectangle.Style>
</Rectangle>");


			// add a couple of tests for custom attached properties which apparently can't be styled
			Assert.Throws<XamlParseException> (delegate { XamlReader.Load(@"
<Rectangle xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"">
	<Rectangle.Style>
		<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""Rectangle"">
			<Setter Property=""AttachedProp"" Value=""5""/>
		</Style>
	</Rectangle.Style>
</Rectangle>"); }, "Custom Attached Property #1");

			Assert.Throws<XamlParseException> (delegate { XamlReader.Load(@"
<Rectangle xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"" xmlns:T=""clr-namespace:MoonTest.System.Windows;assembly=moon-unit"">
	<Rectangle.Style>
		<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""Rectangle"">
			<Setter Property=""(T:StyledPanel.AttachedProp)"" Value=""5""/>
		</Style>
	</Rectangle.Style>
</Rectangle>"); }, "Custom Attached Property #2");

			// this one works, though
			XamlReader.Load(@"
<Rectangle xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"" xmlns:T=""clr-namespace:MoonTest.System.Windows;assembly=moon-unit"">
	<Rectangle.Style>
		<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""Rectangle"">
			<Setter Property=""T:StyledPanel.AttachedProp"" Value=""5""/>
		</Style>
	</Rectangle.Style>
</Rectangle>");

			Assert.Throws<XamlParseException> (delegate { XamlReader.Load(@"
<Rectangle xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"" xmlns:T=""clr-namespace:MoonTest.System.Windows;assembly=moon-unit"">
	<Rectangle.Style>
		<Style xmlns=""http://schemas.microsoft.com/client/2007"" TargetType=""Rectangle"">
			<Setter Property=""T:AttachedProp"" Value=""5""/>
		</Style>
	</Rectangle.Style>
</Rectangle>"); }, "Custom Attached Property #3");
		}
	}
}

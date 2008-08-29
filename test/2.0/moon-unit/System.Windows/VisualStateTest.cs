using System;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;
using System.Collections.Generic;
using Mono.Moonlight.UnitTesting;

namespace MoonTest.System.Windows
{
	[TestClass]
	public class VisualStateTest
	{
		[TestMethod]
		[KnownFailure]
		public void TestParse ()
		{
			VisualState vs = (VisualState)XamlReader.Load (@"<vsm:VisualState xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"" xmlns:vsm=""clr-namespace:System.Windows;assembly=System.Windows"" x:Name=""foo""><Storyboard /></vsm:VisualState>");
			Assert.AreEqual ("foo", vs.Name);
			Assert.IsNotNull (vs.Storyboard);
		}

		[TestMethod]
		public void TestParse_NoManagedNamespace ()
		{
			Assert.Throws (delegate { 
					XamlReader.Load (@"<VisualState xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"" x:Name=""foo""><Storyboard /></VisualState>");
				},
				// "VisualState does not support Storyboard as content."
				typeof (XamlParseException));
		}

		[TestMethod]
		public void TestParse_NoStoryboard ()
		{
			VisualState vs = (VisualState)XamlReader.Load (@"<vsm:VisualState xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"" xmlns:vsm=""clr-namespace:System.Windows;assembly=System.Windows"" x:Name=""foo"" />");
			Assert.IsNull (vs.Storyboard);
		}

		[TestMethod]
		[KnownFailure]
		public void TestParse_NoName ()
		{
			VisualState vs = (VisualState)XamlReader.Load (@"<vsm:VisualState xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"" xmlns:vsm=""clr-namespace:System.Windows;assembly=System.Windows""><Storyboard /></vsm:VisualState>");
			Assert.AreEqual ("", vs.Name);
		}

		[TestMethod]
		[KnownFailure]
		public void TestDefaults ()
		{
			VisualState st = new VisualState ();
			Assert.AreEqual ("", st.Name, "1");
			Assert.IsNull (st.Storyboard, "2");
		}
	}
}

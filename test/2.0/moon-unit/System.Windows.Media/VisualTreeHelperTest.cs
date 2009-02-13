using System;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;
using System.Collections.Generic;
using Mono.Moonlight.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.Silverlight.Testing.UnitTesting.UI;
using Microsoft.Silverlight.Testing.UnitTesting.Metadata.VisualStudio;
using Microsoft.Silverlight.Testing;
using System.Windows.Markup;
using System.Linq;
using System.Threading;

namespace MoonTest.System.Windows.Media
{
	public class TestControl : UserControl
	{
		public TestControl()
		{
			Canvas c = new Canvas { };
			Rectangle r = new Rectangle { Width = 100, Height = 10, Fill = new SolidColorBrush(Colors.Orange) };
			Canvas.SetTop(r, 100);
			Canvas.SetLeft(r, 100);
			r.RenderTransform = new RotateTransform { Angle = 90 };
			c.Children.Add(r);
			
			Content = c;
		}
	}
	[TestClass]
	public class VisualTreeHelperTest : Microsoft.Silverlight.Testing.SilverlightTest
	{
		Panel Root;
	
		[TestInitialize]
		public void Init ()
		{
			Root = new Canvas { HorizontalAlignment = HorizontalAlignment.Left,
								VerticalAlignment = VerticalAlignment.Top,
								Width = 1000,
								Height = 1000,
								Name = "Root"
			};
		}
			
		[TestMethod]
		public void NullTests ()
		{
			Assert.Throws (delegate { VisualTreeHelper.GetChild (null, 1); }, typeof (InvalidOperationException));
			Assert.Throws (delegate { VisualTreeHelper.GetChildrenCount (null); }, typeof (InvalidOperationException));
			Assert.Throws (delegate { VisualTreeHelper.GetParent (null); }, typeof (InvalidOperationException));
		}

		[TestMethod]
		public void GetChild ()
		{
			Canvas p = new Canvas();
			Canvas p2 = new Canvas ();
			p.Children.Add (p2);

			Assert.AreEqual (p2, VisualTreeHelper.GetChild (p, 0));
			Assert.Throws (delegate { VisualTreeHelper.GetChild (p, 1); }, typeof (ArgumentOutOfRangeException));

			Assert.Throws (delegate { VisualTreeHelper.GetChild (new Ellipse(), 1); }, typeof (ArgumentOutOfRangeException));
		}

		[TestMethod]
		public void GetChildrenCount ()
		{
			Canvas p = new Canvas ();

			Assert.AreEqual (0, VisualTreeHelper.GetChildrenCount (p));

			p.Children.Add (new Canvas());

			Assert.AreEqual (1, VisualTreeHelper.GetChildrenCount (p));

			//			p.Resources.Add ("foo", new Canvas());

			Assert.AreEqual (1, VisualTreeHelper.GetChildrenCount (p));

			Assert.AreEqual (0, VisualTreeHelper.GetChildrenCount (new Ellipse()));
		}

		[TestMethod]
		public void GetParent ()
		{
			Canvas p = new Canvas ();
			Canvas p2 = new Canvas ();
			Canvas p3 = new Canvas ();
			SolidColorBrush scb = new SolidColorBrush ();

			p.Children.Add (p2);
			//			p.Resources.Add ("foo", p3);

			p.Background = scb;

			Assert.AreEqual (p, VisualTreeHelper.GetParent (p2));
			//Assert.AreEqual (p, VisualTreeHelper.GetParent (p3));
			Assert.AreEqual (null, VisualTreeHelper.GetParent (p));

			Assert.Throws (delegate { VisualTreeHelper.GetParent (scb); }, typeof (InvalidOperationException));
		}
		
		[TestMethod]
		[Asynchronous]
		public void HitTest1()
		{
			Root.Dispatcher.BeginInvoke(delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), Root));
				Assert.AreEqual(0, hits.Count, "#1");
				this.TestComplete ();
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest2()
		{
			Root.Background = new SolidColorBrush(Colors.Black);
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), Root));
				Assert.AreEqual(1, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(Root.Width, 0), Root));
				Assert.AreEqual(1, hits.Count, "#2");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(Root.Width, Root.Height), Root));
				Assert.AreEqual(0, hits.Count, "#3");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(Root.Width, Root.Height-1), Root));
				Assert.AreEqual(1, hits.Count, "#4");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, Root.Height-1), Root));
				Assert.AreEqual(1, hits.Count, "#5");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, Root.Height), Root));
				Assert.AreEqual(0, hits.Count, "#6");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest3()
		{
			Root.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Black) });
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(5, 5), Root));
				Assert.AreEqual(2, hits.Count, "#1");
				Assert.IsTrue(hits[0] is Rectangle, "#2");
				Assert.IsTrue(hits[1] == Root, "#3");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest4()
		{
			Root.Children.Add((Path)XamlReader.Load(
@"<Path xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"" x:Name=""ShipShell"" Stretch=""Fill"" Stroke=""#FF000000"" Width=""50"" Height=""44"" Canvas.Left=""1"" Canvas.Top=""1"" Data=""M256,82 L241,99 239,129 231,150 206,162 206,176 257,180 311,174 299,155 280,148 273,127 271,98 z"">
	<Path.Fill>
		<RadialGradientBrush>
			<GradientStop Color=""#FFE1D0D0"" Offset=""0""/>
			<GradientStop Color=""#FF673434"" Offset=""1""/>
			<GradientStop Color=""#FFE3D8D8"" Offset=""0.024""/>
		</RadialGradientBrush>
	</Path.Fill>
</Path>"));

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(30, 30), Root));
				Assert.AreEqual(2, hits.Count, "#1");
				Assert.IsTrue(hits[0] is Path, "#2");
				Assert.IsTrue(hits[1] == Root, "#3");
			});
		}
		
		[TestMethod]
		[Asynchronous]
		public void HitTest5()
		{
			Root.Children.Add(new Ellipse { Width = 100, Height = 100 });
			Root.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue) });
			Root.Children.Add((Path)XamlReader.Load(
@"<Path xmlns=""http://schemas.microsoft.com/client/2007"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"" x:Name=""ShipShell"" Stretch=""Fill"" Stroke=""#FF000000"" Width=""50"" Height=""44"" Canvas.Left=""1"" Canvas.Top=""1"" Data=""M256,82 L241,99 239,129 231,150 206,162 206,176 257,180 311,174 299,155 280,148 273,127 271,98 z"">
	<Path.Fill>
		<RadialGradientBrush>
			<GradientStop Color=""#FFE1D0D0"" Offset=""0""/>
			<GradientStop Color=""#FF673434"" Offset=""1""/>
			<GradientStop Color=""#FFE3D8D8"" Offset=""0.024""/>
		</RadialGradientBrush>
	</Path.Fill>
</Path>"));

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(30, 30), Root));
				Assert.AreEqual(3, hits.Count, "#1");
				Assert.IsTrue(hits[0] is Path, "Should be Path, was " + hits[0].GetType ().Name);
				Assert.IsTrue(hits[1] is Rectangle, "Should be Rectangle, was " + hits[1].GetType ().Name);
				Assert.IsTrue(hits[2] is Canvas, "Should be Canvas, was " + hits[2].GetType ().Name);
			});
		}
		
		[TestMethod]
		[Asynchronous]
		public void HitTest6()
		{
			Canvas panel = new Canvas { Width = 1000, Height = 1000, Name = "A" };
			panel.Children.Add(new Ellipse { Width = 100, Height = 100, Name = "A1" });
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue), Name = "A2" });
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue), Name = "A3" });
			Root.Children.Add(panel);

			panel = new Canvas { Width = 1000, Height = 1000, Name = "B" };
			panel.Children.Add(new Ellipse { Width = 100, Height = 100, Name = "B1" });
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue), Name = "B2" });
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue), Name = "B3" });
			Root.Children.Add(panel);

			CreateAsyncTest(Root, delegate {
				List<FrameworkElement> hits = VisualTreeHelper.FindElementsInHostCoordinates(new Point(30, 30), Root).Cast<FrameworkElement>().ToList();
				Assert.AreEqual(7, hits.Count, "#1");
				Assert.AreEqual("B3", hits[0].Name, "#2");
				Assert.AreEqual("B2", hits[1].Name, "#3");
				Assert.AreEqual("B", hits[2].Name, "#4");
				Assert.AreEqual("A3", hits[3].Name, "#2");
				Assert.AreEqual("A2", hits[4].Name, "#3");
				Assert.AreEqual("A", hits[5].Name, "#4");
				Assert.AreEqual("Root", hits[6].Name, "#4");
			});
		}
		
		[TestMethod]
		[Asynchronous]
		public void HitTest7()
		{
			Canvas panel = new Canvas { Width = 1000, Height = 1000, Name = "A" };
			panel.Children.Add(new Ellipse { Width = 100, Height = 100, Name = "A1" });
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue), Name = "A2" });
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue), Name = "A3" });
			Root.Children.Add(panel);

			Canvas.SetZIndex(panel, 1);
			Canvas.SetZIndex(panel.Children[0], 1);
			Canvas.SetZIndex(panel.Children[1], 2);
			Canvas.SetZIndex(panel.Children[2], 0);

			panel = new Canvas { Width = 1000, Height = 1000, Name = "B" };
			panel.Children.Add(new Ellipse { Width = 100, Height = 100, Name = "B1" });
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue), Name = "B2" });
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue), Name = "B3" });
			Root.Children.Add(panel);

			Canvas.SetZIndex(panel, 0);
			Canvas.SetZIndex(panel.Children[0], 20);
			Canvas.SetZIndex(panel.Children[1], 30);
			Canvas.SetZIndex(panel.Children[2], 10);

			CreateAsyncTest(Root, delegate {
				List<FrameworkElement> hits = VisualTreeHelper.FindElementsInHostCoordinates(new Point(30, 30), Root).Cast<FrameworkElement>().ToList();
				Assert.AreEqual(7, hits.Count, "#1");
				Assert.AreEqual("A2", hits[0].Name, "#2");
				Assert.AreEqual("A3", hits[1].Name, "#3");
				Assert.AreEqual("A", hits[2].Name, "#4");
				Assert.AreEqual("B2", hits[3].Name, "#2");
				Assert.AreEqual("B3", hits[4].Name, "#3");
				Assert.AreEqual("B", hits[5].Name, "#4");
				Assert.AreEqual("Root", hits[6].Name, "#4");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest8()
		{
			Root.Width = 0;
			Root.Height = 1;
			Root.Background = new SolidColorBrush(Colors.Black);
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), Root));
				Assert.AreEqual(1, hits.Count, "#1");
			});
		}

		[TestMethod]
		[Asynchronous]
		[MoonlightBug]
		public void HitTest8b()
		{
			Root.Width = 0;
			Root.Height = 1;
			Root.Background = new SolidColorBrush(Colors.Black);
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Rect(0, 0,2,2), Root));
				Assert.AreEqual(1, hits.Count, "#1");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest9()
		{
			Root.Width = 0;
			Root.Height = 0;
			Root.Background = new SolidColorBrush(Colors.Black);
			CreateAsyncTest(Root, delegate
			{
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), Root));
				Assert.AreEqual(0, hits.Count, "#1");
			});
		}
		
		[TestMethod]
		[Asynchronous]
		public void HitTest10()
		{
			Root.MaxWidth = 50;
			Root.MaxHeight = 50;
			Root.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Black) });
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(75, 75), Root));
				Assert.AreEqual(2, hits.Count, "#1");
			});
		}
		
		[TestMethod]
		[Asynchronous]
		public void HitTest11()
		{
			Root.Children.Add(new Rectangle { Width = 100, Height = 100,
				Fill = new SolidColorBrush(Colors.Black),
				Name = "A"
			});
			
			Root.Children.Add(new Rectangle { Width = 100, Height = 100,
				Fill = new SolidColorBrush(Colors.Black),
				IsHitTestVisible = false
			});
			CreateAsyncTest(Root, delegate {
				List<FrameworkElement> hits = VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), Root).Cast<FrameworkElement> ().ToList ();
				Assert.AreEqual(2, hits.Count, "#1");
				Assert.AreEqual ("A", hits[0].Name);
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest12()
		{
			Root.MaxWidth = 50;
			Root.MaxHeight = 50;
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), Root));
				Assert.AreEqual(0, hits.Count, "#1");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest13()
		{
			Root.Children.Add(new Rectangle { Width = 100, Height = 100,
				Fill = new SolidColorBrush(Colors.Black),
				IsHitTestVisible = false
			});
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), Root));
				Assert.AreEqual(0, hits.Count, "#1");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest14()
		{
			Root.IsHitTestVisible = false;
			Root.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Black) });
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), Root));
				Assert.AreEqual(0, hits.Count, "#1");
			});
		}


		[TestMethod]
		[Asynchronous]
		public void HitTest15()
		{
			Canvas panel = new Canvas { Width = 1000, Height = 1000, Name = "A" };
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue), Name = "A1" });
			Root.Children.Add(panel);

			panel = new Canvas { Width = 1000, Height = 1000, Name = "B", IsHitTestVisible = false };
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Blue), Name = "B1" });
			Root.Children.Add(panel);

			CreateAsyncTest(Root, delegate {
				List<FrameworkElement> hits = VisualTreeHelper.FindElementsInHostCoordinates(new Point(30, 30), Root).Cast<FrameworkElement>().ToList();
				Assert.AreEqual(3, hits.Count, "#1");
				Assert.AreEqual("A1", hits[0].Name, "#2");
				Assert.AreEqual("A", hits[1].Name, "#3");
				Assert.AreEqual("Root", hits[2].Name, "#4");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest16()
		{
			Canvas panel = new Canvas { Width = 1000, Height = 1000, Name = "B"};
			panel.Children.Add(new Rectangle { Width = 100, Height = 100, IsHitTestVisible = false, Fill = new SolidColorBrush(Colors.Blue), Name = "B1" });
			Root.Children.Add(panel);

			CreateAsyncTest(Root, delegate {
				List<FrameworkElement> hits = VisualTreeHelper.FindElementsInHostCoordinates(new Point(30, 30), Root).Cast<FrameworkElement>().ToList();
				Assert.AreEqual(0, hits.Count, "#1");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest17()
		{
			Root.MaxWidth = 50;
			Root.MaxHeight = 50;
			Root.Children.Add(new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Black) });
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(100, 100), Root));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(100, 99), Root));
				Assert.AreEqual(2, hits.Count, "#2");
			});
		}
					
		[TestMethod]
		[Asynchronous]
		public void HitTest18()
		{
			Border b = new Border {CornerRadius = new CornerRadius(50),
				Width = 100,
				Height = 100,
				Background = new SolidColorBrush (Colors.Black)
			};
			Root.Children.Add(b);

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(10, 10), Root));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(50, 50), Root));
				Assert.AreEqual(2, hits.Count, "#2");
			});
		}
					
		[TestMethod]
		[Asynchronous]
		public void HitTest19()
		{
			Rectangle r = new Rectangle { Width = 100, Height = 10, Fill = new SolidColorBrush( Colors.Black) };
			r.Clip = new RectangleGeometry { Rect = new Rect(50, 0, 50, 10) };
			Root.Children.Add(r);

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(10, 5), Root));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(75, 5), Root));
				Assert.AreEqual(2, hits.Count, "#1");
			});
		}
		
		[TestMethod]
		[Asynchronous]
		public void HitTest20()
		{
			Rectangle r = new Rectangle {
				Width = 100,
				Height = 10,
				Fill = new SolidColorBrush(Colors.Green),
				RenderTransform = new RotateTransform { Angle = 90 }
			};
			Canvas.SetLeft(r, 10);
			Root.Children.Add(r);
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(50, 5), Root));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(5, 50), Root));
				Assert.AreEqual(2, hits.Count, "#2");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest21()
		{
			Rectangle r = new Rectangle {
				Width = 100,
				Height = 10,
				Fill = new SolidColorBrush(Colors.Green),
				RenderTransform = new RotateTransform { Angle = 45 }
			};
			Canvas.SetLeft(r, 10);
			Root.Children.Add(r);
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(50, 5), Root));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(5, 50), Root));
				Assert.AreEqual(0, hits.Count, "#2");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(50, 50), Root));
				Assert.AreEqual(2, hits.Count, "#3");
			});
		}
					
		[TestMethod]
		[Asynchronous]
		[Ignore ("This makes moon hang")]
		public void HitTest22()
		{
			PathFigure figure = new PathFigure { IsFilled = true, StartPoint = new Point(10, 0) };
			figure.Segments.Add(new LineSegment { Point = new Point(20, 10) });
			figure.Segments.Add(new LineSegment { Point = new Point(0, 10) });
			figure.Segments.Add(new LineSegment { Point = new Point(10, 0) });

			PathGeometry g = new PathGeometry();
			g.Figures.Add(figure);

			Path p = new Path { Fill = new SolidColorBrush(Colors.Black), Data = g };
			Root.Children.Add(p);

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), Root));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(10, 0), Root));
				Assert.AreEqual(2, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 10), Root));
				Assert.AreEqual(0, hits.Count, "#1");
			});
		}
					
		[TestMethod]
		[Asynchronous]
		public void HitTest23()
		{
			Rectangle r = new Rectangle { Width = 100, Height = 10, Fill = new SolidColorBrush( Colors.Black) };
			r.Clip = new RectangleGeometry { Rect = new Rect(50, 0, 50, 10) };
			Root.Children.Add(r);
			Root.Clip = new RectangleGeometry { Rect = new Rect (0, 0, 1, 1) };

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(10, 5), Root));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(75, 5), Root));
				Assert.AreEqual (0, hits.Count, "#2");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest24()
		{
			Rectangle r = new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Black) };
			Canvas.SetLeft(r, 100);
			Canvas.SetTop(r, 100);

			Root.Children.Add(r);
			CreateAsyncTest(Root, delegate
			{
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(50, 50), Root));
				Assert.AreEqual(0, hits.Count, "#1");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest25()
		{
			Root.Background = new SolidColorBrush(Colors.Brown);
			Rectangle r = new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Black) };
			Canvas.SetLeft(r, 100);
			Canvas.SetTop(r, 100);

			Root.Children.Add(r);
			CreateAsyncTest(Root, delegate
			{
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(50, 50), Root));
				Assert.AreEqual(1, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(150, 150), Root));
				Assert.AreEqual(2, hits.Count, "#1");
			});
		}

		[TestMethod]
		[Asynchronous]
		[MoonlightBug]
		public void HitTest26()
		{
			Rectangle r = new Rectangle { Width = 100, Height = 100, Fill = new SolidColorBrush(Colors.Black) };
			Canvas.SetLeft(r, 100);
			Canvas.SetTop(r, 100);

			Canvas main = new Canvas { Background = new SolidColorBrush (Colors.Magenta) };
			Canvas.SetLeft(main, 100);
			Canvas.SetTop(main, 100);

			main.Children.Add(r);
			main.Children.Add(new Rectangle { Width = 10, Height = 10, Fill = new SolidColorBrush(Colors.Orange) });

			Root.Children.Add(main);
			CreateAsyncTest(Root, delegate
			{
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(5, 5), Root));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(100, 100), Root));
				Assert.AreEqual(3, hits.Count, "#2");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(199, 199), Root));
				Assert.AreEqual(0, hits.Count, "#3");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(200, 200), Root));
				Assert.AreEqual(3, hits.Count, "#4");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest27()
		{
			Root.Children.Add(new TestControl());
			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(101, 101), Root));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(100, 100), Root));
				Assert.AreEqual(4, hits.Count, "#2");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(91, 100), Root));
				Assert.AreEqual(4, hits.Count, "#3");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(89, 100), Root));
				Assert.AreEqual(0, hits.Count, "#4");
			});
		}

		[TestMethod]
		[Asynchronous]
		public void HitTest28()
		{
			Rectangle r = new Rectangle { Stroke = new SolidColorBrush(Colors.Blue), Width = 100, Height = 100, StrokeThickness = 0 };
			Root.Children.Add(r);

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(10, 10), r));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), r));
				Assert.AreEqual(0, hits.Count, "#2");
			});
		}

		[TestMethod]
		[Asynchronous]
		[MoonlightBug]
		public void HitTest29()
		{
			Rectangle r = new Rectangle { Stroke = new SolidColorBrush(Colors.Blue), Width = 100, Height = 100, StrokeThickness = 1 };
			Root.Children.Add(r);

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), r));
				Assert.AreEqual(1, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(r.Width, 0), r));
				Assert.AreEqual(1, hits.Count, "#2");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(r.Width, r.Height), r));
				Assert.AreEqual(0, hits.Count, "#3");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(r.Width, r.Height - 1), r));
				Assert.AreEqual(1, hits.Count, "#4");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, r.Height - 1), r));
				Assert.AreEqual(1, hits.Count, "#5");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, r.Height), r));
				Assert.AreEqual(0, hits.Count, "#6");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(10, 10), r));
				Assert.AreEqual(0, hits.Count, "#7");
			});
		}

		[TestMethod]
		[Asynchronous]
		[MoonlightBug]
		public void HitTest30()
		{
			Border b = new Border { Width = 100, Height = 100, BorderBrush = new SolidColorBrush(Colors.Green), BorderThickness = new Thickness(0) };
			Root.Children.Add(b);

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(10, 10), b));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), b));
				Assert.AreEqual(1, hits.Count, "#2");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(1, 0), b));
				Assert.AreEqual(0, hits.Count, "#3");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 1), b));
				Assert.AreEqual(1, hits.Count, "#4");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, 0), b));
				Assert.AreEqual(1, hits.Count, "#5");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width - 1, 0), b));
				Assert.AreEqual(0, hits.Count, "#6");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, 1), b));
				Assert.AreEqual(1, hits.Count, "#7");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, b.Height - 1), b));
				Assert.AreEqual(1, hits.Count, "#8");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(20, 20), b));
				Assert.AreEqual(0, hits.Count, "#9");
			});
		}

		[TestMethod]
		[Asynchronous]
		[MoonlightBug]
		public void HitTest30b()
		{
			Border b = new Border { Width = 100, Height = 100, BorderBrush = new SolidColorBrush(Colors.Green), BorderThickness = new Thickness(10) };
			Root.Children.Add(b);

			CreateAsyncTest(Root, delegate
			{
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(10, 10), b));
				Assert.AreEqual(1, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), b));
				Assert.AreEqual(1, hits.Count, "#2");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(1, 0), b));
				Assert.AreEqual(1, hits.Count, "#3");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 1), b));
				Assert.AreEqual(1, hits.Count, "#4");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, 0), b));
				Assert.AreEqual(1, hits.Count, "#5");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width - 1, 0), b));
				Assert.AreEqual(1, hits.Count, "#6");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, 1), b));
				Assert.AreEqual(1, hits.Count, "#7");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, b.Height - 1), b));
				Assert.AreEqual(1, hits.Count, "#8");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(20, 20), b));
				Assert.AreEqual(0, hits.Count, "#9");
			});
		}

		[TestMethod]
		[Asynchronous]
		[MoonlightBug]
		public void HitTest31()
		{
			Border b = new Border { Width = 100, Height = 100, BorderBrush = new SolidColorBrush(Colors.Green), BorderThickness = new Thickness(0) };
			Rectangle r = new Rectangle { Width = 10, Height = 10, Fill = new SolidColorBrush(Colors.Blue) };
			Canvas.SetLeft(r, 70);
			Canvas.SetTop(r, 70);
			b.Child = r;
			Root.Children.Add(b);

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(10, 10), b));
				Assert.AreEqual(0, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), b));
				Assert.AreEqual(1, hits.Count, "#2");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(1, 0), b));
				Assert.AreEqual(0, hits.Count, "#3");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 1), b));
				Assert.AreEqual(1, hits.Count, "#4");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, 0), b));
				Assert.AreEqual(1, hits.Count, "#5");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width - 1, 0), b));
				Assert.AreEqual(0, hits.Count, "#6");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, 1), b));
				Assert.AreEqual(1, hits.Count, "#7");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, b.Height - 1), b));
				Assert.AreEqual(1, hits.Count, "#8");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(20, 20), b));
				Assert.AreEqual(0, hits.Count, "#9");
			});
		}

		[TestMethod]
		[Asynchronous]
		[MoonlightBug]
		public void HitTest31b()
		{
			Border b = new Border { Width = 100, Height = 100, BorderBrush = new SolidColorBrush(Colors.Green), BorderThickness = new Thickness(10) };
			Rectangle r = new Rectangle { Width = 10, Height = 10, Fill = new SolidColorBrush(Colors.Blue) };
			Canvas.SetLeft(r, 70);
			Canvas.SetTop(r, 70);
			b.Child = r;
			Root.Children.Add(b);

			CreateAsyncTest(Root, delegate {
				List<UIElement> hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(10, 10), b));
				Assert.AreEqual(1, hits.Count, "#1");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 0), b));
				Assert.AreEqual(1, hits.Count, "#2");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(1, 0), b));
				Assert.AreEqual(1, hits.Count, "#3");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(0, 1), b));
				Assert.AreEqual(1, hits.Count, "#4");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, 0), b));
				Assert.AreEqual(1, hits.Count, "#5");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width - 1, 0), b));
				Assert.AreEqual(1, hits.Count, "#6");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, 1), b));
				Assert.AreEqual(1, hits.Count, "#7");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(b.Width, b.Height - 1), b));
				Assert.AreEqual(1, hits.Count, "#8");
				hits = new List<UIElement>(VisualTreeHelper.FindElementsInHostCoordinates(new Point(20, 20), b));
				Assert.AreEqual(0, hits.Count, "#9");
			});
		}
					
		[TestMethod]
		[Asynchronous]
		[MoonlightBug]
		public void AsyncLoaded ()
		{
			try {
				ManualResetEvent handle = new ManualResetEvent (false);
				Border b = new Border { Width = 100, Height = 100, BorderBrush = new SolidColorBrush (Colors.Green), BorderThickness = new Thickness (10) };
				b.Loaded += delegate { handle.Set (); };

				TestPanel.Children.Add (b);

				Assert.IsFalse (handle.WaitOne (0), "Not syncronous");

				EnqueueConditional (delegate {
					if (handle.WaitOne (10000))
						return true;
					throw new Exception ("Loaded was never executed");
				});
			} finally {
				EnqueueTestComplete ();
			}
		}

		[TestMethod]
		[Asynchronous]
		[ExpectedException(typeof (Exception))]
		public void AsyncLoaded2 ()
		{
			try {
				ManualResetEvent handle = new ManualResetEvent (false);
				Border b = new Border { Width = 100, Height = 100, BorderBrush = new SolidColorBrush (Colors.Green), BorderThickness = new Thickness (10) };
				
				TestPanel.Children.Add (b);

				b.Loaded += delegate { handle.Set (); };
				Assert.IsFalse (handle.WaitOne (0), "Not syncronous");

				EnqueueConditional (delegate {
					if (handle.WaitOne (500))
						return true;
					throw new Exception ("Loaded was never executed");
				});
			} finally {
				EnqueueTestComplete ();
			}
		}
	}
}
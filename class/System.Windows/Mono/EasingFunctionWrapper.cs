
using System;
using System.Text;
using System.Windows;
using System.Windows.Media.Animation;

namespace Mono {

	internal delegate double EasingFunctionCallback (double normalizedTime);

	internal sealed class EasingFunctionWrapper : EasingFunctionBase {

		IEasingFunction function;

		public EasingFunctionWrapper (IEasingFunction function)
		{
			this.function = function;
			this.EasingMode = EasingMode.EaseIn;
		}

		public override double EaseInCore (double normalizedTime)
		{
			return function.Ease (normalizedTime);
		}

		public static EasingFunctionCallback CreateSafeEasingFunction (EasingFunctionCallback function)
		{
			return delegate (double normalizedTime) {
				try {
					return function (normalizedTime);
				}
				catch (Exception ex) {
					try {
#if DEBUG
						// if running inside the plugin
						if (System.Windows.Interop.PluginHost.Handle != IntPtr.Zero)
							Helper.ReportException (ex);
						else
#endif
							Console.WriteLine ("Moonlight: Unhandled exception in EasingHelper.CreateSafeEasingFunction: {0}",
									   ex);
					}
					catch {
						// Ignore
					}

					return 0.0;
				}
			};
		}
	}
}

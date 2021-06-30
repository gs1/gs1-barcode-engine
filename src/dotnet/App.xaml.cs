using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace gs1encoders_dotnet
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {

        public static GS1Encoder gs1Encoder = new GS1Encoder();

        /*
         *  Native functions so that we can write our version information to the console (for CI)
         * 
         */
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool AttachConsole(uint dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool FreeConsole();

        const uint ATTACH_PARENT_PROCESS = 0x0ffffffff;  // default value if not specifing a process ID

        private void App_Startup(object sender, StartupEventArgs e)
        {

            string[] arguments = Environment.GetCommandLineArgs();
            if (arguments.Length == 2 && arguments[1].Equals("--version"))
            {
                AttachConsole(ATTACH_PARENT_PROCESS);
                Console.WriteLine("pinvoked DLL version: " + gs1Encoder.Version);
                FreeConsole();
                Shutdown(0);
                return;
            }

            MainWindow mw = new MainWindow
            {
                Title = "GS1 Encoders | Library release: " + gs1Encoder.Version
            };
            mw.Show();

            gs1Encoder.FileInputFlag = false;
            gs1Encoder.OutFile = "";
            gs1Encoder.Format = (int)GS1Encoder.Formats.BMP;
            gs1Encoder.PixMult = 1;

            gs1Encoder.Sym = (int)GS1Encoder.Symbology.DM;
            mw.dataStrTextBox.Text = "(01)02112345678900";
            gs1Encoder.AIdataStr = mw.dataStrTextBox.Text;
            
            mw.LoadControls();

            mw.generateButton.RaiseEvent(new RoutedEventArgs(System.Windows.Controls.Button.ClickEvent));

        }

    }

}

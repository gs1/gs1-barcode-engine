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

        GS1Encoder gs1Encoder = new GS1Encoder();

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
                Console.WriteLine("pinvoked DLL version: " + gs1Encoder.getVersion());
                FreeConsole();
                Shutdown(0);
                return;
            }

            MainWindow mw = new MainWindow();
            mw.Title = "GS1 Encoders | Library release: " + gs1Encoder.getVersion();
            mw.Show();

        }


    }

}

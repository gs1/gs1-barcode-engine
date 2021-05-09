using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

namespace gs1encoders_dotnet
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {

        GS1Encoder gs1Encoder = new GS1Encoder();

        private void App_Startup(object sender, StartupEventArgs e)
        {
            MainWindow mw = new MainWindow();
            mw.Title = "GS1 Encoders | Library release: " + gs1Encoder.getVersion();
            mw.Show();
        }


    }

}

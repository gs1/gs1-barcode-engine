using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO;

namespace gs1encoders_dotnet
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        public void LoadControls()
        {
            dataStrTextBox.Text = App.gs1Encoder.GetDataStr();
            pixMultTextBox.Text = App.gs1Encoder.GetPixMult().ToString();
            XundercutTextBox.Text = App.gs1Encoder.GetXundercut().ToString();
            YundercutTextBox.Text = App.gs1Encoder.GetYundercut().ToString();
            sepHtTextBox.Text = App.gs1Encoder.GetSepHt().ToString();
            segWidthTextBox.Text = App.gs1Encoder.GetSegWidth().ToString();
            linHeightTextBox.Text = App.gs1Encoder.GetLinHeight().ToString();
        }

        private void generateButton_Click(object sender, RoutedEventArgs e)
        {

            try
            {
                App.gs1Encoder.SetSym(9);  // TODO fixme
                App.gs1Encoder.SetDataStr(dataStrTextBox.Text);
                int v;
                if (Int32.TryParse(pixMultTextBox.Text, out v)) App.gs1Encoder.SetPixMult(v);
                if (Int32.TryParse(XundercutTextBox.Text, out v)) App.gs1Encoder.SetXundercut(v);
                if (Int32.TryParse(YundercutTextBox.Text, out v)) App.gs1Encoder.SetYundercut(v);
                if (Int32.TryParse(sepHtTextBox.Text, out v)) App.gs1Encoder.SetSepHt(v);
                if (Int32.TryParse(segWidthTextBox.Text, out v)) App.gs1Encoder.SetSegWidth(v);
                if (Int32.TryParse(linHeightTextBox.Text, out v)) App.gs1Encoder.SetLinHeight(v);
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageTextBox.Text = E.Message;
                return;
            }      

            App.gs1Encoder.Encode();
            byte[] imageData = App.gs1Encoder.GetBuffer();

            using (MemoryStream ms = new MemoryStream(imageData))
            {

                BitmapImage img = new BitmapImage();
                img.BeginInit();
                img.CacheOption = BitmapCacheOption.OnLoad;
                img.StreamSource = ms;
                img.EndInit();
                if (img.CanFreeze)
                    img.Freeze();
                barcodeImage.Source = img;
            }

            LoadControls();

        }

    }
}

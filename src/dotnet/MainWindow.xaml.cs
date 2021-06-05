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
using System.Globalization;

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
            symbologyComboBox.SelectedIndex = App.gs1Encoder.Sym;
            rawDataLabel.Content = "Raw data (FNC1 = \"#\"):   " + App.gs1Encoder.DataStr;
            pixMultTextBox.Text = App.gs1Encoder.PixMult.ToString(CultureInfo.InvariantCulture);
            XundercutTextBox.Text = App.gs1Encoder.Xundercut.ToString(CultureInfo.InvariantCulture);
            YundercutTextBox.Text = App.gs1Encoder.Yundercut.ToString(CultureInfo.InvariantCulture);
            sepHtTextBox.Text = App.gs1Encoder.SepHt.ToString(CultureInfo.InvariantCulture);
            segWidthComboBox.SelectedValue = App.gs1Encoder.DataBarExpandedSegmentsWidth.ToString(CultureInfo.InvariantCulture);
            linHeightTextBox.Text = App.gs1Encoder.GS1_128LinearHeight.ToString(CultureInfo.InvariantCulture);
            qrVersionComboBox.SelectedValue = App.gs1Encoder.QrVersion.ToString(CultureInfo.InvariantCulture);
            qrEClevelComboBox.SelectedValue = App.gs1Encoder.QrEClevel.ToString(CultureInfo.InvariantCulture);
            dmRowsComboBox.SelectedValue = App.gs1Encoder.DmRows.ToString(CultureInfo.InvariantCulture);
        }

        private void generateButton_Click(object sender, RoutedEventArgs e)
        {
            barcodeImage.Source = null;
            rawDataLabel.Content = "";
            errorMessageLabel.Content = "";

            try
            {
                int v;
                App.gs1Encoder.Sym = symbologyComboBox.SelectedIndex;
                if (dataStrTextBox.Text.Length > 0 && dataStrTextBox.Text[0] == '(')
                {
                    App.gs1Encoder.GS1dataStr = dataStrTextBox.Text;
                }
                else
                {
                    App.gs1Encoder.DataStr = dataStrTextBox.Text;
                }
                if (Int32.TryParse(pixMultTextBox.Text, out v)) App.gs1Encoder.PixMult = v;
                if (Int32.TryParse(XundercutTextBox.Text, out v)) App.gs1Encoder.Xundercut = v;
                if (Int32.TryParse(YundercutTextBox.Text, out v)) App.gs1Encoder.Yundercut = v;
                if (Int32.TryParse(sepHtTextBox.Text, out v)) App.gs1Encoder.SepHt = v;
                if (Int32.TryParse((string)segWidthComboBox.SelectedValue, out v)) App.gs1Encoder.DataBarExpandedSegmentsWidth = v;
                if (Int32.TryParse(linHeightTextBox.Text, out v)) App.gs1Encoder.GS1_128LinearHeight = v;
                if (Int32.TryParse((string)qrVersionComboBox.SelectedValue, out v)) App.gs1Encoder.QrVersion = v;
                if (Int32.TryParse((string)qrEClevelComboBox.SelectedValue, out v)) App.gs1Encoder.QrEClevel = v;
                if (Int32.TryParse((string)dmRowsComboBox.SelectedValue, out v)) App.gs1Encoder.DmRows = v;
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                LoadControls();
                return;
            }      

            try
            { 
                App.gs1Encoder.Encode();
            }
            catch (GS1EncoderEncodeException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                LoadControls();
                return;
            }

            byte[] imageData = App.gs1Encoder.GetBuffer();

            /*
            try
            {
            */
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
        /*  
            }        
            catch (Exception)
            {
                errorMessageLabel.Content = "An error occurred generating the barcode image";
                LoadControls();
                return;
            }
        */

            LoadControls();

        }

    }
}

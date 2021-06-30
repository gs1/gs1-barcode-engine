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
using Microsoft.Win32;
using System.IO;
using System.Globalization;
using System.Text.RegularExpressions;

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

            infoLabel.Content = "";
            if (App.gs1Encoder.DataStr.Length > 0 && App.gs1Encoder.DataStr[0] == '#')  // AI data
            {
                if (dataStrTextBox.Text.StartsWith("#"))            
                        infoLabel.Content = "AI data:   " + App.gs1Encoder.AIdataStr;
                else if (dataStrTextBox.Text.StartsWith("("))                   
                        infoLabel.Content = "Encoded data (FNC1 = \"#\"):   " + App.gs1Encoder.DataStr;                
            }

            if (App.gs1Encoder.DataStr.StartsWith("http://") || App.gs1Encoder.DataStr.StartsWith("https://"))
                infoLabel.Content = "Extracted AI data:   " + App.gs1Encoder.AIdataStr;

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
            infoLabel.Content = "";
            errorMessageLabel.Content = "";
            hriTextBox.Text = "";
            SaveBMPButton.IsEnabled = false;
            SavePNGButton.IsEnabled = false;            
            PrintButton.IsEnabled = false;

            try
            {
                int v;
                App.gs1Encoder.Sym = symbologyComboBox.SelectedIndex;
                if (dataStrTextBox.Text.Length > 0 && dataStrTextBox.Text[0] == '(')
                {
                    App.gs1Encoder.AIdataStr = dataStrTextBox.Text;
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

                string scan = App.gs1Encoder.ScanData;
                scan = Regex.Replace(scan, @"\p{Cc}", a => "{" + string.Format("%{0:X2}", (byte)a.Value[0]) + "}");
                errorMessageLabel.Content = "Scan data: " + scan;

                string[] hri = App.gs1Encoder.HRI;
                hriTextBox.Text = "";
                foreach (string ai in hri)
                {
                    hriTextBox.Text += ai + "\n";
                }
            }
            catch (GS1EncoderEncodeException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                LoadControls();
                return;
            }
            
            using (MemoryStream ms = new MemoryStream(App.gs1Encoder.GetBuffer()))
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

            SaveBMPButton.IsEnabled = true;
            SavePNGButton.IsEnabled = true;
            PrintButton.IsEnabled = true;

            LoadControls();

        }

        private void SaveBMPButton_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog();
            saveFileDialog.Filter = "Bitmap Image (*.bmp)|*.bmp";
            if (saveFileDialog.ShowDialog() == true) {
                BitmapEncoder encoder = new BmpBitmapEncoder();
                encoder.Frames.Add(BitmapFrame.Create((BitmapSource)barcodeImage.Source));
                using (var fileStream = new System.IO.FileStream(saveFileDialog.FileName, System.IO.FileMode.Create))
                {
                    encoder.Save(fileStream);
                }
            }                
        }
        
        private void SavePNGButton_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog();
            saveFileDialog.Filter = "Portable Network Graphic (*.png)|*.png";
            if (saveFileDialog.ShowDialog() == true)
            {
                BitmapEncoder encoder = new PngBitmapEncoder();
                encoder.Frames.Add(BitmapFrame.Create((BitmapSource)barcodeImage.Source));
                using (var fileStream = new System.IO.FileStream(saveFileDialog.FileName, System.IO.FileMode.Create))
                {
                    encoder.Save(fileStream);
                }
            }
        }

        private void CopyToClipboardButton_Click(object sender, RoutedEventArgs e)
        {
            Clipboard.SetImage(barcodeImage.Source as BitmapSource);
        }

        private void PrintButton_Click(object sender, RoutedEventArgs e)
        {
            PrintDialog printDialog = new PrintDialog();
            if (printDialog.ShowDialog() == true)
            {
                DrawingVisual visual = new DrawingVisual();
                using (DrawingContext dc = visual.RenderOpen())
                {
                    Rect rc = new Rect(0, 0, ((BitmapImage)barcodeImage.Source).PixelWidth, ((BitmapImage)barcodeImage.Source).PixelHeight);
                    dc.DrawImage((BitmapImage)barcodeImage.Source, rc);
                    
                }
                printDialog.PrintVisual(visual, "Barcode");
            }
        }

        private void hriTextBox_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            Clipboard.SetText(hriTextBox.Text);
        }

        private void symbologyComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            linHeightLabel.IsEnabled = false;
            linHeightTextBox.IsEnabled = false;
            sepHtLabel.IsEnabled = false;
            sepHtTextBox.IsEnabled = false;
            segWidthLabel.IsEnabled = false;
            segWidthComboBox.IsEnabled = false;
            qrVersionLabel.IsEnabled = false;
            qrVersionComboBox.IsEnabled = false;
            qrEClevelLabel.IsEnabled = false;
            qrEClevelComboBox.IsEnabled = false;
            dmRowsLabel.IsEnabled = false;
            dmRowsComboBox.IsEnabled = false;

            switch ((GS1Encoder.Symbology)symbologyComboBox.SelectedIndex) {

                case GS1Encoder.Symbology.EAN13:
                case GS1Encoder.Symbology.EAN8:
                case GS1Encoder.Symbology.UPCA:
                case GS1Encoder.Symbology.UPCE:
                    sepHtLabel.IsEnabled = true;
                    sepHtTextBox.IsEnabled = true;
                    break;

                case GS1Encoder.Symbology.GS1_128_CCA:
                case GS1Encoder.Symbology.GS1_128_CCC:
                    linHeightLabel.IsEnabled = true;
                    linHeightTextBox.IsEnabled = true;
                    sepHtLabel.IsEnabled = true;
                    sepHtTextBox.IsEnabled = true;
                    break;

                case GS1Encoder.Symbology.DataBarExpanded:
                    segWidthLabel.IsEnabled = true;
                    segWidthComboBox.IsEnabled = true;
                    goto case GS1Encoder.Symbology.DataBarOmni;

                case GS1Encoder.Symbology.DataBarOmni:
                case GS1Encoder.Symbology.DataBarStacked:
                case GS1Encoder.Symbology.DataBarStackedOmni:
                case GS1Encoder.Symbology.DataBarTruncated:
                case GS1Encoder.Symbology.DataBarLimited:
                    sepHtLabel.IsEnabled = true;
                    sepHtTextBox.IsEnabled = true;
                    break;

                case GS1Encoder.Symbology.DM:
                    dmRowsLabel.IsEnabled = true;
                    dmRowsComboBox.IsEnabled = true;
                    break;

                case GS1Encoder.Symbology.QR:
                    qrVersionLabel.IsEnabled = true;
                    qrVersionComboBox.IsEnabled = true;
                    qrEClevelLabel.IsEnabled = true;
                    qrEClevelComboBox.IsEnabled = true;
                    break;

            }            

        }

        private void infoLabel_MouseDown(object sender, MouseButtonEventArgs e)
        {
            string content = (string)infoLabel.Content;
            content = Regex.Replace(content,".*:\\s+","");
            Clipboard.SetText(content);
        }
    }
}

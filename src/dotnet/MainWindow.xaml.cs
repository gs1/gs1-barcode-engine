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

        private bool _disableEvents = false;

        public MainWindow()
        {
            InitializeComponent();
        }

        public void LoadDataValues()
        {
            _disableEvents = true;
            symbologyComboBox.SelectedIndex = App.gs1Encoder.Sym;
            pixMultTextBox.Text = App.gs1Encoder.PixMult.ToString(CultureInfo.InvariantCulture);
            XundercutTextBox.Text = App.gs1Encoder.Xundercut.ToString(CultureInfo.InvariantCulture);
            YundercutTextBox.Text = App.gs1Encoder.Yundercut.ToString(CultureInfo.InvariantCulture);
            sepHtTextBox.Text = App.gs1Encoder.SepHt.ToString(CultureInfo.InvariantCulture);
            segWidthComboBox.SelectedValue = App.gs1Encoder.DataBarExpandedSegmentsWidth.ToString(CultureInfo.InvariantCulture);
            linHeightTextBox.Text = App.gs1Encoder.GS1_128LinearHeight.ToString(CultureInfo.InvariantCulture);
            qrVersionComboBox.SelectedValue = App.gs1Encoder.QrVersion.ToString(CultureInfo.InvariantCulture);
            qrEClevelComboBox.SelectedValue = App.gs1Encoder.QrEClevel.ToString(CultureInfo.InvariantCulture);
            dmRowsComboBox.SelectedValue = App.gs1Encoder.DmRows.ToString(CultureInfo.InvariantCulture);
            _disableEvents = false;
        }

        private void generateButton_Click(object sender, RoutedEventArgs e)
        {

            clearRender();

            try
            {
                App.gs1Encoder.Sym = symbologyComboBox.SelectedIndex;
                if (dataStrTextBox.Text.Length > 0 && dataStrTextBox.Text[0] == '(')
                {
                    App.gs1Encoder.AIdataStr = dataStrTextBox.Text;
                }
                else
                {
                    App.gs1Encoder.DataStr = dataStrTextBox.Text;
                }
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                LoadDataValues();
                return;
            }      

            try
            { 
                App.gs1Encoder.Encode();

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
                LoadDataValues();
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

            SaveButton.IsEnabled = true;

            LoadDataValues();

        }
        
        private void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog();
            saveFileDialog.Filter = "Bitmap Image (*.bmp)|*.bmp|Portable Network Graphic (*.png)|*.png";
            if (saveFileDialog.ShowDialog() == true)
            {
                BitmapEncoder encoder;
                if (saveFileDialog.FilterIndex == 0)
                {
                    encoder = new BmpBitmapEncoder();                    
                } else
                {
                    encoder = new PngBitmapEncoder();
                }
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

            clearRender();

        }

        private void infoLabel_MouseDown(object sender, MouseButtonEventArgs e)
        {
            string content = (string)infoLabel.Content;
            content = Regex.Replace(content,".*:\\s+","");
            Clipboard.SetText(content);
        }

        private void applicationComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

            double minX = 0, maxX =0, targetX = 0;

            if (_disableEvents) return;

            clearRender();

            if (applicationComboBox.SelectedValue.ToString().Equals("Custom"))
            {
                minXdimensionTextBox.IsEnabled = true;
                maxXdimensionTextBox.IsEnabled = true;
                targetXdimensionTextBox.IsEnabled = true;
            }
            else
            {
                minXdimensionTextBox.IsEnabled = false;
                maxXdimensionTextBox.IsEnabled = false;
                targetXdimensionTextBox.IsEnabled = false;
            }

            DataBarOmni_ComboBoxItem.IsEnabled = false;
            DataBarTruncated_ComboBoxItem.IsEnabled = false;
            DataBarStacked_ComboBoxItem.IsEnabled = false;
            DataBarStackedOmni_ComboBoxItem.IsEnabled = false;
            DataBarLimited_ComboBoxItem.IsEnabled = false;
            DataBarExpanded_ComboBoxItem.IsEnabled = false;
            UPCA_ComboBoxItem.IsEnabled = false;
            UPCE_ComboBoxItem.IsEnabled = false;
            EAN13_ComboBoxItem.IsEnabled = false;
            EAN8_ComboBoxItem.IsEnabled = false;
            GS1_128_CCA_ComboBoxItem.IsEnabled = false;
            GS1_128_CCC_ComboBoxItem.IsEnabled = false;
            QR_ComboBoxItem.IsEnabled = false;
            DM_ComboBoxItem.IsEnabled = false;

            switch (applicationComboBox.SelectedValue.ToString())
            {
                case "Custom":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0; targetX = 0; maxX = 0;
                    break;

                case "Table 1 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    break;

                case "Table 1 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    break;

                case "Table 1 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.375; targetX = 0.625; maxX = 0.990;
                    break;

                case "Table 1 AI (8200)":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.396; targetX = 0.495; maxX = 0.743;
                    break;

                case "Table 1 Digital Link":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.396; targetX = 0.495; maxX = 0.743;
                    break;

                case "Table 2 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.660; maxX = 0.660;
                    break;

                case "Table 2 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.660; maxX = 0.660;
                    break;

                case "Table 2 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.495; maxX = 1.016;
                    break;

                case "Table 2 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.743; targetX = 0.743; maxX = 1.500;
                    break;

                case "Table 3":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.660; maxX = 0.660;
                    break;

                case "Table 4 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    break;

                case "Table 4 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    break;

                case "Table 4 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.250; targetX = 0.495; maxX = 0.495;
                    break;

                case "Table 4 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.380; targetX = 0.380; maxX = 0.495;
                    break;

                case "Table 5 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.495; maxX = 0.940;
                    break;

                case "Table 5 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.743; targetX = 0.743; maxX = 1.500;
                    break;

                case "Table 6 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.170; targetX = 0.330; maxX = 0.660;
                    break;

                case "Table 6 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.170; targetX = 0.200; maxX = 0.660;
                    break;

                case "Table 6 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.170; targetX = 0.495; maxX = 0.495;
                    break;

                case "Table 6 DataMatrix":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.254; targetX = 0.380; maxX = 0.990;
                    break;

                case "Table 7 DPM":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.254; targetX = 0.300; maxX = 0.615;
                    break;

                case "Table 7 DM Ink":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.254; targetX = 0.300; maxX = 0.615;
                    break;

                case "Table 7 DM DPM-A":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.100; targetX = 0.200; maxX = 0.300;
                    break;

                case "Table 7 DM DPM-B":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.200; targetX = 0.300; maxX = 0.495;
                    break;

                case "Table 8 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.660; maxX = 0.660;
                    break;

                case "Table 8 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.660; maxX = 0.660;
                    break;

                case "Table 8 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.495; maxX = 1.016;
                    break;

                case "Table 8 DataMatrix":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.750; targetX = 0.750; maxX = 1.520;
                    break;

                case "Table 9 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.250; targetX = 0.250; maxX = 0.495;
                    break;

                case "Table 9 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.380; targetX = 0.380; maxX = 0.495;
                    break;

                case "Table 10 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    break;

                case "Table 10 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    break;

                case "Table 10 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    break;

                case "Table 10 DataMatrix":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.396; targetX = 0.495; maxX = 0.990;
                    break;

                case "Table 11 GS1 DataBar":
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    break;

                case "Table 11 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.170; targetX = 0.250; maxX = 0.495;
                    break;

                case "Table 11 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.254; targetX = 0.380; maxX = 0.495;
                    break;

                case "Table 12 unit pack":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.380; targetX = 0.380; maxX = 0.990;
                    break;

                case "Table 12 ag. GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.495; maxX = 1.016;
                    break;

                case "Table 12 ag. 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.750; targetX = 0.750; maxX = 1.520;
                    break;

                case "Table 12 log. GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.495; maxX = 0.940;
                    break;

                case "Table 12 log. 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.750; targetX = 0.750; maxX = 1.520;
                    break;

                case "Table 13 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.940; maxX = 0.940;
                    break;

                case "Table 13 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 3.500; maxX = 3.500;
                    break;

            }

            _disableEvents = true;
            if (symbologyComboBox.SelectedItem != null && !((ComboBoxItem)symbologyComboBox.SelectedItem).IsEnabled)
            {
                symbologyComboBox.SelectedIndex = 0;
                while (!((ComboBoxItem)symbologyComboBox.SelectedItem).IsEnabled)
                    symbologyComboBox.SelectedIndex++;
            }            
            minXdimensionTextBox.Text = minX != 0 ? String.Format("{0:0.000}", minX) : "";
            targetXdimensionTextBox.Text = targetX !=0 ? String.Format("{0:0.000}", targetX) : "";            
            maxXdimensionTextBox.Text = maxX != 0 ? String.Format("{0:0.000}", maxX) : "";
            _disableEvents = false;

            recalculateXdimension();
        }

        private void clearRender()
        {
            infoLabel.Content = "";
            errorMessageLabel.Content = "";
            hriTextBox.Text = "";
            SaveButton.IsEnabled = false;
            barcodeImage.Source = null;            
        }

        private void recalculateXdimension() {

            double res, minX, targetX, maxX;

            // Not initialised yet
            if (actualXLabel == null)
                return;

            actualXLabel.Content = "";            

            if (deviceResolutionTextBox.Text == "" &&
                targetXdimensionTextBox.Text == "" &&
                minXdimensionTextBox.Text == "" &&
                maxXdimensionTextBox.Text == "")
            {
                pixMultTextBox.IsEnabled = true;
                generateButton.IsEnabled = true;
            } else
            {
                pixMultTextBox.IsEnabled = false;
                generateButton.IsEnabled = false;
            }

            if (!Double.TryParse(deviceResolutionTextBox.Text, out res))
            {
                if (deviceResolutionTextBox.Text == "")
                    res = 0;
                else
                {
                    actualXLabel.Content = "Device resolution is not a number";
                }
            }

            if (!Double.TryParse(targetXdimensionTextBox.Text, out targetX))
            {
                if (targetXdimensionTextBox.Text == "")
                    targetX = 0;
                else
                {
                    actualXLabel.Content = "Target X-dimension is not a number";
                    return;
                }
            }

            if (!Double.TryParse(minXdimensionTextBox.Text, out minX))
            {
                if (minXdimensionTextBox.Text == "")
                    minX = 0;
                else
                {
                    actualXLabel.Content = "Minimum X-dimension is not a number";
                    return;
                }
            }

            if (!Double.TryParse(maxXdimensionTextBox.Text, out maxX))
            {
                if (maxXdimensionTextBox.Text == "")
                    maxX = 0;
                else
                {
                    actualXLabel.Content = "Maximum X-dimension is not a number";
                    return;
                }
            }

            // Not using user dimensions
            if (targetX == 0)
            {
                return;
            }
            
            generateButton.IsEnabled = false;

            // User X-dimensions can only be calculated when device resolution is known
            if (deviceResolutionTextBox.Text == "")
            {
                actualXLabel.Content = "Device resolution is not set";
                return;
            }

            try
            {
                App.gs1Encoder.DeviceResolution = res;
                App.gs1Encoder.setXdimension(minX, targetX, maxX);
                actualXLabel.Content = String.Format("Achieved X = {0:N3}mm ({1:N0}% target)",
                    App.gs1Encoder.ActualXdimension,
                    App.gs1Encoder.ActualXdimension / App.gs1Encoder.TargetXdimension * 100);
                generateButton.IsEnabled = true;
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
            }

            _disableEvents = true;
            pixMultTextBox.Text = App.gs1Encoder.PixMult.ToString();
            _disableEvents = false;

        }

        private void deviceResolutionTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            recalculateXdimension();
        }

        private void targetXdimensionTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            recalculateXdimension();
        }

        private void minXdimensionTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            recalculateXdimension();
        }

        private void maxXdimensionTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            recalculateXdimension();
        }

        private void dataStrTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
        }

        private void XundercutTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
        }

        private void YundercutTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
        }

        private void linHeightTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
        }

        private void sepHtTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
        }

        private void segWidthComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            try
            {
                int v;
                if (Int32.TryParse((string)segWidthComboBox.SelectedValue, out v)) App.gs1Encoder.DataBarExpandedSegmentsWidth = v;                
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }

        private void qrVersionComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            try
            {
                int v;
                if (Int32.TryParse((string)qrVersionComboBox.SelectedValue, out v)) App.gs1Encoder.QrVersion = v;                
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }

        private void qrEClevelComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            try
            {
                int v;
                if (Int32.TryParse((string)qrEClevelComboBox.SelectedValue, out v)) App.gs1Encoder.QrEClevel = v;
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }

        private void dmRowsComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            try
            {
                int v;                
                if (Int32.TryParse((string)dmRowsComboBox.SelectedValue, out v)) App.gs1Encoder.DmRows = v;
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }

        private void pixMultTextBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (_disableEvents) return;
            try
            {
                int v;
                if (Int32.TryParse(pixMultTextBox.Text, out v)) App.gs1Encoder.PixMult = v;
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }

        private void pixMultTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
        }

        private void XundercutTextBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (_disableEvents) return;
            try
            {
                int v;
                if (Int32.TryParse(XundercutTextBox.Text, out v)) App.gs1Encoder.Xundercut = v;
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }

        private void YundercutTextBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (_disableEvents) return;
            try
            {
                int v;
                if (Int32.TryParse(YundercutTextBox.Text, out v)) App.gs1Encoder.Yundercut = v;
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }

        private void linHeightTextBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (_disableEvents) return;
            try
            {
                int v;
                if (Int32.TryParse(linHeightTextBox.Text, out v)) App.gs1Encoder.GS1_128LinearHeight = v;
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }

        private void sepHtTextBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (_disableEvents) return;
            try
            {
                int v;
                if (Int32.TryParse(sepHtTextBox.Text, out v)) App.gs1Encoder.SepHt = v;
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }
    }
}

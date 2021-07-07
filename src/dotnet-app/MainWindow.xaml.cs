using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using Microsoft.Win32;
using System.IO;
using System.Globalization;
using System.Text.RegularExpressions;
using GS1.Encoders;

namespace GS1.EncodersApp
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {

        // Events do nothing. For use when updating UI data
        private bool _disableEvents = false;

        public string units = "d/mm";

        public MainWindow()
        {
            InitializeComponent();
        }

        private void clearRender()
        {
            infoLabel.Content = "";
            errorMessageLabel.Content = "";
            hriTextBox.Text = "";
            SaveButton.IsEnabled = false;
            barcodeImage.Source = null;
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
            addCheckDigitCheckBox.IsEnabled = false;
            generateButton.Content = "Generate Barcode";

            switch ((GS1Encoder.Symbology)symbologyComboBox.SelectedIndex)
            {

                case GS1Encoder.Symbology.EAN13:
                case GS1Encoder.Symbology.EAN8:
                case GS1Encoder.Symbology.UPCA:
                case GS1Encoder.Symbology.UPCE:
                    sepHtLabel.IsEnabled = true;
                    sepHtTextBox.IsEnabled = true;
                    addCheckDigitCheckBox.IsEnabled = true;
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
                    sepHtLabel.IsEnabled = true;
                    sepHtTextBox.IsEnabled = true;                    
                    break;

                case GS1Encoder.Symbology.DataBarOmni:
                case GS1Encoder.Symbology.DataBarStacked:
                case GS1Encoder.Symbology.DataBarStackedOmni:
                case GS1Encoder.Symbology.DataBarTruncated:
                case GS1Encoder.Symbology.DataBarLimited:
                    sepHtLabel.IsEnabled = true;
                    sepHtTextBox.IsEnabled = true;
                    addCheckDigitCheckBox.IsEnabled = true;
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

                case GS1Encoder.Symbology.NUMSYMS: // Auto-detect from scan data
                    generateButton.Content = "Process Scan Data";
                    dataStrTextBox.Text = "";
                    break;

            }

            clearRender();

        }

        private void lookupXdimensionForApplication()
        {
            double minX = 0, maxX = 0, targetX = 0;
            double minXi = 0, maxXi = 0, targetXi = 0;

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
                    minXi = 0; targetXi = 0; maxXi = 0;
                    break;

                case "Table 1 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    minXi = 0.0104; targetXi = 0.0130; maxXi = 0.0260;
                    break;

                case "Table 1 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    minXi = 0.0104; targetXi = 0.0130; maxXi = 0.0260;
                    break;

                case "Table 1 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.375; targetX = 0.625; maxX = 0.990;
                    minXi = 0.0148; targetXi = 0.0246; maxXi = 0.0390;
                    break;

                case "Table 1 AI (8200)":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.396; targetX = 0.495; maxX = 0.743;
                    minXi = 0.0150; targetXi = 0.0195; maxXi = 0.0293;
                    break;

                case "Table 1 Digital Link":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.396; targetX = 0.495; maxX = 0.743;
                    minXi = 0.0150; targetXi = 0.0195; maxXi = 0.0293;
                    break;

                case "Table 2 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.660; maxX = 0.660;
                    minXi = 0.0195; targetXi = 0.0260; maxXi = 0.0260;
                    break;

                case "Table 2 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.660; maxX = 0.660;
                    minXi = 0.0195; targetXi = 0.0260; maxXi = 0.0260;
                    break;

                case "Table 2 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.495; maxX = 1.016;
                    minXi = 0.0195; targetXi = 0.0195; maxXi = 0.0400;
                    break;

                case "Table 2 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.743; targetX = 0.743; maxX = 1.500;
                    minXi = 0.0292; targetXi = 0.0292; maxXi = 0.0591;
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
                    minXi = 0.0195; targetXi = 0.0260; maxXi = 0.0260;
                    break;

                case "Table 4 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    minXi = 0.0104; targetXi = 0.0130; maxXi = 0.0260;
                    break;

                case "Table 4 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    minXi = 0.0104; targetXi = 0.0130; maxXi = 0.0260;
                    break;

                case "Table 4 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.250; targetX = 0.495; maxX = 0.495;
                    minXi = 0.00984; targetXi = 0.0195; maxXi = 0.0195;
                    break;

                case "Table 4 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.380; targetX = 0.380; maxX = 0.495;
                    minXi = 0.0150; targetXi = 0.0150; maxXi = 0.0195;
                    break;

                case "Table 5 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.495; maxX = 0.940;
                    minXi = 0.0195; targetXi = 0.0195; maxXi = 0.0370;
                    break;

                case "Table 5 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.743; targetX = 0.743; maxX = 1.500;
                    minXi = 0.0292; targetXi = 0.0292; maxXi = 0.0591;
                    break;

                case "Table 6 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.170; targetX = 0.330; maxX = 0.660;
                    minXi = 0.0067; targetXi = 0.0130; maxXi = 0.0260;
                    break;

                case "Table 6 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.170; targetX = 0.200; maxX = 0.660;
                    minXi = 0.0067; targetXi = 0.0080; maxXi = 0.0260;
                    break;

                case "Table 6 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.170; targetX = 0.495; maxX = 0.495;
                    minXi = 0.0067; targetXi = 0.0195; maxXi = 0.0195;
                    break;

                case "Table 6 DataMatrix":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.254; targetX = 0.380; maxX = 0.990;
                    minXi = 0.0100; targetXi = 0.0150; maxXi = 0.0390;
                    break;

                case "Table 7 DPM":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.254; targetX = 0.300; maxX = 0.615;
                    minXi = 0.0100; targetXi = 0.0118; maxXi = 0.0242;
                    break;

                case "Table 7 DM Ink":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.254; targetX = 0.300; maxX = 0.615;
                    minXi = 0.0100; targetXi = 0.0118; maxXi = 0.0242;
                    break;

                case "Table 7 DM DPM-A":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.100; targetX = 0.200; maxX = 0.300;
                    minXi = 0.0039; targetXi = 0.0079; maxXi = 0.0118;
                    break;

                case "Table 7 DM DPM-B":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.200; targetX = 0.300; maxX = 0.495;
                    minXi = 0.0079; targetXi = 0.0118; maxXi = 0.0195;
                    break;

                case "Table 8 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.660; maxX = 0.660;
                    minXi = 0.0195; targetXi = 0.0260; maxXi = 0.0260;
                    break;

                case "Table 8 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.660; maxX = 0.660;
                    minXi = 0.0195; targetXi = 0.0260; maxXi = 0.0260;
                    break;

                case "Table 8 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.495; maxX = 1.016;
                    minXi = 0.0195; targetXi = 0.0195; maxXi = 0.0400;
                    break;

                case "Table 8 DataMatrix":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.750; targetX = 0.750; maxX = 1.520;
                    minXi = 0.0300; targetXi = 0.0300; maxXi = 0.0600;
                    break;

                case "Table 9 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.250; targetX = 0.250; maxX = 0.495;
                    minXi = 0.0098; targetXi = 0.0098; maxXi = 0.0195;
                    break;

                case "Table 9 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.380; targetX = 0.380; maxX = 0.495;
                    minXi = 0.0150; targetXi = 0.0150; maxXi = 0.0195;
                    break;

                case "Table 10 EAN/UPC":
                    UPCA_ComboBoxItem.IsEnabled = true;
                    UPCE_ComboBoxItem.IsEnabled = true;
                    EAN13_ComboBoxItem.IsEnabled = true;
                    EAN8_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    minXi = 0.0104; targetXi = 0.0130; maxXi = 0.0260;
                    break;

                case "Table 10 GS1 DataBar":
                    DataBarOmni_ComboBoxItem.IsEnabled = true;
                    DataBarTruncated_ComboBoxItem.IsEnabled = true;
                    DataBarStacked_ComboBoxItem.IsEnabled = true;
                    DataBarStackedOmni_ComboBoxItem.IsEnabled = true;
                    DataBarLimited_ComboBoxItem.IsEnabled = true;
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    minXi = 0.0104; targetXi = 0.0130; maxXi = 0.0260;
                    break;

                case "Table 10 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    minXi = 0.0104; targetXi = 0.0130; maxXi = 0.0260;
                    break;

                case "Table 10 DataMatrix":
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.396; targetX = 0.495; maxX = 0.990;
                    minXi = 0.0156; targetXi = 0.0195; maxXi = 0.0390;
                    break;

                case "Table 11 GS1 DataBar":
                    DataBarExpanded_ComboBoxItem.IsEnabled = true;
                    minX = 0.264; targetX = 0.330; maxX = 0.660;
                    minXi = 0.0104; targetXi = 0.0130; maxXi = 0.0260;
                    break;

                case "Table 11 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.170; targetX = 0.250; maxX = 0.495;
                    minXi = 0.0067; targetXi = 0.0098; maxXi = 0.0195;
                    break;

                case "Table 11 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.254; targetX = 0.380; maxX = 0.495;
                    minXi = 0.0100; targetXi = 0.0150; maxXi = 0.0195;
                    break;

                case "Table 12 unit pack":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.380; targetX = 0.380; maxX = 0.990;
                    minXi = 0.0150; targetXi = 0.0150; maxXi = 0.0390;
                    break;

                case "Table 12 ag. GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.495; maxX = 1.016;
                    minXi = 0.0195; targetXi = 0.0195; maxXi = 0.400;
                    break;

                case "Table 12 ag. 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.750; targetX = 0.750; maxX = 1.520;
                    minXi = 0.0295; targetXi = 0.0295; maxXi = 0.0600;
                    break;

                case "Table 12 log. GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.495; maxX = 0.940;
                    minXi = 0.0195; targetXi = 0.0195; maxXi = 0.0370;
                    break;

                case "Table 12 log. 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.750; targetX = 0.750; maxX = 1.520;
                    minXi = 0.0295; targetXi = 0.0295; maxXi = 0.0600;
                    break;

                case "Table 13 GS1-128":
                    GS1_128_CCA_ComboBoxItem.IsEnabled = true;
                    GS1_128_CCC_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 0.940; maxX = 0.940;
                    minXi = 0.0195; targetXi = 0.0195; maxXi = 0.0370;
                    break;

                case "Table 13 2D":
                    QR_ComboBoxItem.IsEnabled = true;
                    DM_ComboBoxItem.IsEnabled = true;
                    minX = 0.495; targetX = 3.500; maxX = 3.500;
                    minXi = 0.0195; targetXi = 0.1378; maxXi = 0.1378;
                    break;

            }

            _disableEvents = true;
            if (symbologyComboBox.SelectedItem != null && !((ComboBoxItem)symbologyComboBox.SelectedItem).IsEnabled)
            {
                symbologyComboBox.SelectedIndex = 0;
                while (!((ComboBoxItem)symbologyComboBox.SelectedItem).IsEnabled)
                    symbologyComboBox.SelectedIndex++;
            }

            if (units == "d/mm")
            {
                minXdimensionTextBox.Text = minX != 0 ? String.Format("{0:0.000}", minX) : "";
                targetXdimensionTextBox.Text = targetX != 0 ? String.Format("{0:0.000}", targetX) : "";
                maxXdimensionTextBox.Text = maxX != 0 ? String.Format("{0:0.000}", maxX) : "";
            }
            else  // DPI
            {
                minXdimensionTextBox.Text = minXi != 0 ? String.Format("{0:0.0000}", minXi) : "";
                targetXdimensionTextBox.Text = targetXi != 0 ? String.Format("{0:0.0000}", targetXi) : "";
                maxXdimensionTextBox.Text = maxXi != 0 ? String.Format("{0:0.0000}", maxXi) : "";
            }

            _disableEvents = false;

            recalculateXdimension();
        }

        private void applicationComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            lookupXdimensionForApplication();
        }

        private void processScanData()
        {
            try
            {
                App.gs1Encoder.ScanData = dataStrTextBox.Text;
            }
            catch (GS1EncoderScanDataException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }

            symbologyComboBox.SelectedIndex = App.gs1Encoder.Sym;

            if (App.gs1Encoder.DataStr.StartsWith("#"))
            {
                dataStrTextBox.Text = App.gs1Encoder.AIdataStr;
            } else
            {
                dataStrTextBox.Text = App.gs1Encoder.DataStr;
            }

            generateButton.RaiseEvent(new RoutedEventArgs(System.Windows.Controls.Button.ClickEvent));
        }

        private void generateButton_Click(object sender, RoutedEventArgs e)
        {

            clearRender();

            if ((GS1Encoder.Symbology)symbologyComboBox.SelectedIndex == GS1Encoder.Symbology.NUMSYMS) {
                processScanData();
                return;
            }

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

        private void hriTextBox_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            Clipboard.SetText(hriTextBox.Text);
        }

        private void infoLabel_MouseDown(object sender, MouseButtonEventArgs e)
        {
            string content = (string)infoLabel.Content;
            content = Regex.Replace(content, ".*:\\s+", "");
            Clipboard.SetText(content);
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
                if (units == "d/mm") {
                    actualXLabel.Content = String.Format("Achieved X = {0:N3}mm ({1:N0}% target)",
                        App.gs1Encoder.ActualXdimension,
                        App.gs1Encoder.ActualXdimension / App.gs1Encoder.TargetXdimension * 100);
                }
                else  // DPI
                {
                    actualXLabel.Content = String.Format("Achieved X = {0:N4}in ({1:N0}% target)",
                        App.gs1Encoder.ActualXdimension,
                        App.gs1Encoder.ActualXdimension / App.gs1Encoder.TargetXdimension * 100);
                }
                generateButton.IsEnabled = true;
            }
            catch (GS1EncoderParameterException E)
            {
                string message = E.Message;
                message = Regex.Replace(message, " units", units == "d/mm" ? "mm" : "in");
                message = Regex.Replace(message, "unit", units == "d/mm" ? "mm" : "inch");
                errorMessageLabel.Content = "Error: " + message;
            }

            _disableEvents = true;
            pixMultTextBox.Text = App.gs1Encoder.PixMult.ToString();
            _disableEvents = false;

        }

        private void genericTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
        }

        private void XdimensionTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            genericTextBox_TextChanged(sender, e);
            recalculateXdimension();
        }

        private void genericComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            try
            {
                int v;
                if (Int32.TryParse((string)((ComboBox)sender).SelectedValue, out v))
                {
                    if (sender == segWidthComboBox) App.gs1Encoder.DataBarExpandedSegmentsWidth = v;
                    if (sender == qrVersionComboBox) App.gs1Encoder.QrVersion = v;
                    if (sender == qrEClevelComboBox) App.gs1Encoder.QrEClevel = v;
                    if (sender == dmRowsComboBox) App.gs1Encoder.DmRows = v;
                }
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }

        private void genericTextBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            try
            {
                int v;
                if (Int32.TryParse((string)((TextBox)sender).Text, out v))
                {
                    if (sender == pixMultTextBox) App.gs1Encoder.PixMult = v;
                    if (sender == XundercutTextBox) App.gs1Encoder.Xundercut = v;
                    if (sender == YundercutTextBox) App.gs1Encoder.Yundercut = v;
                    if (sender == linHeightTextBox) App.gs1Encoder.GS1_128LinearHeight = v;
                    if (sender == sepHtTextBox) App.gs1Encoder.SepHt = v;
                }
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                return;
            }
            LoadDataValues();
        }

        private void errorMessageLabel_MouseDown(object sender, MouseButtonEventArgs e)
        {
            string content = (string)errorMessageLabel.Content;
            if (!content.StartsWith("Scan data:"))
                return;
            content = Regex.Replace(content, ".*:\\s+", "");
            Clipboard.SetText(content);
        }

        private void unitsButton_Click(object sender, RoutedEventArgs e)
        {
            if (units == "d/mm")
            {
                units = "DPI";
                targetXdimensionLabel.Content = Regex.Replace(targetXdimensionLabel.Content.ToString(), "\\(mm\\)", "(inch)");
                minXdimensionLabel.Content = Regex.Replace(minXdimensionLabel.Content.ToString(), "\\(mm\\)", "(inch)");
                maxXdimensionLabel.Content = Regex.Replace(maxXdimensionLabel.Content.ToString(), "\\(mm\\)", "(inch)");
            } else  // DPI
            {
                units = "d/mm";
                targetXdimensionLabel.Content = Regex.Replace(targetXdimensionLabel.Content.ToString(), "\\(inch\\)", "(mm)");
                minXdimensionLabel.Content = Regex.Replace(minXdimensionLabel.Content.ToString(), "\\(inch\\)", "(mm)");
                maxXdimensionLabel.Content = Regex.Replace(maxXdimensionLabel.Content.ToString(), "\\(inch\\)", "(mm)");
            }
            unitsButton.Content = units;
            deviceResolutionTextBox.Text = "";
            lookupXdimensionForApplication();
        }

        private void permitUnknownAIsCheckBox_Click(object sender, RoutedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            App.gs1Encoder.PermitUnknownAIs = permitUnknownAIsCheckBox.IsChecked ?? false;
        }

        private void addCheckDigitCheckBox_Click(object sender, RoutedEventArgs e)
        {
            if (_disableEvents) return;
            clearRender();
            App.gs1Encoder.AddCheckDigit = addCheckDigitCheckBox.IsChecked ?? false;
        }
    }
}

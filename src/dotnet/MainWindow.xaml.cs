﻿using System;
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
            symbologyComboBox.SelectedIndex = App.gs1Encoder.GetSym();
            dataStrTextBox.Text = App.gs1Encoder.GetDataStr();
            pixMultTextBox.Text = App.gs1Encoder.GetPixMult().ToString();
            XundercutTextBox.Text = App.gs1Encoder.GetXundercut().ToString();
            YundercutTextBox.Text = App.gs1Encoder.GetYundercut().ToString();
            sepHtTextBox.Text = App.gs1Encoder.GetSepHt().ToString();
            segWidthComboBox.SelectedValue = App.gs1Encoder.GetSegWidth().ToString();
            linHeightTextBox.Text = App.gs1Encoder.GetLinHeight().ToString();
            qrVersionComboBox.SelectedValue = App.gs1Encoder.GetQrVersion().ToString();
            qrEClevelComboBox.SelectedValue = App.gs1Encoder.GetQrEClevel().ToString();
            dmRowsComboBox.SelectedValue = App.gs1Encoder.GetDmRows().ToString();
//            dmColumnsTextBox.Text = App.gs1Encoder.GetDmColumns().ToString();
        }

        private void generateButton_Click(object sender, RoutedEventArgs e)
        {

            try
            {
                App.gs1Encoder.SetSym(symbologyComboBox.SelectedIndex);
                App.gs1Encoder.SetDataStr(dataStrTextBox.Text);
                int v;
                if (Int32.TryParse(pixMultTextBox.Text, out v)) App.gs1Encoder.SetPixMult(v);
                if (Int32.TryParse(XundercutTextBox.Text, out v)) App.gs1Encoder.SetXundercut(v);
                if (Int32.TryParse(YundercutTextBox.Text, out v)) App.gs1Encoder.SetYundercut(v);
                if (Int32.TryParse(sepHtTextBox.Text, out v)) App.gs1Encoder.SetSepHt(v);
                if (Int32.TryParse((string)segWidthComboBox.SelectedValue, out v)) App.gs1Encoder.SetSegWidth(v);
                if (Int32.TryParse(linHeightTextBox.Text, out v)) App.gs1Encoder.SetLinHeight(v);
                if (Int32.TryParse((string)qrVersionComboBox.SelectedValue, out v)) App.gs1Encoder.SetQrVersion(v);
                if (Int32.TryParse((string)qrEClevelComboBox.SelectedValue, out v)) App.gs1Encoder.SetQrEClevel(v);
                if (Int32.TryParse((string)dmRowsComboBox.Text, out v)) App.gs1Encoder.SetDmRows(v);
//                if (Int32.TryParse(dmColumnsTextBox.Text, out v)) App.gs1Encoder.SetDmColumns(v);
            }
            catch (GS1EncoderParameterException E)
            {
                errorMessageLabel.Content = "Error: " + E.Message;
                LoadControls();
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

        private void ComboBoxItem_Selected(object sender, RoutedEventArgs e)
        {

        }
    }
}

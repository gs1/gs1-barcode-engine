using System;
using System.Runtime.InteropServices;

namespace GS1.Encoders
{

    /// <summary>
    /// Wrapper class for accessing the GS1 Barcode Engine native library from C#.
    ///
    /// Copyright (c) 2021 GS1 AISBL.
    ///
    /// Licensed under the Apache License, Version 2.0 (the "License");
    /// you may not use this file except in compliance with the License.
    ///
    /// You may obtain a copy of the License at
    ///
    ///     http://www.apache.org/licenses/LICENSE-2.0
    ///
    /// Unless required by applicable law or agreed to in writing, software
    /// distributed under the License is distributed on an "AS IS" BASIS,
    /// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    /// See the License for the specific language governing permissions and
    /// limitations under the License.
    ///
    ///
    /// This class implements a P/Invoke wrapper around the GS1 Barcode Engine
    /// native C library that presents its functionality in the form of a
    /// typical C# object interface.
    ///
    /// This class is a very lightweight shim around the native library,
    /// therefore the C# interface is described here in terms of the public
    /// API functions of the native library that each method or property
    /// getter/setter invokes.
    ///
    /// The API reference for the native C library is available here:
    ///
    /// https://gs1.github.io/gs1-encoders/
    ///
    /// </summary>
    public class GS1Encoder
    {

        /// <summary>
        /// List of symbology types, mirroring the corresponding list in the
        /// C library.
        ///
        /// See the native library documentation for details:
        ///
        ///   - enum gs1_encoder_symbologies
        ///
        /// </summary>
        public enum Symbology
        {
            /// <summary>None defined</summary>
            NONE = -1,
            /// <summary>GS1 DataBar Omnidirectional</summary>
            DataBarOmni,
            /// <summary>GS1 DataBar Truncated</summary>
            DataBarTruncated,
            /// <summary>GS1 DataBar Stacked</summary>
            DataBarStacked,
            /// <summary>GS1 DataBar Stacked Omnidirectional</summary>
            DataBarStackedOmni,
            /// <summary>GS1 DataBar Limited</summary>
            DataBarLimited,
            /// <summary>GS1 DataBar Expanded (Stacked)</summary>
            DataBarExpanded,
            /// <summary>UPC-A</summary>
            UPCA,
            /// <summary>UPC-E</summary>
            UPCE,
            /// <summary>EAN-13</summary>
            EAN13,
            /// <summary>EAN-8</summary>
            EAN8,
            /// <summary>GS1-128 with CC-A or CC-B</summary>
            GS1_128_CCA,
            /// <summary>GS1-128 with CC</summary>
            GS1_128_CCC,
            /// <summary>(GS1) QR Code</summary>
            QR,
            /// <summary>(GS1) Data Matrix</summary>
            DM,
            /// <summary>Value is the number of symbologies</summary>
            NUMSYMS,
        };

        /// <summary>
        /// List of output format, mirroring the corresponding list in the
        /// C library.
        ///
        /// See the native library documentation for details:
        ///
        ///   - enum gs1_encoder_formats
        ///
        /// </summary>
        public enum Formats
        {
            /// <summary>Bitmap</summary>
            BMP = 0,
            /// <summary>TIFF</summary>
            TIF = 1,
            /// <summary>Headerless TIFF</summary>
            RAW = 2,
        };

        /// <summary>
        /// List of supported Data Matrix rows sizes, mirroring the
        /// corresponding list in the C library.
        ///
        /// See the native library documentation for details:
        ///
        ///   - enum gs1_encoder_dmRows
        ///
        /// </summary>
        public enum DMrows
        {
            /// <summary>Automatic, based on barcode data.</summary>
            Automatic = 0,
            /// <summary>8x18, 8x32</summary>
            Rows8 = 8,
            /// <summary>10x10</summary>
            Rows10 = 10,
            /// <summary>12x12, 12x26, 12x36</summary>
            Rows12 = 12,
            /// <summary>14x14</summary>
            Rows14 = 14,
            /// <summary>16x16, 16x36, 16x48</summary>
            Rows16 = 16,
            /// <summary>18x18</summary>
            Rows18 = 18,
            /// <summary>20x20</summary>
            Rows20 = 20,
            /// <summary>22x22</summary>
            Rows22 = 22,
            /// <summary>24x24</summary>
            Rows24 = 24,
            /// <summary>26x26</summary>
            Rows26 = 26,
            /// <summary>32x32</summary>
            Rows32 = 32,
            /// <summary>36x36</summary>
            Rows36 = 36,
            /// <summary>40x40</summary>
            Rows40 = 40,
            /// <summary>44x44</summary>
            Rows44 = 44,
            /// <summary>48x48</summary>
            Rows48 = 48,
            /// <summary>52x52</summary>
            Rows52 = 52,
            /// <summary>64x64</summary>
            Rows64 = 64,
            /// <summary>72x72</summary>
            Rows72 = 72,
            /// <summary>80x80</summary>
            Rows80 = 80,
            /// <summary>88x88</summary>
            Rows88 = 88,
            /// <summary>96x96</summary>
            Rows96 = 96,
            /// <summary>104x104</summary>
            Rows104 = 104,
            /// <summary>120x120</summary>
            Rows120 = 120,
            /// <summary>132x132</summary>
            Rows132 = 132,
            /// <summary>144x144</summary>
            Rows144 = 144,
        };

        /// <summary>
        /// List of supported Data Matrix column sizes, mirroring the
        /// corresponding list in the C library.
        ///
        /// See the native library documentation for details:
        ///
        ///   - enum gs1_encoder_dmColumns
        ///
        /// </summary>
        public enum DMcolumns
        {
            /// <summary>Automatic, based on barcode data.</summary>
            Automatic = 0,
            /// <summary>10x10</summary>
            Columns10 = 10,
            /// <summary>12x12</summary>
            Columns12 = 12,
            /// <summary>14x14</summary>
            Columns14 = 14,
            /// <summary>16x16</summary>
            Columns16 = 16,
            /// <summary>18x18</summary>
            Columns18 = 18,
            /// <summary>20x20</summary>
            Columns20 = 20,
            /// <summary>22x22</summary>
            Columns22 = 22,
            /// <summary>24x24</summary>
            Columns24 = 24,
            /// <summary>26x26</summary>
            Columns26 = 26,
            /// <summary>32x32</summary>
            Columns32 = 32,
            /// <summary>36x36</summary>
            Columns36 = 36,
            /// <summary>40x40</summary>
            Columns40 = 40,
            /// <summary>44x44</summary>
            Columns44 = 44,
            /// <summary>48x48</summary>
            Columns48 = 48,
            /// <summary>52x52</summary>
            Columns52 = 52,
            /// <summary>64x64</summary>
            Columns64 = 64,
            /// <summary>72x72</summary>
            Columns72 = 72,
            /// <summary>80x80</summary>
            Columns80 = 80,
            /// <summary>88x8</summary>
            Columns88 = 88,
            /// <summary>96x96</summary>
            Columns96 = 96,
            /// <summary>104x104</summary>
            Columns104 = 104,
            /// <summary>120x120</summary>
            Columns120 = 120,
            /// <summary>132x132</summary>
            Columns132 = 132,
            /// <summary>144x144</summary>
            Columns144 = 144,
        };

        /// <summary>
        /// List of supported QR Code error correction levels, mirroring
        /// the corresponding list in the C library.
        ///
        /// See the native library documentation for details:
        ///
        ///   - enum gs1_encoder_qrEClevel
        ///
        /// </summary>
        public enum QReclevel
        {
            /// <summary>Low error correction (7% damage recovery)</summary>
            L = 1,
            /// <summary>Medium error correction (15% damage recovery)</summary>
            M,
            /// <summary>Quartile error correction (25% damage recovery)</summary>
            Q,
            /// <summary>High error correction (30% damage recovery)</summary>
            H,
        };

        /// <summary>
        /// List of supported QR Code symbol versions, mirroring the
        /// corresponding list in the C library.
        ///
        /// See the native library documentation for details:
        ///
        ///   - enum gs1_encoder_qrVersion
        ///
        /// </summary>
        public enum QRversion
        {
            /// <summary>Automatic, based on barcode data</summary>
            Automatic = 0,
            /// <summary>Version 1, 21x21</summary>
            Version1,
            /// <summary>Version 2, 25x25</summary>
            Version2,
            /// <summary>Version 3, 29x29</summary>
            Version3,
            /// <summary>Version 4, 33x33</summary>
            Version4,
            /// <summary>Version 5, 37x37</summary>
            Version5,
            /// <summary>Version 6, 41x41</summary>
            Version6,
            /// <summary>Version 7, 45x45</summary>
            Version7,
            /// <summary>Version 8, 49x49</summary>
            Version8,
            /// <summary>Version 9, 53x53</summary>
            Version9,
            /// <summary>Version 10, 57x57</summary>
            Version10,
            /// <summary>Version 11, 61x61</summary>
            Version11,
            /// <summary>Version 12, 65x65</summary>
            Version12,
            /// <summary>Version 13, 69x69</summary>
            Version13,
            /// <summary>Version 14, 73x73</summary>
            Version14,
            /// <summary>Version 15, 77x77</summary>
            Version15,
            /// <summary>Version 16, 81x81</summary>
            Version16,
            /// <summary>Version 17, 85x85</summary>
            Version17,
            /// <summary>Version 18, 89x89</summary>
            Version18,
            /// <summary>Version 19, 93x93</summary>
            Version19,
            /// <summary>Version 20, 97x97</summary>
            Version20,
            /// <summary>Version 21, 101x101</summary>
            Version21,
            /// <summary>Version 22, 105x105</summary>
            Version22,
            /// <summary>Version 23, 109x109</summary>
            Version23,
            /// <summary>Version 24, 113x113</summary>
            Version24,
            /// <summary>Version 25, 117x117</summary>
            Version25,
            /// <summary>Version 26, 121x121</summary>
            Version26,
            /// <summary>Version 27, 125x125</summary>
            Version27,
            /// <summary>Version 28, 129x129</summary>
            Version28,
            /// <summary>Version 29, 133x133</summary>
            Version29,
            /// <summary>Version 30, 137x137</summary>
            Version30,
            /// <summary>Version 31, 141x141</summary>
            Version31,
            /// <summary>Version 32, 145x145</summary>
            Version32,
            /// <summary>Version 33, 149x149</summary>
            Version33,
            /// <summary>Version 34, 153x153</summary>
            Version34,
            /// <summary>Version 35, 157x157</summary>
            Version35,
            /// <summary>Version 36, 161x161</summary>
            Version36,
            /// <summary>Version 37, 165x165</summary>
            Version37,
            /// <summary>Version 38, 169x169</summary>
            Version38,
            /// <summary>Version 39, 173x173</summary>
            Version39,
            /// <summary>Version 49, 177x177</summary>
            Version40,
        };

        /// <summary>
        /// The expected name of the GS1 Barcode Engine dynamic-link library
        /// </summary>
        private const String gs1_dll = "gs1encoders.dll";

        /// <summary>
        /// An opaque pointer used by the native code to represent an
        /// "instance" of the library. It is hidden behind the object
        /// interface that is provided to users of this wrapper.
        ///
        /// See the native library documentation for details:
        ///
        ///   - typedef struct gs1_encoder
        ///
        /// </summary>
        private readonly IntPtr ctx;

        /*
         *  Functions imported from the native GS1 Barcode Engine dynamic-link library
         *
         */
        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_init", CallingConvention = CallingConvention.Cdecl)]
        private static extern System.IntPtr gs1_encoder_init(IntPtr mem);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getVersion", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getVersion();

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getErrMsg", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getErrMsg(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getSym", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getSym(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setSym", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setSym(IntPtr ctx, int sym);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getPixMult", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getPixMult(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setPixMult", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setPixMult(IntPtr ctx, int pixMult);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getDeviceResolution", CallingConvention = CallingConvention.Cdecl)]
        private static extern double gs1_encoder_getDeviceResolution(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setDeviceResolution", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setDeviceResolution(IntPtr ctx, double resolution);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setXdimension", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setXdimension(IntPtr ctx, double min, double target, double max);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getTargetXdimension", CallingConvention = CallingConvention.Cdecl)]
        private static extern double gs1_encoder_getTargetXdimension(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getMinXdimension", CallingConvention = CallingConvention.Cdecl)]
        private static extern double gs1_encoder_getMinXdimension(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getMaxXdimension", CallingConvention = CallingConvention.Cdecl)]
        private static extern double gs1_encoder_getMaxXdimension(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getActualXdimension", CallingConvention = CallingConvention.Cdecl)]
        private static extern double gs1_encoder_getActualXdimension(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getXundercut", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getXundercut(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setXundercut", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setXundercut(IntPtr ctx, int Xundercut);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getYundercut", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getYundercut(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setYundercut", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setYundercut(IntPtr ctx, int Yundercut);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getSepHt", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getSepHt(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setSepHt", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setSepHt(IntPtr ctx, int sepHt);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getDataBarExpandedSegmentsWidth", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getDataBarExpandedSegmentsWidth(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setDataBarExpandedSegmentsWidth", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setDataBarExpandedSegmentsWidth(IntPtr ctx, int segWidth);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getGS1_128LinearHeight", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getGS1_128LinearHeight(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setGS1_128LinearHeight", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setGS1_128LinearHeight(IntPtr ctx, int linHeight);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getDmRows", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getDmRows(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setDmRows", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setDmRows(IntPtr ctx, int rows);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getDmColumns", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getDmColumns(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setDmColumns", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setDmColumns(IntPtr ctx, int columns);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getQrVersion", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getQrVersion(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setQrVersion", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setQrVersion(IntPtr ctx, int columns);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getQrEClevel", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getQrEClevel(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setQrEClevel", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setQrEClevel(IntPtr ctx, int columns);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getAddCheckDigit", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_getAddCheckDigit(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setAddCheckDigit", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setAddCheckDigit(IntPtr ctx, [MarshalAs(UnmanagedType.U1)] bool addCheckDigit);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getPermitUnknownAIs", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_getPermitUnknownAIs(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setPermitUnknownAIs", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setPermitUnknownAIs(IntPtr ctx, [MarshalAs(UnmanagedType.U1)] bool permitUnknownAIs);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getFileInputFlag", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_getFileInputFlag(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setFileInputFlag", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setFileInputFlag(IntPtr ctx, [MarshalAs(UnmanagedType.U1)] bool fileInputFlag);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getDataStr", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getDataStr(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setDataStr", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setDataStr(IntPtr ctx, string dataStr);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setAIdataStr", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setAIdataStr(IntPtr ctx, string aiData);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getAIdataStr", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getAIdataStr(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getScanData", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getScanData(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setScanData", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setScanData(IntPtr ctx, string scanData);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getHRI", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getHRI(IntPtr ctx, ref IntPtr hri);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getDataFile", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getDataFile(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setDataFile", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setDataFile(IntPtr ctx, string dataFile);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getOutFile", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getOutFile(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setOutFile", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setOutFile(IntPtr ctx, string outFile);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getFormat", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getFormat(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setFormat", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setFormat(IntPtr ctx, int format);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_encode", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_encode(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getBuffer", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getBuffer(IntPtr ctx, ref IntPtr buf);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getBufferWidth", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getBufferWidth(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getBufferHeight", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getBufferHeight(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getBufferStrings", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getBufferStrings(IntPtr ctx, ref IntPtr strings);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_free", CallingConvention = CallingConvention.Cdecl)]
        private static extern void gs1_encoder_free(IntPtr ctx);


        /*
         *  Methods to provide a wrapper around the functional interface imported from the native library
         *
         */

        // This C# wrapper library throws an excpetion containing the error message whenever
        // an error is returned by the native library. Therefore direct access to the native
        // error message is not necessary.
        private string ErrMsg
        {
            get
            {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getErrMsg(ctx));
            }
        }

        /// <summary>
        /// Constructor that creates an object wrapping an "instance" of the library managed by the native code.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_init()
        ///
        /// </summary>
        public GS1Encoder()
        {
            ctx = gs1_encoder_init(IntPtr.Zero);
            if (ctx == IntPtr.Zero)
                throw new GS1EncoderGeneralException("Failed to initalise GS1 Barcode Engine");
        }

        /// <summary>
        /// Returns the version of the native library.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getVersion()
        ///
        /// </summary>
        public string Version
        {
            get
            {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getVersion());
            }
        }

        /// <summary>
        /// Get/set the symbology type.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getSym()
        ///   - gs1_encoder_setSym()
        ///
        /// </summary>
        public int Sym
        {
            get
            {
                return gs1_encoder_getSym(ctx);
            }
            set
            {
                if (!gs1_encoder_setSym(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the device dots per module.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getPixMult()
        ///   - gs1_encoder_setPixMult()
        ///
        /// </summary>
        public int PixMult
        {
            get
            {
                return gs1_encoder_getPixMult(ctx);
            }
            set
            {
                if (!gs1_encoder_setPixMult(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the device resolution.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getDeviceResolution()
        ///   - gs1_encoder_setDeviceResolution()
        ///
        /// </summary>
        public double DeviceResolution
        {
            get
            {
                return gs1_encoder_getDeviceResolution(ctx);
            }
            set
            {
                if (!gs1_encoder_setDeviceResolution(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get the current target X-dimension.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getTargetXdimension()
        ///
        /// </summary>
        public double TargetXdimension
        {
            get
            {
                return gs1_encoder_getTargetXdimension(ctx);
            }
        }

        /// <summary>
        /// Get the current minimum permissible X-dimension.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getMinXdimension()
        ///
        /// </summary>
        public double MinXdimension
        {
            get
            {
                return gs1_encoder_getMinXdimension(ctx);
            }
        }

        /// <summary>
        /// Get the current minimum permissible X-dimension.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getMaxXdimension()
        ///
        /// </summary>
        public double MaxXdimension
        {
            get
            {
                return gs1_encoder_getMaxXdimension(ctx);
            }
        }

        /// <summary>
        /// Get the actual X-dimension that can be achieved at the current device resolution.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getActualXdimension()
        ///
        /// </summary>
        public double ActualXdimension
        {
            get
            {
                return gs1_encoder_getActualXdimension(ctx);
            }
        }

        /// <summary>
        /// Set the constraints for the X-dimension width.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_setXdimension()
        ///
        /// </summary>
        public void setXdimension(double min, double target, double max)
        {
            if (!gs1_encoder_setXdimension(ctx, min, target, max))
                throw new GS1EncoderParameterException(ErrMsg);
        }

        /// <summary>
        /// Get/set the "add check digit" mode for EAN/UPC and GS1 DataBar symbols.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getAddCheckDigit()
        ///   - gs1_encoder_setAddCheckDigit()
        ///
        /// </summary>
        public bool AddCheckDigit
        {
            get
            {
                return gs1_encoder_getAddCheckDigit(ctx);
            }
            set
            {
                if (!gs1_encoder_setAddCheckDigit(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the "permit unknown AIs" mode.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getPermitUnknownAIs()
        ///   - gs1_encoder_setPermitUnknownAIs()
        ///
        /// </summary>
        public bool PermitUnknownAIs
        {
            get
            {
                return gs1_encoder_getPermitUnknownAIs(ctx);
            }
            set
            {
                if (!gs1_encoder_setPermitUnknownAIs(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the X undercut pixels.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getXundercut()
        ///   - gs1_encoder_setXundercut()
        ///
        /// </summary>
        public int Xundercut
        {
            get
            {
                return gs1_encoder_getXundercut(ctx);
            }
            set
            {
                if (!gs1_encoder_setXundercut(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the Y undercut pixels.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getXundercut()
        ///   - gs1_encoder_setXundercut()
        ///
        /// </summary>
        public int Yundercut
        {
            get
            {
                return gs1_encoder_getYundercut(ctx);
            }
            set
            {
                if (!gs1_encoder_setYundercut(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the current separator height between linear and 2D components.
        ///
        /// See the native library documentation for details.
        ///
        ///   - gs1_encoder_getSepHt()
        ///   - gs1_encoder_setSepHt()
        ///
        /// </summary>
        public int SepHt
        {
            get
            {
                return gs1_encoder_getSepHt(ctx);
            }
            set
            {
                if (!gs1_encoder_setSepHt(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the current number of segments per row for GS1 DataBar Expanded Stacked symbols.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getDataBarExpandedSegmentsWidth()
        ///   - gs1_encoder_setDataBarExpandedSegmentsWidth()
        ///
        /// </summary>
        public int DataBarExpandedSegmentsWidth
        {
            get {
                return gs1_encoder_getDataBarExpandedSegmentsWidth(ctx);
            }
            set {
                if (!gs1_encoder_setDataBarExpandedSegmentsWidth(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the height of GS1-128 linear symbols in modules.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getGS1_128LinearHeight()
        ///   - gs1_encoder_setGS1_128LinearHeight()
        ///
        /// </summary>
        public int GS1_128LinearHeight
        {
            get
            {
                return gs1_encoder_getGS1_128LinearHeight(ctx);
            }
            set
            {
                if (!gs1_encoder_setGS1_128LinearHeight(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the current fixed version number for QR Code symbols.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getQrVersion()
        ///   - gs1_encoder_setQrVersion()
        ///
        /// </summary>
        public int QrVersion
        {
            get
            {
                return gs1_encoder_getQrVersion(ctx);
            }
            set
            {
                if (!gs1_encoder_setQrVersion(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the current error correction level for QR Code symbols.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getQrEClevel()
        ///   - gs1_encoder_setQrEClevel()
        ///
        /// </summary>
        public int QrEClevel
        {
            get
            {
                return gs1_encoder_getQrEClevel(ctx);
            }
            set {
                if (!gs1_encoder_setQrEClevel(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set a fixed number of rows for Data Matrix symbols.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getDmRows()
        ///   - gs1_encoder_setDmRows()
        ///
        /// </summary>
        public int DmRows
        {
            get
            {
                return gs1_encoder_getDmRows(ctx);
            }
            set
            {
                if (!gs1_encoder_setDmRows(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set a fixed number of columns for Data Matrix symbols.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getDmColumns()
        ///   - gs1_encoder_setDmColumns()
        ///
        /// </summary>
        public int DmColumns
        {
            get
            {
                return gs1_encoder_getDmColumns(ctx);
            }
            set
            {
                if (!gs1_encoder_setDmColumns(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set whether a file or buffer us used for the barcode data input.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getFileInputFlag()
        ///   - gs1_encoder_setFileInputFlag()
        ///
        /// </summary>
        public bool FileInputFlag
        {
            get
            {
                return gs1_encoder_getFileInputFlag(ctx);
            }
            set
            {
                if (!gs1_encoder_setFileInputFlag(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the raw barcode data input buffer.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getDataStr()
        ///   - gs1_encoder_setDataStr()
        ///
        /// </summary>
        public string DataStr
        {
            get
            {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getDataStr(ctx));
            }
            set
            {
                if (!gs1_encoder_setDataStr(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the barcode data input buffer using GS1 AI syntax.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getAIdataStr()
        ///   - gs1_encoder_setAIdataStr()
        ///
        /// </summary>
        public string AIdataStr
        {
            get {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getAIdataStr(ctx));
            }
            set
            {
                if (!gs1_encoder_setAIdataStr(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the barcode data input buffer using barcode scan data format.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getScanData()
        ///   - gs1_encoder_setScanData()
        ///
        /// </summary>
        public string ScanData
        {
            get
            {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getScanData(ctx));
            }
            set
            {
                if (!gs1_encoder_setScanData(ctx, value))
                    throw new GS1EncoderScanDataException(ErrMsg);
            }
        }

        /// <summary>
        /// Get the Human-Readable Interpretation ("HRI") text for the current data input buffer as an array of strings.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getHRI()
        ///
        /// </summary>
        public string[] HRI
        {
            get
            {
                IntPtr p = IntPtr.Zero;
                int numAIs = gs1_encoder_getHRI(ctx, ref p);
                IntPtr[] pAI = new IntPtr[numAIs];
                Marshal.Copy(p, pAI, 0, numAIs);
                string[] hri = new string[numAIs];
                for (int i = 0; i < numAIs; i++)
                {
                    hri[i] = Marshal.PtrToStringAnsi(pAI[i]);
                }
                return hri;
            }
        }

        /// <summary>
        /// Get/set the filename for a file containing the barcode data when file input is selected.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getDataFile()
        ///   - gs1_encoder_setDataFile()
        ///
        /// </summary>
        public string DataFile
        {
            get
            {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getDataFile(ctx));
            }
            set
            {
                if (!gs1_encoder_setDataFile(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the current output format type.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getFormat()
        ///   - gs1_encoder_setFormat()
        ///
        /// </summary>
        public int Format
        {
            get {
                return gs1_encoder_getFormat(ctx);
            }
            set
            {
                if (!gs1_encoder_setFormat(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Get/set the current output filename.
        ///
        /// See the native library documentation for details:
        ///
        ///   -  gs1_encoder_getOutFile()
        ///   -  gs1_encoder_setOutFile()
        ///
        /// </summary>
        public string OutFile
        {
            get
            {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getOutFile(ctx));
            }
            set
            {
                if (!gs1_encoder_setOutFile(ctx, value))
                   throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        /// <summary>
        /// Generate a barcode symbol representing the given input data.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_encode()
        ///
        /// </summary>
        public void Encode()
        {
            if (!gs1_encoder_encode(ctx))
                throw new GS1EncoderEncodeException(ErrMsg);
        }

        /// <summary>
        /// Get the output buffer.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getBuffer()
        ///
        /// </summary>
        public byte[] GetBuffer()
        {
            IntPtr buf = new IntPtr();
            int size = gs1_encoder_getBuffer(ctx, ref buf);

            if (size == 0)
                return new byte[0];

            byte[] data = new byte[size];
            Marshal.Copy(buf, data, 0, size);
            return data;
        }

        /// <summary>
        /// Get the number of columns in the output buffer image.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getBufferWidth()
        ///
        /// </summary>
        public int BufferWidth
        {
            get
            {
                return gs1_encoder_getBufferWidth(ctx);
            }

        }

        /// <summary>
        /// Get the number of rows in the output buffer image.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getBufferHeight()
        ///
        /// </summary>
        public int BufferHeight
        {
            get
            {
                return gs1_encoder_getBufferHeight(ctx);
            }

        }

        /// <summary>
        /// Return the output buffer represented as an array of strings.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_getBufferStrings()
        ///
        /// </summary>
        public string[] BufferStrings
        {
            get
            {
                IntPtr p = IntPtr.Zero;
                int rows = gs1_encoder_getBufferStrings(ctx, ref p);
                IntPtr[] pRows = new IntPtr[rows];
                Marshal.Copy(p, pRows, 0, rows);
                string[] bufStrings = new string[rows];
                for (int i = 0; i < rows; i++)
                {
                    bufStrings[i] = Marshal.PtrToStringAnsi(pRows[i]);
                }
                return bufStrings;
            }
        }

        /// <summary>
        /// Destructor that will release the resources allocated by the native library.
        ///
        /// See the native library documentation for details:
        ///
        ///   - gs1_encoder_free()
        ///
        /// </summary>
        ~GS1Encoder()
        {
            gs1_encoder_free(ctx);
        }

    }

    /// <summary>
    /// A custom exception class that is thrown to indicate a general problem
    /// initialising the library, such as when the GS1 Barcode Engine
    /// dynamic-link library isn't accessible.
    /// </summary>
    public class GS1EncoderGeneralException : Exception
    {
        /// <summary>
        /// Error constructor.
        /// </summary>
        /// <param name="message">
        /// Description of the error.
        /// </param>
        public GS1EncoderGeneralException(string message)
           : base(message)
        {
        }
    }

    /// <summary>
    /// A custom exception class that is thrown to indicate a problem with barcode
    /// options that are being set.
    /// </summary>
    public class GS1EncoderParameterException : Exception
    {
        /// <summary>
        /// Error constructor.
        /// </summary>
        /// <param name="message">
        /// Description of the error.
        /// </param>
        public GS1EncoderParameterException(string message)
           : base(message)
        {
        }
    }

    /// <summary>
    /// A custom exception class that is thrown to indicate a problem generating
    /// a barcode symbol.
    /// </summary>
    public class GS1EncoderEncodeException : Exception
    {
        /// <summary>
        /// Error constructor.
        /// </summary>
        /// <param name="message">
        /// Description of the error.
        /// </param>
        public GS1EncoderEncodeException(string message)
           : base(message)
        {
        }
    }

    /// <summary>
    /// A custom exception class that is thrown to indicate an error processing
    /// barcode scan data.
    /// </summary>
    public class GS1EncoderScanDataException : Exception
    {
        /// <summary>
        /// Error constructor.
        /// </summary>
        /// <param name="message">
        /// Description of the error.
        /// </param>
        public GS1EncoderScanDataException(string message)
           : base(message)
        {
        }
    }

}

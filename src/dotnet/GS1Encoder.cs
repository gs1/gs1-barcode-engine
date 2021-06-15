using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace gs1encoders_dotnet
{

    public class GS1Encoder
    {

        public enum Symbology
        {
            NONE = -1,
            DataBarOmni,
            DataBarTruncated,
            DataBarStacked,
            DataBarStackedOmni,
            DataBarLimited,
            DataBarExpanded,
            UPCA,
            UPCE,
            EAN13,
            EAN8,
            GS1_128_CCA,
            GS1_128_CCC,
            QR,
            DM,
            NUMSYMS,
        };

        public enum Formats
        {
            BMP = 0,
            TIF = 1,
            RAW = 2,
        };

        public enum DMrows
        {
            Automatic = 0,              ///< Automatic, based on barcode data
            Rows8 = 8,                ///< 8x18, 8x32
            Rows10 = 10,              ///< 10x10
            Rows12 = 12,              ///< 12x12, 12x26, 12x36
            Rows14 = 14,              ///< 14x14
            Rows16 = 16,              ///< 16x16, 16x36, 16x48
            Rows18 = 18,              ///< 18x18
            Rows20 = 20,              ///< 20x20
            Rows22 = 22,              ///< 22x22
            Rows24 = 24,              ///< 24x24
            Rows26 = 26,              ///< 26x26
            Rows32 = 32,              ///< 32x32
            Rows36 = 36,              ///< 36x36
            Rows40 = 40,              ///< 40x40
            Rows44 = 44,              ///< 44x44
            Rows48 = 48,              ///< 48x48
            Rows52 = 52,              ///< 52x52
            Rows64 = 64,              ///< 64x64
            Rows72 = 72,              ///< 72x72
            Rows80 = 80,              ///< 80x80
            Rows88 = 88,              ///< 88x88
            Rows96 = 96,              ///< 96x96
            Rows104 = 104,            ///< 104x104
            Rows120 = 120,            ///< 120x120
            Rows132 = 132,            ///< 132x132
            Rows144 = 144,            ///< 144x144
        };

        public enum DMcolumns
        {
            Automatic = 0,              ///< Automatic, based on barcode data
            Columns10 = 10,           ///< 10x10
            Columns12 = 12,           ///< 12x12
            Columns14 = 14,           ///< 14x14
            Columns16 = 16,           ///< 16x16, 16x36, 16x48
            Columns18 = 18,           ///< 18x18
            Columns20 = 20,           ///< 20x20
            Columns22 = 22,           ///< 22x22
            Columns24 = 24,           ///< 24x24
            Columns26 = 26,           ///< 26x26, 12x26
            Columns32 = 32,           ///< 32x32
            Columns36 = 36,           ///< 36x36, 12x36, 16x36
            Columns40 = 40,           ///< 40x40
            Columns44 = 44,           ///< 44x44
            Columns48 = 48,           ///< 48x48, 16x48
            Columns52 = 52,           ///< 52x52
            Columns64 = 64,           ///< 64x64
            Columns72 = 72,           ///< 72x72
            Columns80 = 80,           ///< 80x80
            Columns88 = 88,           ///< 88x88
            Columns96 = 96,           ///< 96x96
            Columns104 = 104,         ///< 104x104
            Columns120 = 120,         ///< 120x120
            Columns132 = 132,         ///< 132x132
            Columns144 = 144,         ///< 144x144
        };

        public enum QReclevel
        {
            L = 1,     ///< Low error correction (7% damage recovery)
            M,         ///< Medium error correction (15% damage recovery)
            Q,         ///< Quartile error correction (25% damage recovery)
            H,         ///< High error correction (30% damage recovery)
        };

        public enum QRversion
        {
            Automatic = 0,     ///< Automatic, based on barcode data
            Version1,                 ///< Version 1, 21x21
            Version2,                 ///< Version 2, 25x25
            Version3,                 ///< Version 3, 29x29
            Version4,                 ///< Version 4, 33x33
            Version5,                 ///< Version 5, 37x37
            Version6,                 ///< Version 6, 41x41
            Version7,                 ///< Version 7, 45x45
            Version8,                 ///< Version 8, 49x49
            Version9,                 ///< Version 9, 53x53
            Version10,                ///< Version 10, 57x57
            Version11,                ///< Version 11, 61x61
            Version12,                ///< Version 12, 65x65
            Version13,                ///< Version 13, 69x69
            Version14,                ///< Version 14, 73x73
            Version15,                ///< Version 15, 77x77
            Version16,                ///< Version 16, 81x81
            Version17,                ///< Version 17, 85x85
            Version18,                ///< Version 18, 89x89
            Version19,                ///< Version 19, 93x93
            Version20,                ///< Version 20, 97x97
            Version21,                ///< Version 21, 101x101
            Version22,                ///< Version 22, 105x105
            Version23,                ///< Version 23, 109x109
            Version24,                ///< Version 24, 113x113
            Version25,                ///< Version 25, 117x117
            Version26,                ///< Version 26, 121x121
            Version27,                ///< Version 27, 125x125
            Version28,                ///< Version 28, 129x129
            Version29,                ///< Version 29, 133x133
            Version30,                ///< Version 30, 137x137
            Version31,                ///< Version 31, 141x141
            Version32,                ///< Version 32, 145x145
            Version33,                ///< Version 33, 149x149
            Version34,                ///< Version 34, 153x153
            Version35,                ///< Version 35, 157x157
            Version36,                ///< Version 36, 161x161
            Version37,                ///< Version 37, 165x165
            Version38,                ///< Version 38, 169x169
            Version39,                ///< Version 39, 173x173
            Version40,                ///< Version 49, 177x177
        };

        private readonly IntPtr ctx;
        private const String gs1_dll = "gs1encoders.dll";

        /*
         *  Functions imported from gs1encoders.dll
         *
         */
        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_init", CallingConvention = CallingConvention.Cdecl)]
        private static extern System.IntPtr gs1_encoder_init(IntPtr mem);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getVersion", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
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
        private static extern bool gs1_encoder_setAddCheckDigit(IntPtr ctx, bool addCheckDigit);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getFileInputFlag", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_getFileInputFlag(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setFileInputFlag", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setFileInputFlag(IntPtr ctx, bool fileInputFlag);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getDataStr", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getDataStr(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setDataStr", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setDataStr(IntPtr ctx, string dataStr);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setGS1dataStr", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setGS1dataStr(IntPtr ctx, string gs1data);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getGS1dataStr", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getGS1dataStr(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getScanData", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getScanData(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setScanData", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setScanData(IntPtr ctx, string scanData);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getHRI", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getHRI(IntPtr ctx, ref IntPtr hri);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getDataFile", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getDataFile(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setDataFile", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setDataFile(IntPtr ctx, string dataFile);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getOutFile", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getOutFile(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setOutFile", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setOutFile(IntPtr ctx, string outFile);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getFormat", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getFormat(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setFormat", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setFormat(IntPtr ctx, int format);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_encode", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_encode(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getBuffer", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getBuffer(IntPtr ctx, ref IntPtr buf);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_free", CallingConvention = CallingConvention.Cdecl)]
        private static extern void gs1_encoder_free(IntPtr ctx);

        /*
         *  Methods to provide an OO wrapper around the imported functional interface and to perform marshalling
         *
         */
        public GS1Encoder()
        {
            ctx = gs1_encoder_init(IntPtr.Zero);
            if (ctx == IntPtr.Zero)
                throw new GS1EncoderGeneralException("Failed to initalise GS1 Encoder library");
        }

        public string Version
        {
            get
            {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getVersion());
            }
        }

        public string ErrMsg
        {
            get
            {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getErrMsg(ctx));
            }
        }

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

        public string GS1dataStr
        {
            get {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getGS1dataStr(ctx));
            }
            set
            {
                if (!gs1_encoder_setGS1dataStr(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

        public string ScanData
        {
            get
            {
                return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getScanData(ctx));
            }
            set
            {
                if (!gs1_encoder_setScanData(ctx, value))
                    throw new GS1EncoderParameterException(ErrMsg);
            }
        }

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

        public void Encode()
        {
            if (!gs1_encoder_encode(ctx))
                throw new GS1EncoderEncodeException(ErrMsg);
        }
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

        ~GS1Encoder()
        {
            gs1_encoder_free(ctx);
        }

    }


    public class GS1EncoderGeneralException : Exception
    {
        public GS1EncoderGeneralException(string message)
           : base(message)
        {
        }
    }

    public class GS1EncoderParameterException : Exception
    {
        public GS1EncoderParameterException(string message)
           : base(message)
        {
        }
    }

    public class GS1EncoderEncodeException : Exception
    {
        public GS1EncoderEncodeException(string message)
           : base(message)
        {
        }
    }

}

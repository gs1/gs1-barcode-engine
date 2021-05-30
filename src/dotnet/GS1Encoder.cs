using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace gs1encoders_dotnet
{

    public enum symbology : short
    {
        gs1_encoder_sNONE = -1,         // none defined
        gs1_encoder_sRSS14,             // RSS-14
        gs1_encoder_sRSS14T,            // RSS-14 Truncated
        gs1_encoder_sRSS14S,            // RSS-14 Stacked
        gs1_encoder_sRSS14SO,           // RSS-14 Stacked Omnidirectional
        gs1_encoder_sRSSLIM,            // RSS Limited
        gs1_encoder_sRSSEXP,            // RSS Expanded
        gs1_encoder_sUPCA,              // UPC-A
        gs1_encoder_sUPCE,              // UPC-E
        gs1_encoder_sEAN13,             // EAN-13
        gs1_encoder_sEAN8,              // EAN-8
        gs1_encoder_sUCC128_CCA,        // UCC/EAN-128 with CC-A or CC-B
        gs1_encoder_sUCC128_CCC,        // UCC/EAN-128 with CC-C
        gs1_encoder_sNUMSYMS,           // Number of symbologies
    };

    public class GS1Encoder
    {

        private readonly IntPtr ctx;
        private const String gs1_dll = "gs1encoders.dll";

        /*
         *  Functions imported from gs1encoders.dll
         *
         */
        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_init", CallingConvention = CallingConvention.Cdecl)]
        private static extern System.IntPtr gs1_encoder_init(IntPtr mem);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getVersion", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getVersion(IntPtr ctx);

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

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getRssExpSegWidth", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getRssExpSegWidth(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setRssExpSegWidth", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setRssExpSegWidth(IntPtr ctx, int segWidth);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getUcc128LinHeight", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getUcc128LinHeight(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setUcc128LinHeight", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool gs1_encoder_setUcc128LinHeight(IntPtr ctx, int linHeight);

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

        public string GetVersion()
        {
            return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getVersion(ctx));
        }

        public string GetErrMsg()
        {
            return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getErrMsg(ctx));
        }

        public int GetSym()
        {
            return gs1_encoder_getSym(ctx);
        }

        public void SetSym(int sym)
        {
            if (!gs1_encoder_setSym(ctx, sym))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public int GetPixMult()
        {
            return gs1_encoder_getPixMult(ctx);
        }

        public void SetPixMult(int pixMult)
        {
            if (!gs1_encoder_setPixMult(ctx, pixMult))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public int GetXundercut()
        {
            return gs1_encoder_getXundercut(ctx);
        }

        public void SetXundercut(int Xundercut)
        {
            if (!gs1_encoder_setXundercut(ctx, Xundercut))
                throw new GS1EncoderParameterException(GetErrMsg());
        }
        public int GetYundercut()
        {
            return gs1_encoder_getYundercut(ctx);
        }

        public void SetYundercut(int Yundercut)
        {
            if (!gs1_encoder_setYundercut(ctx, Yundercut))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public int GetSepHt()
        {
            return gs1_encoder_getSepHt(ctx);
        }

        public void SetSepHt(int sepHt)
        {
            if (!gs1_encoder_setSepHt(ctx, sepHt))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public int GetRssExpSegWidth()
        {
            return gs1_encoder_getRssExpSegWidth(ctx);
        }

        public void SetRssExpSegWidth(int segWidth)
        {
            if (!gs1_encoder_setRssExpSegWidth(ctx, segWidth))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public int GetUcc128LinHeight()
        {
            return gs1_encoder_getUcc128LinHeight(ctx);
        }

        public void SetUcc128LinHeight(int linHeight)
        {
            if (!gs1_encoder_setUcc128LinHeight(ctx, linHeight))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public int GetQrVersion()
        {
            return gs1_encoder_getQrVersion(ctx);
        }

        public void SetQrVersion(int version)
        {
            if (!gs1_encoder_setQrVersion(ctx, version))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public int GetQrEClevel()
        {
            return gs1_encoder_getQrEClevel(ctx);
        }

        public void SetQrEClevel(int eclevel)
        {
            if (!gs1_encoder_setQrEClevel(ctx, eclevel))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public bool GetAddCheckDigit()
        {
            return gs1_encoder_getAddCheckDigit(ctx);
        }

        public void SetAddCheckDigit(bool addCheckDigit)
        {
            if (!gs1_encoder_setAddCheckDigit(ctx, addCheckDigit))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public int GetDmRows()
        {
            return gs1_encoder_getDmRows(ctx);
        }

        public void SetDmRows(int rows)
        {
            if (!gs1_encoder_setDmRows(ctx, rows))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public int GetDmColumns()
        {
            return gs1_encoder_getDmColumns(ctx);
        }

        public void SetDmColumns(int columns)
        {
            if (!gs1_encoder_setDmColumns(ctx, columns))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public bool GetFileInputFlag()
        {
            return gs1_encoder_getFileInputFlag(ctx);
        }

        public void SetFileInputFlag(bool fileInputFlag)
        {
            if (!gs1_encoder_setFileInputFlag(ctx, fileInputFlag))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public string GetDataStr()
        {
            return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getDataStr(ctx));
        }

        public void SetDataStr(string dataStr)
        {
            if (!gs1_encoder_setDataStr(ctx, dataStr))
                throw new GS1EncoderParameterException(GetErrMsg());
        }
        public void SetGS1dataStr(string gs1data)
        {
            if (!gs1_encoder_setGS1dataStr(ctx, gs1data))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public string GetDataFile()
        {
            return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getDataFile(ctx));
        }

        public void SetDataFile(string dataFile)
        {
            if (!gs1_encoder_setDataFile(ctx, dataFile))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public int GetFormat()
        {
            return gs1_encoder_getFormat(ctx);
        }

        public void SetFormat(int format)
        {
            if (!gs1_encoder_setFormat(ctx, format))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public string GetOutFile()
        {
            return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getOutFile(ctx));
        }

        public void SetOutFile(string outFile)
        {
            if (!gs1_encoder_setOutFile(ctx, outFile))
                throw new GS1EncoderParameterException(GetErrMsg());
        }

        public void Encode()
        {
            if (!gs1_encoder_encode(ctx))
                throw new GS1EncoderEncodeException(GetErrMsg());
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

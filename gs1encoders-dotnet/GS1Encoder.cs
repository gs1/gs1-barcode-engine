using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace gs1encoders_dotnet
{
    class GS1Encoder
    {

        private IntPtr ctx;
        private const String gs1_dll = "gs1encoders.dll";

        /*
         *  Functions imported from gs1encoders.dll
         * 
         */
        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_init", CallingConvention = CallingConvention.Cdecl)]
        private static extern System.IntPtr gs1_encoder_init();

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getVersion", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr gs1_encoder_getVersion(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_getSym", CallingConvention = CallingConvention.Cdecl)]
        private static extern int gs1_encoder_getSym(IntPtr ctx);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_setSym", CallingConvention = CallingConvention.Cdecl)]
        private static extern void gs1_encoder_setSym(IntPtr ctx, int sym);

        [DllImport(gs1_dll, EntryPoint = "gs1_encoder_free", CallingConvention = CallingConvention.Cdecl)]
        private static extern void gs1_encoder_free(IntPtr ctx);

        /*
         *  Methods to provide an OO wrapper around the imported functional interface and to perform marshalling
         * 
         */
        public GS1Encoder()
        {
            ctx = gs1_encoder_init();
        }

        ~GS1Encoder()
        {           
            gs1_encoder_free(ctx);
        }

        public string getVersion()
        {           
            return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(gs1_encoder_getVersion(ctx));           
        }

        public void setSym(int sym)
        {
            gs1_encoder_setSym(ctx, sym);
        }

        public int getSym()
        {
            return gs1_encoder_getSym(ctx);
        }        

    }
}

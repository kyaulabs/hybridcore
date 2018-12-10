#include "DH1080.h"
#include <string.h>
#include <tcl.h>

// Input:  argv[1] = buffer of 200 bytes
//         argv[2] = buffer of 200 bytes
// Output: argv[1] = Your private key
//         argv[2] = Your public key
int DH1080_gen_tcl(ClientData dummy, Tcl_Interp *interp, int argc, char *argv[])
{
    DH1080_gen(argv[1], argv[2]);

    return TCL_OK;
}

// Input:  argv[1] = Your private key
//         argv[2] = Someones public key
// Output: argv[1] has been destroyed for security reasons
//         argv[2] = the secret key
int DH1080_comp_tcl(ClientData dummy, Tcl_Interp *interp, int argc, char *argv[])
{
    int len1, len2;

    len1=strlen(argv[1]);
    len2=strlen(argv[2]);

    if( (len1 > 181) || (len1 < 180) || (len2 > 181) || (len2 < 180) ) return TCL_ERROR;

    if(DH1080_comp(argv[1], argv[2])==0) return TCL_ERROR;

    return TCL_OK;
}


// Initialization function.
int Dh_Init (Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
        return TCL_ERROR;
    }

    if (Tcl_PkgProvide(interp, "DH1080", "1.0") == TCL_ERROR) {
        return TCL_ERROR;
    }

    DH1080_Init();

    Tcl_CreateCommand(interp, "DH1080gen", (Tcl_CmdProc *)DH1080_gen_tcl, NULL, NULL) ;
    Tcl_CreateCommand(interp, "DH1080comp", (Tcl_CmdProc *)DH1080_comp_tcl, NULL, NULL) ;

    return TCL_OK ;
}

// DeInitializtion function.
int Dh_DeInit (Tcl_Interp *interp, int flags)
{
    if(flags == TCL_UNLOAD_DETACH_FROM_PROCESS) {
        DH1080_DeInit();
    }
}

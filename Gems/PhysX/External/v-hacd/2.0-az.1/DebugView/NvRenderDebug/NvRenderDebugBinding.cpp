#include "NvRenderDebug.h"
#include <stdio.h>

#if NV_SIGNED_LIBRARY
#undef NV_SIGNED_LIBRARY
#endif

#ifdef _MSC_VER

#include <windows.h>

#ifdef NV_SIGNED_LIBRARY
#include "nvSecureLoadLibrary.h"
#endif



namespace RENDER_DEBUG
{


RENDER_DEBUG::RenderDebug *createRenderDebug(RenderDebug::Desc &desc)
{
    RENDER_DEBUG::RenderDebug *ret = NULL;
    UINT errorMode = 0;
    errorMode = SEM_FAILCRITICALERRORS;
    UINT oldErrorMode = SetErrorMode(errorMode);
#ifdef NV_SIGNED_LIBRARY
    HMODULE module = nvLoadLibraryExA(desc.dllName,0,true);
#else
    HMODULE module = LoadLibraryA(desc.dllName);
#endif
    SetErrorMode(oldErrorMode);
    if ( module )
    {
        void *proc = GetProcAddress(module,"createRenderDebugExport");
        if ( proc )
        {
            typedef void * (__cdecl * NX_GetToolkit)(const RenderDebug::Desc &desc);
            ret = (RenderDebug *)((NX_GetToolkit)proc)(desc);
        }
        else
        {
            desc.errorCode = "Unable to located the 'createRenderDebug' export symbol";
        }
    }
    else
    {
        desc.errorCode = "LoadLibrary failed; could not load the requested RenderDebug DLL; are you sure you have the correct file name?";
    }
    return ret;
}


}; // end of namespace

#else

namespace RENDER_DEBUG
{

    RENDER_DEBUG::RenderDebug *createRenderDebug(RenderDebug::Desc & /*desc*/)
    {
        RENDER_DEBUG::RenderDebug *ret = NULL;
        return ret;
    }
}; // end of namespace

#endif
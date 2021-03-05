#include "NvPhysXFramework.h"
#include <stdio.h>

#ifdef _MSC_VER

#include <windows.h>

namespace NV_PHYSX_FRAMEWORK
{


PhysXFramework *createPhysXFramework(uint32_t versionNumber, const char *dllName)
{
    NV_PHYSX_FRAMEWORK::PhysXFramework *ret = NULL;
    UINT errorMode = 0;
    errorMode = SEM_FAILCRITICALERRORS;
    UINT oldErrorMode = SetErrorMode(errorMode);
    HMODULE module = LoadLibraryA(dllName);
    SetErrorMode(oldErrorMode);
    if ( module )
    {
        void *proc = GetProcAddress(module,"createPhysXFrameworkExport");
        if ( proc )
        {
            typedef void * (__cdecl * NX_GetToolkit)(uint32_t versionNumber, const char *dllName);
            ret = (PhysXFramework *)((NX_GetToolkit)proc)(versionNumber, dllName);
        }
    }
    return ret;
}


}; // end of namespace

#else

namespace NV_PHYSX_FRAMEWORK
{

    NV_PHYSX_FRAMEWORK::PhysXFramework *createPhysXFramework(uint32_t versionNumber,const char *dllName)
    {
        NV_PHYSX_FRAMEWORK::PhysXFramework *ret = NULL;
        return ret;
    }
}; // end of namespace

#endif

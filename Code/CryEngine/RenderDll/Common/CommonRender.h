/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once


#include "Cry_Math.h"

#include "Defs.h"
#include "Cry_Color.h"
#include <Cry_Camera.h>
#include <MemoryAccess.h>
#include "STLGlobalAllocator.h"

#if defined(NULL_RENDERER)
#define VSCONST_INSTDATA   40
#define VSCONST_SKINMATRIX (40)
#define NUM_MAX_BONES_PER_GROUP (100)
#define NUM_MAX_BONES_PER_GROUP_WITH_MB (50)
#define VSCONST_NOISE_TABLE 64

#else
#define VSCONST_INSTDATA   0
#define VSCONST_SKINMATRIX 0
#define VSCONST_NOISE_TABLE 0
#define NUM_MAX_BONES_PER_GROUP (250)
#define NUM_MAX_BONES_PER_GROUP_WITH_MB (125)

#endif


//////////////////////////////////////////////////////////////////////
class CRenderer;
extern CRenderer* gRenDev;

class CBaseResource;

//====================================================================

#define CR_LITTLE_ENDIAN

struct SWaveForm;

extern bool gbRgb;

_inline DWORD COLCONV (DWORD clr)
{
    return ((clr & 0xff00ff00) | ((clr & 0xff0000) >> 16) | ((clr & 0xff) << 16));
}
_inline void COLCONV (ColorF& col)
{
    float v = col[0];
    col[0] = col[2];
    col[2] = v;
}

_inline void f2d(double* dst, float* src)
{
    for (int i = 0; i < 16; i++)
    {
        dst[i] = src[i];
    }
}

_inline void d2f(float* dst, double* src)
{
    for (int i = 0; i < 16; i++)
    {
        dst[i] = (float)src[i];
    }
}

//=================================================================

typedef std::map<CCryNameTSCRC, CBaseResource*> ResourcesMap;

typedef ResourcesMap::iterator ResourcesMapItor;

typedef std::vector<CBaseResource*, stl::STLGlobalAllocator<CBaseResource*> > ResourcesList;
typedef std::vector<int, stl::STLGlobalAllocator<int> > ResourceIds;

struct SResourceContainer
{
    ResourcesList m_RList;           // List of objects for access by Id's
    ResourcesMap  m_RMap;            // Map of objects for fast searching
    ResourceIds   m_AvailableIDs;    // Available object Id's for efficient ID's assigning after deleting

    SResourceContainer()
    {
        m_RList.reserve(512);
    }

    ~SResourceContainer();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_RList);
        pSizer->AddObject(m_RMap);
        pSizer->AddObject(m_AvailableIDs);
    }
};

typedef AZStd::unordered_map<CCryNameTSCRC, SResourceContainer*, AZStd::hash<CCryNameTSCRC>, AZStd::equal_to<CCryNameTSCRC>, AZ::StdLegacyAllocator> ResourceClassMap;

typedef ResourceClassMap::iterator ResourceClassMapItor;

class CBaseResource
{
private:
    // Per resource variables
    volatile int32 m_nRefCount;
    int m_nID;
    CCryNameTSCRC m_ClassName;
    CCryNameTSCRC m_NameCRC;

    static ResourceClassMap m_sResources;

public:
    static CryCriticalSection s_cResLock;

    //! Dirty flags will indicate what kind of data was invalidated
    enum EDirtyFlags
    {
        eDeviceResourceDirty = BIT(0),
        eDeviceResourceViewDirty = BIT(1),
    };

    // CCryUnknown interface
    inline void SetRefCounter(int nRefCounter) { m_nRefCount = nRefCounter; }
    virtual int32 AddRef()
    {
        int32 nRef = CryInterlockedIncrement(&m_nRefCount);
        return nRef;
    }
    virtual int32 Release();
    virtual int GetRefCounter() const { return m_nRefCount; }

    // Increment ref count, if not already scheduled for destruction.
    int32 TryAddRef()
    {
        volatile int nOldRef, nNewRef;
        do
        {
            nOldRef = m_nRefCount;
            if (nOldRef == 0)
            {
                return 0;
            }
            nNewRef = nOldRef + 1;
        }
        while (CryInterlockedCompareExchange(alias_cast<volatile LONG*>(&m_nRefCount), nNewRef, nOldRef) != nOldRef);
        return nNewRef;
    }

    // Constructors.
    CBaseResource()
        : m_nRefCount(1) {}
    CBaseResource(const CBaseResource& Src);
    CBaseResource& operator=(const CBaseResource& Src);

    // Destructor.
    virtual ~CBaseResource() {};

    CCryNameTSCRC GetNameCRC() { return m_NameCRC; }
    //inline const char *GetName() const { return m_Name.c_str(); }
    //inline const char *GetClassName() const { return m_ClassName.c_str(); }
    inline int GetID() const { return m_nID; }
    inline void SetID(int nID) { m_nID = nID; }

    virtual bool  IsValid();

    static ILINE int RListIndexFromId(int id) { return id - 1; }
    static ILINE int IdFromRListIndex(int idx) { return idx + 1; }

    static ResourceClassMap& GetMaps() { return m_sResources; }
    static CBaseResource* GetResource(const CCryNameTSCRC& className, int nID, bool bAddRef);
    static CBaseResource* GetResource(const CCryNameTSCRC& className, const CCryNameTSCRC& Name, bool bAddRef);
    static SResourceContainer* GetResourcesForClass(const CCryNameTSCRC& className);
    static void ShutDown();

    bool Register(const CCryNameTSCRC& resName, const CCryNameTSCRC& Name);
    bool UnRegister();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    virtual void InvalidateDeviceResource([[maybe_unused]] uint32 dirtyFlags) {};
};


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

#ifndef CRYINCLUDE_CRY3DENGINE_SURFACETYPEMANAGER_H
#define CRYINCLUDE_CRY3DENGINE_SURFACETYPEMANAGER_H
#pragma once

#include <I3DEngine.h>

#define MAX_SURFACE_ID 255

//////////////////////////////////////////////////////////////////////////
// SurfaceManager is implementing ISurfaceManager interface.
// Register and manages all known to game surface typs.
//////////////////////////////////////////////////////////////////////////
class CSurfaceTypeManager
    : public ISurfaceTypeManager
    , public Cry3DEngineBase
{
public:
    CSurfaceTypeManager(ISystem* pSystem);
    virtual ~CSurfaceTypeManager();

    virtual void LoadSurfaceTypes();

    virtual ISurfaceType* GetSurfaceTypeByName(const char* sName, const char* sWhy = NULL, bool warn = true);
    virtual ISurfaceType* GetSurfaceType(int nSurfaceId, const char* sWhy = NULL);
    virtual ISurfaceTypeEnumerator* GetEnumerator();

    bool RegisterSurfaceType(ISurfaceType* pSurfaceType, bool bDefault = false);
    void UnregisterSurfaceType(ISurfaceType* pSurfaceType);

    ISurfaceType* GetSurfaceTypeFast(int nSurfaceId, const char* sWhy = NULL);

    void RemoveAll();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_nameToSurface);
        for (int i = 0; i < MAX_SURFACE_ID + 1; ++i)
        {
            pSizer->AddObject(m_idToSurface[i]);
        }
    }
private:
    ISystem* m_pSystem;
    int m_lastSurfaceId;

    class CMaterialSurfaceType* m_pDefaultSurfaceType;

    struct SSurfaceRecord
    {
        bool bLoaded;
        ISurfaceType* pSurfaceType;

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(this, sizeof(*this));
        }
    };

    SSurfaceRecord* m_idToSurface[MAX_SURFACE_ID + 1];

    AZStd::mutex m_nameToSurfaceMutex;
    typedef std::map<string, SSurfaceRecord*> NameToSurfaceMap;
    NameToSurfaceMap m_nameToSurface;
};

#endif // CRYINCLUDE_CRY3DENGINE_SURFACETYPEMANAGER_H

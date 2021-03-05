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

#ifndef CRYINCLUDE_CRY3DENGINE_MATMAN_H
#define CRYINCLUDE_CRY3DENGINE_MATMAN_H
#pragma once

#include "Cry3DEngineBase.h"
#include "SurfaceTypeManager.h"
#include <CryThreadSafeRendererContainer.h>
#include "MaterialHelpers.h"
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

// forward declarations.
struct IMaterial;
struct ISurfaceType;
struct ISurfaceTypeManager;
class CMatInfo;

class ManualResetEvent
{
public:
    ManualResetEvent()
        : m_flag(false),
        m_mutex(),
        m_conditionVariable()
    {
    }

    void Wait()
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
        m_conditionVariable.wait(lock, [this]
        {
            return m_flag == true;
        });
    }

    void Set()
    {
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
            m_flag = true;
        }

        m_conditionVariable.notify_all();
    }

    void Unset()
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
        m_flag = false;
    }

    bool IsSet() const
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
        return m_flag;
    }

private:
    bool m_flag;
    mutable AZStd::mutex m_mutex;
    AZStd::condition_variable m_conditionVariable;
};

class UniqueManualEvent
{
public:
    UniqueManualEvent(ManualResetEvent* manualResetEvent, bool hasControl)
        : m_manualResetEvent(manualResetEvent),
        m_hasControl(hasControl)
    {

    }

    //! Indicates if the current thread has control of the event and is blocking other threads from proceeding
    bool HasControl() const
    {
        return m_hasControl;
    }

    void Set()
    {
        if (m_hasControl)
        {
            m_manualResetEvent->Set();
            m_hasControl = false;
        }
    }

    ~UniqueManualEvent()
    {
        Set();
    }

private:
    bool m_hasControl;
    ManualResetEvent* m_manualResetEvent;
};

//////////////////////////////////////////////////////////////////////////
//
// CMatMan is a material manager class.
//
//////////////////////////////////////////////////////////////////////////
class CMatMan
    : public IMaterialManager
    , public Cry3DEngineBase
    , public AzFramework::LegacyAssetEventBus::Handler
{
public:
    CMatMan();
    virtual ~CMatMan();

    void ShutDown();

    // interface IMaterialManager --------------------------------------------------------

    virtual _smart_ptr<IMaterial> CreateMaterial(const char* sMtlName, int nMtlFlags = 0);
    virtual _smart_ptr<IMaterial> FindMaterial(const char* sMtlName) const;
    virtual _smart_ptr<IMaterial> LoadMaterial(const char* sMtlName, bool bMakeIfNotFound = true, bool bNonremovable = false, unsigned long nLoadingFlags = 0);
    virtual _smart_ptr<IMaterial> LoadMaterialFromXml(const char* sMtlName, XmlNodeRef mtlNode);
    virtual void ReloadMaterial(_smart_ptr<IMaterial> pMtl);
    virtual void SetListener(IMaterialManagerListener* pListener) { m_pListener = pListener; };
    virtual _smart_ptr<IMaterial> GetDefaultMaterial();
    virtual _smart_ptr<IMaterial> GetDefaultTerrainLayerMaterial()
    {
        if (!m_bInitialized)
        {
            InitDefaults();
        }
        return m_pDefaultTerrainLayersMtl;
    }
    virtual _smart_ptr<IMaterial> GetDefaultLayersMaterial();
    virtual _smart_ptr<IMaterial> GetDefaultHelperMaterial();
    virtual ISurfaceType* GetSurfaceTypeByName(const char* sSurfaceTypeName, const char* sWhy = NULL);
    virtual int GetSurfaceTypeIdByName(const char* sSurfaceTypeName, const char* sWhy = NULL);
    virtual ISurfaceType* GetSurfaceType(int nSurfaceTypeId, const char* sWhy = NULL)
    {
        return m_pSurfaceTypeManager->GetSurfaceTypeFast(nSurfaceTypeId, sWhy);
    }
    virtual ISurfaceTypeManager* GetSurfaceTypeManager() { return m_pSurfaceTypeManager; }

    _smart_ptr<IMaterial> LoadCGFMaterial(CMaterialCGF* pMaterialCGF, const char* sCgfFilename, unsigned long nLoadingFlags = 0) override;
    virtual _smart_ptr<IMaterial> CloneMaterial(_smart_ptr<IMaterial> pMtl, int nSubMtl = -1);
    virtual _smart_ptr<IMaterial> CloneMultiMaterial(_smart_ptr<IMaterial> pMtl, const char* sSubMtlName = 0);
    virtual void GetLoadedMaterials(AZStd::vector<_smart_ptr<IMaterial>>* pData, uint32& nObjCount) const;

    virtual bool SaveMaterial(XmlNodeRef mtlNode, _smart_ptr<IMaterial> pMtl);
    virtual void CopyMaterial(_smart_ptr<IMaterial> pMtlSrc, _smart_ptr<IMaterial> pMtlDest, EMaterialCopyFlags flags);

    virtual void RenameMaterial(_smart_ptr<IMaterial> pMtl, const char* sNewName);
    virtual void RefreshMaterialRuntime();
    // ------------------------------------------------------------------------------------

    void InitDefaults();

    void PreloadLevelMaterials();
    void DoLoadSurfaceTypesInInit(bool doLoadSurfaceTypesInInit);

    void UpdateShaderItems();
    void RefreshShaderResourceConstants();

    // Load all known game decal materials.
    void PreloadDecalMaterials();

    void SetSketchMode(int mode);
    int  GetSketchMode() { return e_sketch_mode; }
    void SetTexelDensityDebug(int mode);
    int  GetTexelDensityDebug() { return e_texeldensity; }

    //////////////////////////////////////////////////////////////////////////
    ISurfaceType* GetSurfaceTypeFast(int nSurfaceTypeId, const char* sWhy = NULL) { return m_pSurfaceTypeManager->GetSurfaceTypeFast(nSurfaceTypeId, sWhy); }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

private: // -----------------------------------------------------------------------------
    friend class CMatInfo;
    bool Unregister(_smart_ptr<IMaterial> pMat, bool deleteEditorMaterial = true);

    _smart_ptr<IMaterial> CreateMaterialPlaceholder(const char* materialName, int nMtlFlags, const char* textureName, _smart_ptr<IMaterial> existingMtl = nullptr);

    bool LoadMaterialShader(_smart_ptr<IMaterial> pMtl, _smart_ptr<IMaterial> pParentMtl, const char* sShader, uint64 nShaderGenMask, SInputShaderResources& sr, XmlNodeRef& publicsNode);
    bool LoadMaterialLayerSlot(uint32 nSlot, _smart_ptr<IMaterial> pMtl, const char* szShaderName, SInputShaderResources& pBaseResources, XmlNodeRef& pPublicsNode, uint8 nLayerFlags);

    void ParsePublicParams(SInputShaderResources& sr, XmlNodeRef paramsNode);
    AZStd::string UnifyName(const char* sMtlName) const;
    // Can be called after material creation and initialization, to inform editor that new material in engine exist.
    // Only used internally.
    void NotifyCreateMaterial(_smart_ptr<IMaterial> pMtl);
    // Make a valid material from the XML node.
    _smart_ptr<IMaterial> MakeMaterialFromXml(const AZStd::string& sMtlName, XmlNodeRef node, bool bForcePureChild, uint16 sortPrio = 0, _smart_ptr<IMaterial> pExistingMtl = 0, unsigned long nLoadingFlags = 0, _smart_ptr<IMaterial> pParentMtl = 0);

    template<typename T>
    UniqueManualEvent CheckMaterialCache(const AZStd::string& name, T& cachedMaterial);

    _smart_ptr<IMaterial> LoadMaterialInternal(const char* sMtlName, bool bMakeIfNotFound, bool bNonremovable, unsigned long nLoadingFlags);

    // override from LegacyAssetEventBus::Handler
    // Notifies listeners that a file changed
    void OnFileChanged(AZStd::string assetPath) override;
    void OnFileRemoved(AZStd::string assetPath) override;
private:
    typedef AZStd::unordered_map<AZStd::string, _smart_ptr<IMaterial> > MtlNameMap;

    MtlNameMap                                                  m_mtlNameMap;                                   //

    IMaterialManagerListener*           m_pListener;                                     //
    _smart_ptr<IMaterial>               m_pDefaultMtl;                              //
    _smart_ptr<IMaterial>               m_pDefaultLayersMtl;                    //
    _smart_ptr<IMaterial>               m_pDefaultTerrainLayersMtl;     //
    _smart_ptr<IMaterial>               m_pNoDrawMtl;                                   //
    _smart_ptr<IMaterial>               m_pDefaultHelperMtl;

    std::vector<_smart_ptr<CMatInfo> >  m_nonRemovables;                            //

    CSurfaceTypeManager*                m_pSurfaceTypeManager;               //

    //////////////////////////////////////////////////////////////////////////
    // Cached XML parser.
    _smart_ptr<IXmlParser> m_pXmlParser;

    bool m_bInitialized;
    bool m_bLoadSurfaceTypesInInit;

    mutable AZStd::mutex m_nonRemovablesMutex;

    mutable AZStd::recursive_mutex m_materialMapMutex;
    AZStd::unordered_map<AZStd::string, AZStd::unique_ptr<ManualResetEvent>> m_pendingMaterialLoads;

public:
    // Global namespace "instance", not a class "instance", no member-variables, only const functions;
    // Used to encapsulate the material-definition/io into Cry3DEngine (and make it plugable that way).
    static MaterialHelpers s_materialHelpers;

    static int e_sketch_mode;
    static int e_lowspec_mode;
    static int e_pre_sketch_spec;
    static int e_texeldensity;
};

#endif // CRYINCLUDE_CRY3DENGINE_MATMAN_H


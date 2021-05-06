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

#include "LmbrCentral_precompiled.h"

#include <AzCore/IO/GenericStreams.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzCore/std/parallel/binary_semaphore.h>

#include <LmbrCentral/Rendering/MeshAsset.h>
#include "MeshAssetHandler.h"

#include <CryFile.h>
#include <I3DEngine.h>

namespace LmbrCentral
{   
    //////////////////////////////////////////////////////////////////////////
    const AZStd::string MeshAssetHandlerHelper::s_assetAliasToken = "@assets@/";

    // what mesh do we use as a placeholder when its currently busy compiling?
    static const char* g_meshCompilingSubstituteAsset = "engineassets/objects/default.cgf";

    MeshAssetHandlerHelper::MeshAssetHandlerHelper()
        : m_asyncLoadCvar(nullptr)
    {
    }

    void MeshAssetHandlerHelper::StripAssetAlias(const char*& assetPath)
    {
        const size_t assetAliasTokenLen = s_assetAliasToken.size() - 1;
        if (0 == strncmp(assetPath, s_assetAliasToken.c_str(), assetAliasTokenLen))
        {
            assetPath += assetAliasTokenLen;
        }
    }

    ICVar* MeshAssetHandlerHelper::GetAsyncLoadCVar()
    {
        if (!m_asyncLoadCvar)
        {
            m_asyncLoadCvar = gEnv->pConsole->GetCVar(s_meshAssetHandler_AsyncCvar);
        }

        return m_asyncLoadCvar;
    }

    //////////////////////////////////////////////////////////////////////////
    // Static Mesh Asset Handler
    //////////////////////////////////////////////////////////////////////////

    void AsyncStatObjLoadCallback(const AZ::Data::Asset<MeshAsset>& asset, _smart_ptr<IStatObj> statObj)
    {
        if (statObj)
        {
            asset.Get()->m_statObj = statObj;
        }
        else
        {
#if defined(AZ_ENABLE_TRACING)
            AZStd::string assetDescription = asset.ToString<AZStd::string>();
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetDescription, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset.GetId());
            AZ_Error("MeshAssetHandler", false, "Failed to load mesh asset %s", assetDescription.c_str());
#endif // AZ_ENABLE_TRACING
        }
    }

    MeshAssetHandler::~MeshAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr MeshAssetHandler::CreateAsset([[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;

        AZ_Assert(type == AZ::AzTypeInfo<MeshAsset>::Uuid(), "Invalid asset type! We handle only 'MeshAsset'");

        return aznew MeshAsset();
    }

    AZ::Data::AssetId MeshAssetHandler::AssetMissingInCatalog([[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        // if tracing is disabled, we are likely in a situation where we specifically don't want any diagnostic information or "errors" to appear
        // so in that case, don't load anything, don't substitute anything, don't escalate anything, just let the empty blank asset return.
#if defined(AZ_ENABLE_TRACING)
        if (asset.GetId().IsValid())
        {
            // find out whether its still compiling or it will never be available because its source file is missing.
            // this also escalates it, if found, to the top of the build queue:
            AzFramework::AssetSystem::AssetStatus statusResult = AzFramework::AssetSystem::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(statusResult, &AzFramework::AssetSystem::AssetSystemRequests::GetAssetStatusById, asset.GetId());

            if ((statusResult == AzFramework::AssetSystem::AssetStatus_Compiling) || (statusResult == AzFramework::AssetSystem::AssetStatus_Queued))
            {
                // note that we can also check other codes and substitute other meshes if we want, here...
                // its currently compiling and will finish soon.
                // substitute a placeholder mesh:

                if (!m_missingMeshAssetId.IsValid())
                {
                    // substitute the missing mesh assetId so that there's at least something to render that indicates a problem
                    // in builds where there is no diagnostics or tracing, don't substitute anything, to prefer that there's no visual indication that
                    // something is wrong in shipped games.
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(m_missingMeshAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, g_meshCompilingSubstituteAsset, azrtti_typeid<MeshAsset>(), false);
                    AZ_Error("Mesh Asset Handler", m_missingMeshAssetId.IsValid(), "Attempted to substitute %s for a missing asset, but it is also missing!", g_meshCompilingSubstituteAsset);
                }

                if (m_missingMeshAssetId.IsValid())
                {
                    AZ_TracePrintf("MeshAssetHandler", "   - substituting with default asset ID %s\n", m_missingMeshAssetId.ToString<AZStd::string>().c_str());
                    // substitute the missing mesh asset.
                    return m_missingMeshAssetId;
                }
            }
        }
#endif // defined(AZ_ENABLE_TRACING)

        // otherwise, if we get here, it means that either it was truly missing, in which case let an error occur, or the missing default substitute asset
        // is also itself missing!
        return AZ::Data::AssetId();
    }



    void MeshAssetHandler::GetCustomAssetStreamInfoForLoad(AZ::Data::AssetStreamInfo& streamInfo)
    {
        // The StatObj system only takes in a file name for loading, not a memory buffer.
        // If we set our stream data length to 0, the asset system will skip any file I/O for reading the data, and will instead
        // go directly into the AssetHandler for processing.
        streamInfo.m_dataLen = 0;
    }

    AZ::Data::AssetHandler::LoadResult MeshAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        const char* assetPath = stream->GetFilename();

        AZ_Assert(asset.GetType() == AZ::AzTypeInfo<MeshAsset>::Uuid(), "Invalid asset type! We only load 'MeshAsset'");
        if (MeshAsset* meshAsset = asset.GetAs<MeshAsset>())
        {
            AZ_Assert(!meshAsset->m_statObj.get(), "Attempting to create static mesh without cleaning up the old one.");

            // Strip the alias. StatObj instances are stored in a dictionary by their path,
            // so to share instances with legacy cry entities, we need to use the same un-aliased format.
            StripAssetAlias(assetPath);

            // Temporary cvar guard while async loading of legacy mesh formats is stabilized.
            ICVar* cvar = GetAsyncLoadCVar();
            if (!cvar || cvar->GetIVal() == 0)
            {
                if (gEnv->mMainThreadId != CryGetCurrentThreadId())
                {
                    AZStd::binary_semaphore signaller;

                    auto callback = [&asset, &signaller](IStatObj* obj)
                    {
                        AsyncStatObjLoadCallback(asset, obj);
                        signaller.release();
                    };

                    gEnv->p3DEngine->LoadStatObjAsync(callback, assetPath);
                    signaller.acquire();
                }
                else
                {
                    AsyncStatObjLoadCallback(asset, gEnv->p3DEngine->LoadStatObjAutoRef(assetPath));
                }
            }
            else
            {
                _smart_ptr<IStatObj> statObj = gEnv->p3DEngine->LoadStatObjAutoRef(assetPath);

                if (statObj)
                {
                    meshAsset->m_statObj = statObj;
                }
                else
                {
#if defined(AZ_ENABLE_TRACING)
                    AZStd::string assetDescription = asset.GetId().ToString<AZStd::string>();
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetDescription, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset.GetId());
                    AZ_Error("MeshAssetHandler", false, "Failed to load mesh asset \"%s\".", assetDescription.c_str());
#endif // AZ_ENABLE_TRACING
                }
            }

            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }
        return AZ::Data::AssetHandler::LoadResult::Error;
    }

    void MeshAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void MeshAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<MeshAsset>::Uuid());
    }

    void MeshAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<MeshAsset>::Uuid());

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<MeshAsset>::Uuid());
    }

    void MeshAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<MeshAsset>::Uuid());

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType MeshAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<MeshAsset>::Uuid();
    }

    const char* MeshAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Static Mesh";
    }

    const char* MeshAssetHandler::GetGroup() const
    {
        return "Geometry";
    }

    const char* MeshAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/StaticMesh.svg";
    }

    void MeshAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(CRY_GEOMETRY_FILE_EXT);
    }

    GeomCacheAssetHandler::~GeomCacheAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr GeomCacheAssetHandler::CreateAsset([[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;

        AZ_Assert(type == AZ::AzTypeInfo<GeomCacheAsset>::Uuid(), "Invalid asset type! We handle only 'GeomCacheAsset'");

        return aznew GeomCacheAsset();
    }

    void GeomCacheAssetHandler::GetCustomAssetStreamInfoForLoad(AZ::Data::AssetStreamInfo& streamInfo)
    {
        // The GeomCache system only takes in a file name for loading, not a memory buffer.
        // If we set our stream data length to 0, the asset system will skip any file I/O for reading the data, and will instead
        // go directly into the AssetHandler for processing.
        streamInfo.m_dataLen = 0;
    }

    AZ::Data::AssetHandler::LoadResult GeomCacheAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        const char* assetPath = stream->GetFilename();

        AZ_Assert(asset.GetType() == AZ::AzTypeInfo<GeomCacheAsset>::Uuid(), "Invalid asset type! We only load 'GeomCacheAsset'");
        if (GeomCacheAsset* geomCacheAsset = asset.GetAs<GeomCacheAsset>())
        {
            AZ_Assert(!geomCacheAsset->m_geomCache.get(), "Attempting to create geom cache without cleaning up the old one.");

            // Strip the alias. GeomCache instances are stored in a dictionary by their path,
            // so to share instances with legacy cry entities, we need to use the same un-aliased format.
            StripAssetAlias(assetPath);

            //Load GeomCaches synchronously, there is no Async support currently in the 3DEngine.
            //Assets can stream asynchronously but this load step must be synchronous. 

            _smart_ptr<IGeomCache> geomCache = gEnv->p3DEngine->LoadGeomCache(assetPath);

            if (geomCache)
            {
                geomCache->SetProcessedByRenderNode(false);
                geomCacheAsset->m_geomCache = geomCache;
            }
            else
            {
#if defined(AZ_ENABLE_TRACING)
                AZStd::string assetDescription = asset.ToString<AZStd::string>();
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetDescription, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset.GetId());
                AZ_Error("GeomCacheAssetHandler", false, "Failed to load geom cache asset %s", assetDescription.c_str());
#endif // AZ_ENABLE_TRACING
            }

            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }
        return AZ::Data::AssetHandler::LoadResult::Error;
    }

    void GeomCacheAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void GeomCacheAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<GeomCacheAsset>::Uuid());
    }

    void GeomCacheAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<GeomCacheAsset>::Uuid());

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<GeomCacheAsset>::Uuid());
    }

    void GeomCacheAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<GeomCacheAsset>::Uuid());

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType GeomCacheAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<GeomCacheAsset>::Uuid();
    }

    const char* GeomCacheAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Geom Cache";
    }

    const char* GeomCacheAssetHandler::GetGroup() const
    {
        return "Geometry";
    }

    void GeomCacheAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(CRY_GEOM_CACHE_FILE_EXT);
    }

} // namespace LmbrCentral

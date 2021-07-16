/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Asset/BuiltInAssetHandler.h>

namespace AZ
{
    namespace RPI
    {
        BuiltInAssetHandler::AssetHandlerFunctions::AssetHandlerFunctions(CreateFunction createFunction, DeleteFunction deleteFunction)
            : m_create(createFunction)
            , m_destroy(deleteFunction)
        {

        }

        BuiltInAssetHandler::BuiltInAssetHandler(const Data::AssetType& assetType, const AssetHandlerFunctions& handlerFunctions)
            : m_assetType(assetType)
            , m_handlerFunctions(handlerFunctions)
        {
        }

        BuiltInAssetHandler::BuiltInAssetHandler(const Data::AssetType& assetType, CreateFunction createFunction)
            : BuiltInAssetHandler(assetType, AssetHandlerFunctions{ createFunction })
        {
        }

        BuiltInAssetHandler::~BuiltInAssetHandler()
        {
            Unregister();
        }

        void BuiltInAssetHandler::Register()
        {
            AZ_Assert(AZ::Data::AssetManager::IsReady(), "AssetManager isn't ready!");
            AZ::Data::AssetManager::Instance().RegisterHandler(this, m_assetType);
            m_registered = true;
        }

        void BuiltInAssetHandler::Unregister()
        {
            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(this);
            }

            m_registered = false;
        }

        void BuiltInAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
        {
            assetTypes.push_back(m_assetType);
        }

        Data::AssetPtr BuiltInAssetHandler::CreateAsset(const Data::AssetId&, [[maybe_unused]] const Data::AssetType& type)
        {
            AZ_Assert(type == m_assetType, "Handler called with wrong asset type");

            Data::AssetPtr asset = m_handlerFunctions.m_create();

            AZ_Assert(asset, "CreateFunction failed to create an asset of type %s", type.ToString<AZStd::string>().data());

            // The asset has to be initialized in the "Ready" state; if it were in the default "NotLoaded" 
            // state then the asset system will automatically try to load it, which isn't valid because 
            // BuiltInAssetHandler is for hard-coded assets that don't have an presence on disk.
            AZ_Assert(asset->IsReady(), "Asset must be the in the Ready state after CreateFunction is called.");

            return asset;
        }

        void BuiltInAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
        {
            AZ_Assert(ptr->GetType() == m_assetType, "Handler called with wrong asset type");

            m_handlerFunctions.m_destroy(ptr);
        }

        void BuiltInAssetHandler::StandardDestroyFunction(AZ::Data::AssetPtr ptr)
        {
            delete ptr;
        }

        Data::AssetHandler::LoadResult BuiltInAssetHandler::LoadAssetData(
            [[maybe_unused]] const Data::Asset<Data::AssetData>& asset,
            [[maybe_unused]] AZStd::shared_ptr<Data::AssetDataStream> stream,
            [[maybe_unused]] const Data::AssetFilterCB& assetLoadFilterCB)
        {
            // LoadAssetData should never be called on a BuiltIn asset type.
            return Data::AssetHandler::LoadResult::Error;
        }

    } // namespace RPI
} // namespace AZ

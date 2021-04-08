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

#include "TranslationBuilder.h"
#include "TranslationAsset.h"

namespace GraphCanvas
{
    constexpr const char* s_graphCanvasTranslationBuilderName = "GraphCanvasTranslationBuilder";

    AZ::Uuid TranslationAssetWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{459EF910-CAAF-465A-BA19-C91979DA5729}");
    }

    const char* TranslationAssetWorker::GetFingerprintString() const
    {
        if (m_fingerprintString.empty())
        {
            // compute it the first time
            const AZStd::string runtimeAssetTypeId = azrtti_typeid<TranslationAsset>().ToString<AZStd::string>();
            m_fingerprintString = AZStd::string::format("%i%s", GetVersionNumber(), runtimeAssetTypeId.c_str());
        }
        return m_fingerprintString.c_str();
    }

    void TranslationAssetWorker::Activate()
    {
        // Use AssetCatalog service to register ScriptCanvas asset type and extension
        AZ::Data::AssetType assetType(azrtti_typeid<TranslationAsset>());
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, TranslationAsset::GetFileFilter());

        m_assetHandler = AZStd::make_unique<TranslationAssetHandler>();
        if (!AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            AZ::Data::AssetManager::Instance().RegisterHandler(m_assetHandler.get(), assetType);
        }
    }

    void TranslationAssetWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TranslationBuilder.h"
#include "TranslationAsset.h"

namespace GraphCanvas
{
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
        m_assetHandler = AZStd::make_unique<TranslationAssetHandler>();

        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(GetUUID());
    }

    void TranslationAssetWorker::Deactivate()
    {
        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusDisconnect();

        if (AZ::Data::AssetManager::Instance().GetHandler(AZ::Data::AssetType{ azrtti_typeid<TranslationAsset>() }))
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(m_assetHandler.get());
        }
    }

    void TranslationAssetWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

}

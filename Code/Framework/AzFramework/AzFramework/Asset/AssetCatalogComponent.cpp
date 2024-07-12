/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetCatalogComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetCatalog.h>

namespace AzFramework
{
    //=========================================================================
    // DataVersionConverter
    //=========================================================================
    bool AssetCatalogComponentDataVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        (void)context;

        if (classElement.GetVersion() == 1)
        {
            // Old asset path field is gone.
            const int assetRootIndex = classElement.FindElement(AZ_CRC_CE("AssetRoot"));
            if (assetRootIndex >= 0)
            {
                classElement.RemoveElement(assetRootIndex);
            }
        }

        return true;
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void AssetCatalogComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetCatalogComponent, AZ::Component>()
                ->Version(3, &AssetCatalogComponentDataVersionConverter)
                ->Field("CatalogRegistryFile", &AssetCatalogComponent::m_registryFile)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AssetCatalogComponent>(
                    "Asset Catalog", "Maintains a catalog of assets")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                    ;
            }
        }
    }

    //=========================================================================
    // AssetCatalogComponent ctor
    //=========================================================================
    AssetCatalogComponent::AssetCatalogComponent()
    {
    }

    //=========================================================================
    // AssetCatalogComponent dtor
    //=========================================================================
    AssetCatalogComponent::~AssetCatalogComponent()
    {
    }

    //=========================================================================
    // Init
    //=========================================================================
    void AssetCatalogComponent::Init()
    {
    }

    //=========================================================================
    // Activate
    //=========================================================================
    void AssetCatalogComponent::Activate()
    {
        m_catalog.reset(aznew AssetCatalog());
    }

    //=========================================================================
    // Deactivate
    //=========================================================================
    void AssetCatalogComponent::Deactivate()
    {
        m_catalog.reset();

        AzFramework::LegacyAssetEventBus::ClearQueuedEvents();
    }
} // namespace AzFramework

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
            const int assetRootIndex = classElement.FindElement(AZ_CRC("AssetRoot", 0x3195232d));
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
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CustomAssetTypeComponent.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    //=========================================================================
    // Reflect
    //=========================================================================
    void CustomAssetTypeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CustomAssetTypeComponent, AZ::Component>()
                ->Version(2);

            VersionSearchRule::Reflect(context);
            MatchingRule::Reflect(context);
            XmlSchemaAttribute::Reflect(context);
            XmlSchemaElement::Reflect(context);
            SearchRuleDefinition::Reflect(context);
            DependencySearchRule::Reflect(context);
            XmlSchemaAsset::Reflect(context);
            FileTag::FileTagAsset::Reflect(context);
            BenchmarkAsset::Reflect(context);
            BenchmarkSettingsAsset::Reflect(context);

        }
    }

    //=========================================================================
    // Activate
    //=========================================================================
    void CustomAssetTypeComponent::Activate()
    {
        using namespace FileTag;
        m_schemaAssetHandler.reset(aznew AzFramework::GenericAssetHandler<XmlSchemaAsset>(XmlSchemaAsset::GetDisplayName(), XmlSchemaAsset::GetGroup(), XmlSchemaAsset::GetFileFilter()));
        m_schemaAssetHandler->Register();

        m_fileTagAssetHandler.reset(aznew AzFramework::GenericAssetHandler<FileTagAsset>(FileTagAsset::GetDisplayName(), FileTagAsset::GetGroup(), FileTagAsset::Extension()));
        m_fileTagAssetHandler->Register();

        m_benchmarkSettingsAssetAssetHandler.reset(aznew AzFramework::GenericAssetHandler<AzFramework::BenchmarkSettingsAsset>(
            "Benchmark Settings Asset",
            "Other",
            AzFramework::s_benchmarkSettingsAssetExtension));
        m_benchmarkSettingsAssetAssetHandler->Register();

        m_benchmarkAssetAssetHandler.reset(aznew AzFramework::GenericAssetHandler<AzFramework::BenchmarkAsset>(
            "Benchmark Asset",
            "Other",
            AzFramework::s_benchmarkAssetExtension));
        m_benchmarkAssetAssetHandler->Register();
    }

    //=========================================================================
    // Deactivate
    //=========================================================================
    void CustomAssetTypeComponent::Deactivate()
    {
        m_schemaAssetHandler.reset();
        m_fileTagAssetHandler.reset();
        m_benchmarkSettingsAssetAssetHandler.reset();
        m_benchmarkAssetAssetHandler.reset();
    }
} // namespace AzFramework

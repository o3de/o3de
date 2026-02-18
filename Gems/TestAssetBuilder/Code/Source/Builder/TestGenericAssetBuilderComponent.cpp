/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builder/TestGenericAssetBuilderComponent.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Asset/AssetSerializer.h> // needed if you use field<T> on an asset.  The VCI linter lies.
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

AZ::ComponentDescriptor* TestGenericAssetBuilderComponent_CreateDescriptor()
{
    return TestAssetBuilder::TestGenericAssetBuilderComponent::CreateDescriptor();
}

namespace TestAssetBuilder
{
    namespace Details
    {
        AzFramework::GenericAssetHandler<TestGenericAsset>* s_testGenericAssetHandler = nullptr;
        AzFramework::GenericAssetHandler<TestGenericAssetRef>* s_testGenericAssetHandler_Ref = nullptr;

        void RegisterAssethandlersGeneric()
        {
            bool autoProcessToCache = true;
            s_testGenericAssetHandler =
                aznew AzFramework::GenericAssetHandler<TestGenericAsset>(
                    "Automated Test Asset With Deps", 
                    "Other", 
                    "auto_test_depasset", 
                    AZ::Uuid::CreateNull(), 
                    nullptr, 
                    AZ::ObjectStream::ST_XML, 
                    autoProcessToCache);

            s_testGenericAssetHandler_Ref = aznew AzFramework::GenericAssetHandler<TestGenericAssetRef>(
                "Automated Test Asset To Refer To",
                "Other",
                "auto_test_refasset",
                AZ::Uuid::CreateNull(),
                nullptr,
                AZ::ObjectStream::ST_XML,
                autoProcessToCache);

            s_testGenericAssetHandler->Register();
            s_testGenericAssetHandler_Ref->Register();
        }

        void UnregisterAssethandlersGeneric()
        {
            if (s_testGenericAssetHandler)
            {
                s_testGenericAssetHandler->Unregister();
                delete s_testGenericAssetHandler;
                s_testGenericAssetHandler = nullptr;
            }

            if (s_testGenericAssetHandler_Ref)
            {
                s_testGenericAssetHandler_Ref->Unregister();
                delete s_testGenericAssetHandler_Ref;
                s_testGenericAssetHandler_Ref = nullptr;
            }
        }
    } // namespace Details

    void TestGenericAsset::Reflect(AZ::ReflectContext* context)
    {
        auto serialize = azrtti_cast<AZ::SerializeContext*>(context);

        if (serialize)
        {
            serialize->Class<TestGenericAsset, AZ::Data::AssetData>()
                ->Version(2)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("ReferencedAssets_Preload", &TestGenericAsset::m_referencedAsset_Preload)
                ->Field("ReferencedAssets_NoLoad", &TestGenericAsset::m_referencedAsset_NoLoad)
                ->Field("ReferencedAssetId", &TestGenericAsset::m_referencedAssetId);

            serialize->Class<TestGenericAssetRef, AZ::Data::AssetData>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("m_someField", &TestGenericAssetRef::m_someField);

            if (auto edit = serialize->GetEditContext())
            {
                edit->Class<TestGenericAsset>("Test Generic Asset", "A generic asset used for testing asset references")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestGenericAsset::m_referencedAsset_Preload, "Preload Asset Reference", "This asset reference will be set to PreLoad")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestGenericAsset::m_referencedAsset_NoLoad, "NoLoad Asset Reference", "This asset reference will be set to NoLoad")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestGenericAsset::m_referencedAssetId, "AssetIdRef", "This asset reference is purely by ID and will be set to NoLoad")
                        ->Attribute(AZ_CRC_CE("SupportedAssetTypes"), []() {
                                AZStd::vector<AZ::Data::AssetType> supportedAssetTypes;
                                supportedAssetTypes.push_back(AZ::Data::AssetType(s_TestGenericAssetTypeId));
                                return supportedAssetTypes;
                            });

                edit->Class<TestGenericAssetRef>("Test Generic Asset Ref", "A generic asset used for testing asset references (The ref)")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestGenericAssetRef::m_someField, "Just some field", "This is just some field to make sure the asset isn't empty");
            }
        }
    }

    void TestGenericAssetBuilderComponent::Init()
    {
    }

    void TestGenericAssetBuilderComponent::Activate()
    {
        Details::RegisterAssethandlersGeneric();
    }

    void TestGenericAssetBuilderComponent::Deactivate()
    {
        Details::UnregisterAssethandlersGeneric();
    }

    void TestGenericAssetBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        TestGenericAsset::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TestGenericAssetBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }

    void TestGenericAssetBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AzFramework::s_GenericAssetRegistrar);
    }

    void TestGenericAssetBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        AZ_UNUSED(incompatible)
    }

    void TestGenericAssetBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void TestGenericAssetBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }
} // namespace TestAssetBuilder


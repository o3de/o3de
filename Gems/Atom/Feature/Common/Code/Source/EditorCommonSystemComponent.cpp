/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorCommonSystemComponent.h>
#include <Source/Material/UseTextureFunctorSourceData.h>
#include <Source/Material/DrawListFunctorSourceData.h>
#include <Source/Material/SubsurfaceTransmissionParameterFunctorSourceData.h>
#include <Source/Material/Transform2DFunctorSourceData.h>
#include <Source/Material/ConvertEmissiveUnitFunctorSourceData.h>

#include <Atom/Feature/Utils/EditorLightingPreset.h>
#include <Atom/Feature/Utils/EditorModelPreset.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzFramework/Application/Application.h>

#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <Atom/RPI.Edit/Material/LuaMaterialFunctorSourceData.h>

namespace AZ
{
    namespace Render
    {
        //! Main system component for the Atom Common Feature Gem's editor/tools module.
        void EditorCommonSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorCommonSystemComponent, AZ::Component>()
                    ->Version(1)
                    ->Attribute(Edit::Attributes::SystemComponentTags, AZStd::vector<Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorCommonSystemComponent>("Common", "Configures editor- and tool-specific functionality for common render features.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }

                AZ::Render::UseTextureFunctorSourceData::Reflect(context);
                AZ::Render::DrawListFunctorSourceData::Reflect(context);
                AZ::Render::Transform2DFunctorSourceData::Reflect(context);
                AZ::Render::ConvertEmissiveUnitFunctorSourceData::Reflect(context);
                AZ::Render::SubsurfaceTransmissionParameterFunctorSourceData::Reflect(context);

                AZ::Render::EditorLightingPreset::Reflect(context);
                AZ::Render::EditorModelPreset::Reflect(context);
            }
        }

        void EditorCommonSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EditorCommonService", 0x0b32b422));
        }

        void EditorCommonSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("EditorCommonService", 0x0b32b422));
        }

        void EditorCommonSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }

        void EditorCommonSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void EditorCommonSystemComponent::Init()
        {
        }

        void EditorCommonSystemComponent::Activate()
        {
            RPI::MaterialFunctorSourceDataRegistration* materialFunctorRegistration = RPI::MaterialFunctorSourceDataRegistration::Get();
            if (!materialFunctorRegistration)
            {
                // On some host platforms shader processing is not supported and this interface is not available.
                return;
            }

            materialFunctorRegistration->RegisterMaterialFunctor("UseTexture", azrtti_typeid<UseTextureFunctorSourceData>());
            materialFunctorRegistration->RegisterMaterialFunctor("OverrideDrawList",         azrtti_typeid<DrawListFunctorSourceData>());
            materialFunctorRegistration->RegisterMaterialFunctor("Transform2D",              azrtti_typeid<Transform2DFunctorSourceData>());
            materialFunctorRegistration->RegisterMaterialFunctor("ConvertEmissiveUnit",      azrtti_typeid<ConvertEmissiveUnitFunctorSourceData>());
            materialFunctorRegistration->RegisterMaterialFunctor("HandleSubsurfaceScatteringParameters", azrtti_typeid<SubsurfaceTransmissionParameterFunctorSourceData>());
            materialFunctorRegistration->RegisterMaterialFunctor("Lua", azrtti_typeid<RPI::LuaMaterialFunctorSourceData>());
        }

        void EditorCommonSystemComponent::Deactivate()
        {
        }
    } // namespace Render
} // namespace AZ

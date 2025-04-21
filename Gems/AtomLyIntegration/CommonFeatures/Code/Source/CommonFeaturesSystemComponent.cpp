/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/CoreLightsConstants.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignment.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignmentId.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <CommonFeaturesSystemComponent.h>

namespace AZ
{
    namespace Render
    {
        void AtomLyIntegrationCommonFeaturesSystemComponent::Reflect(ReflectContext* context)
        {
            MaterialAssignment::Reflect(context);
            CoreLightConstantsReflect(context);

            if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<AtomLyIntegrationCommonFeaturesSystemComponent, Component>()
                    ->Version(0);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<AtomLyIntegrationCommonFeaturesSystemComponent>("Common", "[Description of functionality provided by this System Component]")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void AtomLyIntegrationCommonFeaturesSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("LyIntegrationCommonFeaturesService"));
        }

        void AtomLyIntegrationCommonFeaturesSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("LyIntegrationCommonFeaturesService"));
        }

        void AtomLyIntegrationCommonFeaturesSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("CommonService"));
        }

        void AtomLyIntegrationCommonFeaturesSystemComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void AtomLyIntegrationCommonFeaturesSystemComponent::Init()
        {
        }

        void AtomLyIntegrationCommonFeaturesSystemComponent::Activate()
        {
            auto modelAssetHandler = azrtti_cast<RPI::ModelAssetHandler*>(Data::AssetManager::Instance().GetHandler(azrtti_typeid<RPI::ModelAsset>()));
            modelAssetHandler->m_componentTypeId = EditorMeshComponentTypeId;
        }

        void AtomLyIntegrationCommonFeaturesSystemComponent::Deactivate()
        {
        }
    } // namespace Render
} // namespace AZ

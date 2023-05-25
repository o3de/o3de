/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "VegetationSystemComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Vegetation/DescriptorListAsset.h>
#include <Vegetation/AreaComponentBase.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/InstanceSpawner.h>
#include <Vegetation/EmptyInstanceSpawner.h>
#include <Vegetation/PrefabInstanceSpawner.h>

AZ_DEFINE_BUDGET(Vegetation);

namespace Vegetation
{
    void InstanceSpawner::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<InstanceSpawner>()
                ->Version(0)
                ;
        }
    }

    namespace Details
    {
        AzFramework::GenericAssetHandler<DescriptorListAsset>* s_vegetationDescriptorListAssetHandler = nullptr;

        void RegisterAssethandlers()
        {
            s_vegetationDescriptorListAssetHandler = aznew AzFramework::GenericAssetHandler<DescriptorListAsset>("Vegetation Descriptor List", "Other", "vegdescriptorlist");
            s_vegetationDescriptorListAssetHandler->Register();
        }

        void UnregisterAssethandlers()
        {
            if (s_vegetationDescriptorListAssetHandler)
            {
                s_vegetationDescriptorListAssetHandler->Unregister();
                delete s_vegetationDescriptorListAssetHandler;
                s_vegetationDescriptorListAssetHandler = nullptr;
            }
        }
    }

    void VegetationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationSystemService"));
    }

    void VegetationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationSystemService"));
    }

    void VegetationSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaSystemService"));
        services.push_back(AZ_CRC_CE("VegetationInstanceSystemService"));
        services.push_back(AZ_CRC_CE("SurfaceDataSystemService"));
    }

    void VegetationSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SurfaceDataProviderService"));
    }

    void VegetationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        InstanceSpawner::Reflect(context);
        EmptyInstanceSpawner::Reflect(context);
        PrefabInstanceSpawner::Reflect(context);
        Descriptor::Reflect(context);
        AreaConfig::Reflect(context);
        AreaComponentBase::Reflect(context);
        DescriptorListAsset::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<VegetationSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<VegetationSystemComponent>("Vegetation System", "Reflects types and defines required services for dynamic vegetation systems to function")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Vegetation")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<FilterRequestBus>("FilterRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetFilterStage", &FilterRequestBus::Events::GetFilterStage)
                ->Event("SetFilterStage", &FilterRequestBus::Events::SetFilterStage)
                ->VirtualProperty("FilterStage", "GetFilterStage", "SetFilterStage");
                ;
        }
    }

    VegetationSystemComponent::VegetationSystemComponent()
    {
    }

    VegetationSystemComponent::~VegetationSystemComponent()
    {
    }

    void VegetationSystemComponent::Activate()
    {
        Details::RegisterAssethandlers();
    }

    void VegetationSystemComponent::Deactivate()
    {
        Details::UnregisterAssethandlers();
    }
}

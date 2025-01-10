/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>
#include <RPI.Private/Module.h>

#include <Atom/RPI.Public/Image/ImageTagSystemComponent.h>
#include <Atom/RPI.Public/Model/ModelTagSystemComponent.h>
#include <RPI.Private/RPISystemComponent.h>
#include <RPI.Private/PassTemplatesAutoLoader.h>

AZ::RPI::Module::Module()
{
    m_descriptors.push_back(AZ::RPI::RPISystemComponent::CreateDescriptor());
    m_descriptors.push_back(AZ::RPI::ImageTagSystemComponent::CreateDescriptor());
    m_descriptors.push_back(AZ::RPI::ModelTagSystemComponent::CreateDescriptor());
    m_descriptors.push_back(AZ::RPI::PassTemplatesAutoLoader::CreateDescriptor());
}

AZ::ComponentTypeList AZ::RPI::Module::GetRequiredSystemComponents() const
{
    return
    {
        azrtti_typeid<AZ::RPI::RPISystemComponent>(),
        azrtti_typeid<AZ::RPI::ImageTagSystemComponent>(),
        azrtti_typeid<AZ::RPI::ModelTagSystemComponent>(),
        azrtti_typeid<AZ::RPI::PassTemplatesAutoLoader>()
    };
}


#ifndef RPI_EDITOR
    #if defined(O3DE_GEM_NAME)
    AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Private), AZ::RPI::Module)
    #else
    AZ_DECLARE_MODULE_CLASS(Gem_Atom_RPI_Private, AZ::RPI::Module)
    #endif
#endif // RPI_EDITOR

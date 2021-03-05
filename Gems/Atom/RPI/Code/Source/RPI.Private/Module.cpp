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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>
#include <RPI.Private/Module.h>

#include <RPI.Private/RPISystemComponent.h>

AZ::RPI::Module::Module()
{
    m_descriptors.push_back(AZ::RPI::RPISystemComponent::CreateDescriptor());
}

AZ::ComponentTypeList AZ::RPI::Module::GetRequiredSystemComponents() const
{
    return
    {
        azrtti_typeid<AZ::RPI::RPISystemComponent>(),
    };
}


#ifndef RPI_EDITOR
  // DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
  // The first parameter should be GemName_GemIdLower
  // The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RPI_Private, AZ::RPI::Module);
#endif // RPI_EDITOR
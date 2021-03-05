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
#include "TouchBending_precompiled.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "Simulation/PhysicsComponent.h"

#if defined(TOUCHBENDING_EDITOR)
#include <Pipeline/TouchBendingSceneSystemComponent.h>
#include <Pipeline/TouchBendingRuleBehavior.h>
#endif //defined TOUCHBENDING_EDITOR

/*!
  There are two purposes to this module.
  The first purpose is to show to 3rd party developers how the SceneAPI
  can be extended to add new Rules (aka Modifiers) to an existing data
  group and further enrich *.assetinfo files to customize what data
  is relevant to export from FBX files.

  The second purpose of this module is to extend the IMeshGroup group,
  by adding a TouchBendingRule and a TouchBendingRuleBehavior component
  to IMeshGroup so an FBX asset can be exported as touch bendable in the CGF.
 */
namespace TouchBending
{
    class TouchBendingModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(TouchBendingModule, "{F95EB8A8-A3BD-449A-98E4-E7DC7D450C30}", AZ::Module);
        AZ_CLASS_ALLOCATOR(TouchBendingModule, AZ::SystemAllocator, 0);

        TouchBendingModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                Simulation::PhysicsComponent::CreateDescriptor(),
#if defined(TOUCHBENDING_EDITOR)
                Pipeline::TouchBendingSceneSystemComponent::CreateDescriptor(),
                Pipeline::TouchBendingRuleBehavior::CreateDescriptor(),
#endif //defined TOUCHBENDING_EDITOR
             });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList
            {
                azrtti_typeid<Simulation::PhysicsComponent>(),
#if defined(TOUCHBENDING_EDITOR)
                azrtti_typeid<Pipeline::TouchBendingSceneSystemComponent>(),
#endif //defined TOUCHBENDING_EDITOR
            };
        }
    };
} //namespace TouchBending

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_TouchBending, TouchBending::TouchBendingModule)

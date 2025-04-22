/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <SceneAPI/SceneData/Behaviors/Registry.h>
#include <SceneAPI/SceneData/Behaviors/AnimationGroup.h>
#include <SceneAPI/SceneData/Behaviors/BlendShapeRuleBehavior.h>
#include <SceneAPI/SceneData/Behaviors/LodRuleBehavior.h>
#include <SceneAPI/SceneData/Behaviors/MaterialRuleBehavior.h>
#include <SceneAPI/SceneData/Behaviors/MeshAdvancedRule.h>
#include <SceneAPI/SceneData/Behaviors/ImportGroup.h>
#include <SceneAPI/SceneData/Behaviors/MeshGroup.h>
#include <SceneAPI/SceneData/Behaviors/ScriptProcessorRuleBehavior.h>
#include <SceneAPI/SceneData/Behaviors/SkeletonGroup.h>
#include <SceneAPI/SceneData/Behaviors/SkinGroup.h>
#include <SceneAPI/SceneData/Behaviors/SkinRuleBehavior.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            void Registry::RegisterComponents(ComponentDescriptorList& components)
            {
                components.insert(components.end(),
                {
                    Behaviors::AnimationGroup::CreateDescriptor(),
                    BlendShapeRuleBehavior::CreateDescriptor(),
                    LodRuleBehavior::CreateDescriptor(),
                    MaterialRuleBehavior::CreateDescriptor(),
                    Behaviors::ImportGroup::CreateDescriptor(),
                    Behaviors::MeshAdvancedRule::CreateDescriptor(),
                    Behaviors::MeshGroup::CreateDescriptor(),
                    Behaviors::ScriptProcessorRuleBehavior::CreateDescriptor(),
                    Behaviors::SkeletonGroup::CreateDescriptor(),
                    Behaviors::SkinGroup::CreateDescriptor(),
                    SkinRuleBehavior::CreateDescriptor()
                });
            }
        } // SceneData
    } // SceneAPI
} // AZ

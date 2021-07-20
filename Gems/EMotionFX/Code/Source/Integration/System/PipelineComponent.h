/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(EMOTIONFXANIMATION_EDITOR)
#pragma once

#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/EMotionFXAllocatorInitializer.h>

#include <AzCore/Module/Environment.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        class PipelineComponent 
            : public AZ::SceneAPI::SceneCore::SceneSystemComponent
        {
        public:
            AZ_COMPONENT(PipelineComponent, "{F74E0D7C-BF22-4BC0-897A-2D80DA960DB0}", AZ::SceneAPI::SceneCore::SceneSystemComponent);

            PipelineComponent();
            ~PipelineComponent() override = default;

            void Activate() override;
            void Deactivate() override;

            static void Reflect(AZ::ReflectContext* context);

        private:
            bool                                                m_EMotionFXInited;
            AZStd::unique_ptr<CommandSystem::CommandManager>    m_commandManager;

            // Creates a static shared pointer using the AZ EnvironmentVariable system.
            // This will prevent the EMotionFXAllocator from destroying too early by the other component
            static AZ::EnvironmentVariable<EMotionFXAllocatorInitializer> s_eMotionFXAllocatorInitializer;
        };
    } // Pipeline
} // EMotionFX
#endif

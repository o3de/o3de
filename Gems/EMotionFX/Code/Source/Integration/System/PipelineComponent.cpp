/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(EMOTIONFXANIMATION_EDITOR)
#include <AzCore/Serialization/SerializeContext.h>
#include <MCore/Source/MCoreSystem.h>
#include <Integration/System/PipelineComponent.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <Integration/System/SystemCommon.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        PipelineComponent::PipelineComponent()
            : m_eMotionFxInited(false)
        {

        }

        void PipelineComponent::Activate()
        {
            if (!m_eMotionFxInited)
            {
                MCore::Initializer::InitSettings coreSettings;
                if (!MCore::Initializer::Init(&coreSettings))
                {
                    AZ_Error("EMotionFX", false, "Failed to initialize EMotion FX SDK Core");
                    return;
                }

                // Initialize EMotion FX runtime.
                EMotionFX::Initializer::InitSettings emfxSettings;
                emfxSettings.m_unitType = MCore::Distance::UNITTYPE_METERS;

                if (!EMotionFX::Initializer::Init(&emfxSettings))
                {
                    AZ_Error("EMotionFX", false, "Failed to initialize EMotion FX SDK Runtime");
                    return;
                }

                // Initialize the EMotionFX command system.
                m_commandManager = AZStd::make_unique<CommandSystem::CommandManager>();
                m_eMotionFxInited = true;
            }
        }

        void PipelineComponent::Deactivate()
        {
            if (m_eMotionFxInited)
            {
                m_eMotionFxInited = false;
                m_commandManager.reset();
                EMotionFX::Initializer::Shutdown();
                MCore::Initializer::Shutdown();
            }
        }

        void PipelineComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<PipelineComponent, AZ::SceneAPI::SceneCore::SceneSystemComponent>()->Version(1);
            }
        }
    } // Pipeline
} // EMotionFX
#endif

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Integration/System/SystemComponent.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/Components/AnimAudioComponent.h>
#include <Integration/Components/AnimGraphComponent.h>
#include <Integration/Components/SimpleMotionComponent.h>
#include <Integration/Components/SimpleLODComponent.h>
#include <AzCore/Module/DynamicModuleHandle.h>

#include <IGem.h>

#if defined (EMOTIONFXANIMATION_EDITOR)
#   include <Integration/System/PipelineComponent.h>
#   include <Integration/Editor/Components/EditorActorComponent.h>
#   include <Integration/Editor/Components/EditorAnimAudioComponent.h>
#   include <Integration/Editor/Components/EditorAnimGraphComponent.h>
#   include <Integration/Editor/Components/EditorSimpleMotionComponent.h>
#   include <Integration/Editor/Components/EditorSimpleLODComponent.h>
#   include <SceneAPIExt/Behaviors/ActorGroupBehavior.h>
#   include <SceneAPIExt/Behaviors/MotionGroupBehavior.h>
#   include <SceneAPIExt/Behaviors/MotionRangeRuleBehavior.h>
#   include <SceneAPIExt/Behaviors/MorphTargetRuleBehavior.h>
#   include <SceneAPIExt/Behaviors/LodRuleBehavior.h>
#   include <SceneAPIExt/Behaviors/SkeletonOptimizationRuleBehavior.h>
#   include <SceneAPIExt/Behaviors/RootMotionExtractionRuleBehavior.h>
#   include <RCExt/Actor/ActorExporter.h>
#   include <RCExt/Actor/ActorGroupExporter.h>
#   include <RCExt/Actor/ActorBuilder.h>
#   include <RCExt/Actor/MorphTargetExporter.h>
#   include <RCExt/Motion/MotionExporter.h>
#   include <RCExt/Motion/MotionGroupExporter.h>
#   include <RCExt/Motion/MotionDataBuilder.h>
#   include <EMotionFXBuilder/EMotionFXBuilderComponent.h>
#endif // EMOTIONFXANIMATION_EDITOR

namespace EMotionFX
{
    namespace Integration
    {
        /**
         * Animation module class for EMotion FX animation gem.
         */
        class EMotionFXIntegrationModule
            : public CryHooksModule
        {
        public:
            AZ_CLASS_ALLOCATOR(EMotionFXIntegrationModule, AZ::SystemAllocator)
            AZ_RTTI(EMotionFXIntegrationModule, "{02533EDC-F2AA-4076-86E9-5E3702202E15}", CryHooksModule);

            EMotionFXIntegrationModule()
                : CryHooksModule()
            {
                // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                m_descriptors.insert(m_descriptors.end(), {
                    // System components
                    SystemComponent::CreateDescriptor(),

                    // Runtime components
                    ActorComponent::CreateDescriptor(),
                    AnimAudioComponent::CreateDescriptor(),
                    AnimGraphComponent::CreateDescriptor(),
                    SimpleMotionComponent::CreateDescriptor(),
                    SimpleLODComponent::CreateDescriptor(),

    #if defined(EMOTIONFXANIMATION_EDITOR)
                    // Pipeline components
                    EMotionFX::Pipeline::PipelineComponent::CreateDescriptor(),

                    // Editor components
                    EditorActorComponent::CreateDescriptor(),
                    EditorAnimAudioComponent::CreateDescriptor(),
                    EditorAnimGraphComponent::CreateDescriptor(),
                    EditorSimpleMotionComponent::CreateDescriptor(),
                    EditorSimpleLODComponent::CreateDescriptor(),

                    // EmotionFX asset builder
                    EMotionFXBuilder::EMotionFXBuilderComponent::CreateDescriptor(),

                    // Actor
                    EMotionFX::Pipeline::Behavior::ActorGroupBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::Behavior::MorphTargetRuleBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::Behavior::LodRuleBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::Behavior::SkeletonOptimizationRuleBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::ActorExporter::CreateDescriptor(),
                    EMotionFX::Pipeline::ActorGroupExporter::CreateDescriptor(),
                    EMotionFX::Pipeline::ActorBuilder::CreateDescriptor(),
                    EMotionFX::Pipeline::MorphTargetExporter::CreateDescriptor(),

                    // Motion
                    EMotionFX::Pipeline::Behavior::MotionGroupBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::Behavior::MotionRangeRuleBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::Behavior::RootMotionExtractionRuleBehavior::CreateDescriptor(),
                    EMotionFX::Pipeline::MotionExporter::CreateDescriptor(),
                    EMotionFX::Pipeline::MotionGroupExporter::CreateDescriptor(),
                    EMotionFX::Pipeline::MotionDataBuilder::CreateDescriptor()
    #endif // EMOTIONFXANIMATION_EDITOR
                });
            }

            ~EMotionFXIntegrationModule()
            {
            }

            /**
             * Add required SystemComponents to the SystemEntity.
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<SystemComponent>(),
                };
            }
        };
    }
}

#if defined(EMOTIONFXANIMATION_EDITOR)
#include <QDir>

// Qt resources are defined in the EMotionFX.Editor static library, so we must
// initialize them manually
extern int qInitResources_Resources();
extern int qCleanupResources_Resources();
namespace {
   struct initializer {
       initializer() { qInitResources_Resources(); }
       ~initializer() { qCleanupResources_Resources(); }
   } dummy;
} // namespace
#endif

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), EMotionFX::Integration::EMotionFXIntegrationModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_EMotionFX, EMotionFX::Integration::EMotionFXIntegrationModule)
#endif

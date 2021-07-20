/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

#include <CommonSystemComponent.h>
#include <FrameCaptureSystemComponent.h>
#include <ProfilingCaptureSystemComponent.h>
#include <ImGui/ImGuiSystemComponent.h>
#include <SkinnedMesh/SkinnedMeshSystemComponent.h>

// Core Lights
#include <CoreLights/CoreLightsSystemComponent.h>


#ifdef ATOM_FEATURE_COMMON_EDITOR
#include <EditorCommonSystemComponent.h>
#include <Material/MaterialConverterSystemComponent.h>
#endif

namespace AZ
{
    namespace Render
    {
        class CommonModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(CommonModule, "{116699A4-176B-41BE-8D07-77590319687B}", AZ::Module);
            AZ_CLASS_ALLOCATOR(CommonModule, AZ::SystemAllocator, 0);

            CommonModule()
                : AZ::Module()
            {
                m_descriptors.insert(m_descriptors.end(), {
                    CoreLightsSystemComponent::CreateDescriptor(),
                    CommonSystemComponent::CreateDescriptor(),
                    FrameCaptureSystemComponent::CreateDescriptor(),
                    ProfilingCaptureSystemComponent::CreateDescriptor(),
                    ImGuiSystemComponent::CreateDescriptor(),
                    SkinnedMeshSystemComponent::CreateDescriptor(),
                    
                    // post effects

#ifdef ATOM_FEATURE_COMMON_EDITOR
                    EditorCommonSystemComponent::CreateDescriptor(),
                    MaterialConverterSystemComponent::CreateDescriptor(),
#endif
                    });
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<CommonSystemComponent>(),
                    azrtti_typeid<FrameCaptureSystemComponent>(),
                    azrtti_typeid<ProfilingCaptureSystemComponent>(),
                    azrtti_typeid<CoreLightsSystemComponent>(),
                    azrtti_typeid<ImGuiSystemComponent>(),
                    azrtti_typeid<SkinnedMeshSystemComponent>(),
#ifdef ATOM_FEATURE_COMMON_EDITOR
                    azrtti_typeid<EditorCommonSystemComponent>(),
#endif
                };
            }
        };
    } // namespace Render
} // namespace AZ

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_Feature_Common, AZ::Render::CommonModule)

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            AZ_CLASS_ALLOCATOR(CommonModule, AZ::SystemAllocator);

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

#if O3DE_HEADLESS_SERVER
    #if defined(O3DE_GEM_NAME)
    AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Headless), AZ::Render::CommonModule)
    #else
    AZ_DECLARE_MODULE_CLASS(Gem_Atom_Feature_Common_Headless, AZ::Render::CommonModule)
    #endif
#else
    #if defined(O3DE_GEM_NAME)
    AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::Render::CommonModule)
    #else
    AZ_DECLARE_MODULE_CLASS(Gem_Atom_Feature_Common, AZ::Render::CommonModule)
    #endif
#endif // O3DE_HEADLESS_SERVER

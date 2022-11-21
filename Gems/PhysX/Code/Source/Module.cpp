/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <Configuration/PhysXSettingsRegistryManager.h>
#include <Source/SystemComponent.h>
#include <ComponentDescriptors.h>

#if defined(PHYSX_EDITOR)
#include <Source/EditorComponentDescriptors.h>
#include <Editor/Source/Components/EditorSystemComponent.h>
#include <Editor/Source/Configuration/PhysXEditorSettingsRegistryManager.h>
#endif // defined(PHYSX_EDITOR)

#include <PhysX_Traits_Platform.h>

#include <System/PhysXSystem.h>
#include <System/PhysXCookingParams.h>

namespace PhysX
{
    class Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(PhysX::Module, "{160C59B1-FA68-4CDC-8562-D1204AB78FC1}", AZ::Module);
        AZ_CLASS_ALLOCATOR(PhysX::Module, AZ::SystemAllocator, 0)

        Module()
            : AZ::Module()
#if defined(PHYSX_EDITOR)
            , m_physXSystem(AZStd::make_unique<PhysXEditorSettingsRegistryManager>(), PxCooking::GetEditTimeCookingParams())
#else
            , m_physXSystem(AZStd::make_unique<PhysXSettingsRegistryManager>(), PxCooking::GetRealTimeCookingParams())
#endif
        {
            // PhysXSystemConfiguration needs to be 16-byte aligned since it contains a SIMD vector4.
            // The vector4 itself is aligned relative to the module class, but if the module class is
            // not also aligned, it will crash. This checks makes sure they will be aligned to 16 bytes.
            static_assert(alignof(PhysX::PhysXSystemConfiguration) == 16);
            static_assert(alignof(PhysX::PhysXSystem) == 16);
            static_assert(alignof(PhysX::Module) == 16);
            
            LoadModules();

            AZStd::list<AZ::ComponentDescriptor*> descriptorsToAdd = GetDescriptors();
            m_descriptors.insert(m_descriptors.end(), descriptorsToAdd.begin(), descriptorsToAdd.end());
#if defined(PHYSX_EDITOR)
            AZStd::list<AZ::ComponentDescriptor*> editorDescriptorsToAdd = GetEditorDescriptors();
            m_descriptors.insert(m_descriptors.end(), editorDescriptorsToAdd.begin(), editorDescriptorsToAdd.end());
#endif // defined(PHYSX_EDITOR)
        }

        virtual ~Module()
        {
            m_physXSystem.Shutdown();

            UnloadModules();

            AZ::GetCurrentSerializeContextModule().Cleanup();
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SystemComponent>()
#if defined(PHYSX_EDITOR)
                , azrtti_typeid<EditorSystemComponent>()
#endif
            };
        }

    private:
        void LoadModules()
        {
#if defined(PHYSX_EDITOR)
            {
                AZStd::unique_ptr<AZ::DynamicModuleHandle> sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
                [[maybe_unused]] bool ok = sceneCoreModule->Load(true/*isInitializeFunctionRequired*/);
                AZ_Error("PhysX::Module", ok, "Error loading SceneCore module");

                m_modules.push_back(AZStd::move(sceneCoreModule));
            }
#endif // defined(PHYSX_EDITOR)

            // Load PhysX SDK dynamic libraries when running on a non-monolithic build.
            // The PhysX Gem module was linked with the PhysX SDK dynamic libraries, but
            // some platforms may not detect the dependency when the gem is loaded, so we
            // may have to load them ourselves.
#if AZ_TRAIT_PHYSX_FORCE_LOAD_MODULES && !defined(AZ_MONOLITHIC_BUILD)
            {
                const AZStd::vector<AZ::OSString> physXModuleNames = { "PhysX", "PhysXCooking", "PhysXFoundation", "PhysXCommon" };
                for (const auto& physXModuleName : physXModuleNames)
                {
                    AZ::OSString modulePathName = physXModuleName;
                    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::ResolveModulePath, modulePathName);

                    AZStd::unique_ptr<AZ::DynamicModuleHandle> physXModule = AZ::DynamicModuleHandle::Create(modulePathName.c_str());
                    bool ok = physXModule->Load(false/*isInitializeFunctionRequired*/);
                    AZ_Error("PhysX::Module", ok, "Error loading %s module", physXModuleName.c_str());

                    m_modules.push_back(AZStd::move(physXModule));
                }
            }
#endif
        }

        void UnloadModules()
        {
            // Unload modules in reserve order that were loaded
            for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it)
            {
                it->reset();
            }
            m_modules.clear();
        }

        /// Required modules to load/unload when PhysX Gem module is created/destroyed
        AZStd::vector<AZStd::unique_ptr<AZ::DynamicModuleHandle>> m_modules;

        PhysXSystem m_physXSystem;
    };
} // namespace PhysX

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_PhysX, PhysX::Module)

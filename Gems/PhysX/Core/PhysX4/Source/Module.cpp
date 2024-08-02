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

#include <System/PhysXSystem.h>
#include <System/PhysXCookingParams.h>

namespace PhysX
{
    class Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(PhysX::Module, "{160C59B1-FA68-4CDC-8562-D1204AB78FC1}", AZ::Module);
        AZ_CLASS_ALLOCATOR(PhysX::Module, AZ::SystemAllocator)

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
                [[maybe_unused]] bool ok = sceneCoreModule->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired);
                AZ_Error("PhysX::Module", ok, "Error loading SceneCore module");

                m_modules.push_back(AZStd::move(sceneCoreModule));
            }
#endif // defined(PHYSX_EDITOR)
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

    // The PhysX::Module also needs to be 16-byte aligned
    static_assert(alignof(PhysX::Module) == 16);

} // namespace PhysX

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), PhysX::Module)
#else
AZ_DECLARE_MODULE_CLASS(Gem_PhysX, PhysX::Module)
#endif

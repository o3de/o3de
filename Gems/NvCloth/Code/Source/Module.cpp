/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <System/SystemComponent.h>
#include <System/FabricCooker.h>
#include <System/TangentSpaceHelper.h>
#include <Components/ClothComponent.h>

#ifdef NVCLOTH_EDITOR
#include <Editor/EditorSystemComponent.h>
#include <Components/EditorClothComponent.h>
#include <Pipeline/SceneAPIExt/ClothRuleBehavior.h>
#endif //NVCLOTH_EDITOR

namespace NvCloth
{
    class Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(Module, "{34C529D4-688F-4B51-BF60-75425754A7E6}", AZ::Module);
        AZ_CLASS_ALLOCATOR(Module, AZ::SystemAllocator);

        Module()
            : AZ::Module()
        {
            SystemComponent::InitializeNvClothLibrary();

            // IFabricCooker and ITangentSpaceHelper interfaces will be available
            // at both runtime and asset build time.
            m_fabricCooker = AZStd::make_unique<FabricCooker>();
            m_tangentSpaceHelper = AZStd::make_unique<TangentSpaceHelper>();

            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                SystemComponent::CreateDescriptor(),
                ClothComponent::CreateDescriptor(),

#ifdef NVCLOTH_EDITOR
                EditorSystemComponent::CreateDescriptor(),
                EditorClothComponent::CreateDescriptor(),
                Pipeline::ClothRuleBehavior::CreateDescriptor(),
#endif //NVCLOTH_EDITOR
            });
        }

        ~Module()
        {
            m_tangentSpaceHelper.reset();
            m_fabricCooker.reset();

            SystemComponent::TearDownNvClothLibrary();
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SystemComponent>()
#ifdef NVCLOTH_EDITOR
                , azrtti_typeid<EditorSystemComponent>()
#endif //NVCLOTH_EDITOR
            };
        }

    private:
        AZStd::unique_ptr<FabricCooker> m_fabricCooker;
        AZStd::unique_ptr<TangentSpaceHelper> m_tangentSpaceHelper;
    };
} // namespace NvCloth

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), NvCloth::Module)
#else
AZ_DECLARE_MODULE_CLASS(Gem_NvCloth, NvCloth::Module)
#endif

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

#include <CrySystemBus.h>
#include <ISystem.h>

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
        , protected CrySystemEventBus::Handler
    {
    public:
        AZ_RTTI(Module, "{34C529D4-688F-4B51-BF60-75425754A7E6}", AZ::Module);
        AZ_CLASS_ALLOCATOR(Module, AZ::SystemAllocator, 0);

        Module()
            : AZ::Module()
        {
            SystemComponent::InitializeNvClothLibrary();

            // IFabricCooker and ITangentSpaceHelper interfaces will be available
            // at both runtime and asset build time.
            m_fabricCooker = AZStd::make_unique<FabricCooker>();
            m_tangentSpaceHelper = AZStd::make_unique<TangentSpaceHelper>();

            CrySystemEventBus::Handler::BusConnect();

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
            CrySystemEventBus::Handler::BusDisconnect();

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

    protected:
        // CrySystemEventBus ...
        void OnCrySystemPreInitialize(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemPostShutdown() override;

    private:
        AZStd::unique_ptr<FabricCooker> m_fabricCooker;
        AZStd::unique_ptr<TangentSpaceHelper> m_tangentSpaceHelper;
    };

    void Module::OnCrySystemPreInitialize(
        [[maybe_unused]] ISystem& system,
        [[maybe_unused]] const SSystemInitParams& systemInitParams)
    {
#if !defined(AZ_MONOLITHIC_BUILD)
        // When module is linked dynamically, we must set our gEnv pointer.
        // When module is linked statically, we'll share the application's gEnv pointer.
        gEnv = system.GetGlobalEnvironment();
#endif
    }

    void Module::OnCrySystemPostShutdown()
    {
#if !defined(AZ_MONOLITHIC_BUILD)
        gEnv = nullptr;
#endif
    }
} // namespace NvCloth

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_NvCloth, NvCloth::Module)

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Module/DynamicModuleHandle.h>

#include "SystemComponentFixture.h"

namespace AZ { class DynamicModuleHandle; }

namespace EMotionFX
{

    template<class... Components>
    class InitSceneAPIFixture : public ComponentFixture<Components...>
    {
        using DynamicModuleHandlePtr = AZStd::unique_ptr<AZ::DynamicModuleHandle>;

    protected:
        InitSceneAPIFixture() = default;
        AZ_DEFAULT_COPY_MOVE(InitSceneAPIFixture);

        void PreStart() override
        {
            const AZStd::vector<AZStd::string> moduleNames {"SceneCore", "SceneData"};
            for (const AZStd::string& moduleName : moduleNames)
            {
                AZStd::unique_ptr<AZ::DynamicModuleHandle> module = AZ::DynamicModuleHandle::Create(moduleName.c_str());
                ASSERT_TRUE(module) << "EMotionFX Editor unit tests failed to create " << moduleName.c_str() << " module.";
                const bool loaded = module->Load();
                ASSERT_TRUE(loaded) << "EMotionFX Editor unit tests failed to load " << moduleName.c_str() << " module.";
                auto init = module->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
                ASSERT_TRUE(init) << "EMotionFX Editor unit tests failed to find the initialization function the " << moduleName.c_str() << " module.";
                (*init)();

                m_modules.emplace_back(AZStd::move(module));
            }

            ComponentFixture<Components...>::PreStart();
        }

        ~InitSceneAPIFixture() override
        {
            // Deactivate the system entity first, releasing references to SceneAPI
            if (this->GetSystemEntity()->GetState() == AZ::Entity::State::Active)
            {
                this->GetSystemEntity()->Deactivate();
            }

            // Remove SceneAPI components before the DLL is uninitialized
            (([&]() {
                auto component = this->GetSystemEntity()->template FindComponent<Components>();
                if (component)
                {
                    this->GetSystemEntity()->RemoveComponent(component);
                    delete component;
                }
            })(), ...);

            // Now tear down SceneAPI
            for (DynamicModuleHandlePtr& module : m_modules)
            {
                auto uninit = module->template GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
                (*uninit)();
                module.reset();
            }
            m_modules.clear();
            m_modules.shrink_to_fit();
        }

        private:
            AZStd::vector<DynamicModuleHandlePtr> m_modules;
    };
} // namespace EMotionFX

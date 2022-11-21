/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Module/DynamicModuleHandle.h>

namespace AZ { class DynamicModuleHandle; }

namespace SceneProcessing
{
    class InitSceneAPIFixture
        : public UnitTest::ScopedAllocatorSetupFixture
    {
        using DynamicModuleHandlePtr = AZStd::unique_ptr<AZ::DynamicModuleHandle>;

    protected:

        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();

            const AZStd::vector<AZStd::string> moduleNames {"SceneCore", "SceneData"};
            for (const AZStd::string& moduleName : moduleNames)
            {
                AZStd::unique_ptr<AZ::DynamicModuleHandle> module = AZ::DynamicModuleHandle::Create(moduleName.c_str());
                ASSERT_TRUE(module) << "Scene Processing Gem unit tests failed to create " << moduleName.c_str() << " module.";
                const bool loaded = module->Load(false);
                ASSERT_TRUE(loaded) << "Scene Processing Gem unit tests failed to load " << moduleName.c_str() << " module.";
                auto init = module->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
                ASSERT_TRUE(init) << "Scene Processing Gem unit tests failed to find the initialization function the " << moduleName.c_str() << " module.";
                (*init)();

                m_modules.emplace_back(AZStd::move(module));
            }
        }

        ~InitSceneAPIFixture() override
        {
            for (DynamicModuleHandlePtr& module : m_modules)
            {
                const auto uninit = module->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
                (*uninit)();
                module.reset();
            }
            m_modules.clear();
            m_modules.shrink_to_fit();
        }

        private:
            AZStd::vector<DynamicModuleHandlePtr> m_modules;
    };
} // namespace SceneProcessing


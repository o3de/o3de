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

#include "ResourceCompilerScene_precompiled.h"
#include <IConfig.h>
#include <SceneConfig.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>

namespace AZ
{
    namespace RC
    {
        SceneConfig::SceneConfig()
        {
            LoadSceneLibrary("SceneCore");
            LoadSceneLibrary("FbxSceneBuilder"); // Still needs to be explicitly loaded in order to be able to get the supported file extensions.
        }

        SceneConfig::~SceneConfig()
        {
            // Explicitly uninitialize all modules. Because we loaded them twice, we need to explicitly uninit them.
            for (AZStd::unique_ptr<AZ::DynamicModuleHandle>& module : m_modules)
            {
                auto uninit = module->GetFunction<UninitializeDynamicModuleFunction>(UninitializeDynamicModuleFunctionName);
                if (uninit)
                {
                    (*uninit)();
                }
            }
        }

        void SceneConfig::LoadSceneLibrary(const char* name)
        {
            AZStd::unique_ptr<DynamicModuleHandle> module = AZ::DynamicModuleHandle::Create(name);
            AZ_Assert(module, "Failed to initialize library '%s'", name);
            if (!module)
            {
                return;
            }
            module->Load(false);

            // Explicitly initialize all modules. Because we're loading them twice (link time, and now-time), we need to explicitly uninit them.
            auto init = module->GetFunction<InitializeDynamicModuleFunction>(InitializeDynamicModuleFunctionName);
            if (init)
            {
                (*init)(AZ::Environment::GetInstance());
            }

            m_modules.push_back(AZStd::move(module));
        }

        size_t SceneConfig::GetErrorCount() const
        {
            return traceHook.GetErrorCount();
        }
    } // namespace RC
} // namespace AZ

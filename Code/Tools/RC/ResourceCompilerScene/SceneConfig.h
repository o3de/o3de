#pragma once

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

#include <ISceneConfig.h>
#include <TraceDrillerHook.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Module/DynamicModuleHandle.h>

namespace AZ
{
    namespace RC
    {
        class SceneConfig
            : public ISceneConfig
        {
        public:
            SceneConfig();
            ~SceneConfig() override;

            size_t GetErrorCount() const override;

        protected:
            virtual void LoadSceneLibrary(const char* name);
            
            AZStd::vector<AZStd::unique_ptr<AZ::DynamicModuleHandle>> m_modules;

            TraceDrillerHook traceHook;
        };
    } // RC
} // AZ

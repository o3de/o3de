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

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <IConvertor.h>

namespace AZ
{
    namespace RC
    {
        class ISceneConfig;
        class SceneConverter
            : public IConvertor
        {
        public:
            explicit SceneConverter(const AZStd::shared_ptr<ISceneConfig>& config);

            void Release() override;
            void Init(const ConvertorInitContext& context) override;

            ICompiler* CreateCompiler() override;
            const char* GetExt(int index) const override;

        private:
            AZStd::shared_ptr<ISceneConfig> m_config;
            AZStd::vector<AZStd::string> m_extensions;
            AZStd::string m_appRoot;
        };
    }
}

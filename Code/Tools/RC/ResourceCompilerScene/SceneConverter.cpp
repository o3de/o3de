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

#include <SceneConverter.h>
#include <SceneCompiler.h>
#include <ISceneConfig.h>
#include <AzCore/std/iterator.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace RC
    {
        SceneConverter::SceneConverter(const AZStd::shared_ptr<ISceneConfig>& config)
            : m_config(config)
        {
            AZStd::unordered_set<AZStd::string> extensions;
            EBUS_EVENT(SceneAPI::Events::AssetImportRequestBus, GetSupportedFileExtensions, extensions);
            m_extensions.reserve(extensions.size() + 1);
            AZStd::move(extensions.begin(), extensions.end(), AZStd::back_inserter(m_extensions));

            AZStd::string manifestExtension;
            EBUS_EVENT(SceneAPI::Events::AssetImportRequestBus, GetManifestExtension, manifestExtension);
            m_extensions.push_back(AZStd::move(manifestExtension));
        }

        void SceneConverter::Release()
        {
            delete this;
        }
        void SceneConverter::Init(const ConvertorInitContext& context)
        {
            m_appRoot = context.appRootPath;
        }


        ICompiler* SceneConverter::CreateCompiler()
        {
            return new SceneCompiler(m_config, m_appRoot.c_str());
        }

        const char* SceneConverter::GetExt(int index) const
        {
            if (index < m_extensions.size())
            {
                const char* result = m_extensions[index].c_str();
                return result[0] == '.' ? result + 1 : result;
            }
            else
            {
                return nullptr;
            }
        }
    }
}

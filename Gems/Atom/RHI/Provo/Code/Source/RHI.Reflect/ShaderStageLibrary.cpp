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

#include <Atom/RHI.Reflect/Provo/ShaderStageLibrary.h>
#include <Atom/RHI.Reflect/Provo/ShaderStageFunction.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Provo
    {
        void ShaderStageLibrary::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderStageLibrary, RHI::ShaderStageLibrary>()
                    ->Version(1)
                    ->Field("m_functions", &ShaderStageLibrary::m_functions);
            }
        }

        RHI::Ptr<ShaderStageLibrary> ShaderStageLibrary::Create()
        {
            return aznew ShaderStageLibrary();
        }

        ShaderStageFunction* ShaderStageLibrary::MakeFunction(const RHI::ShaderStageFunctionId& id, RHI::ShaderStage stage)
        {
            if (m_functions.find(id) == m_functions.end())
            {
                m_functions.emplace(id, stage);
            }
            return &m_functions[id];
        }

        const RHI::ShaderStageFunction* ShaderStageLibrary::FindFunctionInternal(const RHI::ShaderStageFunctionId& id) const
        {
            auto it = m_functions.find(id);
            if (it != m_functions.end())
            {
                return &it->second;
            }
            return nullptr;
        }

        RHI::ResultCode ShaderStageLibrary::FinalizeInternal()
        {
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode ShaderStageLibrary::FinalizeAfterLoadInternal()
        {
            for (auto& function : m_functions)
            {
                function.second.BindToAttributeMap(GetShaderStageAttributeMapList());
            }

            return RHI::ResultCode::Success;
        }
    }
}

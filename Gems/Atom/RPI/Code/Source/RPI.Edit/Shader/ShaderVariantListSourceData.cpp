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

#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderVariantListSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantListSourceData::VariantInfo>()
                    ->Version(1)
                    ->Field("StableId", &VariantInfo::m_stableId)
                    ->Field("Options", &VariantInfo::m_options)
                    ;

                serializeContext->Class<ShaderVariantListSourceData>()
                    ->Version(1)
                    ->Field("Shader", &ShaderVariantListSourceData::m_shaderFilePath)
                    ->Field("Variants", &ShaderVariantListSourceData::m_shaderVariants)
                    ;
            }
        }
    } // namespace RPI
} // namespace AZ

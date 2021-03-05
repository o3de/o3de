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

#include <Atom/RPI.Reflect/Shader/ShaderOutputContract.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderOutputContract::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderOutputContract>()
                    ->Version(0)
                    ->Field("m_requiredColorAttachments", &ShaderOutputContract::m_requiredColorAttachments)
                    ;

                serializeContext->Class<ColorAttachmentInfo>()
                    ->Version(0)
                    ->Field("m_componentCount", &ColorAttachmentInfo::m_componentCount)
                    ;
            }
        }

        HashValue64 ShaderOutputContract::GetHash() const
        {
            HashValue64 hash = HashValue64{ 0 };
            for (const ColorAttachmentInfo& info : m_requiredColorAttachments)
            {
                hash = TypeHash64(info.m_componentCount, hash);
            }
            return hash;
        }
    } // namespace RPI
} // namespace AZ

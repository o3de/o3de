/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

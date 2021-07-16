/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Shader/ShaderInputContract.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderInputContract::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderInputContract>()
                    ->Version(1)
                    ->Field("streamChannels", &ShaderInputContract::m_streamChannels)
                    ;

                serializeContext->Class<StreamChannelInfo>()
                    ->Version(1)
                    ->Field("semantic", &StreamChannelInfo::m_semantic)
                    ->Field("componentCount", &StreamChannelInfo::m_componentCount)
                    ->Field("isOptional", &StreamChannelInfo::m_isOptional)
                    ->Field("streamBoundIndicatorIndex", &StreamChannelInfo::m_streamBoundIndicatorIndex)
                    ;
            }
        }

        HashValue64 ShaderInputContract::GetHash() const
        {
            HashValue64 hash = HashValue64{ 0 };
            for (const StreamChannelInfo& info : m_streamChannels)
            {
                hash = info.m_semantic.GetHash(hash);
                hash = TypeHash64(info.m_componentCount, hash);
            }
            return hash;
        }
    } // namespace RPI
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/containers/vector.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>

namespace AZ
{
    namespace RPI
    {
        //! Describes the set of inputs required by a shader
        struct ATOM_RPI_REFLECT_API ShaderInputContract
        {
            AZ_TYPE_INFO(ShaderInputContract, "{7C86110E-2455-45D0-8362-C31CAF6FEE9B}");
            static void Reflect(ReflectContext* context);

            struct StreamChannelInfo
            {
                AZ_TYPE_INFO(StreamChannelInfo, "{94E66FF9-CF6D-414B-B257-BF2D39CE9220}");

                RHI::ShaderSemantic m_semantic;
                uint32_t m_componentCount = 0;                 //!< Expected number of components in the channel. Corresponds to RHI::GetFormatComponentCount(Format).
                bool m_isOptional = false;                     //!< If true, this stream is optional
                ShaderOptionIndex m_streamBoundIndicatorIndex; //!< If the stream is optional, this index indicates a "*_isBound" shader option that will tell the shader whether the stream is available or not.
            };

            AZStd::vector<StreamChannelInfo> m_streamChannels;
            HashValue64 GetHash() const;
        };
    } // namespace RPI
} // namespace AZ

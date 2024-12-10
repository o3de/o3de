/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RPI
    {
        //! Describes the set of outputs required by a shader, which the render pipeline must bind.
        struct ATOM_RPI_REFLECT_API ShaderOutputContract
        {
            AZ_TYPE_INFO(ShaderOutputContract, "{CE2A07A4-9911-4F98-AA1E-3391D80E6526}");
            static void Reflect(ReflectContext* context);

            struct ATOM_RPI_REFLECT_API ColorAttachmentInfo
            {
                AZ_TYPE_INFO(ColorAttachmentInfo, "{BDFDB9E7-1DF2-441D-BCC5-23E806F845DF}");

                uint32_t m_componentCount = 0; //!< Expected number of texel components. Corresponds to RHI::GetFormatComponentCount(Format).
            };

            AZStd::vector<ColorAttachmentInfo> m_requiredColorAttachments;

            HashValue64 GetHash() const;
        };
    } // namespace RPI
} // namespace AZ

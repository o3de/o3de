/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Utils/TypeHash.h>
#include <Atom/RHI.Reflect/Handle.h>

namespace AZ::RHI
{
    using SpecializationValue = RHI::Handle<uint32_t, struct SpecializationConstant>;

    enum class SpecializationType : uint32_t
    {
        Integer,
        Bool,
        Invalid
    };

    struct SpecializationConstant
    {
        SpecializationConstant() = default;

        Name m_name;
        uint32_t m_id = 0;
        SpecializationValue m_value;
        SpecializationType m_type = SpecializationType::Invalid;

        bool operator==(const SpecializationConstant& rhs) const;
        HashValue64 GetHash() const;
    };
}

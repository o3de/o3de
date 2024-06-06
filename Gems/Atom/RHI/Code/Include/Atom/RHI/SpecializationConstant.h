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
    //! Holds a value for a specialization constant
    using SpecializationValue = RHI::Handle<uint32_t, struct SpecializationConstant>;

    //! Supported types for specialization constants
    enum class SpecializationType : uint32_t
    {
        Integer,
        Bool,
        Invalid
    };

    //! Contains all the necessary information and value of a specialization constant
    //! so it can be used when creating a PipelineState.
    struct SpecializationConstant
    {
        SpecializationConstant() = default;

        //! Name of the constant
        Name m_name;
        //! Id of the constant
        uint32_t m_id = 0;
        //! Value of the constant
        SpecializationValue m_value;
        //! Type of the constant
        SpecializationType m_type = SpecializationType::Invalid;

        bool operator==(const SpecializationConstant& rhs) const;
        //! Returns a hash of the constant
        HashValue64 GetHash() const;
    };
}

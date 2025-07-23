/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace AZ::RHI
{
    // Use "Internal" namespace to avoid exposing "using uint..." to the AZ::RHI namespace
    namespace Internal
    {
        using uint = uint32_t;
#define IndexFormatEnumType : uint32_t
#include "../../../RHI/Assets/ShaderLib/Atom/RHI/IndexFormat.azsli"
#undef IndexFormatEnumType
    } // namespace Internal

    using IndexFormat = Internal::IndexFormat;

    //! @brief Returns the size of the given index format in bytes.
    uint32_t GetIndexFormatSize(IndexFormat indexFormat);
} // namespace AZ::RHI

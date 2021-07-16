/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace AZ::MeshBuilder
{
    template<class IndexType>
    inline static constexpr IndexType InvalidIndexT = static_cast<IndexType>(-1);

    inline static constexpr size_t InvalidIndex = InvalidIndexT<size_t>;
} // namespace AZ::MeshBuilder

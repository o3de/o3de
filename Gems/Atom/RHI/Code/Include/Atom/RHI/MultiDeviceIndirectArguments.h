/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceIndirectArguments.h>
#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceIndirectBufferView.h>

namespace AZ::RHI
{
    using MultiDeviceIndirectArguments = IndirectArgumentsTemplate<MultiDeviceBuffer, MultiDeviceIndirectBufferView>;
} // namespace AZ::RHI

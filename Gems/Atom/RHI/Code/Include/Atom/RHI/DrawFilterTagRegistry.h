/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/TagRegistry.h>
#include <Atom/RHI.Reflect/Handle.h>

namespace AZ::RHI
{
    using DrawFilterTagRegistry = TagRegistry<DrawFilterTag::IndexType, Limits::Pipeline::DrawFilterTagCountMax>;
}

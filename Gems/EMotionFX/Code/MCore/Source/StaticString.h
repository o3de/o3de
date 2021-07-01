/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MCore/Source/StaticAllocator.h>
namespace MCore
{
    using StaticString = AZStd::basic_string<char, AZStd::char_traits<char>, MCore::StaticAllocator>;
} // end namespace MCore

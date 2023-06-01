/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/IO/Path/Path.h>

#include <Compression/CompressionInterfaceStructs.h>

namespace Archive
{
    //! Token that can be used to identify a file within an archive
    enum class ArchiveFileToken : AZ::u64 {};

    static constexpr ArchiveFileToken InvalidArchiveFileToken = static_cast<ArchiveFileToken>(AZStd::numeric_limits<AZ::u64>::max());
} // namespace Archive

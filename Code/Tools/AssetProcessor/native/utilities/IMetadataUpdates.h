/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/IO/Path/Path.h>

namespace AssetProcessor
{
    //! Interface for signalling a pending file move/rename is about to occur
    struct IMetadataUpdates
    {
        AZ_RTTI(IMetadataUpdates, "{8CB894B4-DB4B-4385-BEF5-39505DD951AC}");

        AZ_DISABLE_COPY_MOVE(IMetadataUpdates);

        IMetadataUpdates() = default;
        virtual ~IMetadataUpdates() = default;

        //! Signal to AP that a file is about to be moved/renamed
        virtual void PrepareForFileMove(AZ::IO::PathView oldPath, AZ::IO::PathView newPath) = 0;
    };
}

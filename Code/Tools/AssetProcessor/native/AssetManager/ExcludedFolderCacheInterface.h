/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <QString>

namespace AssetProcessor
{
    class PlatformConfiguration;

    struct ExcludedFolderCacheInterface
    {
        AZ_RTTI(ExcludedFolderCacheInterface, "{3AC471B6-C9F8-49CF-9E9D-237BDF63328C}");
        AZ_DISABLE_COPY_MOVE(ExcludedFolderCacheInterface);

        ExcludedFolderCacheInterface() = default;
        virtual ~ExcludedFolderCacheInterface() = default;

        virtual const AZStd::unordered_set<AZStd::string>& GetExcludedFolders() = 0;
        virtual void FileAdded(QString path) = 0;
    };
}

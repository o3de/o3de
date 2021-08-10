/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "StandardHeaders.h"
#include <AzCore/std/string/string.h>
#include <AzCore/std/function/function_fwd.h>
#include <MCore/Source/StaticString.h>

namespace MCore
{
    // forward declarations
    class CommandManager;

    /**
     * The file system class.
     * This currently contains some utility classes to deal with files on disk.
     */
    class MCORE_API FileSystem
    {
    public:
        /**
         * Save to file secured by a backup file.
         * @param[in] fileName The filename of the file.
         * @param[in] saveFunction Save function used to save the file.
         * @param[in] commandManager Command manager used to add error.
         * @result True in case everything went fine, false it something wrong happened. Check log for these cases.
         */
        static bool SaveToFileSecured(const char* filename, const AZStd::function<bool()>& saveFunction, CommandManager* commandManager = nullptr);

        static StaticString     s_secureSavePath;        /**< The folder path used to keep a backup in SaveToFileSecured. */
    };
} // namespace MCore

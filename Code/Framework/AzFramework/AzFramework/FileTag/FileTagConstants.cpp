/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/FileTag/FileTagConstants.h>

namespace AzFramework
{
    namespace FileTag
    {
        const char* ExcludeFileName = "exclude";
        const char* IncludeFileName = "include";
        const char* FileTags[] = { "ignore", "error", "productdependency", "editoronly", "shader" };
    } // namespace FileTag
} // namespace AzFramework

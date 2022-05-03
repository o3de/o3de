/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>

namespace AWSCore
{
    namespace Platform
    {
        AZ::IO::Path GetJsonSchemaPath()
        {
            static constexpr const char ResourceMapppingJsonSchemaFilePath[] =
                "@products@/resource_mapping_schema.json";

            char resolvedSchemaPath[AZ_MAX_PATH_LEN] = { 0 };
            if (!AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(ResourceMapppingJsonSchemaFilePath, resolvedSchemaPath, AZ_MAX_PATH_LEN))
            {
                return "";
            }

            return resolvedSchemaPath;
        }
    } // namespace Platform
}

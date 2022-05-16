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

            AZ::IO::FixedMaxPath resolvedSchemaPath;
            if (!AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(resolvedSchemaPath, ResourceMapppingJsonSchemaFilePath))
            {
                return AZ::IO::Path{};
            }

            return resolvedSchemaPath.String();
        }
    } // namespace Platform
}

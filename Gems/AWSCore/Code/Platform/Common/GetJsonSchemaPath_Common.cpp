/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Utils/Utils.h>

namespace AWSCore
{
    namespace Platform
    {
        AZ::IO::Path GetJsonSchemaPath()
        {
            static constexpr const char ResourceMapppingJsonSchemaFilePath[] =
                "Gems/AWSCore/resource_mapping_schema.json";

            AZ::IO::Path executablePath = AZ::IO::PathView(AZ::Utils::GetExecutableDirectory());
            AZ::IO::Path jsonSchemaPath = (executablePath / ResourceMapppingJsonSchemaFilePath).LexicallyNormal();

            return jsonSchemaPath;
        }
    } // namespace Platform
} // namespace AWSCore

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

namespace AWSCore
{
    namespace AWSResourceMappingUtils
    {
        AZStd::string FormatRESTApiUrl(
            const AZStd::string& restApiId, const AZStd::string& restApiRegion, const AZStd::string& restApiStage);
    } // namespace AWSResourceMappingUtils
} // namespace AWSCore

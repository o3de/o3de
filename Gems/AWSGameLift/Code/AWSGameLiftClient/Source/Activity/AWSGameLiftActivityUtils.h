/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/gamelift/model/GameProperty.h>

namespace AWSGameLift
{
    namespace AWSGameLiftActivityUtils
    {
        void GetGameProperties(
            const AZStd::unordered_map<AZStd::string, AZStd::string>& sessionProperties,
            Aws::Vector<Aws::GameLift::Model::GameProperty>& outGameProperties,
            AZStd::string& outGamePropertiesOutput);
    } // namespace AWSGameLiftActivityUtils
} // namespace AWSGameLift

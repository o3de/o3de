/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AzToolsFramework
{
    namespace Python
    {
        AZStd::string GetPythonVenvPath(AZStd::string_view engineRoot);

        AZStd::string GetPythonHomePath(AZStd::string_view engineRoot);

        void ReadPythonEggLinkPaths(AZStd::string_view engineRoot, AZStd::vector<AZStd::string>& resultPaths);

    } // namespace Python
} // namespace AzToolsFramework

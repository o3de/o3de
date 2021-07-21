/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>

#include "TranslationResult.h"

namespace ScriptCanvas
{
    class Graph;

    namespace Translation
    {
        AZ::Outcome<Grammar::AbstractCodeModelConstPtr, AZStd::string> ParseGraph(const Grammar::Request& request);

        Result ParseAndTranslateGraph(const Grammar::Request& request);

        Result ToCPlusPlus(const Grammar::Request& request);

        Result ToCPlusPlusAndLua(const Grammar::Request& request);

        Result ToLua(const Grammar::Request& request);

    }
}

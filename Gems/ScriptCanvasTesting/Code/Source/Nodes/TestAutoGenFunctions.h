/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/tuple.h>
#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvasTesting
{
    namespace TestAutoGenFunctions
    {
        float NoArgsReturn();

        AZStd::tuple<AZStd::string, bool> ArgsReturnMulti(double input);

        AZStd::tuple<AZStd::string, bool> NoArgsReturnMulti();

        int MaxReturnByValueInteger(int lhs, int rhs);

        const int* MaxReturnByPointerInteger(const int* lhs, const int* rhs);

        const int& MaxReturnByReferenceInteger(const int& lhs, const int& rhs);

        bool IsPositive(int input);

        bool NegateBranchBooleanNoResult(int input);

        int NegateBranchNonBooleanWithResult(int input);
    } // namespace TestAutoGenFunctions
} // namespace ScriptCanvasTesting

#include "Source/Nodes/TestAutoGenFunctions.generated.h"

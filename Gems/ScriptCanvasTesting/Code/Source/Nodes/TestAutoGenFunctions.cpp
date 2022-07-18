/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestAutoGenFunctions.h"

namespace ScriptCanvasTesting
{
    namespace TestAutoGenFunctions
    {
        float NoArgsReturn()
        {
            return 0.0f;
        }

        AZStd::tuple<AZStd::string, bool> ArgsReturnMulti(double input)
        {
            return input >= 0.0 ? AZStd::make_tuple("positive", true) : AZStd::make_tuple("negative", false);
        }

        AZStd::tuple<AZStd::string, bool> NoArgsReturnMulti()
        {
            return AZStd::make_tuple("no-args", false);
        }

        int MaxReturnByValueInteger(int lhs, int rhs)
        {
            return lhs >= rhs ? lhs : rhs;
        }

        const int* MaxReturnByPointerInteger(const int* lhs, const int* rhs)
        {
            return (lhs && rhs && (*lhs) >= (*rhs)) ? lhs : rhs;
        }

        const int& MaxReturnByReferenceInteger(const int& lhs, const int& rhs)
        {
            return lhs >= rhs ? lhs : rhs;
        }

        bool IsPositive(int input)
        {
            return input > 0;
        }

        bool NegateBranchBooleanNoResult(int input)
        {
            return -1 * input > 0;
        }

        int NegateBranchNonBooleanWithResult(int input)
        {
            return -1 * input;
        }
    } // namespace TestAutoGenFunctions
} // namespace ScriptCanvasTesting

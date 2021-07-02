/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/std/any.h>

namespace UnitTest
{
    template<typename T>
    void ExpectEqAny(const T& expected, const AZStd::any& anyValue, const char* description = nullptr)
    {
        if (anyValue.empty())
        {
            EXPECT_TRUE(false) << (description?description:"Value") << " is empty";
        }
        else if (!anyValue.is<T>())
        {
            EXPECT_TRUE(false) << (description?description:"Value") << " is not of the expected type. Expected " << azrtti_typeid<T>().template ToString<AZStd::string>().data() << " but was " << anyValue.get_type_info().m_id.ToString<AZStd::string>().data();
        }
        else
        {
            T value = AZStd::any_cast<T>(anyValue);
            EXPECT_EQ(expected, value) << (description?description:"");
        }
    }

    template<typename T>
    void ExpectEqAny(const AZStd::any& anyValue, const T& expected, const char* description = nullptr)
    {
        return ExpectEqAny(expected, anyValue, description);
    }
}

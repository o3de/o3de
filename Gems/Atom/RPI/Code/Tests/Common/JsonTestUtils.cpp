/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Common/JsonTestUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <gtest/gtest.h>

namespace UnitTest
{
    bool JsonTestResult::ContainsOutcome(AZ::JsonSerializationResult::Outcomes outcome)
    {
        for (auto& reportList : m_reports)
        {
            for (auto& report : reportList.second)
            {
                if (report.resultCode.GetOutcome() == outcome)
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool JsonTestResult::ContainsMessage(AZStd::string_view jsonFieldPath, AZStd::string_view messageSubstring)
    {
        auto pathIter = m_reports.find(jsonFieldPath);
        if (pathIter == m_reports.end())
        {
            return false;
        }

        for (auto& report : pathIter->second)
        {
            if (AzFramework::StringFunc::Find(report.message, messageSubstring) != AZStd::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    void ExpectSimilarJson(AZStd::string_view a, AZStd::string_view b)
    {
        AZStd::string aStripped = a;
        AZStd::string bStripped = b;
        AzFramework::StringFunc::Strip(aStripped, " \t\r\n");
        AzFramework::StringFunc::Strip(bStripped, " \t\r\n");
        EXPECT_STRCASEEQ(aStripped.c_str(), bStripped.c_str());
    }
}

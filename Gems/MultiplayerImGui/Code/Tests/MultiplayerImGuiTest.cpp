/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <ByteReporter.h>

class MultiplayerImGuiTest
    : public ::UnitTest::AllocatorsTestFixture
{
protected:
    void SetUp() override
    {
        ::UnitTest::AllocatorsTestFixture::SetUp();
    }

    void TearDown() override
    {
        ::UnitTest::AllocatorsTestFixture::TearDown();
    }
};

TEST_F(MultiplayerImGuiTest, Two_Fields_Test)
{
    using namespace MultiplayerDiagnostics;

    EntityReporter baseline;
    {
        EntityReporter reporter;
        reporter.ReportField(1, "component 1", "field 1", 1);
        reporter.ReportField(1, "component 1", "field 2", 1);
        reporter.ReportField(1, "component 1", "field 1", 1);

        baseline.Combine( reporter );
    }

    const AZStd::map<AZStd::string, ComponentReporter>& reports = baseline.GetComponentReports();
    ASSERT_TRUE( reports.size() == 1 );
    for (auto& itemReport : reports)
    {
        ComponentReporter reportForComponent = itemReport.second;

        AZStd::vector<ComponentReporter::Report> report = reportForComponent.GetFieldReports();
        ASSERT_TRUE( report.size() == 2 );

        AZStd::size_t reportIndex = 0;
        for ( const AZStd::pair<AZStd::string, ByteReporter*>& fieldReport : report)
        {
            const AZStd::string& fieldKey = fieldReport.first;
            const ByteReporter* fieldStats = fieldReport.second;

            if (reportIndex == 0)
            {
                ASSERT_TRUE( fieldKey == "field 1" );
                ASSERT_EQ( fieldStats->GetTotalBytes(), 2 );
            }
            else if (reportIndex == 1)
            {
                ASSERT_TRUE( fieldKey == "field 2" );
                ASSERT_EQ( fieldStats->GetTotalBytes(), 1 );
            }
            ++reportIndex;
        }
    }
}

TEST_F(MultiplayerImGuiTest, Two_Components_Test)
{
    using namespace MultiplayerDiagnostics;

    EntityReporter baseline;
    {
        EntityReporter reporter;
        reporter.ReportField(1, "component 1", "field 1", 1);
        reporter.ReportFragmentEnd();
        reporter.ReportField(2, "component 2", "field 1", 2);
        baseline.Combine( reporter );
    }

    AZStd::size_t reportIndex = 0;
    const AZStd::map<AZStd::string, ComponentReporter>& reports = baseline.GetComponentReports();
    ASSERT_TRUE( reports.size() == 2 );
    for (auto& itemReport : reports)
    {
        ComponentReporter reportForComponent = itemReport.second;

        const AZStd::vector<ComponentReporter::Report>& fieldReports = reportForComponent.GetFieldReports();

        ASSERT_TRUE( fieldReports.size() == 1 );

        const AZStd::pair<AZStd::string, ByteReporter*>& fieldReport = fieldReports[0];
        {
            const AZStd::string& fieldKey = fieldReport.first;
            const ByteReporter* fieldStats = fieldReport.second;

            if (reportIndex == 0) // for the first component
            {
                ASSERT_TRUE( fieldKey == "field 1" );
                ASSERT_EQ( fieldStats->GetTotalBytes(), 1 );
            }
            else if (reportIndex == 1) // for the second component
            {
                ASSERT_TRUE( fieldKey == "field 1" );
                ASSERT_EQ( fieldStats->GetTotalBytes(), 2 );
            }
        }

        ++reportIndex;
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

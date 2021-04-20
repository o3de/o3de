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

#include <TestImpactTestUtils.h>

#include <Artifact/Factory/TestImpactModuleCoverageFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    TEST(CoberturaModuleCoveragesFactory, ParseEmptyString_ThrowsArtifactException)
    {
        // Given an empty string
        const AZStd::string rawCoverage;

        try
        {
            // When attempting to parse the empty coverage
            const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

            // Do not expect the parsing to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect an artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(CoberturaModuleCoveragesFactory, ParseInvalidString_ThrowsArtifactException)
    {
        // Given an invalid string
        const AZStd::string rawCoverage = "!@?";

        try
        {
            // When attempting to parse the invalid coverage
            const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

            // Do not expect the parsing to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect an artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(CoberturaModuleCoveragesFactory, ParseEmptyCoverage_ExpectEmptyModuleCoverages)
    {
        // Given an empty coverage string
        const AZStd::string rawCoverage =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617713965\" lines-covered=\"0\" lines-valid=\"0\" version=\"0\">"
            "  <sources/>"
            "  <packages/>"
            "</coverage>"
            "";

        // When attempting to parse the empty coverage
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect an empty module coverages
        EXPECT_TRUE(coverage.empty());
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetLineCoverageA_ReturnsValidLineCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetALineModuleCoverages();

        // Given the raw line coverage output of TestTargetA
        const AZStd::string rawCoverage =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617124634\" lines-covered=\"41\" lines-valid=\"41\" version=\"0\">"
            "  <sources>"
            "    <source>C:</source>"
            "  </sources>"
            "  <packages>"
            "    <package name=\"C:\\Lumberyard\\windows_vs2019\\bin\\debug\\TestImpact.TestTargetA.Tests.dll\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "      <classes>"
            "        <class name=\"TestImpactTestTargetA.cpp\" filename=\"Lumberyard\\Code\\Tools\\TestImpactFramework\\Runtime\\Code\\Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "          <methods/>"
            "          <lines>"
            "            <line number=\"22\" hits=\"1\"/>"
            "            <line number=\"23\" hits=\"1\"/>"
            "            <line number=\"24\" hits=\"1\"/>"
            "            <line number=\"25\" hits=\"1\"/>"
            "            <line number=\"27\" hits=\"1\"/>"
            "            <line number=\"28\" hits=\"1\"/>"
            "            <line number=\"29\" hits=\"1\"/>"
            "            <line number=\"30\" hits=\"1\"/>"
            "            <line number=\"32\" hits=\"1\"/>"
            "            <line number=\"33\" hits=\"1\"/>"
            "            <line number=\"34\" hits=\"1\"/>"
            "            <line number=\"35\" hits=\"1\"/>"
            "            <line number=\"37\" hits=\"1\"/>"
            "            <line number=\"38\" hits=\"1\"/>"
            "            <line number=\"39\" hits=\"1\"/>"
            "            <line number=\"40\" hits=\"1\"/>"
            "            <line number=\"42\" hits=\"1\"/>"
            "            <line number=\"43\" hits=\"1\"/>"
            "            <line number=\"44\" hits=\"1\"/>"
            "            <line number=\"45\" hits=\"1\"/>"
            "            <line number=\"47\" hits=\"1\"/>"
            "            <line number=\"48\" hits=\"1\"/>"
            "            <line number=\"49\" hits=\"1\"/>"
            "            <line number=\"50\" hits=\"1\"/>"
            "            <line number=\"52\" hits=\"1\"/>"
            "            <line number=\"53\" hits=\"1\"/>"
            "            <line number=\"54\" hits=\"1\"/>"
            "            <line number=\"55\" hits=\"1\"/>"
            "            <line number=\"57\" hits=\"1\"/>"
            "            <line number=\"58\" hits=\"1\"/>"
            "            <line number=\"59\" hits=\"1\"/>"
            "            <line number=\"60\" hits=\"1\"/>"
            "            <line number=\"62\" hits=\"1\"/>"
            "            <line number=\"63\" hits=\"1\"/>"
            "            <line number=\"64\" hits=\"1\"/>"
            "            <line number=\"65\" hits=\"1\"/>"
            "            <line number=\"67\" hits=\"1\"/>"
            "            <line number=\"68\" hits=\"1\"/>"
            "            <line number=\"69\" hits=\"1\"/>"
            "            <line number=\"70\" hits=\"1\"/>"
            "            <line number=\"73\" hits=\"1\"/>"
            "          </lines>"
            "        </class>"
            "      </classes>"
            "    </package>"
            "  </packages>"
            "</coverage>"
            "";

        // When the raw line coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetSourceCoverageA_ReturnsValidSourceCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetASourceModuleCoverages();

        // Given the raw source coverage output of TestTargetA
        const AZStd::string rawCoverage =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617117760\" lines-covered=\"0\" lines-valid=\"0\" version=\"0\">"
            "  <sources>"
            "    <source>C:</source>"
            "  </sources>"
            "  <packages>"
            "    <package name=\"C:\\Lumberyard\\windows_vs2019\\bin\\debug\\TestImpact.TestTargetA.Tests.dll\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "      <classes>"
            "        <class name=\"TestImpactTestTargetA.cpp\" filename=\"Lumberyard\\Code\\Tools\\TestImpactFramework\\Runtime\\Code\\Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "          <methods/>"
            "          <lines/>"
            "        </class>"
            "      </classes>"
            "    </package>"
            "  </packages>"
            "</coverage>"
            "";

        // When the raw source coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetLineCoverageB_ReturnsValidLineCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetBLineModuleCoverages();

        // Given the raw line coverage output of TestTargetB
        const AZStd::string rawCoverage =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617124605\" lines-covered=\"29\" lines-valid=\"29\" version=\"0\">"
            "  <sources>"
            "    <source>C:</source>"
            "  </sources>"
            "  <packages>"
            "    <package name=\"C:\\Lumberyard\\windows_vs2019\\bin\\debug\\TestImpact.TestTargetB.Tests.dll\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "      <classes>"
            "        <class name=\"TestImpactTestTargetB.cpp\" filename=\"Lumberyard\\Code\\Tools\\TestImpactFramework\\Runtime\\Code\\Tests\\TestTargetB\\Code\\Tests\\TestImpactTestTargetB.cpp\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "          <methods/>"
            "          <lines>"
            "            <line number=\"29\" hits=\"1\"/>"
            "            <line number=\"30\" hits=\"1\"/>"
            "            <line number=\"31\" hits=\"1\"/>"
            "            <line number=\"32\" hits=\"1\"/>"
            "            <line number=\"34\" hits=\"1\"/>"
            "            <line number=\"35\" hits=\"1\"/>"
            "            <line number=\"36\" hits=\"1\"/>"
            "            <line number=\"37\" hits=\"1\"/>"
            "            <line number=\"39\" hits=\"1\"/>"
            "            <line number=\"40\" hits=\"1\"/>"
            "            <line number=\"41\" hits=\"1\"/>"
            "            <line number=\"42\" hits=\"1\"/>"
            "            <line number=\"44\" hits=\"1\"/>"
            "            <line number=\"45\" hits=\"1\"/>"
            "            <line number=\"46\" hits=\"1\"/>"
            "            <line number=\"47\" hits=\"1\"/>"
            "            <line number=\"49\" hits=\"1\"/>"
            "            <line number=\"50\" hits=\"1\"/>"
            "            <line number=\"51\" hits=\"1\"/>"
            "            <line number=\"52\" hits=\"1\"/>"
            "            <line number=\"54\" hits=\"1\"/>"
            "            <line number=\"55\" hits=\"1\"/>"
            "            <line number=\"56\" hits=\"1\"/>"
            "            <line number=\"57\" hits=\"1\"/>"
            "            <line number=\"59\" hits=\"1\"/>"
            "            <line number=\"66\" hits=\"1\"/>"
            "            <line number=\"68\" hits=\"1\"/>"
            "            <line number=\"75\" hits=\"1\"/>"
            "            <line number=\"78\" hits=\"1\"/>"
            "          </lines>"
            "        </class>"
            "      </classes>"
            "    </package>"
            "  </packages>"
            "</coverage>"
            "";

        // When the raw line coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetSourceCoverageB_ReturnsValidSourceCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetBSourceModuleCoverages();

        // Given the raw source coverage output of TestTargetB
        const AZStd::string rawCoverage =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617117785\" lines-covered=\"0\" lines-valid=\"0\" version=\"0\">"
            "  <sources>"
            "    <source>C:</source>"
            "  </sources>"
            "  <packages>"
            "    <package name=\"C:\\Lumberyard\\windows_vs2019\\bin\\debug\\TestImpact.TestTargetB.Tests.dll\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "      <classes>"
            "        <class name=\"TestImpactTestTargetB.cpp\" filename=\"Lumberyard\\Code\\Tools\\TestImpactFramework\\Runtime\\Code\\Tests\\TestTargetB\\Code\\Tests\\TestImpactTestTargetB.cpp\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "          <methods/>"
            "          <lines/>"
            "        </class>"
            "      </classes>"
            "    </package>"
            "  </packages>"
            "</coverage>"
            "";

        // When the raw source coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetLineCoverageC_ReturnsValidLineCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetCLineModuleCoverages();

        // Given the raw line coverage output of TestTargetC
        const AZStd::string rawCoverage =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617124593\" lines-covered=\"25\" lines-valid=\"25\" version=\"0\">"
            "  <sources>"
            "    <source>C:</source>"
            "  </sources>"
            "  <packages>"
            "    <package name=\"C:\\Lumberyard\\windows_vs2019\\bin\\debug\\TestImpact.TestTargetC.Tests.dll\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "      <classes>"
            "        <class name=\"TestImpactTestTargetC.cpp\" filename=\"Lumberyard\\Code\\Tools\\TestImpactFramework\\Runtime\\Code\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "          <methods/>"
            "          <lines>"
            "            <line number=\"32\" hits=\"1\"/>"
            "            <line number=\"33\" hits=\"1\"/>"
            "            <line number=\"34\" hits=\"1\"/>"
            "            <line number=\"35\" hits=\"1\"/>"
            "            <line number=\"37\" hits=\"1\"/>"
            "            <line number=\"38\" hits=\"1\"/>"
            "            <line number=\"39\" hits=\"1\"/>"
            "            <line number=\"40\" hits=\"1\"/>"
            "            <line number=\"42\" hits=\"1\"/>"
            "            <line number=\"43\" hits=\"1\"/>"
            "            <line number=\"44\" hits=\"1\"/>"
            "            <line number=\"45\" hits=\"1\"/>"
            "            <line number=\"47\" hits=\"1\"/>"
            "            <line number=\"48\" hits=\"1\"/>"
            "            <line number=\"49\" hits=\"1\"/>"
            "            <line number=\"50\" hits=\"1\"/>"
            "            <line number=\"52\" hits=\"1\"/>"
            "            <line number=\"53\" hits=\"1\"/>"
            "            <line number=\"54\" hits=\"1\"/>"
            "            <line number=\"55\" hits=\"1\"/>"
            "            <line number=\"57\" hits=\"1\"/>"
            "            <line number=\"58\" hits=\"1\"/>"
            "            <line number=\"59\" hits=\"1\"/>"
            "            <line number=\"60\" hits=\"1\"/>"
            "            <line number=\"63\" hits=\"1\"/>"
            "          </lines>"
            "        </class>"
            "      </classes>"
            "    </package>"
            "  </packages>"
            "</coverage>"
            "";

        // When the raw line coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetSourceCoverageC_ReturnsValidSourceCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetCSourceModuleCoverages();

        // Given the raw source coverage output of TestTargetC
        const AZStd::string rawCoverage =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617117796\" lines-covered=\"0\" lines-valid=\"0\" version=\"0\">"
            "  <sources>"
            "    <source>C:</source>"
            "  </sources>"
            "  <packages>"
            "    <package name=\"C:\\Lumberyard\\windows_vs2019\\bin\\debug\\TestImpact.TestTargetC.Tests.dll\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "      <classes>"
            "        <class name=\"TestImpactTestTargetC.cpp\" filename=\"Lumberyard\\Code\\Tools\\TestImpactFramework\\Runtime\\Code\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "          <methods/>"
            "          <lines/>"
            "        </class>"
            "      </classes>"
            "    </package>"
            "  </packages>"
            "</coverage>"
            "";

        // When the raw source coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetLineCoverageD_ReturnsValidLineCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetDLineModuleCoverages();

        // Given the raw line coverage output of TestTargetD
        const AZStd::string rawCoverage =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<coverage line-rate=\"0.6470588235294118\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617124579\" lines-covered=\"55\" lines-valid=\"85\" version=\"0\">"
            "  <sources>"
            "    <source>C:</source>"
            "  </sources>"
            "  <packages>"
            "    <package name=\"C:\\Lumberyard\\windows_vs2019\\bin\\debug\\TestImpact.TestTargetD.Tests.dll\" line-rate=\"0.6470588235294118\" branch-rate=\"0\" complexity=\"0\">"
            "      <classes>"
            "        <class name=\"TestImpactTestTargetD.cpp\" filename=\"Lumberyard\\Code\\Tools\\TestImpactFramework\\Runtime\\Code\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line-rate=\"0.6470588235294118\" branch-rate=\"0\" complexity=\"0\">"
            "          <methods/>"
            "          <lines>"
            "            <line number=\"56\" hits=\"1\"/>"
            "            <line number=\"57\" hits=\"1\"/>"
            "            <line number=\"58\" hits=\"1\"/>"
            "            <line number=\"59\" hits=\"1\"/>"
            "            <line number=\"61\" hits=\"1\"/>"
            "            <line number=\"62\" hits=\"0\"/>"
            "            <line number=\"63\" hits=\"0\"/>"
            "            <line number=\"64\" hits=\"0\"/>"
            "            <line number=\"66\" hits=\"1\"/>"
            "            <line number=\"67\" hits=\"1\"/>"
            "            <line number=\"68\" hits=\"1\"/>"
            "            <line number=\"69\" hits=\"1\"/>"
            "            <line number=\"71\" hits=\"1\"/>"
            "            <line number=\"72\" hits=\"1\"/>"
            "            <line number=\"73\" hits=\"1\"/>"
            "            <line number=\"74\" hits=\"1\"/>"
            "            <line number=\"76\" hits=\"1\"/>"
            "            <line number=\"77\" hits=\"1\"/>"
            "            <line number=\"78\" hits=\"1\"/>"
            "            <line number=\"79\" hits=\"1\"/>"
            "            <line number=\"81\" hits=\"1\"/>"
            "            <line number=\"82\" hits=\"1\"/>"
            "            <line number=\"83\" hits=\"1\"/>"
            "            <line number=\"84\" hits=\"1\"/>"
            "            <line number=\"86\" hits=\"1\"/>"
            "            <line number=\"87\" hits=\"1\"/>"
            "            <line number=\"88\" hits=\"1\"/>"
            "            <line number=\"89\" hits=\"1\"/>"
            "            <line number=\"91\" hits=\"1\"/>"
            "            <line number=\"92\" hits=\"0\"/>"
            "            <line number=\"93\" hits=\"0\"/>"
            "            <line number=\"94\" hits=\"0\"/>"
            "            <line number=\"96\" hits=\"1\"/>"
            "            <line number=\"97\" hits=\"0\"/>"
            "            <line number=\"98\" hits=\"0\"/>"
            "            <line number=\"99\" hits=\"0\"/>"
            "            <line number=\"101\" hits=\"1\"/>"
            "            <line number=\"102\" hits=\"1\"/>"
            "            <line number=\"103\" hits=\"1\"/>"
            "            <line number=\"104\" hits=\"1\"/>"
            "            <line number=\"106\" hits=\"1\"/>"
            "            <line number=\"107\" hits=\"0\"/>"
            "            <line number=\"108\" hits=\"0\"/>"
            "            <line number=\"109\" hits=\"0\"/>"
            "            <line number=\"111\" hits=\"1\"/>"
            "            <line number=\"112\" hits=\"0\"/>"
            "            <line number=\"113\" hits=\"0\"/>"
            "            <line number=\"114\" hits=\"0\"/>"
            "            <line number=\"116\" hits=\"1\"/>"
            "            <line number=\"117\" hits=\"0\"/>"
            "            <line number=\"118\" hits=\"0\"/>"
            "            <line number=\"119\" hits=\"0\"/>"
            "            <line number=\"121\" hits=\"1\"/>"
            "            <line number=\"128\" hits=\"1\"/>"
            "            <line number=\"130\" hits=\"1\"/>"
            "            <line number=\"137\" hits=\"1\"/>"
            "            <line number=\"139\" hits=\"1\"/>"
            "            <line number=\"146\" hits=\"1\"/>"
            "            <line number=\"148\" hits=\"1\"/>"
            "            <line number=\"155\" hits=\"1\"/>"
            "            <line number=\"157\" hits=\"1\"/>"
            "            <line number=\"158\" hits=\"1\"/>"
            "            <line number=\"159\" hits=\"1\"/>"
            "            <line number=\"160\" hits=\"1\"/>"
            "            <line number=\"162\" hits=\"1\"/>"
            "            <line number=\"163\" hits=\"0\"/>"
            "            <line number=\"164\" hits=\"0\"/>"
            "            <line number=\"165\" hits=\"0\"/>"
            "            <line number=\"167\" hits=\"1\"/>"
            "            <line number=\"168\" hits=\"1\"/>"
            "            <line number=\"169\" hits=\"1\"/>"
            "            <line number=\"170\" hits=\"1\"/>"
            "            <line number=\"172\" hits=\"1\"/>"
            "            <line number=\"173\" hits=\"0\"/>"
            "            <line number=\"174\" hits=\"0\"/>"
            "            <line number=\"175\" hits=\"0\"/>"
            "            <line number=\"177\" hits=\"1\"/>"
            "            <line number=\"178\" hits=\"0\"/>"
            "            <line number=\"179\" hits=\"0\"/>"
            "            <line number=\"180\" hits=\"0\"/>"
            "            <line number=\"182\" hits=\"1\"/>"
            "            <line number=\"183\" hits=\"0\"/>"
            "            <line number=\"184\" hits=\"0\"/>"
            "            <line number=\"185\" hits=\"0\"/>"
            "            <line number=\"188\" hits=\"1\"/>"
            "          </lines>"
            "        </class>"
            "      </classes>"
            "    </package>"
            "  </packages>"
            "</coverage>"
            "";

        // When the raw line coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetSourceCoverageD_ReturnsValidSourceCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetDSourceModuleCoverages();

        // Given the raw source coverage output of TestTargetD
        const AZStd::string rawCoverage =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617117804\" lines-covered=\"0\" lines-valid=\"0\" version=\"0\">"
            "  <sources>"
            "    <source>C:</source>"
            "  </sources>"
            "  <packages>"
            "    <package name=\"C:\\Lumberyard\\windows_vs2019\\bin\\debug\\TestImpact.TestTargetD.Tests.dll\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "      <classes>"
            "        <class name=\"TestImpactTestTargetD.cpp\" filename=\"Lumberyard\\Code\\Tools\\TestImpactFramework\\Runtime\\Code\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">"
            "          <methods/>"
            "          <lines/>"
            "        </class>"
            "      </classes>"
            "    </package>"
            "  </packages>"
            "</coverage>"
            "";

        // When the raw source coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }
} // namespace UnitTest

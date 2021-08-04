/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        const auto binPath = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_A_BIN);
        AZStd::string root = binPath.RootName().Native();

        const auto srcPath = TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
            / "Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp");
        AZStd::string srcRelativePath = srcPath.RelativePath().Native();

        // Given the raw line coverage output of TestTargetA
        AZStd::string rawCoverage;
        rawCoverage += "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
        rawCoverage += "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617124634\" lines-covered=\"41\" lines-valid=\"41\" version=\"0\">";
        rawCoverage += "  <sources>";
        rawCoverage += AZStd::string::format("    <source>%s</source>", root.c_str());
        rawCoverage += "  </sources>";
        rawCoverage += "  <packages>";
        rawCoverage += AZStd::string::format("    <package name=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", binPath.c_str());
        rawCoverage += "      <classes>";
        rawCoverage += AZStd::string::format("        <class name=\"TestImpactTestTargetA.cpp\" filename=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", srcRelativePath.c_str());
        rawCoverage += "          <methods/>";
        rawCoverage += "          <lines>";
        rawCoverage += "            <line number=\"22\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"23\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"24\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"25\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"27\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"28\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"29\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"30\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"32\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"33\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"34\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"35\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"37\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"38\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"39\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"40\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"42\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"43\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"44\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"45\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"47\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"48\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"49\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"50\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"52\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"53\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"54\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"55\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"57\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"58\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"59\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"60\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"62\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"63\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"64\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"65\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"67\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"68\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"69\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"70\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"73\" hits=\"1\"/>";
        rawCoverage += "          </lines>";
        rawCoverage += "        </class>";
        rawCoverage += "      </classes>";
        rawCoverage += "    </package>";
        rawCoverage += "  </packages>";
        rawCoverage += "</coverage>";

        // When the raw line coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetSourceCoverageA_ReturnsValidSourceCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetASourceModuleCoverages();

        const auto binPath = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_A_BIN);
        AZStd::string root = binPath.RootName().Native();

        const auto srcPath = TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
            / "Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp");
        AZStd::string srcRelativePath = srcPath.RelativePath().Native();

        // Given the raw source coverage output of TestTargetA
        AZStd::string rawCoverage;
        rawCoverage += "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
        rawCoverage += "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617117760\" lines-covered=\"0\" lines-valid=\"0\" version=\"0\">";
        rawCoverage += "  <sources>";
        rawCoverage += AZStd::string::format("    <source>%s</source>", root.c_str());
        rawCoverage += "  </sources>";
        rawCoverage += "  <packages>";
        rawCoverage += AZStd::string::format("    <package name=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", binPath.c_str());
        rawCoverage += "      <classes>";
        rawCoverage += AZStd::string::format("        <class name=\"TestImpactTestTargetA.cpp\" filename=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", srcRelativePath.c_str());
        rawCoverage += "          <methods/>";
        rawCoverage += "          <lines/>";
        rawCoverage += "        </class>";
        rawCoverage += "      </classes>";
        rawCoverage += "    </package>";
        rawCoverage += "  </packages>";
        rawCoverage += "</coverage>";

        // When the raw source coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetLineCoverageB_ReturnsValidLineCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetBLineModuleCoverages();

        const auto binPath = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_B_BIN);
        AZStd::string root = binPath.RootName().Native();

        const auto srcPath = TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
            / "Tests\\TestTargetB\\Code\\Tests\\TestImpactTestTargetB.cpp");
        AZStd::string srcRelativePath = srcPath.RelativePath().Native();

        // Given the raw line coverage output of TestTargetB
        AZStd::string rawCoverage;
        rawCoverage += "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
        rawCoverage += "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617124605\" lines-covered=\"29\" lines-valid=\"29\" version=\"0\">";
        rawCoverage += "  <sources>";
        rawCoverage += AZStd::string::format("    <source>%s</source>", root.c_str());
        rawCoverage += "  </sources>";
        rawCoverage += "  <packages>";
        rawCoverage += AZStd::string::format("    <package name=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", binPath.c_str());
        rawCoverage += "      <classes>";
        rawCoverage += AZStd::string::format("        <class name=\"TestImpactTestTargetB.cpp\" filename=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", srcRelativePath.c_str());
        rawCoverage += "          <methods/>";
        rawCoverage += "          <lines>";
        rawCoverage += "            <line number=\"29\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"30\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"31\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"32\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"34\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"35\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"36\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"37\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"39\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"40\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"41\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"42\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"44\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"45\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"46\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"47\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"49\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"50\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"51\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"52\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"54\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"55\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"56\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"57\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"59\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"66\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"68\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"75\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"78\" hits=\"1\"/>";
        rawCoverage += "          </lines>";
        rawCoverage += "        </class>";
        rawCoverage += "      </classes>";
        rawCoverage += "    </package>";
        rawCoverage += "  </packages>";
        rawCoverage += "</coverage>";

        // When the raw line coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetSourceCoverageB_ReturnsValidSourceCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetBSourceModuleCoverages();

        const auto binPath = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_B_BIN);
        AZStd::string root = binPath.RootName().Native();

        const auto srcPath = TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
            / "Tests\\TestTargetB\\Code\\Tests\\TestImpactTestTargetB.cpp");
        AZStd::string srcRelativePath = srcPath.RelativePath().Native();

        // Given the raw source coverage output of TestTargetB
        AZStd::string rawCoverage;
        rawCoverage += "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
        rawCoverage += "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617117785\" lines-covered=\"0\" lines-valid=\"0\" version=\"0\">";
        rawCoverage += "  <sources>";
        rawCoverage += AZStd::string::format("    <source>%s</source>", root.c_str());
        rawCoverage += "  </sources>";
        rawCoverage += "  <packages>";
        rawCoverage += AZStd::string::format("    <package name=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", binPath.c_str());
        rawCoverage += "      <classes>";
        rawCoverage += AZStd::string::format("        <class name=\"TestImpactTestTargetB.cpp\" filename=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", srcRelativePath.c_str());
        rawCoverage += "          <methods/>";
        rawCoverage += "          <lines/>";
        rawCoverage += "        </class>";
        rawCoverage += "      </classes>";
        rawCoverage += "    </package>";
        rawCoverage += "  </packages>";
        rawCoverage += "</coverage>";

        // When the raw source coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetLineCoverageC_ReturnsValidLineCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetCLineModuleCoverages();

        const auto binPath = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_C_BIN);
        AZStd::string root = binPath.RootName().Native();

        const auto srcPath = TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
            / "Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp");
        AZStd::string srcRelativePath = srcPath.RelativePath().Native();

        // Given the raw line coverage output of TestTargetC
        AZStd::string rawCoverage;
        rawCoverage += "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
        rawCoverage += "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617124593\" lines-covered=\"25\" lines-valid=\"25\" version=\"0\">";
        rawCoverage += "  <sources>";
        rawCoverage += AZStd::string::format("    <source>%s</source>", root.c_str());
        rawCoverage += "  </sources>";
        rawCoverage += "  <packages>";
        rawCoverage += AZStd::string::format("    <package name=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", binPath.c_str());
        rawCoverage += "      <classes>";
        rawCoverage += AZStd::string::format("        <class name=\"TestImpactTestTargetC.cpp\" filename=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", srcRelativePath.c_str());
        rawCoverage += "          <methods/>";
        rawCoverage += "          <lines>";
        rawCoverage += "            <line number=\"32\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"33\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"34\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"35\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"37\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"38\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"39\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"40\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"42\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"43\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"44\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"45\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"47\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"48\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"49\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"50\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"52\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"53\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"54\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"55\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"57\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"58\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"59\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"60\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"63\" hits=\"1\"/>";
        rawCoverage += "          </lines>";
        rawCoverage += "        </class>";
        rawCoverage += "      </classes>";
        rawCoverage += "    </package>";
        rawCoverage += "  </packages>";
        rawCoverage += "</coverage>";

        // When the raw line coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetSourceCoverageC_ReturnsValidSourceCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetCSourceModuleCoverages();

        const auto binPath = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_C_BIN);
        AZStd::string root = binPath.RootName().Native();

        const auto srcPath = TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
            / "Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp");
        AZStd::string srcRelativePath = srcPath.RelativePath().Native();

        // Given the raw source coverage output of TestTargetC
        AZStd::string rawCoverage;
        rawCoverage += "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
        rawCoverage += "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617117796\" lines-covered=\"0\" lines-valid=\"0\" version=\"0\">";
        rawCoverage += "  <sources>";
        rawCoverage += AZStd::string::format("    <source>%s</source>", root.c_str());
        rawCoverage += "  </sources>";
        rawCoverage += "  <packages>";
        rawCoverage += AZStd::string::format("    <package name=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", binPath.c_str());
        rawCoverage += "      <classes>";
        rawCoverage += AZStd::string::format("        <class name=\"TestImpactTestTargetC.cpp\" filename=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", srcRelativePath.c_str());
        rawCoverage += "          <methods/>";
        rawCoverage += "          <lines/>";
        rawCoverage += "        </class>";
        rawCoverage += "      </classes>";
        rawCoverage += "    </package>";
        rawCoverage += "  </packages>";
        rawCoverage += "</coverage>";

        // When the raw source coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetLineCoverageD_ReturnsValidLineCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetDLineModuleCoverages();

        const auto binPath = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_D_BIN);
        AZStd::string root = binPath.RootName().Native();

        const auto srcPath = TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
            / "Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp");
        AZStd::string srcRelativePath = srcPath.RelativePath().Native();

        // Given the raw line coverage output of TestTargetD
        AZStd::string rawCoverage;
        rawCoverage += "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
        rawCoverage += "<coverage line-rate=\"0.6470588235294118\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617124579\" lines-covered=\"55\" lines-valid=\"85\" version=\"0\">";
        rawCoverage += "  <sources>";
        rawCoverage += AZStd::string::format("    <source>%s</source>", root.c_str());
        rawCoverage += "  </sources>";
        rawCoverage += "  <packages>";
        rawCoverage += AZStd::string::format("    <package name=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", binPath.c_str());
        rawCoverage += "      <classes>";
        rawCoverage += AZStd::string::format("        <class name=\"TestImpactTestTargetD.cpp\" filename=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", srcRelativePath.c_str());
        rawCoverage += "          <methods/>";
        rawCoverage += "          <lines>";
        rawCoverage += "            <line number=\"56\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"57\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"58\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"59\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"61\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"62\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"63\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"64\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"66\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"67\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"68\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"69\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"71\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"72\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"73\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"74\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"76\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"77\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"78\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"79\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"81\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"82\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"83\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"84\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"86\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"87\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"88\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"89\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"91\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"92\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"93\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"94\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"96\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"97\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"98\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"99\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"101\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"102\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"103\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"104\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"106\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"107\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"108\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"109\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"111\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"112\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"113\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"114\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"116\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"117\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"118\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"119\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"121\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"128\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"130\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"137\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"139\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"146\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"148\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"155\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"157\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"158\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"159\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"160\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"162\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"163\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"164\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"165\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"167\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"168\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"169\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"170\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"172\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"173\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"174\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"175\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"177\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"178\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"179\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"180\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"182\" hits=\"1\"/>";
        rawCoverage += "            <line number=\"183\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"184\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"185\" hits=\"0\"/>";
        rawCoverage += "            <line number=\"188\" hits=\"1\"/>";
        rawCoverage += "          </lines>";
        rawCoverage += "        </class>";
        rawCoverage += "      </classes>";
        rawCoverage += "    </package>";
        rawCoverage += "  </packages>";
        rawCoverage += "</coverage>";

        // When the raw line coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }

    TEST(CoberturaModuleCoveragesFactory, ParseTestTargetSourceCoverageD_ReturnsValidSourceCoverage)
    {
        const AZStd::vector<TestImpact::ModuleCoverage> expectedCoverage = GetTestTargetDSourceModuleCoverages();

        const auto binPath = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_D_BIN);
        AZStd::string root = binPath.RootName().Native();

        const auto srcPath = TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
            / "Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp");
        AZStd::string srcRelativePath = srcPath.RelativePath().Native();

        // Given the raw source coverage output of TestTargetD
        AZStd::string rawCoverage;
        rawCoverage += "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
        rawCoverage += "<coverage line-rate=\"1\" branch-rate=\"0\" complexity=\"0\" branches-covered=\"0\" branches-valid=\"0\" timestamp=\"1617117804\" lines-covered=\"0\" lines-valid=\"0\" version=\"0\">";
        rawCoverage += "  <sources>";
        rawCoverage += AZStd::string::format("    <source>%s</source>", root.c_str());
        rawCoverage += "  </sources>";
        rawCoverage += "  <packages>";
        rawCoverage += AZStd::string::format("    <package name=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", binPath.c_str());
        rawCoverage += "      <classes>";
        rawCoverage += AZStd::string::format("        <class name=\"TestImpactTestTargetD.cpp\" filename=\"%s\" line-rate=\"1\" branch-rate=\"0\" complexity=\"0\">", srcRelativePath.c_str());
        rawCoverage += "          <methods/>";
        rawCoverage += "          <lines/>";
        rawCoverage += "        </class>";
        rawCoverage += "      </classes>";
        rawCoverage += "    </package>";
        rawCoverage += "  </packages>";
        rawCoverage += "</coverage>";

        // When the raw source coverage text is parsed
        const AZStd::vector<TestImpact::ModuleCoverage> coverage = TestImpact::Cobertura::ModuleCoveragesFactory(rawCoverage);

        // Expect the generated suite data to match that of the raw coverage text
        EXPECT_TRUE(coverage == expectedCoverage);
    }
} // namespace UnitTest

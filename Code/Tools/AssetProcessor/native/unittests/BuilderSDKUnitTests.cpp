/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#include <native/unittests/UnitTestUtils.h>
#endif
#include <QDir>
#include <QTemporaryDir>

namespace UnitTest
{
    // note that this is copied from BuilderSDK.cpp because its intentionally not exposed as publicly available types
    // the game would import it from LmbrCentral's various headers such as MaterialAsset.h
    // but that would require including lmbrcentral into the unit tests, which is not acceptable.
    static AZ::Data::AssetType sliceAssetType("{C62C7A87-9C09-4148-A985-12F2C99C0A45}"); // from SliceAsset.h
    static AZ::Data::AssetType fontAssetType("{57767D37-0EBE-43BE-8F60-AB36D2056EF8}"); // form UiAssetTypes.h

    class BuilderSDKUnitTests : public LeakDetectionFixture
    {
    public:
        // QTemporaryDir autodeletes on destruct.
        BuilderSDKUnitTests()
        {
            m_folder = QDir(m_tempDir.path());
        }

        QDir m_folder;

    private:
        QTemporaryDir m_tempDir;
    };

    TEST_F(BuilderSDKUnitTests, JobProduct_InferredAssetTypes_MatchExpectedValues)
    {
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("no_extension"), AZ::Data::AssetType::CreateNull());
    }

    TEST_F(BuilderSDKUnitTests, XMLParsing_EmptyOrInvalid_MatchesExpectedResult)
    {
        // this must neither crash nor fail to return the null type.
        QString dummyFileName = m_folder.absoluteFilePath("test_empty_xml");
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, ""));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), AZ::Uuid::CreateNull());

        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, "dummy"));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), AZ::Uuid::CreateNull());

        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, "<truncatedfile "));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), AZ::Uuid::CreateNull());

        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, "<truncated attribute=\"test"));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), AZ::Uuid::CreateNull());
    }

    TEST_F(BuilderSDKUnitTests, XMLParsing_BasicTypes_MatchesExpectedResults)
    {
        QString dummyFileName = m_folder.absoluteFilePath("test_basic_file_xml");

        // note that the above file is NOT AN XML FILE so it should not work despite contianing the expected tag.
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, "<fontshader>stuff</fontshader>"));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), AZ::Uuid::CreateNull());

        dummyFileName = m_folder.absoluteFilePath("test_basic_file.xml");
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, "<fontshader>stuff</fontshader>"));
        EXPECT_EQ(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), fontAssetType);

        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(
            dummyFileName,
            "<ObjectStream>stuff</ObjectStream>")); // note - objectstream with no data in it should not crash or return anything useful
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), AZ::Uuid::CreateNull());

        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, "<UnknownThing>stuff</UnknownThing>"));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), AZ::Uuid::CreateNull());
    }

    TEST_F(BuilderSDKUnitTests, XMLParsing_ObjectStreamTypes_MatchesExpectedResults)
    {
        // this must neither crash nor fail to return the null type.
        QString dummyFileName = m_folder.absoluteFilePath("test_objectstream.xml");

        // objectstream missing its 'type' inside the class
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, "<ObjectStream><Class/></ObjectStream>"));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), AZ::Uuid::CreateNull());

        {
            UnitTestUtils::AssertAbsorber absorber;
            // objectstream with an empty string 'type' inside the class
            EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, "<ObjectStream><Class type=\"\"/></ObjectStream>"));
            EXPECT_EQ(
                AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), AZ::Uuid::CreateNull());
        }

        {
            UnitTestUtils::AssertAbsorber absorber;
            // objectstream with invalid 'type' inside the class (bad guid)
            EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, "<ObjectStream><Class type=\"123 NOT A GUID\"/></ObjectStream>"));
            EXPECT_EQ(
                AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()), AZ::Uuid::CreateNull());
            EXPECT_GT(absorber.m_numWarningsAbsorbed, 0);
        }

        // objectstream with an actual 'guid' inside the class (bad guid)
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(
            dummyFileName, "<ObjectStream><Class type=\"{49375937-7F37-41B1-96A5-B099A8657DDE}\"/></ObjectStream>"));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()),
            AZ::Uuid("{49375937-7F37-41B1-96A5-B099A8657DDE}"));

        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(
            dummyFileName, "<ObjectStream><Class type=\"{49375937-7F37-41B1-96A5-B099A8657DDE}\"/></ObjectStream>"));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()),
            AZ::Uuid("{49375937-7F37-41B1-96A5-B099A8657DDE}"));

        // use old format which doesnt use 'class' keyword
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(
            dummyFileName, "<ObjectStream><ASDFASDFASDFASDF type=\"{49375937-7F37-41B1-96A5-B099A8657DDE}\"/></ObjectStream>"));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()),
            AZ::Uuid("{49375937-7F37-41B1-96A5-B099A8657DDE}"));

        // special case - recognize the old UICanvas format :(
        const char* canvasFile =
            "<ObjectStream version=\"1\">"
            "    <Entity type=\"{75651658-8663-478D-9090-2432DFCAFA44}\">"
            "        <uint64 name=\"Id\" value=\"13069065444211002982\" type=\"{D6597933-47CD-4FC8-B911-63F3E2B0993A}\"/>"
            "        <AZStd::string name=\"Name\" value=\"triggerex01.xml\" type=\"{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}\"/>"
            "        <bool name=\"IsDependencyReady\" value=\"true\" type=\"{A0CA880C-AFE4-43CB-926C-59AC48496112}\"/>"
            "        <AZStd::vector name=\"Components\" type=\"{2BADE35A-6F1B-4698-B2BC-3373D010020C}\">"
            "            <UiCanvas name=\"element\" version=\"1\" type=\"{50B8CF6C-B19A-4D86-AFE9-96EFB820D422}\">"
            "            </UiCanvas>"
            "        </AZStd::vector>"
            "    </Entity>"
            "</ObjectStream>";
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(dummyFileName, canvasFile));
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()),
            AZ::Uuid("{E48DDAC8-1F1E-4183-AAAB-37424BCC254B}"));

    }

    TEST_F(BuilderSDKUnitTests, SubIDMath_MatchesExpectedResults)
    {
        AZ::u32 tester = AssetBuilderSDK::ConstructSubID(5, 10);
        EXPECT_EQ(AssetBuilderSDK::GetSubID_ID(tester), 5);
        EXPECT_EQ(AssetBuilderSDK::GetSubID_LOD(tester), 10);
        tester |= 0xFFF00000;

        for (AZ::u32 idx = 0; idx < 9; ++idx)
        {
            EXPECT_EQ(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::ConstructSubID(0, idx)), 0);
            EXPECT_EQ(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::ConstructSubID(0, idx)), idx);
            EXPECT_EQ(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::ConstructSubID(idx, 0)), idx);
            EXPECT_EQ(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::ConstructSubID(idx, 0)), 0);

            EXPECT_EQ(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::ConstructSubID(9 - idx, idx)), 9 - idx);
            EXPECT_EQ(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::ConstructSubID(9 - idx, idx)), idx);
        }

        // make sure that the flags are not disturbed if you modify the id and lods and pass the previous value in.
        // we pass in the existing value as the third param, which should replace the existing ids and lods, but keep the flags.
        tester = AssetBuilderSDK::ConstructSubID(512, 12, tester);
        EXPECT_EQ((tester & 0xFFF00000), 0xFFF00000);
        EXPECT_EQ(AssetBuilderSDK::GetSubID_ID(tester), 512);
        EXPECT_EQ(AssetBuilderSDK::GetSubID_LOD(tester), 12);
    }

    TEST_F(BuilderSDKUnitTests, SubIdGeneration_MatchesExpectedResult)
    {
        // test subid autogeneration

        // files with no UUID and no extension always return null
        EXPECT_EQ(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(AZ::Data::AssetType::CreateNull(), "blah"), 0);

        // files with no UUID and no known extension always return null
        EXPECT_EQ(
            AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(AZ::Data::AssetType::CreateNull(), "blah.whatever"), 0);

        // ("editor") slices always have subid 1
        EXPECT_EQ(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(sliceAssetType, "blah.slice"), 1);
    }

} // namespace UnitTest

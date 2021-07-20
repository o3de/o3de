/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include "native/unittests/UnitTestRunner.h"


using namespace UnitTestUtils;

class BuilderSDKUnitTestFixture
{
public:
    // QTemporaryDir autodeletes on destruct.
    BuilderSDKUnitTestFixture()
    {
        m_folder = QDir(m_tempDir.path());
    }
    
    QDir m_folder;

private:
    QTemporaryDir m_tempDir;
};

namespace
{
    // note that this is copied from BuilderSDK.cpp because its intentionally not exposed as publicly available types
    // the game would import it from LmbrCentral's various headers such as MaterialAsset.h
    // but that would require including lmbrcentral into the unit tests, which is not acceptable.
    static AZ::Data::AssetType textureMipsAssetType("{3918728C-D3CA-4D9E-813E-A5ED20C6821E}");
    static AZ::Data::AssetType skinnedMeshLodsAssetType("{58E5824F-C27B-46FD-AD48-865BA41B7A51}");
    static AZ::Data::AssetType staticMeshLodsAssetType("{9AAE4926-CB6A-4C60-9948-A1A22F51DB23}");
    static AZ::Data::AssetType textureAssetType("{59D5E20B-34DB-4D8E-B867-D33CC2556355}"); // from MaterialAsset.h
    static AZ::Data::AssetType meshAssetType("{C2869E3B-DDA0-4E01-8FE3-6770D788866B}"); // from MeshAsset.h
    static AZ::Data::AssetType skinnedMeshAssetType("{C5D443E1-41FF-4263-8654-9438BC888CB7}"); // from MeshAsset.h
    static AZ::Data::AssetType sliceAssetType("{C62C7A87-9C09-4148-A985-12F2C99C0A45}"); // from SliceAsset.h
    static AZ::Data::AssetType particleAssetType("{6EB56B55-1B58-4EE3-A268-27680338AE56}"); // from ParticleAsset.h
    static AZ::Data::AssetType lensFlareAssetType("{CF44D1F0-F178-4A3D-A9E6-D44721F50C20}"); // from LensFlareAsset.h
    static AZ::Data::AssetType fontAssetType("{57767D37-0EBE-43BE-8F60-AB36D2056EF8}"); // form UiAssetTypes.h
}

class BuilderSDKUnitTests_Basic
    : public UnitTestRun
{
public:

    int UnitTestPriority() const override
    {
        return -3; // do this first before the others
    };

    void StartTest() override
    {
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("no_extension") == AZ::Data::AssetType::CreateNull());
        // .dds should show up as texture
        // .dds.xn should show up as mips
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds") == AZ::Data::AssetType(textureAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.1") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.2") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.3") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.4") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.5") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.6") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.7") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.8") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.9") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.1a") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.2a") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.3a") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.4a") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.5a") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.6a") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.7a") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.8a") == AZ::Data::AssetType(textureMipsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("texture.dds.9a") == AZ::Data::AssetType(textureMipsAssetType));

        // .cgf should show up as staticmesh
        // .cgf.x should static mesh lods
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("mesh.cgf") == AZ::Data::AssetType(meshAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("mesh.cgf.1") == AZ::Data::AssetType(staticMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("mesh.cgf.2") == AZ::Data::AssetType(staticMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("mesh.cgf.3") == AZ::Data::AssetType(staticMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("mesh.cgf.4") == AZ::Data::AssetType(staticMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("mesh.cgf.5") == AZ::Data::AssetType(staticMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("mesh.cgf.6") == AZ::Data::AssetType(staticMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("mesh.cgf.7") == AZ::Data::AssetType(staticMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("mesh.cgf.8") == AZ::Data::AssetType(staticMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("mesh.cgf.9") == AZ::Data::AssetType(staticMeshLodsAssetType));

        // .skin should show up as skin (with lods)
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("ask.skin") == AZ::Data::AssetType(skinnedMeshAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("ask.skin.1") == AZ::Data::AssetType(skinnedMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("ask.skin.2") == AZ::Data::AssetType(skinnedMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("ask.skin.3") == AZ::Data::AssetType(skinnedMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("ask.skin.4") == AZ::Data::AssetType(skinnedMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("ask.skin.5") == AZ::Data::AssetType(skinnedMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("ask.skin.6") == AZ::Data::AssetType(skinnedMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("ask.skin.7") == AZ::Data::AssetType(skinnedMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("ask.skin.8") == AZ::Data::AssetType(skinnedMeshLodsAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("ask.skin.9") == AZ::Data::AssetType(skinnedMeshLodsAssetType));

        // now try to trick it with a simple mixture (complex, evil mixtures such as .skin.cgf.dds.tif.1a are not supported and not expected to function)
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("something_cgf.skin") == AZ::Data::AssetType(skinnedMeshAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("something_skin.dds") == AZ::Data::AssetType(textureAssetType));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName("something_dds.cgf") == AZ::Data::AssetType(meshAssetType));

        Q_EMIT UnitTestPassed();
    }
};

class BuilderSDKUnitTests_XMLParsing_EmptyOrInvalid
    : public UnitTestRun
    , protected BuilderSDKUnitTestFixture
{
public:
    int UnitTestPriority() const override
    {
        return -2;
    };

    void StartTest() override
    {
        // this must neither crash nor fail to return the null type.
        QString dummyFileName = m_folder.absoluteFilePath("test_empty_xml");
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, ""));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid::CreateNull());
        
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "dummy"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid::CreateNull());

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<truncatedfile "));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid::CreateNull());
        
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<truncated attribute=\"test"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid::CreateNull());

        Q_EMIT UnitTestPassed();
    }
};


class BuilderSDKUnitTests_XMLParsing_BasicTypes
    : public UnitTestRun
    , protected BuilderSDKUnitTestFixture
{
public:
    int UnitTestPriority() const override
    {
        return -1;
    };

    void StartTest() override
    {
        // this must neither crash nor fail to return the null type.
        QString dummyFileName = m_folder.absoluteFilePath("test_basic_file_xml");

        // note that the above file is NOT AN XML FILE so it should not work despite contianing the expected tag.
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<fontshader>stuff</fontshader>"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid::CreateNull());

        dummyFileName = m_folder.absoluteFilePath("test_basic_file.xml");
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<fontshader>stuff</fontshader>"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == fontAssetType);
        
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<ParticleLibrary>stuff</ParticleLibrary>"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == particleAssetType);

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<LensFlareLibrary>stuff</LensFlareLibrary>"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == lensFlareAssetType);

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<ObjectStream>stuff</ObjectStream>")); // note - objectstream with no data in it should not crash or return anything useful
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid::CreateNull());

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<UnknownThing>stuff</UnknownThing>"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid::CreateNull());

        Q_EMIT UnitTestPassed();
    }
};


class BuilderSDKUnitTests_XMLParsing_ObjectStreamTypes
    : public UnitTestRun
    , protected BuilderSDKUnitTestFixture
{
public:
    void StartTest() override
    {
        // this must neither crash nor fail to return the null type.
        QString dummyFileName = m_folder.absoluteFilePath("test_objectstream.xml");

        // objectstream missing its 'type' inside the class
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<ObjectStream><Class/></ObjectStream>"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid::CreateNull());

        {
            UnitTestUtils::AssertAbsorber absorber;
            // objectstream with invalid 'type' inside the class
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<ObjectStream><Class type=\"\"/></ObjectStream>"));
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid::CreateNull());
            UNIT_TEST_EXPECT_TRUE(absorber.m_numWarningsAbsorbed > 0);
        }

        {
            UnitTestUtils::AssertAbsorber absorber;
            // objectstream with invalid 'type' inside the class (bad guid)
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<ObjectStream><Class type=\"123 NOT A GUID\"/></ObjectStream>"));
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid::CreateNull());
            UNIT_TEST_EXPECT_TRUE(absorber.m_numWarningsAbsorbed > 0);
        }

        // objectstream with an actual 'guid' inside the class (bad guid)
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<ObjectStream><Class type=\"{49375937-7F37-41B1-96A5-B099A8657DDE}\"/></ObjectStream>"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid("{49375937-7F37-41B1-96A5-B099A8657DDE}"));

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<ObjectStream><Class type=\"{49375937-7F37-41B1-96A5-B099A8657DDE}\"/></ObjectStream>"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid("{49375937-7F37-41B1-96A5-B099A8657DDE}"));

        // use old format which doesnt use 'class' keyword
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, "<ObjectStream><ASDFASDFASDFASDF type=\"{49375937-7F37-41B1-96A5-B099A8657DDE}\"/></ObjectStream>"));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid("{49375937-7F37-41B1-96A5-B099A8657DDE}"));

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
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(dummyFileName, canvasFile));
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferAssetTypeByProductFileName(dummyFileName.toUtf8().data()) == AZ::Uuid("{E48DDAC8-1F1E-4183-AAAB-37424BCC254B}"));

        Q_EMIT UnitTestPassed();
    }
};

class BuilderSDKUnitTests_SubID_Basic
    : public UnitTestRun
{
public:

    int UnitTestPriority() const override
    {
        return -3; // do this first before the others
    };

    void StartTest() override
    {
        // test SUBID MATH

        AZ::u32 tester = AssetBuilderSDK::ConstructSubID(5, 10);
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_ID(tester) == 5);
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_LOD(tester) == 10);
        tester |= 0xFFF00000;

        for (AZ::u32 idx = 0; idx < 9; ++idx)
        {
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::ConstructSubID(0, idx)) == 0);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::ConstructSubID(0, idx)) == idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::ConstructSubID(idx, 0)) == idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::ConstructSubID(idx, 0)) == 0);

            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::ConstructSubID(9 - idx, idx)) == 9 - idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::ConstructSubID(9 - idx, idx)) == idx);
        }

        // make sure that the flags are not disturbed if you modify the id and lods and pass the previous value in.
        // we pass in the existing value as the third param, which should replace the existing ids and lods, but keep the flags.
        tester = AssetBuilderSDK::ConstructSubID(512, 12, tester);
        UNIT_TEST_EXPECT_TRUE( (tester & 0xFFF00000) == 0xFFF00000);
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_ID(tester) == 512);
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_LOD(tester) == 12);

        Q_EMIT UnitTestPassed();
    }
};


class BuilderSDKUnitTests_SubID
    : public UnitTestRun
{
public:

    int UnitTestPriority() const override
    {
        return -1;
    };

    void StartTest() override
    {
        // test subid autogeneration
        
        // files with no UUID and no extension always return null
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(AZ::Data::AssetType::CreateNull(), "blah") == 0);
        
        // files with no UUID and no known extension always return null
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(AZ::Data::AssetType::CreateNull(), "blah.whatever") == 0);

        // ("editor") slices always have subid 1
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(sliceAssetType, "blah.slice") == 1);

        // standard skins, meshes have 0
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(skinnedMeshAssetType, "doesntmatter.whatever") == 0);
        UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(meshAssetType, "doesntmatter.whatever") == 0);

        // _diff textures have that bit set
        UNIT_TEST_EXPECT_TRUE((AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureAssetType, "whatever_diff.dds") & AssetBuilderSDK::SUBID_FLAG_DIFF) == AssetBuilderSDK::SUBID_FLAG_DIFF);
        // regular textures do not.
        UNIT_TEST_EXPECT_TRUE((AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureAssetType, "whatever.dds") & AssetBuilderSDK::SUBID_FLAG_DIFF) == 0);

        // alpha mips have the alpha flag set
        UNIT_TEST_EXPECT_TRUE((AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, "whatever_diff.dds.1a") & AssetBuilderSDK::SUBID_FLAG_ALPHA) == AssetBuilderSDK::SUBID_FLAG_ALPHA);
        // alpha _diff have the diff flag too
        UNIT_TEST_EXPECT_TRUE((AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, "whatever_diff.dds.1a") & AssetBuilderSDK::SUBID_FLAG_DIFF) == AssetBuilderSDK::SUBID_FLAG_DIFF);
        // regular textures do not.
        UNIT_TEST_EXPECT_TRUE((AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureAssetType, "whatever.dds") & AssetBuilderSDK::SUBID_FLAG_ALPHA) == 0);
        
        // check each possible LOD and Alpha LOD:
        for (AZ::u32 idx = 1; idx <= 9; ++idx)
        {
            AZStd::string check;

            // .1 is just that index (when masked)
            check = AZStd::string::format("somefilename.dds.%i", idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, check.c_str())) == idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, check.c_str())) == 0);

            // .1a is the same, but has 
            check = AZStd::string::format("somefilename.dds.%ia", idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, check.c_str())) == idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, check.c_str())) == 0);
            UNIT_TEST_EXPECT_TRUE((AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, check.c_str()) & AssetBuilderSDK::SUBID_FLAG_ALPHA) == AssetBuilderSDK::SUBID_FLAG_ALPHA);
            UNIT_TEST_EXPECT_TRUE((AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, check.c_str()) & AssetBuilderSDK::SUBID_FLAG_DIFF) == 0);
            

            check = AZStd::string::format("somefilename_diff.dds.%ia", idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, check.c_str())) == idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, check.c_str())) == 0);
            UNIT_TEST_EXPECT_TRUE((AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, check.c_str()) & AssetBuilderSDK::SUBID_FLAG_ALPHA) == AssetBuilderSDK::SUBID_FLAG_ALPHA);
            UNIT_TEST_EXPECT_TRUE((AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(textureMipsAssetType, check.c_str()) & AssetBuilderSDK::SUBID_FLAG_DIFF) == AssetBuilderSDK::SUBID_FLAG_DIFF);
            
            check = AZStd::string::format("somefilename.skin.%i", idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(skinnedMeshLodsAssetType, check.c_str())) == idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(skinnedMeshLodsAssetType, check.c_str())) == 0);

            check = AZStd::string::format("somefilename.cgf.%i", idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_LOD(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(staticMeshLodsAssetType, check.c_str())) == idx);
            UNIT_TEST_EXPECT_TRUE(AssetBuilderSDK::GetSubID_ID(AssetBuilderSDK::JobProduct::InferSubIDFromProductFileName(staticMeshLodsAssetType, check.c_str())) == 0);
        }
        Q_EMIT UnitTestPassed();
    }
};

REGISTER_UNIT_TEST(BuilderSDKUnitTests_XMLParsing_ObjectStreamTypes)
REGISTER_UNIT_TEST(BuilderSDKUnitTests_XMLParsing_BasicTypes)
REGISTER_UNIT_TEST(BuilderSDKUnitTests_XMLParsing_EmptyOrInvalid)
REGISTER_UNIT_TEST(BuilderSDKUnitTests_Basic)
REGISTER_UNIT_TEST(BuilderSDKUnitTests_SubID)
REGISTER_UNIT_TEST(BuilderSDKUnitTests_SubID_Basic)

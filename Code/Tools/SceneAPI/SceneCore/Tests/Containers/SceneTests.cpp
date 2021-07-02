/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            // Allocators are setup by the test's environment initialization since they need to be passed to another dll
            using SceneTest = ::testing::Test;

            TEST_F(SceneTest, Constructor_StringRef_HasCorrectName)
            {
                AZStd::string sampleSceneName = "testName";
                Scene testScene(sampleSceneName);

                EXPECT_TRUE(sampleSceneName == testScene.GetName());
            }

            TEST_F(SceneTest, Constructor_StringRefRef_HasCorrectName)
            {
                const char* sampleNameChrStar = "testName";
                AZStd::string sampleSceneName = sampleNameChrStar;
                Scene testScene(AZStd::move(sampleSceneName));

                EXPECT_TRUE(strcmp(sampleNameChrStar, testScene.GetName().c_str()) == 0);
            }

            TEST_F(SceneTest, Constructor_EmptyStrRef_HasCorrectName)
            {
                AZStd::string sampleSceneName = "";
                Scene testScene(sampleSceneName);

                EXPECT_TRUE(sampleSceneName == testScene.GetName().c_str());
            }

            class SceneFilenameTests
                : public ::testing::Test
            {
            public:
                SceneFilenameTests()
                    : m_testId(Uuid::CreateString("{C9B909EE-0751-4BD7-B68B-B2C48D535396}"))
                    , m_testScene("testScene")
                {
                }

            protected:
                AZ::Uuid m_testId;
                Scene m_testScene;
            };

            TEST_F(SceneFilenameTests, SetSource_StringRef_SourceFileRegistered)
            {
                AZStd::string testFilename = "testFilename.fbx";
                m_testScene.SetSource(testFilename, m_testId);
                const AZStd::string compareFilename = m_testScene.GetSourceFilename();
                EXPECT_STREQ(testFilename.c_str(), compareFilename.c_str());
                EXPECT_EQ(m_testId, m_testScene.GetSourceGuid());
            }

            TEST_F(SceneFilenameTests, SetSource_StringRefRef_SourceFileRegistered)
            {
                const char* testChrFilename = "testFilename.fbx";
                AZStd::string testFilename = testChrFilename;
                m_testScene.SetSource(AZStd::move(testFilename), m_testId);
                const AZStd::string compareFilename = m_testScene.GetSourceFilename();
                EXPECT_STREQ(testChrFilename, compareFilename.c_str());
                EXPECT_EQ(m_testId, m_testScene.GetSourceGuid());
            }

            TEST_F(SceneFilenameTests, SetManifestFilename_StringRef_ManifestFileRegistered)
            {
                AZStd::string testFilename = "testFilename.assetinfo";
                m_testScene.SetManifestFilename(testFilename);
                const AZStd::string compareFilename = m_testScene.GetManifestFilename();
                EXPECT_STREQ(testFilename.c_str(), compareFilename.c_str());
            }

            TEST_F(SceneFilenameTests, SetManifestFilename_StringRefRef_ManifestFileRegistered)
            {
                const char* testChrFilename = "testFilename.assetinfo";
                AZStd::string testFilename = testChrFilename;
                m_testScene.SetManifestFilename(AZStd::move(testFilename));
                const AZStd::string compareFilename = m_testScene.GetManifestFilename();
                EXPECT_STREQ(testChrFilename, compareFilename.c_str());
            }
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <Application.h>
#include <ProjectUtils.h>
#include <ProjectManager_Test_Traits_Platform.h>

#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QApplication>
#include <QKeyEvent>
#include <QDir>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        class ProjectManagerUtilsTests
            : public ::UnitTest::ScopedAllocatorSetupFixture
        {
        public:
            ProjectManagerUtilsTests()
            {
                m_application = AZStd::make_unique<ProjectManager::Application>();
                m_application->Init(false);

                QDir dir;
                dir.mkdir("ProjectA");
                dir.mkdir("ProjectB");

                QFile origFile("ProjectA/origFile.txt");
                if (origFile.open(QIODevice::ReadWrite))
                {
                    QTextStream stream(&origFile);
                    stream << "orig" << Qt::endl;
                    origFile.close();
                }

                QFile replaceFile("ProjectA/replaceFile.txt");
                if (replaceFile.open(QIODevice::ReadWrite))
                {
                    QTextStream stream(&replaceFile);
                    stream << "replace" << Qt::endl;
                    replaceFile.close();
                }
            }

            ~ProjectManagerUtilsTests()
            {
                QDir dirA("ProjectA");
                dirA.removeRecursively();

                QDir dirB("ProjectB");
                dirB.removeRecursively();

                m_application.reset();
            }

            AZStd::unique_ptr<ProjectManager::Application> m_application;
        };

#if AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        TEST_F(ProjectManagerUtilsTests, DISABLED_MoveProject_Succeeds)
#else
        TEST_F(ProjectManagerUtilsTests, MoveProject_Succeeds)
#endif // !AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        {
            EXPECT_TRUE(MoveProject(
                QDir::currentPath() + QDir::separator() + "ProjectA",
                QDir::currentPath() + QDir::separator() + "ProjectB",
                nullptr, true));

            QFileInfo origFile("ProjectA/origFile.txt");
            EXPECT_TRUE(!origFile.exists());

            QFileInfo replaceFile("ProjectA/replaceFile.txt");
            EXPECT_TRUE(!replaceFile.exists());

            QFileInfo origFileMoved("ProjectB/origFile.txt");
            EXPECT_TRUE(origFileMoved.exists() && origFileMoved.isFile());

            QFileInfo replaceFileMoved("ProjectB/replaceFile.txt");
            EXPECT_TRUE(replaceFileMoved.exists() && replaceFileMoved.isFile());
        }

#if AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        TEST_F(ProjectManagerUtilsTests, DISABLED_ReplaceFile_Succeeds)
#else
        TEST_F(ProjectManagerUtilsTests, ReplaceFile_Succeeds)
#endif // !AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        {
            EXPECT_TRUE(ReplaceFile("ProjectA/origFile.txt", "ProjectA/replaceFile.txt", nullptr, false));

            QFile origFile("ProjectA/origFile.txt");
            if (origFile.open(QIODevice::ReadOnly))
            {
                QTextStream stream(&origFile);
                QString line = stream.readLine();
                EXPECT_EQ(line, "replace");

                origFile.close();
            }
            else
            {
                FAIL();
            }
        }
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager

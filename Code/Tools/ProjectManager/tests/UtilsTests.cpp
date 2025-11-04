/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <Application.h>
#include <ProjectUtils.h>
#include <ProjectManagerDefs.h>
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
            : public ::UnitTest::LeakDetectionFixture
        {
        public:

            ProjectManagerUtilsTests()
            {
                m_projectAPath = QDir(m_testFolder.GetDirectory()).path() + QDir::separator() + "ProjectA";
                m_projectBPath = QDir(m_testFolder.GetDirectory()).path() + QDir::separator() + "ProjectB";

                // Replaces first 'A' with 'B'
                m_projectABuildPath = QString("%1%2%3").arg(m_projectAPath, QDir::separator(), ProjectBuildDirectoryName);
                m_projectBBuildPath = QString("%1%2%3").arg(m_projectBPath, QDir::separator(), ProjectBuildDirectoryName);

                QDir dir;
                dir.mkpath(m_projectABuildPath);
                dir.mkdir(m_projectBPath);

                m_projectAOrigFilePath = QString("%1%2%3").arg(m_projectAPath, QDir::separator(), "origFile.txt");
                m_projectBOrigFilePath = QString("%1%2%3").arg(m_projectBPath, QDir::separator(), "origFile.txt");
                QFile origFile(m_projectAOrigFilePath);
                if (origFile.open(QIODevice::ReadWrite))
                {
                    QTextStream stream(&origFile);
                    stream << "orig" << Qt::endl;
                    origFile.close();
                }

                m_projectAReplaceFilePath = QString("%1%2%3").arg(m_projectAPath, QDir::separator(), "replaceFile.txt");
                m_projectBReplaceFilePath = QString("%1%2%3").arg(m_projectBPath, QDir::separator(), "replaceFile.txt");
                QFile replaceFile(m_projectAReplaceFilePath);
                if (replaceFile.open(QIODevice::ReadWrite))
                {
                    QTextStream stream(&replaceFile);
                    stream << "replace" << Qt::endl;
                    replaceFile.close();
                }

                m_projectABuildFilePath = QString("%1%2%3").arg(m_projectABuildPath, QDir::separator(), "build.obj");
                m_projectBBuildFilePath = QString("%1%2%3").arg(m_projectBBuildPath, QDir::separator(), "build.obj");
                QFile buildFile(m_projectABuildFilePath);
                if (buildFile.open(QIODevice::ReadWrite))
                {
                    QTextStream stream(&buildFile);
                    stream << "x0FFFFFFFF" << Qt::endl;
                    buildFile.close();
                }
            }

            ~ProjectManagerUtilsTests()
            {
                QDir dirA(m_projectAPath);
                dirA.removeRecursively();

                QDir dirB(m_projectBPath);
                dirB.removeRecursively();
            }

            QString m_projectAPath;
            QString m_projectAOrigFilePath;
            QString m_projectAReplaceFilePath;
            QString m_projectABuildPath;
            QString m_projectABuildFilePath;
            QString m_projectBPath;
            QString m_projectBOrigFilePath;
            QString m_projectBReplaceFilePath;
            QString m_projectBBuildPath;
            QString m_projectBBuildFilePath;
            AZ::Test::ScopedAutoTempDirectory m_testFolder;

        };

#if AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        TEST_F(ProjectManagerUtilsTests, DISABLED_MoveProject_MovesExpectedFiles)
#else
        TEST_F(ProjectManagerUtilsTests, MoveProject_MovesExpectedFiles)
#endif // !AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        {
            EXPECT_TRUE(MoveProject(
                m_projectAPath,
                m_projectBPath, nullptr,
                true, /*displayProgress=*/ false));

            QFileInfo origFile(m_projectAOrigFilePath);
            EXPECT_FALSE(origFile.exists());

            QFileInfo replaceFile(m_projectAReplaceFilePath);
            EXPECT_FALSE(replaceFile.exists());

            QFileInfo origFileMoved(m_projectBOrigFilePath);
            EXPECT_TRUE(origFileMoved.exists() && origFileMoved.isFile());

            QFileInfo replaceFileMoved(m_projectBReplaceFilePath);
            EXPECT_TRUE(replaceFileMoved.exists() && replaceFileMoved.isFile());
        }

#if AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        TEST_F(ProjectManagerUtilsTests, DISABLED_MoveProject_DoesntMoveBuild)
#else
        TEST_F(ProjectManagerUtilsTests, MoveProject_DoesntMoveBuild)
#endif // !AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        {
            EXPECT_TRUE(MoveProject(
                m_projectAPath,
                m_projectBPath, nullptr,
                true, /*displayProgress=*/ false));

            QFileInfo origFile(m_projectAOrigFilePath);
            EXPECT_FALSE(origFile.exists());

            QFileInfo origFileMoved(m_projectBOrigFilePath);
            EXPECT_TRUE(origFileMoved.exists() && origFileMoved.isFile());

            QDir buildDir(m_projectBBuildPath);
            EXPECT_FALSE(buildDir.exists());
        }

#if AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        TEST_F(ProjectManagerUtilsTests, DISABLED_CopyProject_CopiesExpectedFiles)
#else
        TEST_F(ProjectManagerUtilsTests, CopyProject_CopiesExpectedFiles)
#endif // !AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        {
            EXPECT_TRUE(CopyProject(
                m_projectAPath,
                m_projectBPath, nullptr,
                true, /*displayProgress=*/ false));

            QFileInfo origFile(m_projectAOrigFilePath);
            EXPECT_TRUE(origFile.exists());

            QFileInfo replaceFile(m_projectAReplaceFilePath);
            EXPECT_TRUE(replaceFile.exists());

            QFileInfo origFileMoved(m_projectBOrigFilePath);
            EXPECT_TRUE(origFileMoved.exists() && origFileMoved.isFile());

            QFileInfo replaceFileMoved(m_projectBReplaceFilePath);
            EXPECT_TRUE(replaceFileMoved.exists() && replaceFileMoved.isFile());
        }

#if AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        TEST_F(ProjectManagerUtilsTests, DISABLED_CopyProject_DoesntCopyBuild)
#else
        TEST_F(ProjectManagerUtilsTests, CopyProject_DoesntCopyBuild)
#endif // !AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        {
            EXPECT_TRUE(CopyProject(
                m_projectAPath,
                m_projectBPath, nullptr,
                true, /*displayProgress=*/ false));

            QFileInfo origFile(m_projectAOrigFilePath);
            EXPECT_TRUE(origFile.exists());

            QFileInfo origFileMoved(m_projectBOrigFilePath);
            EXPECT_TRUE(origFileMoved.exists() && origFileMoved.isFile());

            QDir buildDir(m_projectBBuildPath);
            EXPECT_FALSE(buildDir.exists());
        }

#if AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        TEST_F(ProjectManagerUtilsTests, DISABLED_ReplaceFile_Succeeds)
#else
        TEST_F(ProjectManagerUtilsTests, ReplaceFile_Succeeds)
#endif // !AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
        {
            EXPECT_TRUE(ReplaceProjectFile(m_projectAOrigFilePath, m_projectAReplaceFilePath, nullptr, false));

            QFile origFile(m_projectAOrigFilePath);
            EXPECT_TRUE(origFile.open(QIODevice::ReadOnly));
            {
                QTextStream stream(&origFile);
                QString line = stream.readLine();
                EXPECT_EQ(line, "replace");

                origFile.close();
            }
        }
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager

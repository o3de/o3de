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

#include <AzCore/UnitTest/TestTypes.h>
#include <Application.h>
#include <ProjectUtils.h>

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

        TEST_F(ProjectManagerUtilsTests, MoveProject_Succeeds)
        {
            EXPECT_TRUE(MoveProject(
                QDir::currentPath() + QDir::separator() + "ProjectA",
                QDir::currentPath() + QDir::separator() + "ProjectB",
                m_application->GetMainWindow().get(), true));

            QFileInfo origFile("ProjectA/origFile.txt");
            EXPECT_TRUE(!origFile.exists());

            QFileInfo replaceFile("ProjectA/replaceFile.txt");
            EXPECT_TRUE(!replaceFile.exists());

            QFileInfo origFileMoved("ProjectB/origFile.txt");
            EXPECT_TRUE(origFileMoved.exists() && origFileMoved.isFile());

            QFileInfo replaceFileMoved("ProjectB/replaceFile.txt");
            EXPECT_TRUE(replaceFileMoved.exists() && replaceFileMoved.isFile());
        }

        TEST_F(ProjectManagerUtilsTests, ReplaceFile_Succeeds)
        {
            // Skip messagebox
            QTimer::singleShot(0, [=]()
                {
                    int keyToPress = Qt::Key_Enter;

                    QWidget* widget = QApplication::activeModalWidget();
                    if (widget)
                    {
                        QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, keyToPress, Qt::NoModifier);
                        QCoreApplication::postEvent(widget, event);
                    }
                });

            EXPECT_TRUE(ReplaceFile("ProjectA/origFile.txt", "ProjectA/replaceFile.txt", m_application->GetMainWindow().get()));

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

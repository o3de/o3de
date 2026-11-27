/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Logger/TraceLogger.h>

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

namespace UnitTest
{
    class TraceLoggerTest : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            m_previousFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_previousFileIO);
        }

    protected:
        void SetProjectAliases(const QDir& root)
        {
            m_fileIO.SetAlias("@user@", root.filePath("user").toUtf8().constData());
            m_fileIO.SetAlias("@products@", root.filePath("products").toUtf8().constData());
        }

        AZ::IO::LocalFileIO m_fileIO;
        AZ::IO::FileIOBase* m_previousFileIO = nullptr;
    };

    TEST_F(TraceLoggerTest, OpenLogFilePreservesConfiguredLogAlias)
    {
        QTemporaryDir tempDir;
        ASSERT_TRUE(tempDir.isValid());
        const QDir root(tempDir.path());
        SetProjectAliases(root);
        m_fileIO.SetAlias("@log@", root.filePath("isolated").toUtf8().constData());
        ASSERT_NE(m_fileIO.GetAlias("@log@"), nullptr);
        const AZStd::string configuredAlias = m_fileIO.GetAlias("@log@");

        {
            AzToolsFramework::TraceLogger logger;
            logger.OpenLogFile("trace.log", true);
        }

        EXPECT_STREQ(configuredAlias.c_str(), m_fileIO.GetAlias("@log@"));
        EXPECT_TRUE(QFileInfo::exists(root.filePath("isolated/trace.log")));
        EXPECT_FALSE(QFileInfo::exists(root.filePath("user/log/trace.log")));
    }

    TEST_F(TraceLoggerTest, OpenLogFileUsesDefaultWhenLogAliasIsMissing)
    {
        QTemporaryDir tempDir;
        ASSERT_TRUE(tempDir.isValid());
        const QDir root(tempDir.path());
        SetProjectAliases(root);
        ASSERT_EQ(m_fileIO.GetAlias("@log@"), nullptr);

        {
            AzToolsFramework::TraceLogger logger;
            logger.OpenLogFile("trace.log", true);
        }

        EXPECT_NE(m_fileIO.GetAlias("@log@"), nullptr);
        EXPECT_TRUE(QFileInfo::exists(root.filePath("user/log/trace.log")));
    }
} // namespace UnitTest

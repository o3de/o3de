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

#include <TestImpactFramework/TestImpactException.h>

#include <TestImpactChangeListFactory.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    TEST(ChangeListFactoryTest, NoRawData_ExpectArtifactException)
    {
        // Given an empty unified diff string
        const AZStd::string unifiedDiff;

        try
        {
            // When attempting to construct the change list
            const TestImpact::ChangeList changeList = TestImpact::UnifiedDiff::ChangeListFactory(unifiedDiff);

            // Do not expect this statement to be reachable
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::Exception& e)
        {
            // Expect an exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(ChangeListFactoryTest, NoChanges_ExpectArtifactException)
    {
        // Given a unified diff string with no changes
        const AZStd::string unifiedDiff = "On this day in 1738 absolutely nothing happened";

        try
        {
            // When attempting to construct the change list
            const TestImpact::ChangeList changeList = TestImpact::UnifiedDiff::ChangeListFactory(unifiedDiff);

            // Do not expect this statement to be reachable
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::Exception& e)
        {
            // Expect an exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(ChangeListFactoryTest, CreateOnly_ExpectValidChangeListWithFileCreateOperations)
    {
        // Given a unified diff with only one file creation and no file updates or deletions
        const AZStd::string unifiedDiff =
            "From f642a2f698452fc18484758b0046132415f09467 Mon Sep 17 00:00:00 2001\n"
            "From: user <user@website.com>\n"
            "Date: Sat, 13 Mar 2021 22:58:07 +0000\n"
            "Subject: Test\n"
            "\n"
            "---\n"
            " New.txt            | 1 +\n"
            " create mode 100644 New.txt\n"
            "diff --git a/New.txt b/New.txt\n"
            "new file mode 100644\n"
            "index 0000000..30d74d2\n"
            "--- /dev/null\n"
            "+++ b/New.txt\n"
            "@@ -0,0 +1 @@\n"
            "+test\n"
            "\\ No newline at end of file\n"
            "-- \n"
            "2.30.0.windows.2\n"
            "\n"
            "\n";

        // When attempting to construct the change list
        const TestImpact::ChangeList changeList = TestImpact::UnifiedDiff::ChangeListFactory(unifiedDiff);

        // Expect the change list to contain the 1 created file
        EXPECT_EQ(changeList.m_createdFiles.size(), 1);
        EXPECT_TRUE(
            AZStd::find(changeList.m_createdFiles.begin(), changeList.m_createdFiles.end(), "New.txt") != changeList.m_createdFiles.end());

        // Expect the change list to contain no updated files
        EXPECT_TRUE(changeList.m_updatedFiles.empty());

        // Expect the change list to contain no deleted files
        EXPECT_TRUE(changeList.m_deletedFiles.empty());
    }

    TEST(ChangeListFactoryTest, UpdateOnly_ExpectValidChangeListWithFileUpdateOperations)
    {
        // Given a unified diff with only one file update and no file creations or deletions
        const AZStd::string unifiedDiff =
            "From f642a2f698452fc18484758b0046132415f09467 Mon Sep 17 00:00:00 2001\n"
            "From: user <user@website.com>\n"
            "Date: Sat, 13 Mar 2021 22:58:07 +0000\n"
            "Subject: Test\n"
            "\n"
            "---\n"
            " A.txt              | 2 +-\n"
            "diff --git a/A.txt b/A.txt\n"
            "index 7c4a013..e132db2 100644\n"
            "--- a/A.txt\n"
            "+++ b/A.txt\n"
            "@@ -1 +1 @@\n"
            "-aaa\n"
            "\\ No newline at end of file\n"
            "+zzz\n"
            "\\ No newline at end of file\n"
            "-- \n"
            "2.30.0.windows.2\n"
            "\n"
            "\n";

        // When attempting to construct the change list
        const TestImpact::ChangeList changeList = TestImpact::UnifiedDiff::ChangeListFactory(unifiedDiff);

        // Expect the change list to contain no created files
        EXPECT_TRUE(changeList.m_createdFiles.empty());

        // Expect the change list to contain one updated file
        EXPECT_EQ(changeList.m_updatedFiles.size(), 1);
        EXPECT_TRUE(
            AZStd::find(changeList.m_updatedFiles.begin(), changeList.m_updatedFiles.end(), "A.txt") != changeList.m_updatedFiles.end());

        // Expect the change list to contain no deleted files
        EXPECT_TRUE(changeList.m_deletedFiles.empty());
    }

    TEST(ChangeListFactoryTest, DeleteOnly_ExpectValidChangeListWithFileDeleteOperations)
    {
        // Given a unified diff with only one file deletion and no file creations or updates
        const AZStd::string unifiedDiff =
            "From f642a2f698452fc18484758b0046132415f09467 Mon Sep 17 00:00:00 2001\n"
            "From: user <user@website.com>\n"
            "Date: Sat, 13 Mar 2021 22:58:07 +0000\n"
            "Subject: Test\n"
            "\n"
            "---\n"
            " B.txt              | 1 -\n"
            " delete mode 100644 B.txt\n"
            "diff --git a/B.txt b/B.txt\n"
            "deleted file mode 100644\n"
            "index 01f02e3..0000000\n"
            "--- a/B.txt\n"
            "+++ /dev/null\n"
            "@@ -1 +0,0 @@\n"
            "-bbb\n"
            "\\ No newline at end of file\n"
            "-- \n"
            "2.30.0.windows.2\n"
            "\n"
            "\n";

        // When attempting to construct the change list
        const TestImpact::ChangeList changeList = TestImpact::UnifiedDiff::ChangeListFactory(unifiedDiff);

        // Expect the change list to contain no created files
        EXPECT_TRUE(changeList.m_createdFiles.empty());

        // Expect the change list to contain no updated files
        EXPECT_TRUE(changeList.m_updatedFiles.empty());

        // Expect the change list to contain one deleted file
        EXPECT_EQ(changeList.m_deletedFiles.size(), 1);
        EXPECT_TRUE(
            AZStd::find(changeList.m_deletedFiles.begin(), changeList.m_deletedFiles.end(), "B.txt") != changeList.m_deletedFiles.end());
    }

    TEST(ChangeListFactoryTest, ParseUnifiedDiffWithAllPossibleOperations_ExpectChangeListMatchingOperations)
    {
        // Given a unified diff with created files, updated files, deleted files, renamed files and moved files
        const AZStd::string unifiedDiff =
            "From f642a2f698452fc18484758b0046132415f09467 Mon Sep 17 00:00:00 2001\n"
            "From: user <user@website.com>\n"
            "Date: Sat, 13 Mar 2021 22:58:07 +0000\n"
            "Subject: Test\n"
            "\n"
            "---\n"
            " A.txt              | 2 +-\n"
            " B.txt              | 1 -\n"
            " D.txt => Foo/D.txt | 0\n"
            " E.txt => Foo/Y.txt | 0\n"
            " New.txt            | 1 +\n"
            " C.txt => X.txt     | 0\n"
            " 6 files changed, 2 insertions(+), 2 deletions(-)\n"
            " delete mode 100644 B.txt\n"
            " rename D.txt => Foo/D.txt (100%)\n"
            " rename E.txt => Foo/Y.txt (100%)\n"
            " create mode 100644 New.txt\n"
            " rename C.txt => X.txt (100%)\n"
            "\n"
            "diff --git a/A.txt b/A.txt\n"
            "index 7c4a013..e132db2 100644\n"
            "--- a/A.txt\n"
            "+++ b/A.txt\n"
            "@@ -1 +1 @@\n"
            "-aaa\n"
            "\\ No newline at end of file\n"
            "+zzz\n"
            "\\ No newline at end of file\n"
            "diff --git a/B.txt b/B.txt\n"
            "deleted file mode 100644\n"
            "index 01f02e3..0000000\n"
            "--- a/B.txt\n"
            "+++ /dev/null\n"
            "@@ -1 +0,0 @@\n"
            "-bbb\n"
            "\\ No newline at end of file\n"
            "diff --git a/D.txt b/Foo/D.txt\n"
            "similarity index 100%\n"
            "rename from D.txt\n"
            "rename to Foo/D.txt\n"
            "diff --git a/E.txt b/Foo/Y.txt\n"
            "similarity index 100%\n"
            "rename from E.txt\n"
            "rename to Foo/Y.txt\n"
            "diff --git a/New.txt b/New.txt\n"
            "new file mode 100644\n"
            "index 0000000..30d74d2\n"
            "--- /dev/null\n"
            "+++ b/New.txt\n"
            "@@ -0,0 +1 @@\n"
            "+test\n"
            "\\ No newline at end of file\n"
            "diff --git a/C.txt b/X.txt\n"
            "similarity index 100%\n"
            "rename from C.txt\n"
            "rename to X.txt\n"
            "-- \n"
            "2.30.0.windows.2\n"
            "\n"
            "\n";

        // When attempting to construct the change list
        const TestImpact::ChangeList changeList = TestImpact::UnifiedDiff::ChangeListFactory(unifiedDiff);

        // Expect the change list to contain the 4 created files
        EXPECT_EQ(changeList.m_createdFiles.size(), 4);
        EXPECT_TRUE(
            AZStd::find(changeList.m_createdFiles.begin(), changeList.m_createdFiles.end(), "Foo/D.txt") !=
            changeList.m_createdFiles.end());
        EXPECT_TRUE(
            AZStd::find(changeList.m_createdFiles.begin(), changeList.m_createdFiles.end(), "Foo/Y.txt") !=
            changeList.m_createdFiles.end());
        EXPECT_TRUE(
            AZStd::find(changeList.m_createdFiles.begin(), changeList.m_createdFiles.end(), "X.txt") != changeList.m_createdFiles.end());
        EXPECT_TRUE(
            AZStd::find(changeList.m_createdFiles.begin(), changeList.m_createdFiles.end(), "New.txt") != changeList.m_createdFiles.end());

        // Expect the change list to contain the 1 updated file
        EXPECT_EQ(changeList.m_updatedFiles.size(), 1);
        EXPECT_TRUE(
            AZStd::find(changeList.m_updatedFiles.begin(), changeList.m_updatedFiles.end(), "A.txt") != changeList.m_updatedFiles.end());

        // Expect the change list to contain the 4 deleted files
        EXPECT_EQ(changeList.m_deletedFiles.size(), 4);
        EXPECT_TRUE(
            AZStd::find(changeList.m_deletedFiles.begin(), changeList.m_deletedFiles.end(), "B.txt") != changeList.m_deletedFiles.end());
        EXPECT_TRUE(
            AZStd::find(changeList.m_deletedFiles.begin(), changeList.m_deletedFiles.end(), "D.txt") != changeList.m_deletedFiles.end());
        EXPECT_TRUE(
            AZStd::find(changeList.m_deletedFiles.begin(), changeList.m_deletedFiles.end(), "E.txt") != changeList.m_deletedFiles.end());
        EXPECT_TRUE(
            AZStd::find(changeList.m_deletedFiles.begin(), changeList.m_deletedFiles.end(), "C.txt") != changeList.m_deletedFiles.end());
    }
} // namespace UnitTest

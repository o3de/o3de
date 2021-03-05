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
#include <ResourceCompiler.h>
#include <AzTest/AzTest.h>
#include <AzCore/Memory/AllocatorScope.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <fstream>

class ResourceCompilerTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    virtual ~ResourceCompilerTestEnvironment()
    {}

protected:
    void SetupEnvironment() override
    {
        m_allocatorScope.ActivateAllocators();
        AZ::IO::FileIOBase::SetInstance(nullptr); // The API requires the old instance to be destroyed first
        AZ::IO::FileIOBase::SetInstance(new AZ::IO::LocalFileIO());
    }

    void TeardownEnvironment() override
    {
        m_allocatorScope.DeactivateAllocators();
    }
    AZ::AllocatorScope<AZ::OSAllocator, AZ::SystemAllocator, AZ::LegacyAllocator, CryStringAllocator> m_allocatorScope;
};

AZ_UNIT_TEST_HOOK(new ResourceCompilerTestEnvironment)

TEST(CollectFilesTest, CollectFiles_PatternWithAndWithoutWildcards_Success)
{
    UnitTest::ScopedTemporaryDirectory tempDir;

    AZStd::string tempRoot;

    AZ::StringFunc::Path::Join(tempDir.GetDirectory(), "root", tempRoot);
    AZ::IO::LocalFileIO::GetInstance()->CreatePath(tempRoot.c_str());

    AZStd::string tempFile;
    AZ::StringFunc::Path::Join(tempRoot.c_str(), "temp.xml", tempFile);
    std::ofstream outFile(tempFile.c_str(), std::ofstream::out | std::ofstream::app);
    outFile << "Temp";
    outFile.close();
    AZStd::string sourceRoots{ tempRoot };

    AZStd::string tempSubFolder;
    AZ::StringFunc::Path::Join(tempRoot.c_str(), "foldera", tempSubFolder);
    AZ::IO::LocalFileIO::GetInstance()->CreatePath(tempSubFolder.c_str());
    
    AZ::StringFunc::Path::Join(tempSubFolder.c_str(), "tempfile1.xml", tempFile);
    std::ofstream outFile1(tempFile.c_str(), std::ofstream::out | std::ofstream::app);
    outFile1 << "Tempfile1";
    outFile1.close();

    AZ::StringFunc::Path::Join(tempRoot.c_str(), "folderb", tempSubFolder);
    AZ::IO::LocalFileIO::GetInstance()->CreatePath(tempSubFolder.c_str());

    AZ::StringFunc::Path::Join(tempSubFolder.c_str(), "tempfile2.xml", tempFile);
    std::ofstream outFile2(tempFile.c_str(), std::ofstream::out | std::ofstream::app);
    outFile2 << "Tempfile2";
    outFile2.close();

    ResourceCompiler testResourceCompiler;

    testResourceCompiler.RegisterDefaultKeys();

    testResourceCompiler.GetMultiplatformConfig().init(1, 0, &testResourceCompiler);
    testResourceCompiler.GetMultiplatformConfig().getConfig().SetKeyValue(eCP_PriorityCmdline, "sourceroot", sourceRoots.c_str());

    std::vector<RcFile> resultList;
    testResourceCompiler.CollectFilesToCompile("temp.xml", resultList);
    EXPECT_EQ(resultList.size(), 1);

    testResourceCompiler.CollectFilesToCompile("foldera/*.xml", resultList);
    EXPECT_EQ(resultList.size(), 1);

    testResourceCompiler.CollectFilesToCompile("foldera/*.xml;temp.xml;folderb/*.xml", resultList);
    EXPECT_EQ(resultList.size(), 3);
}

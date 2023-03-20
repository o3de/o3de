/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/IO/FileReader.h>
#include <FileIOBaseTestTypes.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    template <typename FileIOType>
    class FileReaderTestFixture
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            if constexpr (AZStd::is_same_v<FileIOType, TestFileIOBase>)
            {
                m_fileIo = AZStd::make_unique<TestFileIOBase>();
            }
        }

        void TearDown() override
        {
            m_fileIo.reset();
        }

    protected:
        AZStd::unique_ptr<AZ::IO::FileIOBase> m_fileIo{};
    };

    using FileIOTypes = ::testing::Types<void, TestFileIOBase>;

    TYPED_TEST_CASE(FileReaderTestFixture, FileIOTypes);

    TYPED_TEST(FileReaderTestFixture, ConstructorWithFilePath_OpensFileSuccessfully)
    {
        AZ::IO::FileReader fileReader(this->m_fileIo.get(), AZ::IO::SystemFile::GetNullFilename());
        EXPECT_TRUE(fileReader.IsOpen());
    }

    TYPED_TEST(FileReaderTestFixture, Open_OpensFileSucessfully)
    {
        AZ::IO::FileReader fileReader;
        fileReader.Open(this->m_fileIo.get(), AZ::IO::SystemFile::GetNullFilename());
        EXPECT_TRUE(fileReader.IsOpen());
    }

    TYPED_TEST(FileReaderTestFixture, Eof_OnNULDeviceFile_Succeeds)
    {
        AZ::IO::FileReader fileReader(this->m_fileIo.get(), AZ::IO::SystemFile::GetNullFilename());
        EXPECT_TRUE(fileReader.Eof());
    }

    TYPED_TEST(FileReaderTestFixture, GetFilePath_ReturnsNULDeviceFilename_Succeeds)
    {
        AZ::IO::FileReader fileReader(this->m_fileIo.get(), AZ::IO::SystemFile::GetNullFilename());
        AZ::IO::FixedMaxPath filePath;
        EXPECT_TRUE(fileReader.GetFilePath(filePath));
        AZ::IO::FixedMaxPath nulFilename{ AZ::IO::SystemFile::GetNullFilename() };
        if (this->m_fileIo)
        {
            EXPECT_TRUE(this->m_fileIo->ResolvePath(nulFilename, nulFilename));
        }
        EXPECT_EQ(nulFilename, filePath);
    }

}   // namespace UnitTest

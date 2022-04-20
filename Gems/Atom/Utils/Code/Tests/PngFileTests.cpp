/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/PngFile.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace UnitTest
{
    using namespace AZ::Utils;

    class PngFileTests
        : public AllocatorsFixture
    {
    protected:
        AZ::IO::Path m_testImageFolder;
        AZ::IO::Path m_tempPngFilePath;
        AZStd::vector<uint8_t> m_primaryColors3x1;
        AZStd::unique_ptr<AZ::IO::FileIOBase> m_localFileIO;

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            AZ::SettingsRegistryImpl localRegistry;
            localRegistry.Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, AZ::Test::GetEngineRootPath());

            // Look up the path to the Atom Gem Folder(don't assume it is in the Engine root)
            // via searching through the gem paths that are registered in the o3de manifest files
            // Adding the Atom gem as an active gem allows the alias of @gemroot:atom@ to be
            // set in the fileIO
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ManifestGemsPaths(localRegistry);
            AZ::Test::AddActiveGem("Atom", localRegistry, AZ::IO::FileIOBase::GetInstance());

            ASSERT_TRUE(localRegistry.Get(m_testImageFolder.Native(), FixedValueString::format("%s/Atom/Path",
                AZ::SettingsRegistryMergeUtils::ManifestGemsRootKey)));

            m_testImageFolder /= "Utils/Code/Tests/PngTestImages";
            
            m_tempPngFilePath = m_testImageFolder / "temp.png";
                        
            m_localFileIO.reset(aznew AZ::IO::LocalFileIO());
            AZ::IO::FileIOBase::SetInstance(m_localFileIO.get());

            AZ::IO::FileIOBase::GetInstance()->Remove(m_tempPngFilePath.c_str());

            m_primaryColors3x1 = {
                255u,   0u,   0u, 255u,
                0u,   255u,   0u, 255u,
                0u,     0u, 255u, 255u
            };
        }

        void TearDown() override
        {
            m_testImageFolder = AZ::IO::Path{};
            m_tempPngFilePath = AZ::IO::Path{};
            m_primaryColors3x1 = AZStd::vector<uint8_t>{};

            AZ::IO::FileIOBase::SetInstance(nullptr);
            m_localFileIO.reset();

            AllocatorsFixture::TearDown();
        }

        struct Color3 : public AZStd::array<uint8_t, 3>
        {
            using Base = AZStd::array<uint8_t, 3>;
            Color3(uint8_t r, uint8_t g, uint8_t b) : Base({r, g, b}) {}
            Color3(const uint8_t* raw) : Base({raw[0], raw[1], raw[2]}) {}
        };
        
        struct Color4 : public AZStd::array<uint8_t, 4>
        {
            using Base = AZStd::array<uint8_t, 4>;
            Color4(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : Base({r, g, b, a}) {}
            Color4(const uint8_t* raw) : Base({raw[0], raw[1], raw[2], raw[3]}) {}
        };
    };

    TEST_F(PngFileTests, LoadRgb)
    {
        PngFile image = PngFile::Load((m_testImageFolder / "ColorChart_rgb.png").c_str());
        EXPECT_TRUE(image.IsValid());
        EXPECT_EQ(image.GetBufferFormat(), PngFile::Format::RGB);
        EXPECT_EQ(image.GetWidth(), 3);
        EXPECT_EQ(image.GetHeight(), 2);
        EXPECT_EQ(image.GetBuffer().size(), 18);
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  0), Color3(255u,   0u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  3), Color3(0u,   255u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  6), Color3(0u,     0u, 255u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  9), Color3(255u, 255u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 12), Color3(0u,   255u, 255u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 15), Color3(255u,   0u, 255u));
    }

    TEST_F(PngFileTests, LoadRgba)
    {
        PngFile image = PngFile::Load((m_testImageFolder / "ColorChart_rgba.png").c_str());
        EXPECT_TRUE(image.IsValid());
        EXPECT_EQ(image.GetBufferFormat(), PngFile::Format::RGBA);
        EXPECT_EQ(image.GetWidth(), 3);
        EXPECT_EQ(image.GetHeight(), 2);
        EXPECT_EQ(image.GetBuffer().size(), 24);
        EXPECT_EQ(Color4(image.GetBuffer().begin() +  0), Color4(255u,   0u,   0u, 200u));
        EXPECT_EQ(Color4(image.GetBuffer().begin() +  4), Color4(0u,   255u,   0u, 150u));
        EXPECT_EQ(Color4(image.GetBuffer().begin() +  8), Color4(0u,     0u, 255u, 100u));
        EXPECT_EQ(Color4(image.GetBuffer().begin() + 12), Color4(255u, 255u,   0u, 125u));
        EXPECT_EQ(Color4(image.GetBuffer().begin() + 16), Color4(0u,   255u, 255u, 175u));
        EXPECT_EQ(Color4(image.GetBuffer().begin() + 20), Color4(255u,   0u, 255u,  75u));
    }
    
    TEST_F(PngFileTests, LoadRgbaStripAlpha)
    {
        PngFile::LoadSettings loadSettings;
        loadSettings.m_stripAlpha = true;

        PngFile image = PngFile::Load((m_testImageFolder / "ColorChart_rgba.png").c_str(), loadSettings);
        // Note these checks are identical to the LoadRgb test.
        EXPECT_TRUE(image.IsValid());
        EXPECT_EQ(image.GetBufferFormat(), PngFile::Format::RGB);
        EXPECT_EQ(image.GetWidth(), 3);
        EXPECT_EQ(image.GetHeight(), 2);
        EXPECT_EQ(image.GetBuffer().size(), 18);
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  0), Color3(255u,   0u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  3), Color3(0u,   255u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  6), Color3(0u,     0u, 255u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  9), Color3(255u, 255u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 12), Color3(0u,   255u, 255u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 15), Color3(255u,   0u, 255u));
    }
    
    TEST_F(PngFileTests, LoadColorPaletteTwoBits)
    {
        PngFile image = PngFile::Load((m_testImageFolder / "ColorPalette_2bit.png").c_str());
        EXPECT_TRUE(image.IsValid());
        EXPECT_EQ(image.GetBufferFormat(), PngFile::Format::RGB);
        EXPECT_EQ(image.GetWidth(), 1);
        EXPECT_EQ(image.GetHeight(), 3);
        EXPECT_EQ(image.GetBuffer().size(), 9);
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  0), Color3(255u,   0u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  3), Color3(0u,   255u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  6), Color3(0u,     0u, 255u));
    }

    TEST_F(PngFileTests, LoadGrayscaleOneBit)
    {
        PngFile image = PngFile::Load((m_testImageFolder / "GrayPalette_1bit.png").c_str());
        EXPECT_TRUE(image.IsValid());
        EXPECT_EQ(image.GetBufferFormat(), PngFile::Format::RGB);
        EXPECT_EQ(image.GetWidth(), 1);
        EXPECT_EQ(image.GetHeight(), 2);
        EXPECT_EQ(image.GetBuffer().size(), 6);
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  0), Color3(0u,     0u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  3), Color3(255u, 255u, 255u));
    }
    
    TEST_F(PngFileTests, LoadRgba64Bits)
    {
        PngFile image = PngFile::Load((m_testImageFolder / "Gradient_rgb_16bpc.png").c_str());
        EXPECT_TRUE(image.IsValid());
        EXPECT_EQ(image.GetBufferFormat(), PngFile::Format::RGB);
        EXPECT_EQ(image.GetWidth(), 5);
        EXPECT_EQ(image.GetHeight(), 1);
        EXPECT_EQ(image.GetBuffer().size(), 15);
        // The values in this file are 30.0f, 30.1f, 30.2f, 30.3f, 30.4f. But we use PNG_TRANSFORM_STRIP_16 to reduce them to 8 bits per channel for simplicity.
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  0), Color3(76u,   0u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  3), Color3(77u,   0u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  6), Color3(77u,   0u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() +  9), Color3(77u,   0u,   0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 12), Color3(77u,   0u,   0u));
    }
    
    TEST_F(PngFileTests, CreateCopy)
    {
        AZStd::vector<uint8_t> data = m_primaryColors3x1;

        PngFile savedImage = PngFile::Create(AZ::RHI::Size{3, 1, 0}, AZ::RHI::Format::R8G8B8A8_UNORM, data);
        EXPECT_TRUE(savedImage.IsValid());
        EXPECT_EQ(savedImage.GetWidth(), 3);
        EXPECT_EQ(savedImage.GetHeight(), 1);
        EXPECT_EQ(savedImage.GetBuffer(), data);
    }

    TEST_F(PngFileTests, CreateMove)
    {
        AZStd::vector<uint8_t> data = m_primaryColors3x1;

        PngFile savedImage = PngFile::Create(AZ::RHI::Size{3, 1, 0}, AZ::RHI::Format::R8G8B8A8_UNORM, AZStd::move(data));
        EXPECT_TRUE(savedImage.IsValid());
        EXPECT_EQ(savedImage.GetWidth(), 3);
        EXPECT_EQ(savedImage.GetHeight(), 1);
        EXPECT_EQ(savedImage.GetBuffer(), m_primaryColors3x1);
        EXPECT_TRUE(data.empty()); // The data should have been moved
    }

    TEST_F(PngFileTests, SaveRgba)
    {
        PngFile savedImage = PngFile::Create(AZ::RHI::Size{3, 1, 0}, AZ::RHI::Format::R8G8B8A8_UNORM, m_primaryColors3x1);
        bool result = savedImage.Save(m_tempPngFilePath.c_str());
        EXPECT_TRUE(result);

        PngFile loadedImage = PngFile::Load(m_tempPngFilePath.c_str());
        EXPECT_TRUE(loadedImage.IsValid());
        EXPECT_EQ(loadedImage.GetBufferFormat(), savedImage.GetBufferFormat());
        EXPECT_EQ(loadedImage.GetWidth(), savedImage.GetWidth());
        EXPECT_EQ(loadedImage.GetHeight(), savedImage.GetHeight());
        EXPECT_EQ(loadedImage.GetBuffer(), savedImage.GetBuffer());
    }
    
    TEST_F(PngFileTests, SaveRgbaStripAlpha)
    {
        PngFile savedImage = PngFile::Create(AZ::RHI::Size{3, 1, 0}, AZ::RHI::Format::R8G8B8A8_UNORM, m_primaryColors3x1);

        PngFile::SaveSettings saveSettings;
        saveSettings.m_stripAlpha = true;

        bool result = savedImage.Save(m_tempPngFilePath.c_str(), saveSettings);
        EXPECT_TRUE(result);

        // The alpha was stripped when saving. Now we load the data without stripping anything and should find
        // that there is no alpha channel.

        PngFile loadedImage = PngFile::Load(m_tempPngFilePath.c_str());

        // The dimensions are the same...
        EXPECT_TRUE(loadedImage.IsValid());
        EXPECT_EQ(loadedImage.GetWidth(), savedImage.GetWidth());
        EXPECT_EQ(loadedImage.GetHeight(), savedImage.GetHeight());

        // ... but the format is different
        EXPECT_NE(loadedImage.GetBufferFormat(), savedImage.GetBufferFormat());
        EXPECT_EQ(loadedImage.GetBufferFormat(), PngFile::Format::RGB);

        // ... and the loaded data is smaller
        EXPECT_NE(loadedImage.GetBuffer(), savedImage.GetBuffer());
        EXPECT_EQ(Color3(loadedImage.GetBuffer().begin() +  0), Color3(255u,   0u,   0u));
        EXPECT_EQ(Color3(loadedImage.GetBuffer().begin() +  3), Color3(0u,   255u,   0u));
        EXPECT_EQ(Color3(loadedImage.GetBuffer().begin() +  6), Color3(0u,     0u, 255u));
    }
    
    TEST_F(PngFileTests, Error_CreateUnsupportedFormat)
    {
        AZStd::vector<uint8_t> data = m_primaryColors3x1;

        AZStd::string gotErrorMessage;

        PngFile savedImage = PngFile::Create(AZ::RHI::Size{3, 1, 0}, AZ::RHI::Format::R32_UINT, data,
            [&gotErrorMessage](const char* errorMessage) { gotErrorMessage = errorMessage; });

        EXPECT_FALSE(savedImage.IsValid());
        EXPECT_TRUE(gotErrorMessage.find("unsupported format R32_UINT") != AZStd::string::npos);
    }
    
    TEST_F(PngFileTests, Error_CreateIncorrectBufferSize)
    {
        AZStd::vector<uint8_t> data = m_primaryColors3x1;

        AZStd::string gotErrorMessage;

        PngFile savedImage = PngFile::Create(AZ::RHI::Size{3, 2, 0}, AZ::RHI::Format::R8G8B8A8_UNORM, data,
            [&gotErrorMessage](const char* errorMessage) { gotErrorMessage = errorMessage; });

        EXPECT_FALSE(savedImage.IsValid());
        EXPECT_TRUE(gotErrorMessage.find("does not match") != AZStd::string::npos);
    }
    
    TEST_F(PngFileTests, Error_LoadFileNotFound)
    {
        AZStd::string gotErrorMessage;

        PngFile::LoadSettings loadSettings;
        loadSettings.m_errorHandler = [&gotErrorMessage](const char* errorMessage) { gotErrorMessage = errorMessage; };

        PngFile image = PngFile::Load((m_testImageFolder / "DoesNotExist.png").c_str(), loadSettings);
        EXPECT_FALSE(image.IsValid());
        EXPECT_TRUE(gotErrorMessage.find("not open file") != AZStd::string::npos);
    }
    
    TEST_F(PngFileTests, Error_LoadEmptyFile)
    {
        AZStd::string gotErrorMessage;

        PngFile::LoadSettings loadSettings;
        loadSettings.m_errorHandler = [&gotErrorMessage](const char* errorMessage) { gotErrorMessage = errorMessage; };

        PngFile image = PngFile::Load((m_testImageFolder / "EmptyFile.png").c_str(), loadSettings);
        EXPECT_FALSE(image.IsValid());
        EXPECT_TRUE(gotErrorMessage.find("Invalid png header") != AZStd::string::npos);
    }
    
    TEST_F(PngFileTests, Error_LoadNotPngFile)
    {
        AZStd::string gotErrorMessage;

        PngFile::LoadSettings loadSettings;
        loadSettings.m_errorHandler = [&gotErrorMessage](const char* errorMessage) { gotErrorMessage = errorMessage; };

        PngFile image = PngFile::Load((m_testImageFolder / "ColorChart_rgba.jpg").c_str(), loadSettings);
        EXPECT_FALSE(image.IsValid());
        EXPECT_TRUE(gotErrorMessage.find("Invalid png header") != AZStd::string::npos);
    }
    
    TEST_F(PngFileTests, Error_SaveInvalidPngFile)
    {
        AZStd::string gotErrorMessage;

        PngFile::SaveSettings saveSettings;
        saveSettings.m_errorHandler = [&gotErrorMessage](const char* errorMessage) { gotErrorMessage = errorMessage; };
        
        PngFile savedImage;
        bool result = savedImage.Save(m_tempPngFilePath.c_str(), saveSettings);
        EXPECT_FALSE(result);
        EXPECT_TRUE(gotErrorMessage.find("PngFile is invalid") != AZStd::string::npos);
        EXPECT_FALSE(AZ::IO::FileIOBase::GetInstance()->Exists(m_tempPngFilePath.c_str()));
    }

    TEST_F(PngFileTests, LoadRgbFromMemoryBuffer)
    {
        // This is an in-memory copy of the ColorChart_rgb.png test file.
        AZStd::fixed_vector<uint8_t, 126> pngBuffer =
        {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
            0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,

            0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02,
            0x08, 0x02, 0x00, 0x00, 0x00, 0x12, 0x16, 0xf1,

            0x4d, 0x00, 0x00, 0x00, 0x01, 0x73, 0x52, 0x47,
            0x42, 0x00, 0xae, 0xce, 0x1c, 0xe9, 0x00, 0x00,

            0x00, 0x04, 0x67, 0x41, 0x4d, 0x41, 0x00, 0x00,
            0xb1, 0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00,

            0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00,
            0x0e, 0xc3, 0x00, 0x00, 0x0e, 0xc3, 0x01, 0xc7,

            0x6f, 0xa8, 0x64, 0x00, 0x00, 0x00, 0x13, 0x49,
            0x44, 0x41, 0x54, 0x18, 0x57, 0x63, 0xf8, 0xcf,

            0xc0, 0x00, 0xc1, 0x4c, 0x10, 0xea, 0x3f, 0x03,
            0x03, 0x00, 0x3b, 0xec, 0x05, 0xfd, 0x6a, 0x50,

            0x07, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
            0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
        };

        PngFile image = PngFile::LoadFromBuffer(pngBuffer);
        EXPECT_TRUE(image.IsValid());
        EXPECT_EQ(image.GetBufferFormat(), PngFile::Format::RGB);
        EXPECT_EQ(image.GetWidth(), 3);
        EXPECT_EQ(image.GetHeight(), 2);
        EXPECT_EQ(image.GetBuffer().size(), 18);
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 0), Color3(255u, 0u, 0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 3), Color3(0u, 255u, 0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 6), Color3(0u, 0u, 255u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 9), Color3(255u, 255u, 0u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 12), Color3(0u, 255u, 255u));
        EXPECT_EQ(Color3(image.GetBuffer().begin() + 15), Color3(255u, 0u, 255u));
    }

    TEST_F(PngFileTests, ErrorCannotLoadEmptyMemoryBuffer)
    {
        AZStd::vector<uint8_t> pngBuffer;

        AZStd::string gotErrorMessage;

        PngFile::LoadSettings loadSettings;
        loadSettings.m_errorHandler = [&gotErrorMessage](const char* errorMessage)
        {
            gotErrorMessage = errorMessage;
        };

        PngFile image = PngFile::LoadFromBuffer(pngBuffer, loadSettings);
        EXPECT_FALSE(image.IsValid());
        EXPECT_TRUE(gotErrorMessage.find("Buffer is empty") != AZStd::string::npos);
    }

} // namespace UnitTest

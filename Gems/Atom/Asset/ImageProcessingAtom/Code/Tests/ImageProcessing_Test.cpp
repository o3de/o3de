/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>

#include <AzQtComponents/Utilities/QtPluginPaths.h>

#include <AzCore/AzCore_Traits_Platform.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

#include <AzFramework/IO/LocalFileIO.h>

#include <Atom/ImageProcessing/ImageObject.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/ImageConvert.h>
#include <Processing/ImageToProcess.h>
#include <Processing/ImageAssetProducer.h>
#include <Processing/ImageFlags.h>
#include <ImageLoader/ImageLoaders.h>

#include <Compressors/Compressor.h>

#include <Converters/Cubemap.h>

#include <BuilderSettings/BuilderSettingManager.h>
#include <BuilderSettings/CubemapSettings.h>
#include <BuilderSettings/ImageProcessingDefines.h>
#include <BuilderSettings/PresetSettings.h>

#include <Editor/EditorCommon.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>

#include <QFileInfo>
#include <qdir.h>
#include <QDirIterator>
#include <QIODevice>

#include <array>
#include <utility>

//Enable generate image files for result of some tests.
//This is slow and only useful for debugging. This should be disabled for unit test
//#define DEBUG_OUTPUT_IMAGES

//There are some test functions in this test which are DISABLED. They were mainly for programming tests.
//It's only recommended to enable them for programming test purpose.

#include <AzCore/UnitTest/TestTypes.h>
#include <ImageBuilderComponent.h>

using namespace ImageProcessingAtom;

namespace UnitTest
{
    // Expose AZ::AssetManagerComponent::Reflect function for testing
    class MyAssetManagerComponent
        : public AZ::AssetManagerComponent
    {
    public:
        static void Reflect(ReflectContext* reflection)
        {
            AZ::AssetManagerComponent::Reflect(reflection);
        };
    };

    class ImageProcessingTest
        : public ::testing::Test
        , public AllocatorsBase
        , public AZ::ComponentApplicationBus::Handler
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationMessages.
        AZ::ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&) override { }
        void RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&) override { }
        void RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&) override { }
        void RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&) override { }
        void SignalEntityActivated(AZ::Entity*) override { }
        void SignalEntityDeactivated(AZ::Entity*) override { }
        bool AddEntity(AZ::Entity*) override { return false; }
        bool RemoveEntity(AZ::Entity*) override { return false; }
        bool DeleteEntity(const AZ::EntityId&) override { return false; }
        Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
        AZ::SerializeContext* GetSerializeContext() override { return m_context.get(); }
        AZ::BehaviorContext*  GetBehaviorContext() override { return nullptr; }
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return m_jsonRegistrationContext.get(); }
        const char* GetAppRoot() const override { return nullptr; }
        const char* GetEngineRoot() const override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        void EnumerateEntities(const AZ::ComponentApplicationRequests::EntityCallback& /*callback*/) override {}
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        //////////////////////////////////////////////////////////////////////////

    protected:

        AZStd::unique_ptr<AZ::SerializeContext> m_context;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<AZ::JsonSystemComponent> m_jsonSystemComponent;
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
        AZStd::string m_gemFolder;
        AZStd::string m_outputRootFolder;
        AZStd::string m_outputFolder;

        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;

        void SetUp() override
        {
            AllocatorsBase::SetupAllocator();

            // Adding this handler to allow utility functions access the serialize context
            ComponentApplicationBus::Handler::BusConnect();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            // AssetManager required to generate image assets
            AZ::Data::AssetManager::Descriptor desc;
            AZ::Data::AssetManager::Create(desc);

            AZ::NameDictionary::Create();

            m_assetHandlers.emplace_back(AZ::RPI::MakeAssetHandler<AZ::RPI::ImageMipChainAssetHandler>());
            m_assetHandlers.emplace_back(AZ::RPI::MakeAssetHandler<AZ::RPI::StreamingImageAssetHandler>());
            m_assetHandlers.emplace_back(AZ::RPI::MakeAssetHandler<AZ::RPI::StreamingImagePoolAssetHandler>());

            BuilderSettingManager::CreateInstance();

            //prepare reflection
            m_context = AZStd::make_unique<AZ::SerializeContext>();
            AZ::Name::Reflect(m_context.get());
            BuilderPluginComponent::Reflect(m_context.get());
            AZ::DataPatch::Reflect(m_context.get());
            AZ::RHI::ReflectSystemComponent::Reflect(m_context.get());
            AZ::RPI::ImageMipChainAsset::Reflect(m_context.get());
            AZ::RPI::ImageAsset::Reflect(m_context.get());
            AZ::RPI::StreamingImageAsset::Reflect(m_context.get());
            MyAssetManagerComponent::Reflect(m_context.get());

            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
            m_jsonSystemComponent = AZStd::make_unique<AZ::JsonSystemComponent>();
            m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
            AZ::Name::Reflect(m_jsonRegistrationContext.get());
            BuilderPluginComponent::Reflect(m_jsonRegistrationContext.get());

            // Setup job context for job system
            JobManagerDesc jobManagerDesc;
            JobManagerThreadDesc threadDesc;
#if AZ_TRAIT_SET_JOB_PROCESSOR_ID
            threadDesc.m_cpuId = 0; // Don't set processors IDs on windows
#endif 

            uint32_t numWorkerThreads = AZStd::thread::hardware_concurrency();

            for (unsigned int i = 0; i < numWorkerThreads; ++i)
            {
                jobManagerDesc.m_workerThreads.push_back(threadDesc);
#if AZ_TRAIT_SET_JOB_PROCESSOR_ID
                threadDesc.m_cpuId++;
#endif 
            }

            m_jobManager = AZStd::make_unique<JobManager>(jobManagerDesc);
            m_jobContext = AZStd::make_unique<JobContext>(*m_jobManager);
            JobContext::SetGlobalContext(m_jobContext.get());

            // Startup default local FileIO (hits OSAllocator) if not already setup.
            if (AZ::IO::FileIOBase::GetInstance() == nullptr)
            {
                AZ::IO::FileIOBase::SetInstance(aznew AZ::IO::LocalFileIO());
            }

            //load qt plug-ins for some image file formats support
            AzQtComponents::PrepareQtPaths();

            m_gemFolder = AZ::Test::GetEngineRootPath() + "/Gems/Atom/Asset/ImageProcessingAtom/";
            m_outputFolder = m_gemFolder + AZStd::string("Code/Tests/TestAssets/temp/");

            m_defaultSettingFolder = m_gemFolder + AZStd::string("Config/");
            m_testFileFolder = m_gemFolder + AZStd::string("Code/Tests/TestAssets/");

            InitialImageFilenames();

            ImageProcessingAtomEditor::EditorHelper::InitPixelFormatString();
        }

        void TearDown() override
        {
            m_gemFolder = AZStd::string();
            m_outputFolder = AZStd::string();
            m_defaultSettingFolder = AZStd::string();
            m_testFileFolder = AZStd::string();

            m_imagFileNameMap = AZStd::map<ImageFeature, AZStd::string>();
            m_assetHandlers = AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>>();

            delete AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);

            JobContext::SetGlobalContext(nullptr);
            m_jobContext = nullptr;
            m_jobManager = nullptr;

            m_jsonRegistrationContext->EnableRemoveReflection();
            m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
            BuilderPluginComponent::Reflect(m_jsonRegistrationContext.get());
            AZ::Name::Reflect(m_jsonRegistrationContext.get());
            m_jsonRegistrationContext->DisableRemoveReflection();
            m_jsonRegistrationContext.reset();
            m_jsonSystemComponent.reset();

            m_context.reset();
            BuilderSettingManager::DestroyInstance();

            CPixelFormats::DestroyInstance();

            AZ::NameDictionary::Destroy();

            AZ::Data::AssetManager::Destroy();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
            ComponentApplicationBus::Handler::BusDisconnect();
            AllocatorsBase::TeardownAllocator();
        }

        //enum names for Images with specific identification
        enum ImageFeature
        {
            Image_20X16_RGBA8_Png = 0,
            Image_32X32_16bit_F_Tif,
            Image_32X32_32bit_F_Tif,
            Image_200X200_RGB8_Jpg,
            Image_512X288_RGB8_Tga,
            Image_1024X1024_RGB8_Tif,
            Image_UpperCase_Tga,
            Image_1024x1024_normal_tiff,
            Image_128x128_Transparent_Tga,
            Image_237x177_RGB_Jpg,
            Image_GreyScale_Png,
            Image_Alpha8_64x64_Mip7_Dds,
            Image_BGRA_64x64_Mip7_Dds,
            Image_Luminance8bpp_66x33_dds,
            Image_BGR_64x64_dds,
            Image_defaultprobe_cm_1536x256_64bits_tif,
            Image_workshop_iblskyboxcm_exr
        };

        //image file names for testing
        AZStd::map<ImageFeature, AZStd::string> m_imagFileNameMap;

        AZStd::string m_defaultSettingFolder;
        AZStd::string m_testFileFolder;

        //initialize image file names for testing
        void InitialImageFilenames()
        {
            m_imagFileNameMap[Image_20X16_RGBA8_Png] = m_testFileFolder + "20x16_32bit.png";
            m_imagFileNameMap[Image_32X32_16bit_F_Tif] = m_testFileFolder + "32x32_16bit_f.tif";
            m_imagFileNameMap[Image_32X32_32bit_F_Tif] = m_testFileFolder + "32x32_32bit_f.tif";
            m_imagFileNameMap[Image_200X200_RGB8_Jpg] = m_testFileFolder + "200x200_24bit.jpg";
            m_imagFileNameMap[Image_512X288_RGB8_Tga] = m_testFileFolder + "512x288_24bit.tga";
            m_imagFileNameMap[Image_1024X1024_RGB8_Tif] = m_testFileFolder + "1024x1024_24bit.tif";
            m_imagFileNameMap[Image_UpperCase_Tga] = m_testFileFolder + "uppercase.TGA";
            m_imagFileNameMap[Image_1024x1024_normal_tiff] = m_testFileFolder + "1024x1024_normal.tiff";
            m_imagFileNameMap[Image_128x128_Transparent_Tga] = m_testFileFolder + "128x128_RGBA8.tga";
            m_imagFileNameMap[Image_237x177_RGB_Jpg] = m_testFileFolder + "237x177_RGB.jpg";
            m_imagFileNameMap[Image_GreyScale_Png] = m_testFileFolder + "greyscale.png";
            m_imagFileNameMap[Image_Alpha8_64x64_Mip7_Dds] = m_testFileFolder + "Alpha8_64x64_Mip7.dds";
            m_imagFileNameMap[Image_BGRA_64x64_Mip7_Dds] = m_testFileFolder + "BGRA_64x64_MIP7.dds";
            m_imagFileNameMap[Image_Luminance8bpp_66x33_dds] = m_testFileFolder + "Luminance8bpp_66x33.dds";
            m_imagFileNameMap[Image_BGR_64x64_dds] = m_testFileFolder + "RGBA_64x64.dds";
            m_imagFileNameMap[Image_defaultprobe_cm_1536x256_64bits_tif] = m_testFileFolder + "defaultProbe_cm.tif";
            m_imagFileNameMap[Image_workshop_iblskyboxcm_exr] = m_testFileFolder + "workshop_iblskyboxcm.exr";
        }

    public:
        void SetOutputSubFolder(const char* subFolderName)
        {
            if (subFolderName)
            {
                m_outputFolder = m_outputRootFolder + "/" + subFolderName;
            }
            else
            {
                m_outputFolder  = m_outputRootFolder;
            }
        }

        //helper function to save an image object to a file through QtImage
        void SaveImageToFile([[maybe_unused]] const IImageObjectPtr imageObject, [[maybe_unused]] const AZStd::string imageName, [[maybe_unused]] AZ::u32 maxMipCnt = 100)
        {
    #ifndef DEBUG_OUTPUT_IMAGES
            return;
    #else
            if (imageObject == nullptr)
            {
                return;
            }

            // create dir if it doesn't exist
            QDir dir;
            QDir outputDir(m_outputFolder.c_str());
            if (!outputDir.exists())
            {
                dir.mkpath(m_outputFolder.c_str());
            }

            //save origin file pixel format so we could use it to generate name later
            EPixelFormat originPixelFormat = imageObject->GetPixelFormat();

            //convert to RGBA8 before can be exported.
            ImageToProcess imageToProcess(imageObject);
            imageToProcess.ConvertFormat(ePixelFormat_R8G8B8A8);

            IImageObjectPtr finalImage = imageToProcess.Get();

            //for each mipmap
            for (uint32 mip = 0; mip < finalImage->GetMipCount() && mip < maxMipCnt; mip++)
            {
                uint8* imageBuf;
                uint32 pitch;
                finalImage->GetImagePointer(mip, imageBuf, pitch);
                uint32 width = finalImage->GetWidth(mip);
                uint32 height = finalImage->GetHeight(mip);
                uint32 originalSize = imageObject->GetMipBufSize(mip);

                //generate file name
                char filePath[2048];
                azsprintf(filePath, "%s%s_%s_mip%d_%dx%d_%d.png", m_outputFolder.data(), imageName.c_str()
                    , CPixelFormats::GetInstance().GetPixelFormatInfo(originPixelFormat)->szName
                    , mip, width, height, originalSize);

                QImage qimage(imageBuf, width, height, pitch, QImage::Format_RGBA8888);
                qimage.save(filePath);
            }
    #endif
        }

        static bool GetComparisonResult(IImageObjectPtr image1, IImageObjectPtr image2, QString& output)
        {
            bool isImageLoaded = true;
            bool isDifferent = false;

            if (image1 == nullptr)
            {
                isImageLoaded = false;
                output += ",Image 1 does not exist. ";
            }

            if (image2 == nullptr)
            {
                isImageLoaded = false;
                output += ",Image 2 does not exist. ";
            }

            if (!isImageLoaded)
            {
                return (!image1 && !image2) ? false : true;
            }

            // Mip
            int mip1 = image1->GetMipCount();
            int mip2 = image2->GetMipCount();
            int mipDiff = abs(mip1 - mip2);

            isDifferent |= mipDiff != 0;

            // Format
            EPixelFormat format1 = image1->GetPixelFormat();
            EPixelFormat format2 = image2->GetPixelFormat();

            isDifferent |= (format1 != format2);

            // Flag
            AZ::u32 flag1 = image1->GetImageFlags();
            AZ::u32 flag2 = image2->GetImageFlags();

            isDifferent |= (flag1 != flag2);

            // Size
            int memSize1 = image1->GetTextureMemory();
            int memSize2 = image2->GetTextureMemory();
            int memDiff = abs(memSize1 - memSize2);

            isDifferent |= memDiff != 0;

            // Error
            float error = GetErrorBetweenImages(image1, image2);

            static float EPSILON = 0.000001f;
            isDifferent |= abs(error) >= EPSILON;

            output += QString(",%1/%2,%3,%4/%5,%6/%7,").arg(QString::number(mip1, 'f', 1), QString::number(mip2, 'f', 1), QString::number(mipDiff),
                QString(ImageProcessingAtomEditor::EditorHelper::s_PixelFormatString[format1]),
                QString(ImageProcessingAtomEditor::EditorHelper::s_PixelFormatString[format2]),
                QString::number(flag1, 16), QString::number(flag2, 16));

            output += QString("%1/%2,%3,%4").arg(QString(ImageProcessingAtomEditor::EditorHelper::GetFileSizeString(memSize1).c_str()),
                QString(ImageProcessingAtomEditor::EditorHelper::GetFileSizeString(memSize2).c_str()),
                QString(ImageProcessingAtomEditor::EditorHelper::GetFileSizeString(memDiff).c_str()),
                QString::number(error, 'f', 8));


            return isDifferent;
        }

    };

    // test CPixelFormats related functions
    TEST_F(ImageProcessingTest, TestPixelFormats)
    {
        CPixelFormats& pixelFormats = CPixelFormats::GetInstance();

        //for all the non-compressed textures, if there minimum required texture size is 1x1
        for (uint32 i = 0; i < ePixelFormat_Count; i++)
        {
            EPixelFormat pixelFormat = (EPixelFormat)i;
            if (pixelFormats.IsPixelFormatUncompressed(pixelFormat))
            {
                //square, power of 2 sizes for uncompressed format which minimum required size is 1x1
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 128) == 8);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 64, 64) == 7);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 4, 4) == 3);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 2, 2) == 2);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 1, 1) == 1);

                //non-square, power of 2 sizes for uncompressed format which minimum required size is 1x1
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 64) == 8);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 32) == 8);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 32, 2) == 6);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 2, 1) == 2);

                //Non power of 2 sizes for uncompressed format which minimum required size is 1x1
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 64) == 8);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 32) == 8);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 32, 2) == 6);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 2, 1) == 2);
            }
        }

        //check function IsImageSizeValid && EvaluateImageDataSize function
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 2, 1, false) == false);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 16, 16, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 16, 32, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 34, 34, false) == false);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 256, 256, false) == true);

        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 2, 1, false) == false);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 16, 16, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 16, 32, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 34, 34, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 256, 256, false) == true);

        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 2, 1, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 16, 16, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 16, 32, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 34, 34, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 256, 256, false) == true);
    }

    TEST_F(ImageProcessingTest, TestCubemapLayouts)
    {
        {
            IImageObjectPtr srcImage(LoadImageFromFile(m_imagFileNameMap[Image_defaultprobe_cm_1536x256_64bits_tif]));
            ImageToProcess imageToProcess(srcImage);

            imageToProcess.ConvertCubemapLayout(CubemapLayoutVertical);
            ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) * 6 == imageToProcess.Get()->GetHeight(0));
            SaveImageToFile(imageToProcess.Get(), "Vertical", 100);

            imageToProcess.ConvertCubemapLayout(CubemapLayoutHorizontalCross);
            ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) * 3 == imageToProcess.Get()->GetHeight(0) * 4);
            SaveImageToFile(imageToProcess.Get(), "HorizontalCross", 100);

            imageToProcess.ConvertCubemapLayout(CubemapLayoutVerticalCross);
            ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) * 4 == imageToProcess.Get()->GetHeight(0) * 3);
            SaveImageToFile(imageToProcess.Get(), "VerticalCross", 100);

            imageToProcess.ConvertCubemapLayout(CubemapLayoutHorizontal);
            ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) == imageToProcess.Get()->GetHeight(0) * 6);
            SaveImageToFile(imageToProcess.Get(), "VerticalHorizontal", 100);
        }
    }

    // test image file loading
    TEST_F(ImageProcessingTest, TestImageLoaders)
    {
        //file extension support for different loader
        ASSERT_TRUE(IsExtensionSupported("jpg") == true);
        ASSERT_TRUE(IsExtensionSupported("JPG") == true);
        ASSERT_TRUE(IsExtensionSupported(".JPG") == false);
        ASSERT_TRUE(IsExtensionSupported("tga") == true);
        ASSERT_TRUE(IsExtensionSupported("TGA") == true);
        ASSERT_TRUE(IsExtensionSupported("tif") == true);
        ASSERT_TRUE(IsExtensionSupported("tiff") == true);

        IImageObjectPtr img;
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_1024X1024_RGB8_Tif]));

        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetWidth(0) == 1024);
        ASSERT_TRUE(img->GetHeight(0) == 1024);
        ASSERT_TRUE(img->GetMipCount() == 1);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8X8);

        //load png
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_20X16_RGBA8_Png]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetWidth(0) == 20);
        ASSERT_TRUE(img->GetHeight(0) == 16);
        ASSERT_TRUE(img->GetMipCount() == 1);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

        //load jpg
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_200X200_RGB8_Jpg]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetWidth(0) == 200);
        ASSERT_TRUE(img->GetHeight(0) == 200);
        ASSERT_TRUE(img->GetMipCount() == 1);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

        //tga
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_512X288_RGB8_Tga]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetWidth(0) == 512);
        ASSERT_TRUE(img->GetHeight(0) == 288);
        ASSERT_TRUE(img->GetMipCount() == 1);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

        //image with upper case extension
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_UpperCase_Tga]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

        //16bits float tif
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_32X32_16bit_F_Tif]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R16G16B16A16F);

        //32bits float tif
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_32X32_32bit_F_Tif]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R32G32B32A32F);

        // DDS files
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_Alpha8_64x64_Mip7_Dds]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_A8);
        ASSERT_TRUE(img->GetMipCount() == 7);
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_BGRA_64x64_Mip7_Dds]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_B8G8R8A8);
        ASSERT_TRUE(img->GetMipCount() == 7);
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_Luminance8bpp_66x33_dds]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_A8);
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_BGR_64x64_dds]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_B8G8R8);

        // Exr file
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_workshop_iblskyboxcm_exr]));
        ASSERT_TRUE(img != nullptr);
    }

    TEST_F(ImageProcessingTest, PresetSettingCopyAssignmentOperatorOverload_WithDynamicallyAllocatedSettings_ReturnsTwoSeparateAllocations)
    {
        PresetSettings presetSetting;
        presetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
        presetSetting.m_cubemapSetting = AZStd::unique_ptr<CubemapSettings>(new CubemapSettings());

        // Explicit invoke assignment operator by splitting the operation into two lines.
        PresetSettings otherPresetSetting;
        otherPresetSetting = presetSetting;

        EXPECT_NE(otherPresetSetting.m_cubemapSetting, presetSetting.m_cubemapSetting);
        EXPECT_NE(otherPresetSetting.m_mipmapSetting, presetSetting.m_mipmapSetting);
    }

    TEST_F(ImageProcessingTest, PresetSettingCopyConstructor_WithDynamicallyAllocatedSettings_ReturnsTwoSeparateAllocations)
    {
        PresetSettings presetSetting;
        presetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
        presetSetting.m_cubemapSetting = AZStd::unique_ptr<CubemapSettings>(new CubemapSettings());

        PresetSettings otherPresetSetting(presetSetting);

        EXPECT_NE(otherPresetSetting.m_cubemapSetting, presetSetting.m_cubemapSetting);
        EXPECT_NE(otherPresetSetting.m_mipmapSetting, presetSetting.m_mipmapSetting);
    }

    TEST_F(ImageProcessingTest, PresetSettingEqualityOperatorOverload_WithIdenticalSettings_ReturnsEquivalent)
    {
        PresetSettings presetSetting;
        PresetSettings otherPresetSetting(presetSetting);

        EXPECT_TRUE(otherPresetSetting == presetSetting);
    }

    TEST_F(ImageProcessingTest, PresetSettingEqualityOperatorOverload_WithDifferingDynamicallyAllocatedSettings_ReturnsUnequivalent)
    {
        PresetSettings presetSetting;
        presetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
        presetSetting.m_mipmapSetting->m_type = MipGenType::gaussian;

        PresetSettings otherPresetSetting(presetSetting);
        otherPresetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
        otherPresetSetting.m_mipmapSetting->m_type = MipGenType::blackmanHarris;

        EXPECT_FALSE(otherPresetSetting == presetSetting);
    }

    //this test is to test image data won't be lost between uncompressed formats (for low to high precision or same precision)
    TEST_F(ImageProcessingTest, TestConvertFormatUncompressed)
    {
        //source image
        IImageObjectPtr srcImage(LoadImageFromFile(m_imagFileNameMap[Image_200X200_RGB8_Jpg]));
        ImageToProcess imageToProcess(srcImage);

        //image pointers to hold precessed images for comparison
        IImageObjectPtr dstImage1, dstImage2, dstImage3, dstImage4, dstImage5;

        //compare four channels pixel formats
        //we will convert to target format then convert back to RGBX8 so they can compare to easy other
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
        dstImage1 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16B16A16);
        ASSERT_FALSE(srcImage->CompareImage(imageToProcess.Get())); //this is different than source image
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
        dstImage2 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16B16A16F);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
        dstImage3 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R32G32B32A32F);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
        dstImage4 = imageToProcess.Get();

        ASSERT_TRUE(dstImage2->CompareImage(dstImage1));
        ASSERT_TRUE(dstImage3->CompareImage(dstImage1));
        ASSERT_TRUE(dstImage4->CompareImage(dstImage1));

        // three channels formats
        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage1 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R9G9B9E5);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage2 = imageToProcess.Get();

        ASSERT_TRUE(dstImage2->CompareImage(dstImage1));

        //convert image to all one channel formats then convert them back to RGBX8 for comparison
        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage1 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage2 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16F);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage3 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R32F);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage4 = imageToProcess.Get();

        ASSERT_TRUE(dstImage2->CompareImage(dstImage1));
        ASSERT_TRUE(dstImage3->CompareImage(dstImage1));
        ASSERT_TRUE(dstImage4->CompareImage(dstImage1));

        //convert image to all two channels formats then convert them back to RGBX8 for comparison
        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage1 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage2 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16F);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage3 = imageToProcess.Get();

        ASSERT_TRUE(dstImage2->CompareImage(dstImage1));
        ASSERT_TRUE(dstImage3->CompareImage(dstImage1));
    }

    TEST_F(ImageProcessingTest, TestConvertFormatCompressed)
    {
        IImageObjectPtr srcImage;

        //images to be tested
        static const int imageCount = 4;
        ImageFeature images[imageCount] = {
            Image_20X16_RGBA8_Png,
            Image_237x177_RGB_Jpg,
            Image_128x128_Transparent_Tga,
            Image_defaultprobe_cm_1536x256_64bits_tif};

        // collect all compressed pixel formats
        AZStd::vector<EPixelFormat> compressedFormats;
        for (uint32 i = 0; i < ePixelFormat_Count; i++)
        {
            EPixelFormat pixelFormat = (EPixelFormat)i;
            auto formatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(pixelFormat);
            if (formatInfo->bCompressed)
            {
                // skip ASTC formats which are tested in TestConvertASTCCompressor
                if (!IsASTCFormat(pixelFormat))
                {
                    compressedFormats.push_back(pixelFormat);
                }
            }
        }

        for (int imageIdx = 0; imageIdx < imageCount; imageIdx++)
        {
            //get image's name and it will be used for output file name
            QFileInfo fi(m_imagFileNameMap[images[imageIdx]].c_str());
            AZStd::string imageName = fi.baseName().toUtf8().constData();

            srcImage = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[images[imageIdx]]));
            ImageToProcess imageToProcess(srcImage);

            //test ConvertFormat functions against all the pixel formats
            for (EPixelFormat pixelFormat : compressedFormats)
            {
                // 
                if (!CPixelFormats::GetInstance().IsImageSizeValid(pixelFormat, srcImage->GetWidth(0), srcImage->GetHeight(0), false))
                {
                    continue;
                }
#if defined(AZ_ENABLE_TRACING)
                auto formatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(pixelFormat);
#endif
                ColorSpace sourceColorSpace = srcImage->HasImageFlags(EIF_SRGBRead) ? ColorSpace::sRGB : ColorSpace::linear;
                ICompressorPtr compressor = ICompressor::FindCompressor(pixelFormat, sourceColorSpace, true);

                if (!compressor)
                {
                    AZ_Warning("test", false, "unsupported format: %s",  formatInfo->szName);
                    continue;
                }

                imageToProcess.Set(srcImage);
                imageToProcess.ConvertFormat(pixelFormat);

                ASSERT_TRUE(imageToProcess.Get());
                ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == pixelFormat);

                //convert back to an uncompressed format and expect it will be successful
                imageToProcess.ConvertFormat(srcImage->GetPixelFormat());
                ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == srcImage->GetPixelFormat());

                // Save the image to a file so we can check the visual result
                AZStd::string outputName = AZStd::string::format("%s_%s", imageName.c_str(), compressor->GetName());
                SaveImageToFile(imageToProcess.Get(), outputName, 1);
            }
        }
    }
        
    TEST_F(ImageProcessingTest, Test_ConvertAllAstc_Success)
    {
        // Compress/Decompress to all astc formats (LDR)
        auto imageIdx = Image_237x177_RGB_Jpg;
        IImageObjectPtr srcImage = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[imageIdx]));
        QFileInfo fi(m_imagFileNameMap[imageIdx].c_str());
        AZStd::string imageName = fi.baseName().toUtf8().constData();
        for (uint32 i = 0; i < ePixelFormat_Count; i++)
        {
            EPixelFormat pixelFormat = (EPixelFormat)i;
            if (IsASTCFormat(pixelFormat))
            {
                ImageToProcess imageToProcess(srcImage);
                imageToProcess.ConvertFormat(pixelFormat);

                ASSERT_TRUE(imageToProcess.Get());
                ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == pixelFormat);
                ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) == srcImage->GetWidth(0));
                ASSERT_TRUE(imageToProcess.Get()->GetHeight(0) == srcImage->GetHeight(0));

                // convert back to an uncompressed format and expect it will be successful
                imageToProcess.ConvertFormat(srcImage->GetPixelFormat());
                ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == srcImage->GetPixelFormat());

                // save the image to a file so we can check the visual result
                AZStd::string outputName = AZStd::string::format("ASTC_%s", imageName.c_str());
                SaveImageToFile(imageToProcess.Get(), outputName, 1);
            }
        }
    }
        
    TEST_F(ImageProcessingTest, Test_ConvertHdrToAstc_Success)
    {
        // Compress/Decompress HDR
        auto imageIdx = Image_defaultprobe_cm_1536x256_64bits_tif;
        IImageObjectPtr srcImage = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[imageIdx]));

        EPixelFormat dstFormat = ePixelFormat_ASTC_4x4;
        ImageToProcess imageToProcess(srcImage);
        imageToProcess.ConvertFormat(ePixelFormat_ASTC_4x4);
                
        ASSERT_TRUE(imageToProcess.Get());
        ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == dstFormat);
        ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) == srcImage->GetWidth(0));
        ASSERT_TRUE(imageToProcess.Get()->GetHeight(0) == srcImage->GetHeight(0));
                                
        //convert back to an uncompressed format and expect it will be successful
        imageToProcess.ConvertFormat(srcImage->GetPixelFormat());
        ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == srcImage->GetPixelFormat());
                                
        //save the image to a file so we can check the visual result
        SaveImageToFile(imageToProcess.Get(), "ASTC_HDR", 1);
    }
    
    TEST_F(ImageProcessingTest, Test_AstcNormalPreset_Success)
    {
        // Normal.preset which uses ASTC as output format
        // This test compress a normal texture and its mipmaps

        auto outcome = BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder);
        ASSERT_TRUE(outcome.IsSuccess());

        AZStd::string inputFile;
        AZStd::vector<AssetBuilderSDK::JobProduct> outProducts;

        inputFile = m_imagFileNameMap[Image_1024x1024_normal_tiff];
        IImageObjectPtr srcImage = IImageObjectPtr(LoadImageFromFile(inputFile));

        ImageConvertProcess* process = CreateImageConvertProcess(inputFile, m_outputFolder, "ios", outProducts, m_context.get());

        const PresetSettings* preset = &process->GetInputDesc()->m_presetSetting;

        if (process != nullptr)
        {
            process->ProcessAll();

            //get process result
            ASSERT_TRUE(process->IsSucceed());
            auto outputImage = process->GetOutputImage();
            ASSERT_TRUE(outputImage->GetPixelFormat() == preset->m_pixelFormat);
            ASSERT_TRUE(outputImage->GetWidth(0) == srcImage->GetWidth(0));
            ASSERT_TRUE(outputImage->GetHeight(0) == srcImage->GetHeight(0));
            
            SaveImageToFile(outputImage, "ASTC_Normal", 10);

            delete process;
        }
    }

    TEST_F(ImageProcessingTest, DISABLED_TestImageFilter)
    {
        AZStd::string testImageFile = m_imagFileNameMap[Image_1024X1024_RGB8_Tif];
        IImageObjectPtr srcImage, dstImage;

        QFileInfo fi(testImageFile.c_str());
        AZStd::string imageName = fi.baseName().toUtf8().constData();

        //load source image and convert it to RGBA32F
        srcImage = IImageObjectPtr(LoadImageFromFile(testImageFile));
        ImageToProcess imageToProcess(srcImage);
        imageToProcess.ConvertFormat(ePixelFormat_R32G32B32A32F);
        srcImage = imageToProcess.Get();

        //create destination image with same size and mipmaps
        dstImage = IImageObjectPtr(
            IImageObject::CreateImage(srcImage->GetWidth(0), srcImage->GetHeight(0), 3,
                ePixelFormat_R32G32B32A32F));

        //for each filters
        const std::array<std::pair<MipGenType, AZStd::string>, 7> allFilters =
        {
            {
                {MipGenType::point, "point"},
                {MipGenType::box, "box" },
                { MipGenType::triangle, "triangle" },
                { MipGenType::quadratic, "Quadratic" },
                { MipGenType::blackmanHarris, "blackmanHarris" },
                { MipGenType::kaiserSinc, "kaiserSinc" }
            }
        };

        for (std::pair<MipGenType, AZStd::string> filter : allFilters)
        {
            for (uint mip = 0; mip < dstImage->GetMipCount(); mip++)
            {
                FilterImage(filter.first, MipGenEvalType::sum,
                    0, 0, imageToProcess.Get(), 0, dstImage, mip, nullptr, nullptr);
            }
            SaveImageToFile(dstImage, imageName + "_" + filter.second);
        }
    }

    TEST_F(ImageProcessingTest, TestColorSpaceConversion)
    {
        IImageObjectPtr srcImage(LoadImageFromFile(m_imagFileNameMap[Image_GreyScale_Png]));

        ImageToProcess imageToProcess(srcImage);
        imageToProcess.GammaToLinearRGBA32F(true);
        SaveImageToFile(imageToProcess.Get(), "GammaTolinear_DeGamma", 1);
        imageToProcess.LinearToGamma();
        SaveImageToFile(imageToProcess.Get(), "LinearToGamma_DeGamma", 1);
    }

    TEST_F(ImageProcessingTest, VerifyRestrictedPlatform)
    {
        auto outcome = BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder);
        ASSERT_TRUE(outcome.IsSuccess());
        PlatformNameList platforms = BuilderSettingManager::Instance()->GetPlatformList();

    #ifndef AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
        EXPECT_THAT(platforms, testing::UnorderedPointwise(testing::Eq(), {"pc", "linux", "mac", "ios", "android"}));
    #endif //AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
    }

    //test image conversion for builder
    TEST_F(ImageProcessingTest, TestBuilderImageConvertor)
    {
        //load builder presets
        auto outcome = BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder);
        ASSERT_TRUE(outcome.IsSuccess());

        AZStd::string inputFile;
        AZStd::vector<AssetBuilderSDK::JobProduct> outProducts;

        inputFile = m_imagFileNameMap[Image_128x128_Transparent_Tga];
        ImageConvertProcess* process = CreateImageConvertProcess(inputFile, m_outputFolder, "pc", outProducts, m_context.get());

        if (process != nullptr)
        {
            //the process can be stopped if the job is canceled or the worker is shutting down
            int step = 0;
            while (!process->IsFinished())
            {
                process->UpdateProcess();
                step++;
            }

            //get process result
            ASSERT_TRUE(process->IsSucceed());

            SaveImageToFile(process->GetOutputImage(), "rgb", 10);
            SaveImageToFile(process->GetOutputAlphaImage(), "alpha", 10);

            process->GetAppendOutputProducts(outProducts);

            delete process;
        }
    }

    TEST_F(ImageProcessingTest, TestIblSkyboxPreset)
    {
        //load builder presets
        auto outcome = BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder);
        ASSERT_TRUE(outcome.IsSuccess());

        AZStd::string inputFile;
        AZStd::vector<AssetBuilderSDK::JobProduct> outProducts;

        inputFile = m_imagFileNameMap[Image_workshop_iblskyboxcm_exr];
        ImageConvertProcess* process = CreateImageConvertProcess(inputFile, m_outputFolder, "pc", outProducts, m_context.get());

        if (process != nullptr)
        {
            process->ProcessAll();

            //get process result
            ASSERT_TRUE(process->IsSucceed());

            auto specularImage = process->GetOutputIBLSpecularCubemap();
            auto diffuseImage = process->GetOutputIBLDiffuseCubemap();
            ASSERT_TRUE(process->GetOutputImage());
            ASSERT_TRUE(specularImage);
            ASSERT_TRUE(diffuseImage);

            // output converted result if save image is enabled
            SaveImageToFile(process->GetOutputImage(), "ibl_skybox", 10);
            SaveImageToFile(specularImage, "ibl_specular", 10);
            SaveImageToFile(diffuseImage, "ibl_diffuse", 10);

            delete process;
        }
    }

    TEST_F(ImageProcessingTest, TextureSettingReflect_SerializingModernDataInAndOut_WritesAndParsesFileAccurately)
    {
        AZStd::string filepath = "test.xml";

        // Fill-in structure with test data
        TextureSettings fakeTextureSettings;
        fakeTextureSettings.m_preset = "testPreset";
        fakeTextureSettings.m_sizeReduceLevel = 0;
        fakeTextureSettings.m_suppressEngineReduce = true;
        fakeTextureSettings.m_enableMipmap = false;
        fakeTextureSettings.m_maintainAlphaCoverage = true;
        fakeTextureSettings.m_mipAlphaAdjust = { 0xDEAD, 0xBADBEEF, 0xBADC0DE, 0xFEEFEE, 0xBADF00D, 0xC0FFEE };
        fakeTextureSettings.m_mipGenEval = MipGenEvalType::max;
        fakeTextureSettings.m_mipGenType = MipGenType::quadratic;

        // Write test data to file
        auto writeOutcome = TextureSettings::WriteTextureSetting(filepath, fakeTextureSettings, m_context.get());
        EXPECT_TRUE(writeOutcome.IsSuccess());

        // Parse test data to file
        TextureSettings parsedFakeTextureSettings;
        auto readOutcome = TextureSettings::LoadTextureSetting(filepath, parsedFakeTextureSettings, m_context.get());
        EXPECT_TRUE(readOutcome.IsSuccess());
        EXPECT_TRUE(parsedFakeTextureSettings.Equals(fakeTextureSettings, m_context.get()));

        // Delete temp data
        AZ::IO::FileIOBase::GetInstance()->Remove(filepath.c_str());
    }

} // UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);



/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystem.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAssetCreator.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>

#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Settings/SettingsRegistry.h>

ATOM_RPI_PUBLIC_API AZ_DECLARE_BUDGET(RPI);

namespace AZ
{

    namespace
    {
        const char* MemoryBudgetSettingPath = "/O3DE/Atom/RPI/Initialization/ImageSystemDescriptor/SystemStreamingImagePoolSize";
        const char* MipBiasSettingPath = "/O3DE/Atom/RPI/Initialization/ImageSystemDescriptor/SystemStreamingImagePoolMipBias";
        
        size_t cvar_r_streamingImagePoolBudgetMb_Init()
        {
            u64 value = 0;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                settingsRegistry->Get(value, MemoryBudgetSettingPath);
            }
            return aznumeric_cast<size_t>(value);
        }

        int16_t cvar_r_streamingImageMipBias_Init()
        {
            s64 value = 0;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                settingsRegistry->Get(value, MipBiasSettingPath);
            }
            return aznumeric_cast<int16_t>(value);
        }

        void cvar_r_streamingImagePoolBudgetMb_Changed(const size_t& value)
        {
            if (auto* imageSystem = RPI::ImageSystemInterface::Get())
            {
                Data::Instance<RPI::StreamingImagePool> pool = imageSystem->GetSystemStreamingPool();
                size_t newBudget = value * 1024 * 1024;
                [[maybe_unused]] bool success = pool->SetMemoryBudget(newBudget);
                AZ_Warning("StreamingImagePool", success, "Can't update StreamingImagePool's memory budget to %uM", value);
            }
            else
            {
                // Update setting registry value which is used for image system initialization
                if (auto settingsRegistry = AZ::SettingsRegistry::Get())
                {
                    settingsRegistry->Set(MemoryBudgetSettingPath, aznumeric_cast<u64>(value));
                }
            }
        }

        void cvar_r_streamingImageMipBias_Changed(const int16_t& value)
        {
            if (auto* imageSystem = RPI::ImageSystemInterface::Get())
            {
                Data::Instance<RPI::StreamingImagePool> pool = imageSystem->GetSystemStreamingPool();
                pool->SetMipBias(value);
            }
            else
            {
                // Update setting registry value which is used for image system initialization
                if (auto settingsRegistry = AZ::SettingsRegistry::Get())
                {
                    settingsRegistry->Set(MipBiasSettingPath, aznumeric_cast<s64>(value));
                }
            }
        }
    }

    // cvars for changing streaming image pool budget and setup mip bias of streaming controller
    AZ_CVAR(size_t, r_streamingImagePoolBudgetMb, cvar_r_streamingImagePoolBudgetMb_Init(), cvar_r_streamingImagePoolBudgetMb_Changed, ConsoleFunctorFlags::DontReplicate, "Change gpu memory budget for the RPI system streaming image pool");
    AZ_CVAR(int16_t, r_streamingImageMipBias, cvar_r_streamingImageMipBias_Init(), cvar_r_streamingImageMipBias_Changed, ConsoleFunctorFlags::DontReplicate, "Set a mipmap bias for all streamable images created from the system streaming image pool");

    namespace RPI
    {
        ImageSystemInterface* ImageSystemInterface::Get()
        {
            return Interface<ImageSystemInterface>::Get();
        }

        void ImageSystem::Reflect(AZ::ReflectContext* context)
        {
            ImageAsset::Reflect(context);
            ImageMipChainAsset::Reflect(context);
            ImageSystemDescriptor::Reflect(context);
            StreamingImageAsset::Reflect(context);
            StreamingImagePoolAsset::Reflect(context);
            StreamingImageControllerAsset::Reflect(context);
            AttachmentImageAsset::Reflect(context);
        }

        void ImageSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<ImageAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<AttachmentImageAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<ImageMipChainAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<StreamingImageAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<StreamingImagePoolAssetHandler>());
        }

        void ImageSystem::Init(const ImageSystemDescriptor& desc)
        {
            // Register attachment image instance database.
            {
                Data::InstanceHandler<AttachmentImage> handler;
                handler.m_createFunction = [](Data::AssetData* imageAsset)
                {
                    return AttachmentImage::CreateInternal(*(azrtti_cast<AttachmentImageAsset*>(imageAsset)));
                };
                Data::InstanceDatabase<AttachmentImage>::Create(azrtti_typeid<AttachmentImageAsset>(), handler);
            }

            {
                Data::InstanceHandler<AttachmentImagePool> handler;
                handler.m_createFunction = [](Data::AssetData* poolAsset)
                {
                    return AttachmentImagePool::CreateInternal(*(azrtti_cast<ResourcePoolAsset*>(poolAsset)));
                };
                Data::InstanceDatabase<AttachmentImagePool>::Create(azrtti_typeid<ResourcePoolAsset>(), handler);
            }

            // Register streaming image instance database.

            {
                Data::InstanceHandler<StreamingImage> handler;
                handler.m_createFunction = [](Data::AssetData* imageAsset)
                {
                    return StreamingImage::CreateInternal(*(azrtti_cast<StreamingImageAsset*>(imageAsset)));
                };
                Data::InstanceDatabase<StreamingImage>::Create(azrtti_typeid<StreamingImageAsset>(), handler);
            }

            {
                Data::InstanceHandler<StreamingImagePool> handler;
                handler.m_createFunction = [this](Data::AssetData* poolAsset)
                {
                    Data::Instance<StreamingImagePool> instance =
                        StreamingImagePool::CreateInternal(*(azrtti_cast<StreamingImagePoolAsset*>(poolAsset)));
                    if (instance)
                    {
                        m_activeStreamingPoolMutex.lock();
                        m_activeStreamingPools.emplace_back(instance.get());
                        m_activeStreamingPoolMutex.unlock();
                    }
                    return instance;
                };
                handler.m_deleteFunction = [this](StreamingImagePool* pool)
                {
                    m_activeStreamingPoolMutex.lock();
                    auto findIt = AZStd::find(m_activeStreamingPools.begin(), m_activeStreamingPools.end(), pool);
                    AZ_Assert(findIt != m_activeStreamingPools.end(), "Pool must exist in the container.");
                    m_activeStreamingPools.erase(findIt);
                    m_activeStreamingPoolMutex.unlock();

                    delete pool;
                };
                Data::InstanceDatabase<StreamingImagePool>::Create(azrtti_typeid<StreamingImagePoolAsset>(), handler);
            }

            CreateDefaultResources(desc);

            Interface<ImageSystemInterface>::Register(this);

            m_initialized = true;
        }

        void ImageSystem::Shutdown()
        {
            if (!m_initialized)
            {
                return;
            }
            Interface<ImageSystemInterface>::Unregister(this);

            m_systemImages.clear();
            m_systemAttachmentImages.clear();
            m_systemStreamingPool = nullptr;
            m_systemAttachmentPool = nullptr;

            Data::InstanceDatabase<AttachmentImage>::Destroy();
            Data::InstanceDatabase<AttachmentImagePool>::Destroy();
            Data::InstanceDatabase<StreamingImage>::Destroy();
            Data::InstanceDatabase<StreamingImagePool>::Destroy();

            m_activeStreamingPools.clear();
            m_initialized = false;
        }

        void ImageSystem::Update()
        {
            AZ_PROFILE_SCOPE(RPI, "ImageSystem: Update");

            AZStd::lock_guard<AZStd::mutex> lock(m_activeStreamingPoolMutex);
            for (StreamingImagePool* imagePool : m_activeStreamingPools)
            {
                imagePool->Update();
            }
        }

        const Data::Instance<StreamingImagePool>& ImageSystem::GetSystemStreamingPool() const
        {
            return m_systemStreamingPool;
        }
        
        const Data::Instance<StreamingImagePool>& ImageSystem::GetStreamingPool() const
        {
            return GetSystemStreamingPool();
        }

        const Data::Instance<AttachmentImagePool>& ImageSystem::GetSystemAttachmentPool() const
        {
            return m_systemAttachmentPool;
        }

        const Data::Instance<Image>& ImageSystem::GetSystemImage(SystemImage simpleImage) const
        {
            return m_systemImages[static_cast<size_t>(simpleImage)];
        }

        const Data::Instance<AttachmentImage>& ImageSystem::GetSystemAttachmentImage(RHI::Format format)
        {
            {
                AZStd::shared_lock<AZStd::shared_mutex> lock(m_systemAttachmentImagesUpdateMutex);

                auto it = m_systemAttachmentImages.find(format);
                if (it != m_systemAttachmentImages.end())
                {
                    return it->second;
                }
            }

            // Take a full lock while the map is updated.
            AZStd::lock_guard<AZStd::shared_mutex> lock(m_systemAttachmentImagesUpdateMutex);

            // Double check map in case another thread created an attachment image for this format while this
            // thread waited on the lock.
            auto it = m_systemAttachmentImages.find(format);
            if (it != m_systemAttachmentImages.end())
            {
                return it->second;
            }
            
            RHI::ImageBindFlags formatBindFlag = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite;

            switch (format)
            {
            case RHI::Format::D16_UNORM:
            case RHI::Format::D32_FLOAT:
                formatBindFlag = RHI::ImageBindFlags::Depth | RHI::ImageBindFlags::ShaderRead;
                break;
            case RHI::Format::D16_UNORM_S8_UINT:
            case RHI::Format::D24_UNORM_S8_UINT:
            case RHI::Format::D32_FLOAT_S8X24_UINT:
                formatBindFlag = RHI::ImageBindFlags::DepthStencil | RHI::ImageBindFlags::ShaderRead;
                break;
            }

            RHI::ImageDescriptor imageDescriptor;
            imageDescriptor.m_size = RHI::Size(1, 1, 1);
            imageDescriptor.m_format = format;
            imageDescriptor.m_arraySize = 1;
            imageDescriptor.m_bindFlags = formatBindFlag;
            imageDescriptor.m_sharedQueueMask = RHI::HardwareQueueClassMask::All;

            RPI::CreateAttachmentImageRequest createImageRequest;
            createImageRequest.m_imagePool = m_systemAttachmentPool.get();
            createImageRequest.m_imageDescriptor = imageDescriptor;
            createImageRequest.m_imageName = "SystemAttachmentImage";
            createImageRequest.m_isUniqueName = false;

            auto systemAttachmentImage = RPI::AttachmentImage::Create(createImageRequest);
            m_systemAttachmentImages[format] = systemAttachmentImage;
            return m_systemAttachmentImages[format];
        }

        bool ImageSystem::RegisterAttachmentImage(AttachmentImage* attachmentImage)
        {
            if (!attachmentImage)
            {
                return false;
            }

            auto itr = m_registeredAttachmentImages.find(attachmentImage->GetAttachmentId());
            if (itr != m_registeredAttachmentImages.end())
            {
                AZ_Assert(false, "AttachmangeImage with name '%s' was already registered", attachmentImage->GetAttachmentId().GetCStr());
                return false;
            }

            m_registeredAttachmentImages[attachmentImage->GetAttachmentId()] = attachmentImage;

            return true;
        }

        void ImageSystem::UnregisterAttachmentImage(AttachmentImage* attachmentImage)
        {
            if (!attachmentImage)
            {
                return;
            }
            auto itr = m_registeredAttachmentImages.find(attachmentImage->GetAttachmentId());
            if (itr != m_registeredAttachmentImages.end())
            {
                m_registeredAttachmentImages.erase(itr);
            }
        }

        Data::Instance<AttachmentImage> ImageSystem::FindRegisteredAttachmentImage(const Name& uniqueName) const
        {
            auto itr = m_registeredAttachmentImages.find(uniqueName);

            if (itr != m_registeredAttachmentImages.end())
            {
                return itr->second;
            }
            return nullptr;
        }

        void ImageSystem::CreateDefaultResources(const ImageSystemDescriptor& desc)
        {
            struct SystemImageDescriptor
            {
                SystemImageDescriptor(const Color color, const char* name)
                    : m_color{color}
                    , m_name{name}
                    , m_assetId{Uuid::CreateName(name)}
                {}

                Color m_color;
                const char* m_name;
                Data::AssetId m_assetId;
            };

            const SystemImageDescriptor systemImageDescriptors[static_cast<uint32_t>(SystemImage::Count)] =
            {
                { Color(1.0f, 1.0f, 1.0f, 1.0f),  "Image_White" },
                { Color(0.0f, 0.0f, 0.0f, 1.0f),  "Image_Black" },
                { Color(0.5f, 0.5f, 0.5f, 1.0f),  "Image_Grey" },
                { Color(1.0f, 0.0f, 1.0f, 1.0f),  "Image_Magenta" }
            };

            static_assert(AZ_ARRAY_SIZE(systemImageDescriptors) == static_cast<size_t>(SystemImage::Count), "System image arrays do not match.");

            struct SystemImagePoolDescriptor
            {
                SystemImagePoolDescriptor(size_t budgetInBytes, const char* name)
                    : m_budgetInBytes{budgetInBytes}
                    , m_name{name}
                    , m_assetId{Uuid::CreateName(name)}
                {}

                size_t m_budgetInBytes;
                const char* m_name;
                Data::AssetId m_assetId;
            };

            // Sync values from ImageSystemDescriptor back to the cvars
            // Note 1: we need the sync here because one instance of the cvars might be initialized early than setting registry,
            // so it can't be initialized properly. See cvar_r_streamingImagePoolBudgetMb_Init and cvar_r_streamingImageMipBias_Init
            // Note 2: we need to use PerformCommand instead of assign value directly because of this issue https://github.com/o3de/o3de/issues/5537            
            AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();
            if (console)
            {
                AZ::CVarFixedString commandString = AZ::CVarFixedString::format("r_streamingImagePoolBudgetMb %" PRIu64, desc.m_systemStreamingImagePoolSize);
                console->PerformCommand(commandString.c_str());
                commandString = AZ::CVarFixedString::format("r_streamingImageMipBias %" PRId16, desc.m_systemStreamingImagePoolMipBias);
                console->PerformCommand(commandString.c_str());
            }

            const SystemImagePoolDescriptor systemStreamingPoolDescriptor{ desc.m_systemStreamingImagePoolSize, "ImageSystem::SystemStreamingImagePool" };
            const SystemImagePoolDescriptor systemAttachmentPoolDescriptor{desc.m_systemAttachmentImagePoolSize, "ImageSystem::AttachmentImagePool" };

            // Create the system streaming pool
            {
                AZStd::unique_ptr<RHI::StreamingImagePoolDescriptor> imagePoolDescriptor = AZStd::make_unique<RHI::StreamingImagePoolDescriptor>();
                imagePoolDescriptor->m_budgetInBytes = systemStreamingPoolDescriptor.m_budgetInBytes;

                Data::Asset<StreamingImagePoolAsset> poolAsset;

                StreamingImagePoolAssetCreator poolAssetCreator;
                poolAssetCreator.Begin(systemStreamingPoolDescriptor.m_assetId);
                poolAssetCreator.SetPoolDescriptor(AZStd::move(imagePoolDescriptor));
                poolAssetCreator.SetPoolName(systemStreamingPoolDescriptor.m_name);
                [[maybe_unused]] const bool created = poolAssetCreator.End(poolAsset);
                AZ_Assert(created, "Failed to build streaming image pool");

                m_systemStreamingPool = StreamingImagePool::FindOrCreate(poolAsset);
                m_systemStreamingPool->SetMipBias(desc.m_systemStreamingImagePoolMipBias);
            }

            // Create the system attachment pool.
            {
                AZStd::unique_ptr<RHI::ImagePoolDescriptor> imagePoolDescriptor = AZStd::make_unique<RHI::ImagePoolDescriptor>();
                imagePoolDescriptor->m_budgetInBytes = systemAttachmentPoolDescriptor.m_budgetInBytes;
                imagePoolDescriptor->m_bindFlags =
                    RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::ShaderWrite |
                    RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil |
                    RHI::ImageBindFlags::CopyRead | RHI::ImageBindFlags::CopyWrite;

                RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
                if (RHI::CheckBitsAll(device->GetFeatures().m_shadingRateTypeMask, RHI::ShadingRateTypeFlags::PerRegion))
                {
                    imagePoolDescriptor->m_bindFlags |= RHI::ImageBindFlags::ShadingRate;
                }

                Data::Asset<ResourcePoolAsset> poolAsset;

                ResourcePoolAssetCreator poolAssetCreator;
                poolAssetCreator.Begin(systemAttachmentPoolDescriptor.m_assetId);
                poolAssetCreator.SetPoolDescriptor(AZStd::move(imagePoolDescriptor));
                poolAssetCreator.SetPoolName(systemAttachmentPoolDescriptor.m_name);
                [[maybe_unused]] const bool created = poolAssetCreator.End(poolAsset);
                AZ_Assert(created, "Failed to build attachment image pool");

                m_systemAttachmentPool = AttachmentImagePool::FindOrCreate(poolAsset);
            }

            // Create the set of system images.
            {
                const size_t systemImageCount = static_cast<size_t>(SystemImage::Count);
                m_systemImages.resize(systemImageCount);

                for (size_t imageIndex = 0; imageIndex < systemImageCount; imageIndex++)
                {
                    const uint32_t colorU32 = systemImageDescriptors[imageIndex].m_color.ToU32();

                    m_systemImages[imageIndex] = StreamingImage::CreateFromCpuData(
                        *m_systemStreamingPool,
                        RHI::ImageDimension::Image2D,
                        RHI::Size{ 1, 1, 1 },
                        RHI::Format::R8G8B8A8_UNORM_SRGB,
                        &colorU32,
                        sizeof(uint32_t));
                }
            }
        }
    } // namespace RPI
}// namespace AZ

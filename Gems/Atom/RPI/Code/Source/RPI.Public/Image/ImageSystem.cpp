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
#include <Atom/RPI.Public/Image/DefaultStreamingImageController.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAssetCreator.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>

#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RHI/ImagePool.h>

#include <Atom/RHI/RHISystemInterface.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Math/Color.h>

AZ_DECLARE_BUDGET(RPI);

namespace AZ
{
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
            DefaultStreamingImageControllerAsset::Reflect(context);
            AttachmentImageAsset::Reflect(context);
        }

        void ImageSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<AttachmentImageAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<ImageMipChainAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<StreamingImageAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<StreamingImagePoolAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<BuiltInAssetHandler>(
                azrtti_typeid<DefaultStreamingImageControllerAsset>(),
                []() { return aznew DefaultStreamingImageControllerAsset(); }));
        }

        void ImageSystem::Init(const ImageSystemDescriptor& desc)
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

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
                handler.m_createFunction = [device](Data::AssetData* poolAsset)
                {
                    return AttachmentImagePool::CreateInternal(*device, *(azrtti_cast<ResourcePoolAsset*>(poolAsset)));
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
                handler.m_createFunction = [this, device](Data::AssetData* poolAsset)
                {
                    Data::Instance<StreamingImagePool> instance = StreamingImagePool::CreateInternal(*device, *(azrtti_cast<StreamingImagePoolAsset*>(poolAsset)));
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

            // Register streaming image controller instance database.
            {
                Data::InstanceHandler<StreamingImageController> handler;
                handler.m_createFunction = DefaultStreamingImageController::CreateInternal;
                Data::InstanceDatabase<StreamingImageController>::Create(azrtti_typeid<StreamingImageControllerAsset>(), handler);
            }

            m_defaultStreamingImageControllerAsset = 
                Data::AssetManager::Instance().CreateAsset<DefaultStreamingImageControllerAsset>(
                    DefaultStreamingImageControllerAsset::BuiltInAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

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

            m_defaultStreamingImageControllerAsset.Release();
            m_systemImages.clear();
            m_systemStreamingPool = nullptr;
            m_systemAttachmentPool = nullptr;
            m_assetStreamingPool = nullptr;

            Data::InstanceDatabase<AttachmentImage>::Destroy();
            Data::InstanceDatabase<AttachmentImagePool>::Destroy();
            Data::InstanceDatabase<StreamingImage>::Destroy();
            Data::InstanceDatabase<StreamingImagePool>::Destroy();
            Data::InstanceDatabase<StreamingImageController>::Destroy();

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
            return m_assetStreamingPool;
        }

        const Data::Instance<AttachmentImagePool>& ImageSystem::GetSystemAttachmentPool() const
        {
            return m_systemAttachmentPool;
        }

        const Data::Instance<Image>& ImageSystem::GetSystemImage(SystemImage simpleImage) const
        {
            return m_systemImages[static_cast<size_t>(simpleImage)];
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

            const SystemImagePoolDescriptor systemStreamingPoolDescriptor{ desc.m_systemStreamingImagePoolSize, "ImageSystem::SystemStreamingImagePool" };
            const SystemImagePoolDescriptor systemAttachmentPoolDescriptor{desc.m_systemAttachmentImagePoolSize, "ImageSystem::AttachmentImagePool" };
            const SystemImagePoolDescriptor assetStreamingPoolDescriptor{ desc.m_assetStreamingImagePoolSize, "ImageSystem::AssetStreamingImagePool" };

            // Create the system streaming pool
            {
                AZStd::unique_ptr<RHI::StreamingImagePoolDescriptor> imagePoolDescriptor = AZStd::make_unique<RHI::StreamingImagePoolDescriptor>();
                imagePoolDescriptor->m_budgetInBytes = systemStreamingPoolDescriptor.m_budgetInBytes;

                Data::Asset<StreamingImagePoolAsset> poolAsset;

                StreamingImagePoolAssetCreator poolAssetCreator;
                poolAssetCreator.Begin(systemStreamingPoolDescriptor.m_assetId);
                poolAssetCreator.SetPoolDescriptor(AZStd::move(imagePoolDescriptor));
                poolAssetCreator.SetControllerAsset(m_defaultStreamingImageControllerAsset);
                poolAssetCreator.SetPoolName(systemStreamingPoolDescriptor.m_name);
                [[maybe_unused]] const bool created = poolAssetCreator.End(poolAsset);
                AZ_Assert(created, "Failed to build streaming image pool");

                m_systemStreamingPool = StreamingImagePool::FindOrCreate(poolAsset);
            }

            // Create the streaming pool for streaming image from asset
            {
                AZStd::unique_ptr<RHI::StreamingImagePoolDescriptor> imagePoolDescriptor = AZStd::make_unique<RHI::StreamingImagePoolDescriptor>();
                imagePoolDescriptor->m_budgetInBytes = assetStreamingPoolDescriptor.m_budgetInBytes;

                Data::Asset<StreamingImagePoolAsset> poolAsset;

                StreamingImagePoolAssetCreator poolAssetCreator;
                poolAssetCreator.Begin(assetStreamingPoolDescriptor.m_assetId);
                poolAssetCreator.SetPoolDescriptor(AZStd::move(imagePoolDescriptor));
                poolAssetCreator.SetControllerAsset(m_defaultStreamingImageControllerAsset);
                poolAssetCreator.SetPoolName(assetStreamingPoolDescriptor.m_name);
                [[maybe_unused]] const bool created = poolAssetCreator.End(poolAsset);
                AZ_Assert(created, "Failed to build streaming image pool for assets");

                m_assetStreamingPool = StreamingImagePool::FindOrCreate(poolAsset);
            }
            // Create the system attachment pool.
            {
                AZStd::unique_ptr<RHI::ImagePoolDescriptor> imagePoolDescriptor = AZStd::make_unique<RHI::ImagePoolDescriptor>();
                imagePoolDescriptor->m_budgetInBytes = systemAttachmentPoolDescriptor.m_budgetInBytes;
                imagePoolDescriptor->m_bindFlags =
                    RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::ShaderWrite |
                    RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil |
                    RHI::ImageBindFlags::CopyRead | RHI::ImageBindFlags::CopyWrite;

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


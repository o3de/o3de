/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Atom/RHI.Reflect/Base.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Asset/BuiltInAssetHandler.h>
#include <Atom/RPI.Reflect/Image/DefaultStreamingImageControllerAsset.h>
#include <Atom/RPI.Reflect/Image/ImageSystemDescriptor.h>

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>

#include <AtomCore/Instance/Instance.h>
#include <AtomCore/Instance/InstanceId.h>

namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }

    namespace RPI
    {
        class ImageSystem final
            : public ImageSystemInterface
        {
        public:
            static void Reflect(AZ::ReflectContext* context);
            static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

            void Init(const ImageSystemDescriptor& desc);
            void Shutdown();

            //! Performs a streaming controller update tick, which will fetch / evict mips
            //! based on priority. This should be called once per frame. GPU uploads are
            //! performed during this call.
            void Update() override;

            //////////////////////////////////////////////////////////////////////////
            // ImageSystemInterface
            const Data::Instance<Image>& GetSystemImage(SystemImage systemImage) const override;
            const Data::Instance<StreamingImagePool>& GetSystemStreamingPool() const override;
            const Data::Instance<AttachmentImagePool>& GetSystemAttachmentPool() const override;
            const Data::Instance<StreamingImagePool>& GetStreamingPool() const override;
            //////////////////////////////////////////////////////////////////////////

        private:
            void CreateDefaultResources(const ImageSystemDescriptor& desc);

            // The set of active (externally created) streaming image pools tracked by the system.
            AZStd::mutex m_activeStreamingPoolMutex;
            AZStd::vector<StreamingImagePool*> m_activeStreamingPools;

            Data::Instance<StreamingImagePool> m_systemStreamingPool;
            Data::Instance<AttachmentImagePool> m_systemAttachmentPool;

            // Streaming image pool for streaming image created from asset 
            Data::Instance<StreamingImagePool> m_assetStreamingPool;

            Data::Asset<DefaultStreamingImageControllerAsset> m_defaultStreamingImageControllerAsset;

            AZStd::fixed_vector<Data::Instance<Image>, static_cast<uint32_t>(SystemImage::Count)> m_systemImages;

            bool m_initialized = false;
        };
    }
}

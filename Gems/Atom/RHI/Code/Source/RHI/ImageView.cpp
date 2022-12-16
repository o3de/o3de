/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImageView.h>

namespace AZ
{
    namespace RHI
    {
        ResultCode ImageView::Init(const Image& image, const ImageViewDescriptor& viewDescriptor)
        {
            if (!ValidateForInit(image, viewDescriptor))
            {
                return ResultCode::InvalidOperation;
            }

            m_descriptor = viewDescriptor;
            m_hash = image.GetHash();
            m_hash = TypeHash64(m_descriptor.GetHash(), m_hash);
            return ResourceView::Init(image);
        }

        bool ImageView::ValidateForInit(const Image& image, const ImageViewDescriptor& viewDescriptor) const
        {
            if (Validation::IsEnabled())
            {
                if (IsInitialized())
                {
                    AZ_Warning("ImageView", false, "Image view already initialized");
                    return false;
                }

                if (!image.IsInitialized())
                {
                    AZ_Warning("ImageView", false, "Attempting to create view from uninitialized image '%s'.", image.GetName().GetCStr());
                    return false;
                }

                if (!CheckBitsAll(image.GetDescriptor().m_bindFlags, viewDescriptor.m_overrideBindFlags))
                {
                    AZ_Warning("ImageView", false, "Image view has bind flags that are incompatible with the underlying image.");
            
                    return false;
                }
            }

            return true;
        }

        const ImageViewDescriptor& ImageView::GetDescriptor() const
        {
            return m_descriptor;
        }

        const Image& ImageView::GetImage() const
        {
            return static_cast<const Image&>(GetResource());
        }

        Ptr<DeviceResourceView> ImageView::GetDeviceResourceView(int deviceIndex) const
        {
            return GetDeviceImageView(deviceIndex);
        }

        bool ImageView::IsFullView() const
        {
            const ImageDescriptor& imageDescriptor = GetImage().GetDescriptor();
            return
                m_descriptor.m_arraySliceMin == 0 &&
                (m_descriptor.m_arraySliceMax + 1) >= imageDescriptor.m_arraySize &&
                m_descriptor.m_mipSliceMin == 0 &&
                (m_descriptor.m_mipSliceMax + 1) >= imageDescriptor.m_mipLevels;
        }

        HashValue64 ImageView::GetHash() const
        {
            return m_hash;
        }

        ResultCode ImageView::InitInternal(const Resource& resource)
        {
            const Image& image = static_cast<const Image&>(resource);

            IterateDevices(
                [this, &image](int deviceIndex)
                {
                    m_deviceImageViews[deviceIndex] = Factory::Get().CreateImageView();
                    m_deviceImageViews[deviceIndex]->Init(*image.m_deviceImages.at(deviceIndex), m_descriptor);
                    return true;
                });
            return ResultCode::Success;
        }

        void ImageView::ShutdownInternal()
        {
            InvalidateInternal();
        }

        ResultCode ImageView::InvalidateInternal()
        {
            m_deviceImageViews.clear();
            return ResultCode::Success;
        }
    }
}

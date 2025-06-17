/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/DeviceImage.h>

namespace AZ::RHI
{
    ResultCode DeviceImageView::Init(const DeviceImage& image, const ImageViewDescriptor& viewDescriptor)
    {
        if (!ValidateForInit(image, viewDescriptor))
        {
            return ResultCode::InvalidOperation;
        }

        m_descriptor = viewDescriptor;
        m_hash = image.GetHash();
        m_hash = TypeHash64(m_descriptor.GetHash(), m_hash);
        return DeviceResourceView::Init(image);
    }

    bool DeviceImageView::ValidateForInit(const DeviceImage& image, const ImageViewDescriptor& viewDescriptor) const
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Warning("DeviceImageView", false, "Image view already initialized");
                return false;
            }

            if (!image.IsInitialized())
            {
                AZ_Warning("DeviceImageView", false, "Attempting to create view from uninitialized image '%s'.", image.GetName().GetCStr());
                return false;
            }

            if (!CheckBitsAll(image.GetDescriptor().m_bindFlags, viewDescriptor.m_overrideBindFlags))
            {
                AZ_Warning("DeviceImageView", false, "Image view has bind flags that are incompatible with the underlying image.");
            
                return false;
            }
        }

        return true;
    }

    const ImageViewDescriptor& DeviceImageView::GetDescriptor() const
    {
        return m_descriptor;
    }

    const DeviceImage& DeviceImageView::GetImage() const
    {
        return static_cast<const DeviceImage&>(GetResource());
    }

    bool DeviceImageView::IsFullView() const
    {
        const ImageDescriptor& imageDescriptor = GetImage().GetDescriptor();
        return
            m_descriptor.m_arraySliceMin == 0 &&
            (m_descriptor.m_arraySliceMax + 1) >= imageDescriptor.m_arraySize &&
            m_descriptor.m_mipSliceMin == 0 &&
            (m_descriptor.m_mipSliceMax + 1) >= imageDescriptor.m_mipLevels;
    }

    HashValue64 DeviceImageView::GetHash() const
    {
        return m_hash;
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI/DeviceResourceView.h>

namespace AZ::RHI
{
    class DeviceImage;

    //! DeviceImageView contains a platform-specific descriptor mapping to a sub-region of an image resource.
    //! It associates 1-to-1 with a ImageViewDescriptor. Image views map to a subset of image sub-resources
    //! (mip levels / array slices). They can additionally override the base format of the image
    class DeviceImageView
        : public DeviceResourceView
    {
    public:
        AZ_RTTI(DeviceImageView, "{F2BDEE1F-DEFD-4443-9012-A28AED028D7B}", DeviceResourceView);
        virtual ~DeviceImageView() = default;

        static constexpr uint32_t InvalidBindlessIndex = 0xFFFFFFFF;

        //! Initializes the image view.
        ResultCode Init(const DeviceImage& image, const ImageViewDescriptor& viewDescriptor);

        //! Returns the view descriptor used at initialization time.
        const ImageViewDescriptor& GetDescriptor() const;

        //! Returns the image associated with this view.
        const DeviceImage& GetImage() const;

        //! Returns whether the view covers the entire image (i.e. isn't just a subset).
        bool IsFullView() const override final;

        //! Returns the hash of the view.
        HashValue64 GetHash() const;

        virtual uint32_t GetBindlessReadIndex() const
        {
            return InvalidBindlessIndex;
        }

        virtual uint32_t GetBindlessReadWriteIndex() const
        {
            return InvalidBindlessIndex;
        }

    protected:
        HashValue64 m_hash = HashValue64{ 0 };

    private:
        bool ValidateForInit(const DeviceImage& image, const ImageViewDescriptor& viewDescriptor) const;

        // The RHI descriptor for this view.
        ImageViewDescriptor m_descriptor;
    };
}

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

#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/Image.h>

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

        bool ImageView::IsFullView() const
        {
            const ImageDescriptor& imageDescriptor = GetImage().GetDescriptor();
            return
                m_descriptor.m_arraySliceMin == 0 &&
                (m_descriptor.m_arraySliceMax + 1) >= imageDescriptor.m_arraySize &&
                m_descriptor.m_mipSliceMin == 0 &&
                (m_descriptor.m_mipSliceMax + 1) >= imageDescriptor.m_mipLevels;
        }
    }
}

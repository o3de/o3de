/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/StreamingImageContext.h>
#include <Atom/RPI.Public/Image/StreamingImageController.h>

namespace AZ
{
    namespace RPI
    {
        StreamingImage* StreamingImageContext::TryGetImage() const
        {
            return m_streamingImage;
        }

        uint16_t StreamingImageContext::GetTargetMip() const
        {
            return m_mipLevelTarget;
        }

        size_t StreamingImageContext::GetLastAccessTimestamp() const
        {
            return m_lastAccessTimestamp;
        }

        void StreamingImageContext::UpdateMipStats()
        {
            m_mipLevelTargetAdjusted = m_streamingImage->m_streamingController->GetImageTargetMip(m_streamingImage);
            m_residentMip = m_streamingImage->GetResidentMipLevel();

            if (m_residentMip > m_mipLevelTargetAdjusted)
            {
                m_missingMips = m_residentMip - m_mipLevelTargetAdjusted;
            }
            else
            {
                m_missingMips = 0;
            }

            const size_t mipChainTailIndex = m_streamingImage->m_imageAsset->GetMipChainCount() - 1;
            size_t tailMip = m_streamingImage->m_imageAsset->GetMipLevel(mipChainTailIndex);
            if (tailMip > m_residentMip)
            {
                m_evictableMips = aznumeric_cast<uint16_t>(tailMip) - m_residentMip;
            }
            else
            {
                m_evictableMips = 0;
            }

            // get the length of the resident mip
            RHI::Size mipSize = m_streamingImage->m_imageAsset->GetImageDescriptor().m_size.GetReducedMip(aznumeric_cast<uint32_t>(m_residentMip));
            m_residentMipSize = mipSize.m_width > mipSize.m_height? mipSize.m_width : mipSize.m_height;
        }
    }
}

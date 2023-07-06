/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/SwapChainDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void SwapChainDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<SwapChainDimensions>()
                ->Version(1)
                ->Field("m_imageCount", &SwapChainDimensions::m_imageCount)
                ->Field("m_imageWidth", &SwapChainDimensions::m_imageWidth)
                ->Field("m_imageHeight", &SwapChainDimensions::m_imageHeight)
                ->Field("m_imageFormat", &SwapChainDimensions::m_imageFormat);

            serializeContext->Class<SwapChainDescriptor, ResourcePoolDescriptor>()
                ->Version(1)
                ->Field("m_dimensions", &SwapChainDescriptor::m_dimensions)
                ->Field("m_verticalSyncInterval", &SwapChainDescriptor::m_verticalSyncInterval);
        }
    }
}

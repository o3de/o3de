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

#include <Atom/RHI.Reflect/SwapChainDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RHI
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
}

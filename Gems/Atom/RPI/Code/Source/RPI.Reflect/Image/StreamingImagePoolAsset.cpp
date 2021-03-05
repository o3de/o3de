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

#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        const char* StreamingImagePoolAsset::DisplayName = "StreamingImagePool";
        const char* StreamingImagePoolAsset::Group = "Image";
        const char* StreamingImagePoolAsset::Extension = "streamingimagepool";

        void StreamingImagePoolAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<StreamingImagePoolAsset, Data::AssetData>()
                    ->Version(0)
                    ->Field("m_poolDescriptor", &StreamingImagePoolAsset::m_poolDescriptor)
                    ->Field("m_controllerAsset", &StreamingImagePoolAsset::m_controllerAsset)
                    ->Field("m_poolName", &StreamingImagePoolAsset::m_poolName)
                    ;
            }
        }

        void StreamingImagePoolAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        const RHI::StreamingImagePoolDescriptor& StreamingImagePoolAsset::GetPoolDescriptor() const
        {
            return *m_poolDescriptor;
        }

        const Data::Asset<StreamingImageControllerAsset>& StreamingImagePoolAsset::GetControllerAsset() const
        {
            return m_controllerAsset;
        }

        AZStd::string_view StreamingImagePoolAsset::GetPoolName() const
        {
            return m_poolName;
        }
    }
}

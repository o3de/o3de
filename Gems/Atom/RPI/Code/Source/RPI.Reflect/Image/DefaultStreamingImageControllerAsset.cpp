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

#include <Atom/RPI.Reflect/Image/DefaultStreamingImageControllerAsset.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        const Data::AssetId DefaultStreamingImageControllerAsset::BuiltInAssetId("{D551D634-C025-4E67-A73A-CB10CD3DE72A}");

        DefaultStreamingImageControllerAsset::DefaultStreamingImageControllerAsset()
        {
            m_status = AssetStatus::Ready;
        }

        void DefaultStreamingImageControllerAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DefaultStreamingImageControllerAsset, StreamingImageControllerAsset>();
            }
        }
    }
}

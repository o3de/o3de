/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

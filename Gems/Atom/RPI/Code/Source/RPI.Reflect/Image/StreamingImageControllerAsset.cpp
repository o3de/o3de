/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/StreamingImageControllerAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void StreamingImageControllerAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<StreamingImageControllerAsset, Data::AssetData>();
            }
        }
    }
}

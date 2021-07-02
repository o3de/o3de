/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/Serialization/SerializeContext.h>

// Graph Canvas
#include <GraphCanvas/Types/EntitySaveData.h>

// Graph Model
#include <GraphModel/Integration/GraphCanvasMetadata.h>

namespace GraphModelIntegration
{
    void GraphCanvasMetadata::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        if (serializeContext)
        {
            serializeContext->Class<GraphCanvasMetadata>()
                ->Version(0)
                ->Field("m_sceneMetadata", &GraphCanvasMetadata::m_sceneMetadata)
                ->Field("m_nodeMetadata", &GraphCanvasMetadata::m_nodeMetadata)
                ->Field("m_otherMetadata", &GraphCanvasMetadata::m_otherMetadata)
                ;
        }
    }
} // namespace ShaderCanvas

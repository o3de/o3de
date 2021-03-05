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

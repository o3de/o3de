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

#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void GridComponentConfig::Reflect(ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<GridComponentConfig, ComponentConfig>()
                    ->Version(0)
                    ->Field("gridSize", &GridComponentConfig::m_gridSize)
                    ->Field("axisColor", &GridComponentConfig::m_axisColor)
                    ->Field("primarySpacing", &GridComponentConfig::m_primarySpacing)
                    ->Field("primaryColor", &GridComponentConfig::m_primaryColor)
                    ->Field("secondarySpacing", &GridComponentConfig::m_secondarySpacing)
                    ->Field("secondaryColor", &GridComponentConfig::m_secondaryColor)
                    ;
            }
        }
    } // namespace Render
} // namespace AZ

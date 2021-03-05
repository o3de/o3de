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

namespace LmbrCentral
{
    namespace ClassConverters
    {
        template <typename Shape, typename ShapeConfig>
        bool UpgradeShapeComponentConfigToShape(AZ::u32 version, const char* shapeName, AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (version <= 2)
            {
                const int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
                if (configIndex != -1)
                {
                    // cache shape config
                    ShapeConfig configuration;
                    classElement.GetSubElement(configIndex).GetDataHierarchy<ShapeConfig>(context, configuration);
                    // remove existing config from stream
                    classElement.RemoveElement(configIndex);
                    // add shape to shape component
                    const int shapeIndex = classElement.AddElement<Shape>(context, shapeName);
                    if (shapeIndex != -1)
                    {
                        // add shape config to shape
                        classElement.GetSubElement(shapeIndex).AddElementWithData<ShapeConfig>(context, "Configuration", configuration);
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }

            return true;
        }
    }
}
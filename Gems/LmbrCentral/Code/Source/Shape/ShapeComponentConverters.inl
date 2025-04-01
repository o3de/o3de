/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                const int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
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

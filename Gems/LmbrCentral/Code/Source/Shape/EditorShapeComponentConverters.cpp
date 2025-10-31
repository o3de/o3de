/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorShapeComponentConverters.h"

#include "EditorCapsuleShapeComponent.h"
#include "EditorCylinderShapeComponent.h"
#include "EditorPolygonPrismShapeComponent.h"
#include "EditorSphereShapeComponent.h"
#include "EditorBoxShapeComponent.h"
#include "ShapeComponentConverters.h"
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        bool DeprecateEditorSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Cache the Configuration
            SphereShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<SphereShapeConfig>(configuration);
            }
            else
            {
                return false;
            }

            // Convert to EditorSphereShapeComponent
            const bool result = classElement.Convert<EditorSphereShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<SphereShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<SphereShapeConfig>(context, configuration);
                    return true;
                }

                return false;
            }

            return false;
        }

        bool UpgradeEditorSphereShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            const AZ::u32 version = classElement.GetVersion();
            if (version <= 1)
            {
                SphereShapeConfig configuration;
                int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).GetData(configuration);
                }
                else
                {
                    return false;
                }

                // Convert to EditorSphereShapeComponent
                const bool result = classElement.Convert<EditorSphereShapeComponent>(context);
                if (result)
                {
                    configIndex = classElement.AddElement<SphereShapeConfig>(context, "Configuration");
                    if (configIndex != -1)
                    {
                        classElement.GetSubElement(configIndex).SetData<SphereShapeConfig>(context, configuration);
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

            return UpgradeShapeComponentConfigToShape<SphereShape, SphereShapeConfig>(version, "SphereShape", context, classElement);;
        }

        bool DeprecateEditorBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Cache the Configuration
            BoxShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<BoxShapeConfig>(configuration);
            }
            else
            {
                return false;
            }

            // Convert to EditorBoxShapeComponent
            if (classElement.Convert(context, EditorBoxShapeComponentTypeId))
            {
                configIndex = classElement.AddElement<BoxShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<BoxShapeConfig>(context, configuration);
                    return true;
                }

                return false;
            }

            return false;
        }

        bool UpgradeEditorBoxShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            const AZ::u32 version = classElement.GetVersion();

            if (version <= 1)
            {
                // Cache the Configuration
                BoxShapeConfig configuration;
                int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).GetData(configuration);
                }
                else
                {
                    return false;
                }

                // Convert to EditorBoxShapeComponent
                if (classElement.Convert(context, EditorBoxShapeComponentTypeId))
                {
                    configIndex = classElement.AddElement<BoxShapeConfig>(context, "Configuration");
                    if (configIndex != -1)
                    {
                        classElement.GetSubElement(configIndex).SetData<BoxShapeConfig>(context, configuration);
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

            // upgrade for editor and runtime components at this stage are the same
            return UpgradeShapeComponentConfigToShape<BoxShape, BoxShapeConfig>(version, "BoxShape", context, classElement);
        }

        bool DeprecateEditorCylinderColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Cache the Configuration
            CylinderShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<CylinderShapeConfig>(configuration);
            }
            else
            {
                return false;
            }

            // Convert to EditorCylinderShapeComponent
            const bool result = classElement.Convert<EditorCylinderShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<CylinderShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<CylinderShapeConfig>(context, configuration);
                    return true;
                }

                return false;
            }

            return false;
        }

        bool UpgradeEditorCylinderShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            const AZ::u32 version = classElement.GetVersion();
            if (version <= 1)
            {
                // Cache the Configuration
                CylinderShapeConfig configuration;
                int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).GetData(configuration);
                }
                else
                {
                    return false;
                }

                // Convert to EditorCylinderShapeComponent
                const bool result = classElement.Convert<EditorCylinderShapeComponent>(context);
                if (result)
                {
                    configIndex = classElement.AddElement<CylinderShapeConfig>(context, "Configuration");
                    if (configIndex != -1)
                    {
                        classElement.GetSubElement(configIndex).SetData<CylinderShapeConfig>(context, configuration);
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

            // upgrade for editor and runtime components at this stage are the same
            return UpgradeShapeComponentConfigToShape<CylinderShape, CylinderShapeConfig>(version, "CylinderShape", context, classElement);;
        }

        bool DeprecateEditorCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Cache the Configuration
            CapsuleShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<CapsuleShapeConfig>(configuration);
            }
            else
            {
                return false;
            }

            // Convert to EditorCapsuleShapeComponent
            const bool result = classElement.Convert<EditorCapsuleShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<CapsuleShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<CapsuleShapeConfig>(context, configuration);
                    return true;
                }

                return false;
            }

            return false;
        }

        bool UpgradeEditorCapsuleShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            const AZ::u32 version = classElement.GetVersion();
            if (version <= 1)
            {
                // Cache the Configuration
                CapsuleShapeConfig configuration;
                int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).GetData(configuration);
                }
                else
                {
                    return false;
                }

                // Convert to EditorCapsuleShapeComponent
                const bool result = classElement.Convert<EditorCapsuleShapeComponent>(context);
                if (result)
                {
                    configIndex = classElement.AddElement<CapsuleShapeConfig>(context, "Configuration");
                    if (configIndex != -1)
                    {
                        classElement.GetSubElement(configIndex).SetData<CapsuleShapeConfig>(context, configuration);
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

            return UpgradeShapeComponentConfigToShape<CapsuleShape, CapsuleShapeConfig>(version, "CapsuleShape", context, classElement);
        }

        bool UpgradeEditorPolygonPrismShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                // Cache the Configuration
                PolygonPrismShape configuration;
                int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).GetData(configuration);
                }
                else
                {
                    return false;
                }

                // Convert to EditorPolygonPrismComponent
                const bool result = classElement.Convert<EditorPolygonPrismShapeComponent>(context);
                if (result)
                {
                    configIndex = classElement.AddElement<PolygonPrismShape>(context, "Configuration");
                    if (configIndex != -1)
                    {
                        classElement.GetSubElement(configIndex).SetData<PolygonPrismShape>(context, configuration);
                        return true;
                    }

                    return false;
                }

                return false;
            }

            return true;
        }
    }
}

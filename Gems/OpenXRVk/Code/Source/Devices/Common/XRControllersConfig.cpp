/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

#include "XRControllersConfig.h"

namespace OpenXRVk
{
        void XRControllersConfig::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<XRControllersConfig>()
                    ->Field("controlItemType", &XRControllersConfig::m_controlItemType)
                    ->Field("parameterControlLabel", &XRControllersConfig::m_animGraphParameter)
                    ->Field("controlActionLabel", &XRControllersConfig::m_actionName)
                ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<XRControllersConfig>("XRControllersConfig", "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &XRControllersConfig::m_controlItemType, "Type of the item", "Is it button or grip slider or thumbstick")
                            ->EnumAttribute(XRControllersConfig::ControlItemType::Boolean, "Button (boolean)")
                            ->EnumAttribute(XRControllersConfig::ControlItemType::Float, "Trigger (float)")
                            ->EnumAttribute(XRControllersConfig::ControlItemType::Vector2, "Thumbstick (360 degrees)")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &XRControllersConfig::m_animGraphParameter, "AnimGraph parameter", "Name of parameter that controls an animation of the control item in AnimGraph")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &XRControllersConfig::m_actionName, "Action name", "OpenXRActionsInterface ActionHandle label")
                    ;
                }
            }
        }

} // namespace OpenXRVk

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

// Editor
#include "PropertyCtrl.h"
#include "PropertyResourceCtrl.h"
#include "PropertyGenericCtrl.h"
#include "PropertyMiscCtrl.h"
#include "PropertyMotionCtrl.h"

void RegisterReflectedVarHandlers()
{
    static bool registered = false;
    if (!registered)
    {
        registered = true;
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew FileResourceSelectorWidgetHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SequencePropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SequenceIdPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew LocalStringPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew LightAnimationPropertyHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew UserPopupWidgetHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew FloatCurveHandler());
        EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew MotionPropertyWidgetHandler());
    }
}


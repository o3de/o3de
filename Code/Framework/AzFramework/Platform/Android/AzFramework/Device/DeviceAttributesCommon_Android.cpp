/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Console/ConsoleTypeHelpers.h>
#include <AzFramework/Device/DeviceAttributeDeviceModel.h>
#include <AzFramework/Device/DeviceAttributeRAM.h>

namespace AzFramework
{
    DeviceAttributeDeviceModel::DeviceAttributeDeviceModel()
    {
        static constexpr const char* JavaFieldName = "MODEL";
        AZ::Android::JNI::Object obj("android/os/Build");
        obj.RegisterStaticField(JavaFieldName, "Ljava/lang/String;");
        m_value = obj.GetStaticStringField(JavaFieldName);
    }

    DeviceAttributeRAM::DeviceAttributeRAM()
    {
        static constexpr const char* JavaFuntionNameGetDeviceRamInGB = "GetDeviceRamInGB";
        AZ::Android::JNI::Object obj("com/amazon/lumberyard/AndroidDeviceManager");
        obj.RegisterStaticMethod(JavaFuntionNameGetDeviceRamInGB, "()F");
        m_valueInGiB = obj.InvokeStaticFloatMethod(JavaFuntionNameGetDeviceRamInGB);
        m_value = AZ::ConsoleTypeHelpers::ValueToString(m_valueInGiB);
    }
} // AzFramework


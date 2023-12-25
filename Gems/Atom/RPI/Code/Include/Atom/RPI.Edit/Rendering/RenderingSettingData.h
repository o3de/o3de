/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include "RenderingSettingDataSerializer.h"

// Rendering Settings

class EditorRenderingSettingData final
{
public:
    AZ_TYPE_INFO(EditorRenderingSettingData, "{EB103C0C-20FA-468F-9C79-DD99805B7DE1}");
    static void Reflect(AZ::ReflectContext* context);
    static void UnReflect();

    EditorRenderingSettingData();

    AZStd::string m_type = "JsonSerialization";
    int m_version = 1;
    AZStd::string m_className = "RenderPipelineDescriptor";
    AZ::RPI::RenderPipelineDescriptor m_renderPipelineDescriptor;
};

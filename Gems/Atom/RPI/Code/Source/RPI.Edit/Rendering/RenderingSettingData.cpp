/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
//#include "../../../Include/Atom/RPI.Edit/Rendering/RenderingSettingData.h"
#include <Atom/RPI.Edit/Rendering/RenderingSettingData.h>
// Rendering Settings

void EditorRenderingSettingData::Reflect(AZ::ReflectContext* context)
{
    AZ::JsonRegistrationContext* jsonRegistrationContext = nullptr;
    EBUS_EVENT_RESULT(jsonRegistrationContext, AZ::ComponentApplicationBus, GetJsonRegistrationContext);
    AZ_Assert(jsonRegistrationContext, "Serialization context not available");
    jsonRegistrationContext->Serializer<RenderingSettingDataSerializer>()->HandlesType<EditorRenderingSettingData>();
    jsonRegistrationContext->Serializer<RenderingPipelineDescriptorSerializer>()->HandlesType<AZ::RPI::RenderPipelineDescriptor>();
    jsonRegistrationContext->Serializer<RenderingSettingsSerializer>()->HandlesType<AZ::RPI::PipelineRenderSettings>();
    jsonRegistrationContext->Serializer<MultisampleStateSerializer>()->HandlesType<AZ::RHI::MultisampleState>();
    if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<EditorRenderingSettingData>()
            ->Version(1)
            ->Field("Type", &EditorRenderingSettingData::m_type)
            ->Field("Version", &EditorRenderingSettingData::m_version)
            ->Field("ClassName", &EditorRenderingSettingData::m_className)
            ->Field("ClassData", &EditorRenderingSettingData::m_renderPipelineDescriptor);
    }
}

void EditorRenderingSettingData::UnReflect()
{
    AZ::JsonRegistrationContext* jsonRegistrationContext = nullptr;
    EBUS_EVENT_RESULT(jsonRegistrationContext, AZ::ComponentApplicationBus, GetJsonRegistrationContext);
    AZ_Assert(jsonRegistrationContext, "Serialization context not available");
    jsonRegistrationContext->EnableRemoveReflection();
    jsonRegistrationContext->Serializer<RenderingSettingDataSerializer>()->HandlesType<EditorRenderingSettingData>();
    jsonRegistrationContext->Serializer<RenderingPipelineDescriptorSerializer>()->HandlesType<AZ::RPI::RenderPipelineDescriptor>();
    jsonRegistrationContext->Serializer<RenderingSettingsSerializer>()->HandlesType<AZ::RPI::PipelineRenderSettings>();
    jsonRegistrationContext->Serializer<MultisampleStateSerializer>()->HandlesType<AZ::RHI::MultisampleState>();
    jsonRegistrationContext->DisableRemoveReflection();
}

EditorRenderingSettingData::EditorRenderingSettingData()
{

}

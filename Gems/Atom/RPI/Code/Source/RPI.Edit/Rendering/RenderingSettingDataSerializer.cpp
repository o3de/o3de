/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Rendering/RenderingSettingDataSerializer.h>
#include <Atom/RPI.Edit/Rendering/RenderingSettingData.h>

AZ_CLASS_ALLOCATOR_IMPL(RenderingSettingsSerializer, AZ::SystemAllocator, 0);
AZ_CLASS_ALLOCATOR_IMPL(RenderingPipelineDescriptorSerializer, AZ::SystemAllocator, 0);
AZ_CLASS_ALLOCATOR_IMPL(RenderingSettingDataSerializer, AZ::SystemAllocator, 0);
AZ_CLASS_ALLOCATOR_IMPL(MultisampleStateSerializer, AZ::SystemAllocator, 0);

namespace JSR = AZ::JsonSerializationResult;

// Rendering Settings
AZ::JsonSerializationResult::Result MultisampleStateSerializer::Load(
    void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
{
    AZ_Assert(
        azrtti_typeid<AZ::RHI::MultisampleState>() == outputValueTypeId,
        "Unable to deserialize MultisampleState to json because the provided type is %s",
        outputValueTypeId.ToString<AZStd::string>().c_str());
    AZ_UNUSED(outputValueTypeId);
    AZ::RHI::MultisampleState* multisampleState = reinterpret_cast<AZ::RHI::MultisampleState*>(outputValue);
    AZ_Assert(multisampleState, "Output value for MultisampleStateSerializer can't be null.");

    if (!inputValue.IsObject())
    {
        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "MultisampleState data must be a JSON object");
    }
    JSR::ResultCode result(JSR::Tasks::ReadField);

    result.Combine(ContinueLoadingFromJsonObjectField(&multisampleState->m_samples, azrtti_typeid<int>(), inputValue, "samples", context));
    if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
    {
        return context.Report(result, "Successfully loaded MultisampleState.");
    }
    return context.Report(result, "Partially loaded MultisampleState.");
}

AZ::JsonSerializationResult::Result MultisampleStateSerializer::Store(
    [[maybe_unused]] rapidjson::Value& outputValue,
    const void* inputValue,
    [[maybe_unused]] const void* defaultValue,
    const AZ::Uuid& valueTypeId,
    AZ::JsonSerializerContext& context)
{
    AZ_Assert(
        azrtti_typeid<AZ::RHI::MultisampleState>() == valueTypeId,
        "Unable to serialize MultisampleState to json because the provided type is %s",
        valueTypeId.ToString<AZStd::string>().c_str());
    AZ_UNUSED(valueTypeId);

    const AZ::RHI::MultisampleState* multisampleState = reinterpret_cast<const AZ::RHI::MultisampleState*>(inputValue);
    AZ_Assert(multisampleState, "Input value for MultisampleStateSerializer can't be null.");

    JSR::ResultCode resultCode(JSR::Tasks::ReadField);

    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "samples", &multisampleState->m_samples, nullptr, azrtti_typeid<int>(),
        context));
    return context.Report(resultCode, "Processed MultisampleState.");
}

AZ::JsonSerializationResult::Result RenderingSettingsSerializer::Load(
    void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
{
    AZ_Assert(
        azrtti_typeid<AZ::RPI::PipelineRenderSettings>() == outputValueTypeId,
        "Unable to deserialize RenderingSettingData to json because the provided type is %s",
        outputValueTypeId.ToString<AZStd::string>().c_str());
    AZ_UNUSED(outputValueTypeId);
    AZ::RPI::PipelineRenderSettings* renderSettings = reinterpret_cast<AZ::RPI::PipelineRenderSettings*>(outputValue);
    AZ_Assert(renderSettings, "Output value for RenderingSettingsSerializer can't be null.");

    if (!inputValue.IsObject())
    {
        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "RenderSetting data must be a JSON object");
    }
    JSR::ResultCode result(JSR::Tasks::ReadField);

    result.Combine(ContinueLoadingFromJsonObjectField(
        &renderSettings->m_multisampleState, azrtti_typeid<AZ::RHI::MultisampleState>(), inputValue, "MultisampleState", context));
    if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
    {
        return context.Report(result, "Successfully loaded RenderingSettingData.");
    }
    return context.Report(result, "Partially loaded RenderingSettings.");
}

AZ::JsonSerializationResult::Result RenderingSettingsSerializer::Store(
    [[maybe_unused]] rapidjson::Value& outputValue,
    const void* inputValue,
    [[maybe_unused]] const void* defaultValue,
    const AZ::Uuid& valueTypeId,
    AZ::JsonSerializerContext& context)
{
    AZ_Assert(
        azrtti_typeid<AZ::RPI::PipelineRenderSettings>() == valueTypeId,
        "Unable to serialize PipelineRenderSettings to json because the provided type is %s", valueTypeId.ToString<AZStd::string>().c_str());
    AZ_UNUSED(valueTypeId);

    const AZ::RPI::PipelineRenderSettings* renderSettings = reinterpret_cast<const AZ::RPI::PipelineRenderSettings*>(inputValue);
    AZ_Assert(renderSettings, "Input value for RenderingSettingsSerializer can't be null.");

    JSR::ResultCode resultCode(JSR::Tasks::ReadField);

    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "MultisampleState", &renderSettings->m_multisampleState, nullptr, azrtti_typeid<AZ::RHI::MultisampleState>(), context));
    return context.Report(resultCode, "Processed PipelineRenderSettings.");
}

AZ::JsonSerializationResult::Result RenderingPipelineDescriptorSerializer::Load(
    void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
{
    AZ_Assert(
        azrtti_typeid<AZ::RPI::RenderPipelineDescriptor>() == outputValueTypeId,
        "Unable to deserialize RenderPipelineDescriptor to json because the provided type is %s",
        outputValueTypeId.ToString<AZStd::string>().c_str());
    AZ_UNUSED(outputValueTypeId);
    AZ::RPI::RenderPipelineDescriptor* renderPipelineDescriptor = reinterpret_cast<AZ::RPI::RenderPipelineDescriptor*>(outputValue);
    AZ_Assert(renderPipelineDescriptor, "Output value for RenderPipelineDescriptor can't be null.");

    if (!inputValue.IsObject())
    {
        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "RenderPipelineDescriptor must be a JSON object");
    }
    JSR::ResultCode result(JSR::Tasks::ReadField);
    result.Combine(
        ContinueLoadingFromJsonObjectField(&renderPipelineDescriptor->m_name, azrtti_typeid<AZStd::string>(), inputValue, "Name", context));
    result.Combine(
        ContinueLoadingFromJsonObjectField(&renderPipelineDescriptor->m_mainViewTagName, azrtti_typeid<AZStd::string>(), inputValue, "MainViewTag", context));
    result.Combine(
        ContinueLoadingFromJsonObjectField(&renderPipelineDescriptor->m_rootPassTemplate, azrtti_typeid<AZStd::string>(), inputValue, "RootPassTemplate", context));
    result.Combine(
        ContinueLoadingFromJsonObjectField(&renderPipelineDescriptor->m_allowModification, azrtti_typeid<bool>(), inputValue, "AllowModification", context));
    result.Combine(ContinueLoadingFromJsonObjectField(
        &renderPipelineDescriptor->m_renderSettings, azrtti_typeid<AZ::RPI::PipelineRenderSettings>(), inputValue, "RenderSettings", context));
    result.Combine(
        ContinueLoadingFromJsonObjectField(&renderPipelineDescriptor->m_defaultAAMethod, azrtti_typeid<AZStd::string>(), inputValue, "DefaultAAMethod", context));
    
    if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
    {
        return context.Report(result, "Successfully loaded RenderingPipelineDescriptor.");
    }
    return context.Report(result, "Partially loaded RenderingPipelineDescriptor.");
}

AZ::JsonSerializationResult::Result RenderingPipelineDescriptorSerializer::Store(
    [[maybe_unused]] rapidjson::Value& outputValue,
    const void* inputValue,
    [[maybe_unused]] const void* defaultValue,
    const AZ::Uuid& valueTypeId,
    AZ::JsonSerializerContext& context)
{
    AZ_Assert(
        azrtti_typeid<AZ::RPI::RenderPipelineDescriptor>() == valueTypeId, "Unable to serialize RenderingPipelineDescriptor to json because the provided type is %s",
        valueTypeId.ToString<AZStd::string>().c_str());
    AZ_UNUSED(valueTypeId);

    const AZ::RPI::RenderPipelineDescriptor* renderPipelineDescriptor = reinterpret_cast<const AZ::RPI::RenderPipelineDescriptor*>(inputValue);
    AZ_Assert(renderPipelineDescriptor, "Input value for RenderingPipelineDescriptorSerializer can't be null.");

    JSR::ResultCode resultCode(JSR::Tasks::ReadField);
    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "Name", &renderPipelineDescriptor->m_name, nullptr, azrtti_typeid<AZStd::string>(), context));
    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "MainViewTag", &renderPipelineDescriptor->m_mainViewTagName, nullptr, azrtti_typeid<AZStd::string>(), context));
    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "RootPassTemplate", &renderPipelineDescriptor->m_rootPassTemplate, nullptr, azrtti_typeid<AZStd::string>(), context));
    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "AllowModification", &renderPipelineDescriptor->m_allowModification, nullptr, azrtti_typeid<bool>(), context));
    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "RenderSettings", &renderPipelineDescriptor->m_renderSettings, nullptr, azrtti_typeid<AZ::RPI::PipelineRenderSettings>(), context));
    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "DefaultAAMethod", &renderPipelineDescriptor->m_defaultAAMethod, nullptr, azrtti_typeid<AZStd::string>(), context));

    return context.Report(resultCode, "Processed RenderingPipelineDescriptor.");
}

AZ::JsonSerializationResult::Result RenderingSettingDataSerializer::Load(
    void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
{
    AZ_Assert(
        azrtti_typeid<EditorRenderingSettingData>() == outputValueTypeId,
        "Unable to deserialize RenderingSettingData to json because the provided type is %s",
        outputValueTypeId.ToString<AZStd::string>().c_str());
    AZ_UNUSED(outputValueTypeId);
    EditorRenderingSettingData* renderSettingData = reinterpret_cast<EditorRenderingSettingData*>(outputValue);
    AZ_Assert(renderSettingData, "Output value for RenderingSettingDataSerializer can't be null.");

    if (!inputValue.IsObject())
    {
        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "RenderSetting data must be a JSON object");
    }
    JSR::ResultCode result(JSR::Tasks::ReadField);

    result.Combine(ContinueLoadingFromJsonObjectField(
        &renderSettingData->m_renderPipelineDescriptor, azrtti_typeid<AZ::RPI::RenderPipelineDescriptor>(), inputValue, "ClassData", context));
    if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
    {
        return context.Report(result, "Successfully loaded RenderingSettingData.");
    }
    return context.Report(result, "Partially loaded RenderingSettingData.");
}

AZ::JsonSerializationResult::Result RenderingSettingDataSerializer::Store(
    [[maybe_unused]] rapidjson::Value& outputValue,
    const void* inputValue,
    [[maybe_unused]] const void* defaultValue,
    const AZ::Uuid& valueTypeId,
    AZ::JsonSerializerContext& context)
{
    AZ_Assert(
        azrtti_typeid<EditorRenderingSettingData>() == valueTypeId,
        "Unable to serialize RenderingSettingData to json because the provided type is %s",
        valueTypeId.ToString<AZStd::string>().c_str());
    AZ_UNUSED(valueTypeId);

    const EditorRenderingSettingData* renderSettingData = reinterpret_cast<const EditorRenderingSettingData*>(inputValue);
    AZ_Assert(renderSettingData, "Input value for RenderingSettingDataSerializer can't be null.");

    JSR::ResultCode resultCode(JSR::Tasks::ReadField);
    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "Type", &renderSettingData->m_type, nullptr, azrtti_typeid<AZStd::string>(), context));
    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "Version", &renderSettingData->m_version, nullptr, azrtti_typeid<int>(), context));
    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "ClassName", &renderSettingData->m_className, nullptr, azrtti_typeid<AZStd::string>(), context));
    resultCode.Combine(ContinueStoringToJsonObjectField(
        outputValue, "ClassData", &renderSettingData->m_renderPipelineDescriptor, nullptr, azrtti_typeid<AZ::RPI::RenderPipelineDescriptor>(), context));

    return context.Report(resultCode, "Processed particle.");
}


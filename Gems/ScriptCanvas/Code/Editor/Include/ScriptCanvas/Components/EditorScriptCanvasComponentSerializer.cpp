/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
    
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponentSerializer.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorScriptCanvasComponentSerializer, SystemAllocator);

    JsonSerializationResult::Result EditorScriptCanvasComponentSerializer::Load
        ( void* outputValue
        , [[maybe_unused]] const Uuid& outputValueTypeId
        , const rapidjson::Value& inputValue
        , JsonDeserializerContext& context)
    {
        using namespace ScriptCanvas;
        namespace JSR = JsonSerializationResult;

        AZ_Assert(outputValueTypeId == azrtti_typeid<ScriptCanvasEditor::EditorScriptCanvasComponent>()
            , "EditorScriptCanvasComponentSerializer Load against output typeID that was not EditorScriptCanvasComponent");
        AZ_Assert(outputValue, "EditorScriptCanvasComponentSerializer Load against null output");

        // load as parent class
        JsonSerializationResult::ResultCode result = BaseJsonSerializer::Load(outputValue
            , azrtti_typeid<AzToolsFramework::Components::EditorComponentBase>(), inputValue, context);

        auto outputComponent = reinterpret_cast<ScriptCanvasEditor::EditorScriptCanvasComponent*>(outputValue);

        AZStd::string failureMessage = "EditorScriptCanvasComponentSerializer Load failed to load EditorScriptCanvasComponent";

        // load child data one by one
        if (result.GetProcessing() != JSR::Processing::Halted)
        {
            if (auto configurationMember = inputValue.FindMember("configuration"); configurationMember != inputValue.MemberEnd())
            {
                // version latest
                result.Combine(ContinueLoading
                    ( &outputComponent->m_configuration
                    , azrtti_typeid(outputComponent->m_configuration)
                    , configurationMember->value
                    , context));
            }
            else
            {
                // version latest - 1
                AZStd::string sourceName;
                if (auto sourceNameMember = inputValue.FindMember("m_name"); sourceNameMember != inputValue.MemberEnd())
                {
                    result.Combine(ContinueLoading(&sourceName, azrtti_typeid(sourceName), sourceNameMember->value, context));
                }

                ScriptCanvasBuilder::BuildVariableOverrides overrides;
                if (auto overridesMember = inputValue.FindMember("runtimeDataOverrides"); overridesMember != inputValue.MemberEnd())
                {
                    result.Combine(ContinueLoading(&overrides, azrtti_typeid(overrides), overridesMember->value, context));
                }

                SourceHandle sourceHandle;
                if (auto sourceHandleMember = inputValue.FindMember("sourceHandle"); sourceHandleMember != inputValue.MemberEnd())
                {
                    // file was saved with SourceHandle data
                    result.Combine(ContinueLoading(&sourceHandle, azrtti_typeid(sourceHandle), sourceHandleMember->value, context));
                }
                else if (auto assetHolderMember = inputValue.FindMember("m_assetHolder"); assetHolderMember != inputValue.MemberEnd())
                {
                    // version latest - 2
                    const rapidjson::Value& assetHolderValue = assetHolderMember->value;
                    if (auto assetMember = assetHolderValue.FindMember("m_asset"); assetMember != assetHolderValue.MemberEnd())
                    {
                        // manually load the old asset info data
                        const rapidjson::Value& assetValue = assetMember->value;

                        AZ::Data::AssetId assetId{};
                        if (auto assetIdMember = assetValue.FindMember("assetId"); assetIdMember != assetValue.MemberEnd())
                        {
                            result.Combine(ContinueLoading(&assetId, azrtti_typeid(assetId), assetIdMember->value, context));
                        }

                        AZStd::string path{};
                        if (auto pathMember = assetValue.FindMember("assetHint"); pathMember != assetValue.MemberEnd())
                        {
                            result.Combine(ContinueLoading(&path, azrtti_typeid(path), pathMember->value, context));
                        }

                        sourceHandle = SourceHandle::FromRelativePath(nullptr, assetId.m_guid, path);
                    }
                }

                outputComponent->m_configuration.m_propertyOverrides = AZStd::move(overrides);
                outputComponent->m_configuration.m_sourceHandle = AZStd::move(sourceHandle);
            }
        }

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "EditorScriptCanvasComponentSerializer Load finished loading EditorScriptCanvasComponent"
            : failureMessage.c_str());
    }
}

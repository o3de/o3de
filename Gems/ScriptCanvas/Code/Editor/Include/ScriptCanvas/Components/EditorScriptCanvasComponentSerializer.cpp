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
    AZ_CLASS_ALLOCATOR_IMPL(EditorScriptCanvasComponentSerializer, SystemAllocator, 0);

    JsonSerializationResult::Result EditorScriptCanvasComponentSerializer::Load
        ( void* outputValue
        , const Uuid& outputValueTypeId
        , const rapidjson::Value& inputValue
        , JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(outputValueTypeId == azrtti_typeid<ScriptCanvasEditor::EditorScriptCanvasComponent>()
            , "EditorScriptCanvasComponentSerializer Load against output typeID that was not EditorScriptCanvasComponent");
        AZ_Assert(outputValue, "EditorScriptCanvasComponentSerializer Load against null output");

        // load as parent class
        auto outputComponent = reinterpret_cast<ScriptCanvasEditor::EditorScriptCanvasComponent*>(outputValue);
        JsonSerializationResult::ResultCode result = BaseJsonSerializer::Load(outputValue
            , azrtti_typeid<AzToolsFramework::Components::EditorComponentBase>(), inputValue, context);

        // load child data one by one...
        result.Combine(BaseJsonSerializer::Load(outputValue, outputValueTypeId, inputValue, context));

        if (result.GetProcessing() != JSR::Processing::Halted)
        {
            result.Combine(ContinueLoadingFromJsonObjectField
                ( &outputComponent->m_name
                , azrtti_typeid(outputComponent->m_name)
                , inputValue
                , "m_name"
                , context));

            result.Combine(ContinueLoadingFromJsonObjectField
                ( &outputComponent->m_runtimeDataIsValid
                , azrtti_typeid(outputComponent->m_runtimeDataIsValid)
                , inputValue
                , "runtimeDataIsValid"
                , context));

            result.Combine(ContinueLoadingFromJsonObjectField
                ( &outputComponent->m_variableOverrides
                , azrtti_typeid(outputComponent->m_variableOverrides)
                , inputValue
                , "runtimeDataOverrides"
                , context));

            auto assetHolderMember = inputValue.FindMember("m_assetHolder");
            if (assetHolderMember == inputValue.MemberEnd())
            {
                // file was saved with SourceHandle data
                result.Combine(ContinueLoadingFromJsonObjectField
                    ( &outputComponent->m_sourceHandle
                    , azrtti_typeid(outputComponent->m_sourceHandle)
                    , inputValue
                    , "sourceHandle"
                    , context));
            }
            else
            {
                // manually load the old asset info data
                const rapidjson::Value& assetHolderValue = assetHolderMember->value;
                if (auto assetMember = assetHolderValue.FindMember("m_asset"); assetMember != assetHolderValue.MemberEnd())
                {
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

                    if (result.GetProcessing() != JSR::Processing::Halted)
                    {
                        outputComponent->InitializeSource(ScriptCanvasEditor::SourceHandle(nullptr, assetId.m_guid, path));
                    }
                }
            }
        }

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "EditorScriptCanvasComponentSerializer Load finished loading EditorScriptCanvasComponent"
            : "EditorScriptCanvasComponentSerializer Load failed to load EditorScriptCanvasComponent");
    }
}

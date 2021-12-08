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

        auto outputComponent = reinterpret_cast<ScriptCanvasEditor::EditorScriptCanvasComponent*>(outputValue);
        JsonSerializationResult::ResultCode result = BaseJsonSerializer::Load(outputValue, outputValueTypeId, inputValue, context);

        if (result.GetProcessing() != JSR::Processing::Halted)
        {
            auto assetHolderMember = inputValue.FindMember("m_assetHolder");
            if (assetHolderMember != inputValue.MemberEnd())
            {
                ScriptCanvasEditor::ScriptCanvasAssetHolder assetHolder;
                result.Combine
                    ( ContinueLoading(&assetHolder
                        , azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAssetHolder>(), assetHolderMember->value, context));

                if (result.GetProcessing() != JSR::Processing::Halted)
                {
                    outputComponent->InitializeSource
                        ( ScriptCanvasEditor::SourceHandle(nullptr, assetHolder.GetAssetId().m_guid, assetHolder.GetAssetHint()));
                }
            }
        }

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "EditorScriptCanvasComponentSerializer Load finished loading EditorScriptCanvasComponent"
            : "EditorScriptCanvasComponentSerializer Load failed to load EditorScriptCanvasComponent");
    }
}

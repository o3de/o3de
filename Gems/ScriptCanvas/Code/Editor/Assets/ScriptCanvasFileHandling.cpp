/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/string/string_view.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorUtils.h>
#include <ScriptCanvas/Core/GraphSerialization.h>
#include <ScriptCanvas/Core/SerializationListener.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>

namespace ScriptCanvas
{
    AZStd::string FileLoadResult::ToString() const
    {
        if (m_isSuccess)
        {
            return "Success";
        }
        else
        {
            return AZStd::string::format("Failure@@@ File Read: %s@@@Deserialize: %s@@@Json: %s",
                m_fileReadErrors.c_str(), m_deserializeResult.m_errors.c_str(), m_deserializeResult.m_jsonResults.c_str());
        }
    }

    AZ::Outcome<SourceTree, AZStd::string> LoadEditorAssetTree(SourceHandle handle, SourceTree* parent)
    {
        if (!ScriptCanvasEditor::CompleteDescriptionInPlace(handle))
        {
            return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to describe graph from %s", handle.ToString().c_str()));
        }

        if (!handle.Get())
        {
            auto loadResult = LoadFromFile(handle.AbsolutePath().Native());
            if (!loadResult)
            {
                return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to load graph from %s: %s"
                    , handle.ToString().c_str(), loadResult.ToString().c_str()));
            }

            auto absolutePath = handle.AbsolutePath();
            handle = SourceHandle::FromRelativePath(loadResult.m_handle.Data(), handle.Id(), handle.RelativePath().c_str());
            handle = SourceHandle::MarkAbsolutePath(handle, absolutePath);
        }

        AZStd::vector<SourceHandle> dependentAssets;
        const auto subgraphInterfaceAssetTypeID = azrtti_typeid<AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>>();

        auto onBeginElement = [&subgraphInterfaceAssetTypeID, &dependentAssets]
            ( void* instance
            , const AZ::SerializeContext::ClassData* classData
            , const AZ::SerializeContext::ClassElement* classElement) -> bool
        {
            if (classElement)
            {
                // if we are a pointer, then we may be pointing to a derived type.
                if (classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
                {
                    // if ptr is a pointer-to-pointer, cast its value to a void* (or const void*) and dereference to get to the actual object pointer.
                    instance = *static_cast<void**>(instance);
                }
            }

            const auto azTypeId = classData->m_azRtti->GetTypeId();
            if (azTypeId == subgraphInterfaceAssetTypeID)
            {
                auto id = reinterpret_cast<AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>*>(instance)->GetId();
                dependentAssets.push_back(SourceHandle(nullptr, id.m_guid));
            }

            return true;
        };

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "LoadEditorAssetTree() ailed to retrieve serialize context!");

        const ScriptCanvasEditor::EditorGraph* graph = handle.Get();
        serializeContext->EnumerateObject(graph, onBeginElement, nullptr, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

        SourceTree result;

        for (auto& dependentAsset : dependentAssets)
        {
            auto loadDependentOutcome = LoadEditorAssetTree(dependentAsset, &result);
            if (!loadDependentOutcome.IsSuccess())
            {
                return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to load graph from %s: %s"
                    , dependentAsset.ToString().c_str(), loadDependentOutcome.GetError().c_str()));
            }

            result.m_dependencies.push_back(loadDependentOutcome.TakeValue());
        }

        if (parent)
        {
            result.SetParent(*parent);
        }

        result.m_source = AZStd::move(handle);
        return AZ::Success(result);
    }

    FileLoadResult LoadFromFile
        ( AZStd::string_view path
        , MakeInternalGraphEntitiesUnique makeEntityIdsUnique
        , LoadReferencedAssets loadReferencedAssets)
    {
        namespace JSRU = AZ::JsonSerializationUtils;

        FileLoadResult result;

        auto fileStringOutcome = AZ::Utils::ReadFile<AZStd::string>(path);
        if (!fileStringOutcome)
        {
            result.m_isSuccess = false;
            result.m_fileReadErrors = fileStringOutcome.TakeError();
            return result;
        }

        const auto& asString = fileStringOutcome.GetValue();
        result.m_deserializeResult = Deserialize(asString, makeEntityIdsUnique, loadReferencedAssets);
        result.m_isSuccess = result.m_deserializeResult;

        result.m_handle = SourceHandle::FromRelativePath(result.m_deserializeResult.m_graphDataPtr, path);
        return result;
    }

}

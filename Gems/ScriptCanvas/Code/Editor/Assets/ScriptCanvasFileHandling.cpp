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
#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorUtils.h>
#include <ScriptCanvas/Core/GraphSerialization.h>
#include <ScriptCanvas/Core/SerializationListener.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>

namespace ScriptCanvasFileHandlingCpp
{
    class SourceTreeLoader
    {
    public:
        AZ_CLASS_ALLOCATOR(SourceTreeLoader, AZ::SystemAllocator, 0);

        ScriptCanvas::SourceHandle m_source;
        AZStd::vector<SourceTreeLoader> m_dependencies;

        void AssignParents(SourceTreeLoader& parent)
        {
            m_parent = &parent;

            for (auto& dep : m_dependencies)
            {
                dep.AssignParents(*this);
            }
        }

        ScriptCanvas::SourceTree ConvertToSourceTree() const
        {
            ScriptCanvas::SourceTree sourceTree;
            sourceTree.m_source = m_source;

            for (auto& dep : m_dependencies)
            {
                sourceTree.m_dependencies.push_back(dep.ConvertToSourceTree());
            }

            return sourceTree;
        }

        AZStd::optional<SourceTreeLoader> FindDependency(ScriptCanvas::SourceHandle dependency)
        {
            if (m_source.AnyEquals(dependency) && m_source.Get())
            {
                return *this;
            }

            for (auto& dep : m_dependencies)
            {
                if (auto depOptional = dep.FindDependency(dependency))
                {
                    return *depOptional;
                }
            }

            return AZStd::nullopt;
        }

        bool IsLoading(ScriptCanvas::SourceHandle dependency, AZStd::string& path)
        {
            path += m_source.RelativePath().c_str();
            
            if (m_source.AnyEquals(dependency))
            {
                return true;
            }
            else if (m_parent)
            {
                path += " =>\n";
                return m_parent->IsLoading(dependency, path);
            }
            else
            {
                return false;
            }
        }

        SourceTreeLoader* ModRoot()
        {
            if (!m_parent)
            {
                return this;
            }

            return m_parent->ModRoot();
        }

    private:
        SourceTreeLoader* m_parent = nullptr;
    };

    AZ::Outcome<void, AZStd::string> LoadEditorAssetTree(SourceTreeLoader& result)
    {
        using namespace ScriptCanvas;

        if (!ScriptCanvasEditor::CompleteDescriptionInPlace(result.m_source))
        {
            return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to describe graph from %s", result.m_source.ToString().c_str()));
        }

        if (!result.m_source.Get())
        {
            auto loadResult = LoadFromFile(result.m_source.AbsolutePath().Native());
            if (!loadResult)
            {
                return AZ::Failure
                    ( AZStd::string::format("LoadEditorAssetTree failed to load graph from %s: %s"
                    , result.m_source.ToString().c_str()
                    , loadResult.ToString().c_str()));
            }

            auto absolutePath = result.m_source.AbsolutePath();
            result.m_source = SourceHandle::FromRelativePath(loadResult.m_handle.Data(), result.m_source.Id(), result.m_source.RelativePath().c_str());
            result.m_source = SourceHandle::MarkAbsolutePath(result.m_source, absolutePath);
        }

        const auto subgraphInterfaceAssetTypeID = azrtti_typeid<AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>>();

        AZStd::vector<SourceTreeLoader> possibleDependencies;

        auto onBeginElement = [&subgraphInterfaceAssetTypeID, &possibleDependencies]
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
                SourceTreeLoader dependencyLoader;
                dependencyLoader.m_source = ScriptCanvas::SourceHandle(nullptr, id.m_guid);
                possibleDependencies.push_back(dependencyLoader);
            }

            return true;
        };

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "LoadEditorAssetTree() ailed to retrieve serialize context!");

        const ScriptCanvasEditor::EditorGraph* graph = result.m_source.Get();
        serializeContext->EnumerateObject(graph, onBeginElement, nullptr, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

        for (auto& possibleDependency : possibleDependencies)
        {
            // do not count locally defined functions as dependencies
            if (!result.m_source.AnyEquals(possibleDependency.m_source))
            {
                // now that circular dependencies can be guarded against, load each dependency from disk only once
                {
                    SourceTreeLoader dependencyLoader;
                    dependencyLoader.m_source = possibleDependency.m_source;
                    result.m_dependencies.push_back(dependencyLoader);
                    result.m_dependencies.back().AssignParents(result);
                }

                SourceTreeLoader& dependentAsset = result.m_dependencies.back();
                AZStd::string path;
                if (result.IsLoading(dependentAsset.m_source, path))
                {   // check for circular dependencies...
                    return AZ::Failure(AZStd::string::format("LoadEditorAsset tree failed to load. Circular dependency detected: %s", path.c_str()));
                }
                else if (auto sourceTreeOptional = result.ModRoot()->FindDependency(dependentAsset.m_source))
                {   // check at the level root if the dependency is loaded already...
                    dependentAsset.m_source = sourceTreeOptional->m_source;
                }
                else
                {   // ...and if not, load and add the value to the in process-loads at this step...
                    auto loadDependentOutcome = ScriptCanvasFileHandlingCpp::LoadEditorAssetTree(dependentAsset);
                    if (!loadDependentOutcome.IsSuccess())
                    {
                        return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to load graph from %s: %s"
                            , dependentAsset.m_source.ToString().c_str(), loadDependentOutcome.GetError().c_str()));
                    }
                }
            }
        }

        return AZ::Success();
    }

}

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

    AZ::Outcome<SourceTree, AZStd::string> LoadEditorAssetTree(SourceHandle sourceHandle)
    {
        using namespace ScriptCanvasFileHandlingCpp;

        SourceTreeLoader result;
        result.m_source = sourceHandle;

        auto loadOutome = LoadEditorAssetTree(result);

        if (loadOutome.IsSuccess())
        {
            return AZ::Success(result.ConvertToSourceTree());
        }
        else
        {
            return AZ::Failure(loadOutome.TakeError());
        }
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
        if (!result.m_deserializeResult.m_isSuccessful)
        {
            result.m_fileReadErrors = "Script Canvas Graph Deserialization Failed - " + result.m_deserializeResult.m_errors + "\n";
        }

        result.m_handle = SourceHandle::FromRelativePath(result.m_deserializeResult.m_graphDataPtr, path);
        return result;
    }

}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>

#include "Attributes.h"
#include "Core.h"
#include <Core/Graph.h>

namespace ScriptCanvas
{
    AZ_CVAR(bool, g_saveRuntimeAssetsAsPlainTextForDebug, false, {}, AZ::ConsoleFunctorFlags::Null
        , "Save runtime assets as plain text rather than binary for debug purposes.");

    AZ_CVAR(bool, g_saveEditorAssetsAsPlainTextForDebug, false, {}, AZ::ConsoleFunctorFlags::Null
        , "Save editor assets as plain text rather than binary for debug purposes.");

    ScopedAuxiliaryEntityHandler::ScopedAuxiliaryEntityHandler(AZ::Entity* buildEntity)
        : m_buildEntity(buildEntity)
        , m_wasAdded(false)
    {
        if (AZ::Interface<AZ::ComponentApplicationRequests>::Get() != nullptr)
        {
            AZ::Interface<AZ::ComponentApplicationRequests>::Get()->RemoveEntity(buildEntity);
        }

        if (buildEntity->GetState() == AZ::Entity::State::Constructed)
        {
            buildEntity->Init();
            m_wasAdded = true;
        }
    }

    ScopedAuxiliaryEntityHandler::~ScopedAuxiliaryEntityHandler()
    {
        if (!m_wasAdded)
        {
            if (AZ::Interface<AZ::ComponentApplicationRequests>::Get() != nullptr)
            {
                AZ::Interface<AZ::ComponentApplicationRequests>::Get()->AddEntity(m_buildEntity);
            }
        }
    }

    bool IsNamespacePathEqual(const NamespacePath& lhs, const NamespacePath& rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }
        auto lhsIter = lhs.begin();
        auto rhsIter = rhs.begin();

        for (; lhsIter != lhs.end(); ++lhsIter, ++rhsIter)
        {
            if (!AZ::StringFunc::Equal(lhsIter->c_str(), rhsIter->c_str()))
            {
                return false;
            }
        }

        return true;
    }

    void DependencyReport::MergeWith(const DependencyReport& other)
    {
        auto unite = [](auto& target, const auto& source)
        {
            for (const auto& entry : source)
            {
                target.insert(entry);
            }
        };

        unite(nativeLibraries, other.nativeLibraries);
        unite(userSubgraphs, other.userSubgraphs);
        unite(scriptEventsAssetIds, other.scriptEventsAssetIds);
        unite(userSubgraphAssetIds, other.userSubgraphAssetIds);
    }

    DependencyReport DependencyReport::NativeLibrary(const AZStd::string& library)
    {
        DependencyReport report;
        report.nativeLibraries.insert({ library });
        return report;
    }

    bool SlotIdVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        //! Version 1: Slot Ids contained a Crc32 hash of the name given
        //! Version 2+: Slot Ids now contain a random Uuid
        //! The converter below reads in the old GraphCanvas node <-> ScriptCanvas node map and then iterates over all the 
        //! GraphCanvas nodes adding a reference to the ScriptCanvas node in it's user data field
        if (classElement.GetVersion() <= 1)
        {
            if (!classElement.RemoveElementByName(AZ_CRC_CE("m_id")))
            {
                return false;
            }

            if (classElement.RemoveElementByName(AZ_CRC_CE("m_name")))
            {
                return false;
            }

            classElement.AddElementWithData(context, "m_id", AZ::Uuid::CreateRandom());
        }

        return true;
    }

    void SlotId::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SlotId>()
                ->Version(2, &SlotIdVersionConverter)
                ->Field("m_id", &SlotId::m_id)
                ;
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SlotId>("SlotId")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All);
        }
    }

    void VersionData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VersionData>()
                ->Version(2, [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
                    {
                        if (classElement.GetVersion() < 2)
                        {
                            FileVersion fileVersion = ScriptCanvas::FileVersion::Initial;
                            classElement.AddElementWithData(context, "_fileVersion", fileVersion);
                        }

                        return true;
                    })
                ->Field("_grammarVersion", &VersionData::grammarVersion)
                ->Field("_runtimeVersion", &VersionData::runtimeVersion)
                ->Field("_fileVersion", &VersionData::fileVersion)
                ;
        }
    }

    VersionData VersionData::Latest()
    {
        VersionData data;
        data.MarkLatest();
        return data;
    }

    void VersionData::MarkLatest()
    {
        grammarVersion = GrammarVersion::Current;
        runtimeVersion = RuntimeVersion::Current;
        fileVersion = FileVersion::Current;
    }

    SourceHandle::SourceHandle()
        : m_id(AZ::Uuid::CreateNull())
    {}

    SourceHandle::SourceHandle(const SourceHandle& data, const AZ::Uuid& id)
        : m_data(data.m_data)
        , m_id(id)
    {
        SanitizePath();
    }

    SourceHandle::SourceHandle(ScriptCanvas::DataPtr graph, const AZ::Uuid& id)
        : m_data(graph)
        , m_id(id)
    {
        SanitizePath();
    }

    SourceHandle::SourceHandle(const SourceHandle& source)
        : m_data(source.m_data)
        , m_id(source.Id())
        , m_relativePath(source.m_relativePath)
        , m_absolutePath(source.m_absolutePath)
    {
        SanitizePath();
    }

    SourceHandle::SourceHandle(const SourceHandle& data, const AZ::Uuid& id, const AZ::IO::Path& path)
        : m_data(data.m_data)
        , m_id(id)
        , m_relativePath(path)
    {
        SanitizePath();
    }

    SourceHandle::SourceHandle(ScriptCanvas::DataPtr graph, const AZ::Uuid& id, const AZ::IO::Path& path)
        : m_data(graph)
        , m_id(id)
        , m_relativePath(path)
    {
        SanitizePath();
    }

    SourceHandle::SourceHandle(const SourceHandle& data, const AZ::IO::Path& path)
        : m_data(data.m_data)
        , m_id(AZ::Uuid::CreateNull())
        , m_relativePath(path)
    {
        SanitizePath();
    }

    SourceHandle::SourceHandle(ScriptCanvas::DataPtr graph, const AZ::IO::Path& path)
        : m_data(graph)
        , m_id(AZ::Uuid::CreateNull())
        , m_relativePath(path)
    {
        SanitizePath();
    }

    const AZ::IO::Path& SourceHandle::AbsolutePath() const
    {
        return m_absolutePath;
    }

    bool SourceHandle::AnyEquals(const SourceHandle& other) const
    {
        return m_data && m_data == other.m_data
            || !m_id.IsNull() && m_id == other.m_id
            || !m_relativePath.empty() && m_relativePath == other.m_relativePath
            || !m_absolutePath.empty() && m_absolutePath == other.m_absolutePath;
    }

    void SourceHandle::Clear()
    {
        m_data = nullptr;
        m_id = AZ::Uuid::CreateNull();
        m_relativePath.clear();
        m_absolutePath.clear();
    }

    DataPtr SourceHandle::Data() const
    {
        return m_data;
    }

    // return a SourceHandle with only the Id and Path, but without a pointer to the data
    SourceHandle SourceHandle::Describe() const
    {
        return MarkAbsolutePath(SourceHandle(nullptr, m_id, m_relativePath), m_absolutePath);
    }

    SourceHandle SourceHandle::FromRelativePath(const SourceHandle& data, const AZ::Uuid& id, const AZ::IO::Path& path)
    {
        return SourceHandle(data, id, path);
    }

    SourceHandle SourceHandle::FromRelativePath(ScriptCanvas::DataPtr graph, const AZ::Uuid& id, const AZ::IO::Path& path)
    {
        return SourceHandle(graph, id, path);
    }

    SourceHandle SourceHandle::FromRelativePath(const SourceHandle& data, const AZ::IO::Path& path)
    {
        return SourceHandle(data, path);
    }

    SourceHandle SourceHandle::FromRelativePath(ScriptCanvas::DataPtr graph, const AZ::IO::Path& path)
    {
        return SourceHandle(graph, path);
    }

    SourceHandle SourceHandle::FromRelativePathAndScanFolder
        ( AZStd::string_view relativePath
        , AZStd::string_view scanFolder
        , const AZ::Uuid& sourceId)
    {
        auto handle = SourceHandle::FromRelativePath(nullptr, sourceId, relativePath);
        AZ::IO::Path path(scanFolder);
        path /= relativePath;
        handle = SourceHandle::MarkAbsolutePath(handle, path.MakePreferred());
        return handle;
    }

    ScriptCanvasEditor::GraphPtrConst SourceHandle::Get() const
    {
        return m_data ? m_data->GetEditorGraph() : nullptr;
    }

    const AZ::Uuid& SourceHandle::Id() const
    {
        return m_id;
    }

    bool SourceHandle::IsDescriptionValid() const
    {
        return !m_id.IsNull() && (!m_relativePath.empty());
    }

    bool SourceHandle::IsGraphValid() const
    {
        return m_data != nullptr;
    }

    SourceHandle SourceHandle::MarkAbsolutePath(const SourceHandle& data, const AZ::IO::Path& path)
    {
        SourceHandle result(data);
        result.m_absolutePath = path;
        result.m_absolutePath.MakePreferred();
        return result;
    }

    ScriptCanvasEditor::GraphPtr SourceHandle::Mod() const
    {
        return m_data ? m_data->ModEditorGraph() : nullptr;
    }

    AZStd::string SourceHandle::Name() const
    {
        return AZStd::string::format("%.*s", AZ_STRING_ARG(m_relativePath.Filename().Native()));
    }

    bool SourceHandle::operator==(const SourceHandle& other) const
    {
        return m_data.get() == other.m_data.get()
            && m_id == other.m_id
            && m_relativePath == other.m_relativePath
            && m_absolutePath == other.m_absolutePath;
    }

    bool SourceHandle::operator!=(const SourceHandle& other) const
    {
        return !(*this == other);
    }

    const AZ::IO::Path& SourceHandle::RelativePath() const
    {
        return m_relativePath;
    }

    void SourceHandle::SanitizePath()
    {
        m_relativePath.MakePreferred();
    }

    bool SourceHandle::PathEquals(const SourceHandle& other) const
    {
        return m_relativePath == other.m_relativePath;
    }

    void SourceHandle::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SourceHandle>()
                ->Version(1)
                ->Field("id", &SourceHandle::m_id)
                ->Field("path", &SourceHandle::m_relativePath)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SourceHandle>("Source Handle", "Script Canvas Source File")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/ScriptCanvas.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/ScriptCanvas/Viewport/ScriptCanvas.svg")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "Script Canvas")
                    ->Attribute(AZ::Edit::Attributes::SourceAssetFilterPattern, "*.scriptcanvas")
                    ;
            }
        }
        else if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SourceHandle>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "scriptcanvas")
                ->Attribute(AZ::Script::Attributes::Module, "scriptcanvas")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ;

            behaviorContext->Method("SourceHandleFromPath", [](AZStd::string_view pathStringView)->SourceHandle {  return FromRelativePath(DataPtr{}, AZ::IO::Path(pathStringView)); })
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "scriptcanvas")
                ->Attribute(AZ::Script::Attributes::Module, "scriptcanvas")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ;
        }
    }

    AZStd::string SourceHandle::ToString() const
    {
        return AZStd::string::format
            ( "ID: %s, Name: %s"
            , m_id.IsNull() ? "<null id>" : m_id.ToString<AZStd::string>().c_str()
            , !m_relativePath.empty() ? m_relativePath.c_str() : "<no name>"
            );
    }

    const Graph* ScriptCanvasData::GetGraph() const
    {
        return AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(m_scriptCanvasEntity.get());
    }

    const ScriptCanvasEditor::EditorGraph* ScriptCanvasData::GetEditorGraph() const
    {
        return reinterpret_cast<const ScriptCanvasEditor::EditorGraph*>(GetGraph());
    }

    Graph* ScriptCanvasData::ModGraph()
    {
        return AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(m_scriptCanvasEntity.get());
    }

    ScriptCanvasEditor::EditorGraph* ScriptCanvasData::ModEditorGraph()
    {
        return reinterpret_cast<ScriptCanvasEditor::EditorGraph*>(ModGraph());
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/NamedEntityId.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/any.h>
#include <AzCore/std/hash.h>
#include <Core/NamedId.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

#define OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED

namespace AZ
{
    class Entity;
    class ReflectContext;

    template<typename t_Attribute, typename t_Container>
    bool ReadAttribute(t_Attribute& resultOut, AttributeId id, const t_Container& attributes)
    {
        AZ::Attribute* attribute = FindAttribute(id, attributes);
        return attribute && AZ::AttributeReader(nullptr, attribute).Read<t_Attribute>(resultOut);
    }
}

namespace ScriptCanvas
{
    // A place holder identifier for the AZ::Entity that owns the graph.
    // The actual value in each location initialized to GraphOwnerId is populated with the owning entity at editor-time, Asset Processor-time, or runtime, as soon as the owning entity is known.
    using GraphOwnerIdType = AZ::EntityId;
    static const GraphOwnerIdType GraphOwnerId = AZ::EntityId(0xacedc0de);

    // A place holder identifier for unique runtime graph on Entity that is running more than one instance of the same graph.
    // This allows multiple instances of the same graph to be addressed individually on the same entity.
    // The actual value in each location initialized to UniqueId is populated at run-time.
    using RuntimeIdType = AZ::EntityId;
    static const RuntimeIdType UniqueId = AZ::EntityId(0xfee1baad);

    constexpr const char* k_EventOutPrefix = "ExecutionSlot:";

    constexpr const char* k_OnVariableWriteEventName = "OnVariableValueChanged";
    constexpr const char* k_OnVariableWriteEbusName = "VariableNotification";

    constexpr const AZStd::string_view k_VersionExplorerWindow = "VersionExplorerWindow";

    class Node;
    class Edge;
    class Graph;

    using GraphPtr = Graph*;
    using GraphPtrConst = const Graph*;

    using ID = AZ::EntityId;

    using NamespacePath = AZStd::vector<AZStd::string>;
    bool IsNamespacePathEqual(const NamespacePath& lhs, const NamespacePath& rhs);

    using NodeIdList = AZStd::vector<ID>;
    using NodePtrList = AZStd::vector<Node*>;
    using NodePtrConstList = AZStd::vector<const Node*>;

    enum class PropertyStatus : AZ::u8
    {
        Getter,
        None,
        Setter,
    };

    enum class GrammarVersion : int
    {
        Initial = -1,
        BackendSplit = 0,

        // add new entries above
        Current,
    };

    enum class RuntimeVersion : int
    {
        Initial = -1,
        DirectTraversal = 0,

        // add new entries above
        Current,
    };

    enum class FileVersion : int
    {
        Initial = -1,
        JSON = 0,

        // add new entries above
        Current,
    };

    struct VersionData
    {
        AZ_TYPE_INFO(VersionData, "{52036892-DA63-4199-AC6A-9BAFE6B74EFC}");

        static void Reflect(AZ::ReflectContext* context);

        static VersionData Latest();

        GrammarVersion grammarVersion = GrammarVersion::Initial;
        RuntimeVersion runtimeVersion = RuntimeVersion::Initial;
        FileVersion fileVersion = FileVersion::Initial;

        bool operator == (const VersionData& rhs) const
        {
            return grammarVersion == rhs.grammarVersion
                && runtimeVersion == rhs.runtimeVersion
                && fileVersion == rhs.fileVersion;
        }

        bool IsLatest() const
        {
            return (*this) == Latest();
        }

        void MarkLatest();
    };

    enum class EventType
    {
        Broadcast,
        BroadcastQueue,
        Event,
        EventQueue,
        Count,
    };

    enum class ExecutionMode : AZ::u8
    {
        Interpreted,
        Native,
        COUNT
    };

    struct SlotId
    {
        AZ_TYPE_INFO(SlotId, "{14C629F6-467B-46FE-8B63-48FDFCA42175}");

        AZ::Uuid m_id{ AZ::Uuid::CreateNull() };

        SlotId() = default;

        SlotId(const SlotId&) = default;

        explicit SlotId(const AZ::Uuid& uniqueId)
            : m_id(uniqueId)
        {}

        //! AZ::Uuid has a constructor not marked as explicit that accepts a const char*
        //! Adding a constructor which accepts a const char* and deleting it prevents
        //! AZ::Uuid from being initialized with c-strings
        explicit SlotId(const char* str) = delete;

        static void Reflect(AZ::ReflectContext* context);

        bool IsValid() const
        {
            return m_id != AZ::Uuid::CreateNull();
        }

        AZStd::string ToString() const
        {
            return m_id.ToString<AZStd::string>();
        }

        bool operator==(const SlotId& rhs) const
        {
            return m_id == rhs.m_id;
        }

        bool operator!=(const SlotId& rhs) const
        {
            return m_id != rhs.m_id;
        }

    };

    struct GraphIdentifier final
    {
        AZ_CLASS_ALLOCATOR(GraphIdentifier, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(GraphIdentifier, "{0DAFC7EF-D23A-4353-8DA5-7D0CC186D8E3}");

        AZ::ComponentId m_componentId = 0;
        AZ::Data::AssetId m_assetId;

        GraphIdentifier() = default;

        GraphIdentifier(const AZ::Data::AssetId assetId, const AZ::ComponentId& componentId)
            : m_componentId(componentId)
            , m_assetId(assetId)
        {}

        bool operator==(const GraphIdentifier& other) const;

        AZStd::string ToString() const;
    };

    using PropertyFields = AZStd::vector<AZStd::pair<AZStd::string_view, SlotId>>;

    using NamedActiveEntityId = AZ::NamedEntityId;
    using NamedNodeId = NamedId<AZ::EntityId>;
    using NamedSlotId = NamedId<SlotId>;

    using NodeTypeIdentifier = AZStd::size_t;
    using EBusEventId = AZ::Crc32;
    using EBusBusId = AZ::Crc32;
    using ScriptCanvasId = AZ::EntityId;
    enum class AzEventIdentifier : size_t {};

    struct RuntimeVariable
    {
        AZ_TYPE_INFO(RuntimeVariable, "{6E969359-5AF5-4ECA-BE89-A96AB30A624E}");
        AZ_CLASS_ALLOCATOR(RuntimeVariable, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        AZStd::any value;

        RuntimeVariable() = default;
        RuntimeVariable(const RuntimeVariable&) = default;
        RuntimeVariable(RuntimeVariable&&) = default;
        explicit RuntimeVariable(const AZStd::any& source);
        explicit RuntimeVariable(AZStd::any&& source);

        RuntimeVariable& operator=(const RuntimeVariable&) = default;
        RuntimeVariable& operator=(RuntimeVariable&&) = default;
    };

    struct NamespacePathHasher
    {
        AZ_FORCE_INLINE size_t operator()(const NamespacePath& path) const
        {
            return AZStd::hash_range(path.begin(), path.end());
        }
    };

    using DependencySet = AZStd::unordered_set<NamespacePath, NamespacePathHasher>;

    struct DependencyReport
    {
        static DependencyReport NativeLibrary(const AZStd::string& library);

        DependencySet nativeLibraries;
        AZStd::unordered_set<AZ::Data::AssetId> scriptEventsAssetIds;
        DependencySet userSubgraphs;
        AZStd::unordered_set<AZ::Data::AssetId> userSubgraphAssetIds;

        void MergeWith(const DependencyReport& other);
    };

    struct OrderedDependencies
    {
        DependencyReport source;
        AZStd::vector<AZ::Data::AssetId> orderedAssetIds;
    };

    //! Globally accessible Script Canvas settings, we use these to pass user provided settings
    //! into the Script Canvas code
    class ScriptCanvasSettingsRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual bool CanShowNetworkSettings() { return false; }

    };

    using ScriptCanvasSettingsRequestBus = AZ::EBus<ScriptCanvasSettingsRequests>;

    class ScopedAuxiliaryEntityHandler
    {
    public:
        ScopedAuxiliaryEntityHandler(AZ::Entity* buildEntity);
        ~ScopedAuxiliaryEntityHandler();

    private:
        bool m_wasAdded = false;
        AZ::Entity* m_buildEntity = nullptr;
    };

    void ReflectEventTypeOnDemand(const AZ::TypeId& typeId, AZStd::string_view name, AZ::IRttiHelper* rttiHelper = nullptr);
}

namespace ScriptCanvas
{
    class ScriptCanvasData;

    using DataPtr = AZStd::intrusive_ptr<ScriptCanvasData>;
    using DataPtrConst = AZStd::intrusive_ptr<const ScriptCanvasData>;
}

namespace ScriptCanvasEditor
{
    class Graph;
    
    using GraphPtr = Graph*;
    using GraphPtrConst = const Graph*;

    class SourceHandle
    {
    public:
        AZ_TYPE_INFO(SourceHandle, "{65855A98-AE2F-427F-BFC8-69D45265E312}");
        AZ_CLASS_ALLOCATOR(SourceHandle, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        SourceHandle();

        SourceHandle(const SourceHandle& data, const AZ::Uuid& id, const AZ::IO::Path& path);

        SourceHandle(ScriptCanvas::DataPtr graph, const AZ::Uuid& id, const AZ::IO::Path& path);

        SourceHandle(const SourceHandle& data, const AZ::IO::Path& path);

        SourceHandle(ScriptCanvas::DataPtr graph, const AZ::IO::Path& path);

        bool AnyEquals(const SourceHandle& other) const;

        void Clear();

        // return a SourceHandle with only the Id and Path, but without a pointer to the data
        SourceHandle Describe() const;

        GraphPtrConst Get() const;

        const AZ::Uuid& Id() const;

        bool IsDescriptionValid() const;

        bool IsGraphValid() const;

        GraphPtr Mod() const;

        bool operator==(const SourceHandle& other) const;

        bool operator!=(const SourceHandle& other) const;

        const AZ::IO::Path& Path() const;

        bool PathEquals(const SourceHandle& other) const;

        AZStd::string ToString() const;

    private:
        ScriptCanvas::DataPtr m_data;
        AZ::Uuid m_id = AZ::Uuid::CreateNull();
        AZ::IO::Path m_path;
    };
}

namespace ScriptCanvas
{
    class ScriptCanvasData
        : public AZStd::intrusive_refcount<AZStd::atomic_uint, AZStd::intrusive_default_delete>
    {
    public:

        AZ_RTTI(ScriptCanvasData, "{1072E894-0C67-4091-8B64-F7DB324AD13C}");
        AZ_CLASS_ALLOCATOR(ScriptCanvasData, AZ::SystemAllocator, 0);
        ScriptCanvasData() = default;
        virtual ~ScriptCanvasData() = default;
        ScriptCanvasData(ScriptCanvasData&& other);
        ScriptCanvasData& operator=(ScriptCanvasData&& other);

        static void Reflect(AZ::ReflectContext* reflectContext);

        AZ::Entity* GetScriptCanvasEntity() const { return m_scriptCanvasEntity.get(); }

        const Graph* GetGraph() const;

        const ScriptCanvasEditor::Graph* GetEditorGraph() const;

        Graph* ModGraph();

        ScriptCanvasEditor::Graph* ModEditorGraph();

        AZStd::unique_ptr<AZ::Entity> m_scriptCanvasEntity;
    private:
        ScriptCanvasData(const ScriptCanvasData&) = delete;
    };
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::SlotId>
    {
        using argument_type = ScriptCanvas::SlotId;
        using result_type = AZStd::size_t;

        inline size_t operator()(const argument_type& ref) const
        {
            return AZStd::hash<AZ::Uuid>()(ref.m_id);
        }
    };

    template<>
    struct hash<ScriptCanvasEditor::SourceHandle>
    {
        using argument_type = ScriptCanvasEditor::SourceHandle;
        using result_type = AZStd::size_t;

        inline size_t operator()(const argument_type& handle) const
        {
            size_t h = 0;
            hash_combine(h, handle.Id());
            hash_combine(h, handle.Path());
            hash_combine(h, handle.Get());
            return h;
        }
    };
}

#define SCRIPT_CANVAS_INFINITE_LOOP_DETECTION_COUNT (2000000)

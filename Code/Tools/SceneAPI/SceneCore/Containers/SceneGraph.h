#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <stdint.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>
#include <SceneAPI/SceneCore/Containers/Views/View.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IGraphObject;
        }
        namespace SceneBuilder
        {
            struct QueueNode;
            struct ImportContext;
        }

        namespace Containers
        {
            // The SceneGraph allows for hierarchical storage of arbitrary data in a tree like fashion.
            //      The internal storage is based on child-sibling representation: https://en.wikipedia.org/wiki/Left-child_right-sibling_binary_tree
            //
            //      The SceneGraph uses a naming convention where the name of a node is concatenated with it's parent and separate by a dot ('.')
            //      such that child node B of parent node A will have the name "A.B". Note that by default the scene graph has a nameless root node.
            //
            //      There are 2 approaches to manipulating the SceneGraph. The first is direct manipulation by using NodeIndex. This supports
            //      navigating the graph, manipulating the graph hierarchy and manipulating the stored values. This approach is the most
            //      flexible and allows for the most control, but requires a lot of code support. This approach is best use while constructing
            //      the graph for a scene.
            //      The second option is by combining views. This allows for navigating the graph and manipulating stored values, but not
            //      for manipulating the graph hierarchy. Views use iterators and can therefore be used in most STL(-like) algorithms as well
            //      as ranged based loops, but have restrictions in what they can do. This approach is best used while inspecting or exporting
            //      the graph for a scene.
            class SceneGraph
            {
            public:
                // Index for a node.
                //      Instead of using just a plain int, this is it's own type to reduce the
                //      risk of invalid indices being passed.
                class NodeIndex
                {
                    friend class SceneGraph;
                    friend struct SceneBuilder::QueueNode;
                    friend struct SceneBuilder::ImportContext;
                public:
                    // Type needs to be able to store an index in a NodeHeader (currently 21-bits).
                    using IndexType = uint32_t;

                    NodeIndex() = default;
                    NodeIndex(const NodeIndex& rhs) = default;
                    NodeIndex& operator=(const NodeIndex& rhs) = default;

                    // Returns whether or not the node index is valid.
                    //      Note that this function reports explicit invalid nodes, such as the invalid node that's returned when a name can't be found, and
                    //      whether it's valid before any changes are made. If changes to the SceneGraph are made it will not be able to detect that a
                    //      previously valid index has become invalid.
                    inline bool IsValid() const;

                    inline bool operator==(NodeIndex rhs) const;
                    inline bool operator!=(NodeIndex rhs) const;

                    inline IndexType AsNumber() const;
                    inline s32 Distance(NodeIndex rhs) const;

                private:
                    static const IndexType INVALID_INDEX = static_cast<IndexType>(-1);

                    inline explicit NodeIndex(IndexType value);

                    IndexType m_value = NodeIndex::INVALID_INDEX;
                };

                // NodeHeader contains the relationship a node has with its surrounding nodes and additional information about a node.
                //      Note that this is always passed by value, so direct access to the member variables doesn't risk unwanted changes.
                struct NodeHeader
                {
                    // Number of bits used for storing an index into the stored data. Currently 21 bits, which will support about 2 million nodes.
                    static const uint32_t INDEX_BIT_COUNT = 21;
                    static const uint64_t INVALID_INDEX = (1 << INDEX_BIT_COUNT) - 1; // Largest possible value for the index bit count.

                    // End point nodes are nodes that are not allowed to have children.
                    uint64_t m_isEndPoint : 1;
                    uint64_t m_parentIndex: INDEX_BIT_COUNT;
                    uint64_t m_siblingIndex: INDEX_BIT_COUNT;
                    uint64_t m_childIndex: INDEX_BIT_COUNT;

                    inline bool HasParent() const;
                    inline bool HasSibling() const;
                    inline bool HasChild() const;
                    inline bool IsEndPoint() const;

                    inline NodeIndex GetParentIndex() const;
                    inline NodeIndex GetSiblingIndex() const;
                    inline NodeIndex GetChildIndex() const;

                    inline NodeHeader();

                    NodeHeader(const NodeHeader& rhs) = default;
                    NodeHeader& operator=(const NodeHeader& rhs) = default;
                };

                class Name
                {
                    friend class SceneGraph;
                public:
                    inline Name();
                    Name(const Name& rhs) = default;
                    inline Name(Name&& rhs);
                    inline Name(AZStd::string&& pathName, size_t nameOffset);

                    Name& operator=(const Name& rhs) = default;
                    inline Name& operator=(Name&& rhs);

                    inline bool operator==(const Name& rhs) const;
                    inline bool operator!=(const Name& rhs) const;

                    // Returns the full unique path for the SceneGraph node.
                    inline const char* GetPath() const;
                    // Returns the name for the SceneGraph Node.
                    inline const char* GetName() const;

                    inline size_t GetPathLength() const;
                    inline size_t GetNameLength() const;

                private:
                    AZStd::string m_path;
                    size_t m_nameOffset;
                };

                inline static AZStd::shared_ptr<const DataTypes::IGraphObject> ConstDataConverter(const AZStd::shared_ptr<DataTypes::IGraphObject>& value);

                using StringHasher = AZStd::hash<AZStd::string>;
                using StringHash = size_t;
                using NameLookup = AZStd::unordered_multimap<StringHash, uint32_t>;

                using HierarchyStorageType = NodeHeader;
                using HierarchyStorage = AZStd::vector<HierarchyStorageType>;
                using HierarchyStorageConstIterator = HierarchyStorage::const_iterator;
                using HierarchyStorageConstData = Views::View<HierarchyStorageConstIterator>;

                using NameStorageType = Name;
                using NameStorage = AZStd::vector<NameStorageType>;
                using NameStorageConstData = Views::View<NameStorage::const_iterator>;

                using ContentStorageType = AZStd::shared_ptr<DataTypes::IGraphObject>;
                using ContentStorage = AZStd::vector<ContentStorageType>;
                using ContentStorageData = Views::View<ContentStorage::const_iterator>;

                using ContentStorageConstDataIteratorWrapper = Views::ConvertIterator<ContentStorage::const_iterator,
                            decltype(ConstDataConverter(nullptr))>;
                using ContentStorageConstData = Views::View<ContentStorageConstDataIteratorWrapper>;

                SCENE_CORE_API SceneGraph();

                inline NodeIndex GetRoot() const;
                SCENE_CORE_API NodeIndex Find(const char* path) const;
                SCENE_CORE_API NodeIndex Find(NodeIndex root, const char* name) const;
                inline NodeIndex Find(const Name& name);
                inline NodeIndex Find(const AZStd::string& path) const;
                inline NodeIndex Find(NodeIndex root, const AZStd::string& name) const;

                inline bool HasNodeContent(NodeIndex node) const;
                inline bool HasNodeSibling(NodeIndex node) const;
                inline bool HasNodeChild(NodeIndex node) const;
                inline bool HasNodeParent(NodeIndex node) const;
                inline bool IsNodeEndPoint(NodeIndex node) const;

                SCENE_CORE_API const Name& GetNodeName(NodeIndex node) const;
                inline AZStd::shared_ptr<DataTypes::IGraphObject> GetNodeContent(NodeIndex node);
                inline AZStd::shared_ptr<const DataTypes::IGraphObject> GetNodeContent(NodeIndex node) const;

                inline NodeIndex GetNodeParent(NodeIndex node) const;
                inline NodeIndex GetNodeParent(NodeHeader node) const;
                inline NodeIndex GetNodeSibling(NodeIndex node) const;
                inline NodeIndex GetNodeSibling(NodeHeader node) const;
                inline NodeIndex GetNodeChild(NodeIndex node) const;
                inline NodeIndex GetNodeChild(NodeHeader node) const;

                inline size_t GetNodeCount() const;

                // Used when switching from index based navigation to iterator based.
                inline HierarchyStorageConstData::iterator ConvertToHierarchyIterator(NodeIndex node) const;
                inline NameStorageConstData::iterator ConvertToNameIterator(NodeIndex node) const;
                inline ContentStorageData::iterator ConvertToStorageIterator(NodeIndex node);
                inline ContentStorageConstData::iterator ConvertToStorageIterator(NodeIndex node) const;
                // Used when switching from iterator based navigation to index based.
                //      Note that any changes made to the SceneGraph using the node index will invalidate
                //      the original iterator.
                inline NodeIndex ConvertToNodeIndex(HierarchyStorageConstData::iterator iterator) const;
                inline NodeIndex ConvertToNodeIndex(NameStorageConstData::iterator iterator) const;
                inline NodeIndex ConvertToNodeIndex(ContentStorageData::iterator iterator) const;
                inline NodeIndex ConvertToNodeIndex(ContentStorageConstData::iterator iterator) const;

                // Adds a child node to the given parent. If the parent already had a child, AddChild will search the sibling
                //      chain for an available spot.
                SCENE_CORE_API NodeIndex AddChild(NodeIndex parent, const char* name);
                SCENE_CORE_API NodeIndex AddChild(NodeIndex parent, const char* name, const AZStd::shared_ptr<DataTypes::IGraphObject>& content);
                SCENE_CORE_API NodeIndex AddChild(NodeIndex parent, const char* name, AZStd::shared_ptr<DataTypes::IGraphObject>&& content);
                // Adds a sibling to given sibling. If the given sibling already has a sibling, the sibling chain is searched
                //      for an available spot. If the parent node is known AddChild can be used to achieve the same effect,
                //      however if index of the (last) added node is available this function can be used to reduce or skip
                //      the search within the sibling chain. This function can therefore be used as an optimization for AddChild,
                //      when more than 1 child is being added.
                SCENE_CORE_API NodeIndex AddSibling(NodeIndex sibling, const char* name);
                SCENE_CORE_API NodeIndex AddSibling(NodeIndex sibling, const char* name, const AZStd::shared_ptr<DataTypes::IGraphObject>& content);
                SCENE_CORE_API NodeIndex AddSibling(NodeIndex sibling, const char* name, AZStd::shared_ptr<DataTypes::IGraphObject>&& content);
                SCENE_CORE_API bool SetContent(NodeIndex node, const AZStd::shared_ptr<DataTypes::IGraphObject>& content);
                SCENE_CORE_API bool SetContent(NodeIndex node, AZStd::shared_ptr<DataTypes::IGraphObject>&& content);
                // Marks a function to no longer accept child nodes.
                SCENE_CORE_API bool MakeEndPoint(NodeIndex node);

                inline HierarchyStorageConstData GetHierarchyStorage() const;
                inline NameStorageConstData GetNameStorage() const;
                inline ContentStorageData GetContentStorage();
                inline ContentStorageConstData GetContentStorage() const;

                // Clears all data stored inside and reads the default root node.
                SCENE_CORE_API void Clear();

                // Checks if the given name can be used as a valid name for a node. This only checks the name validity, not if it's
                //      already in use. Use Find(...) to check if a name is already in use.
                SCENE_CORE_API static bool IsValidName(const char* name);
                // Checks if the given name can be used as a valid name for a node. This only checks the name validity, not if it's
                //      already in use. Use Find(...) to check if a name is already in use.
                inline static bool IsValidName(const AZStd::string& name);

                SCENE_CORE_API static char GetNodeSeperationCharacter();

                static void Reflect(AZ::ReflectContext* context);

            private:
                // Adds a child node to the given parent. AppendChild assumes that checks have already be done to guarantee the given parent
                //      doesn't already have a child.
                NodeIndex::IndexType AppendChild(NodeIndex::IndexType parent, const char* name, AZStd::shared_ptr<DataTypes::IGraphObject>&& content);
                // Add a sibling after the given sibling. AppendSibling assumes that the correct insertion point was found before calling
                //      and the given sibling is the last in line with no siblings following.
                NodeIndex::IndexType AppendSibling(NodeIndex::IndexType sibling, const char* name, AZStd::shared_ptr<DataTypes::IGraphObject>&& content);
                // Appends a new node to the graph and configures its heritage according to the given parent. Connections to the new node
                //      as identified by the returned index are assumed to be setup by either the calling function such as AppendChild or
                //      AppendSibling.
                NodeIndex::IndexType AppendNode(NodeIndex::IndexType parentIndex, const char* name, AZStd::shared_ptr<DataTypes::IGraphObject>&& content);

                NameLookup::const_iterator FindNameLookupIterator(const char* name) const;
                NameLookup::const_iterator FindNameLookupIterator(StringHash hash, const char* name) const;

                AZStd::string CombineName(const char* path, const char* name) const;
                void AddDefaultRoot();

                static const char s_nodeSeperationCharacter = '.';

                NameLookup m_nameLookup;
                HierarchyStorage m_hierarchy;
                NameStorage m_names;
                ContentStorage m_content;
            };
        } // Containers
    } // SceneAPI

    AZ_TYPE_INFO_SPECIALIZE(AZ::SceneAPI::Containers::SceneGraph, "{CAC6556D-D5FE-4D0E-BCCD-8940357C1D35}");
    AZ_TYPE_INFO_SPECIALIZE(AZ::SceneAPI::Containers::SceneGraph::NodeHeader, "{888C32BB-FEE3-4FA1-ADA4-09A58B03562A}");
    AZ_TYPE_INFO_SPECIALIZE(AZ::SceneAPI::Containers::SceneGraph::NodeIndex, "{4AD18037-E629-480D-8165-997A137327FD}");
    AZ_TYPE_INFO_SPECIALIZE(AZ::SceneAPI::Containers::SceneGraph::Name, "{4077AC3C-B301-4F5A-BEA7-54D6511AEC2E}");

} // AZ

#include <SceneAPI/SceneCore/Containers/SceneGraph.inl>

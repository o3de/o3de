/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/GraphObjectProxy.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            const SceneGraph::NodeIndex::IndexType SceneGraph::NodeIndex::INVALID_INDEX;

            static_assert(sizeof(SceneGraph::NodeIndex::IndexType) >= ((SceneGraph::NodeHeader::INDEX_BIT_COUNT / 8) + 1),
                "SceneGraph::NodeIndex is not big enough to store the parent index of a SceneGraph::NodeHeader");

            //
            // SceneGraph
            //
            SceneGraph::SceneGraph()
            {
                AddDefaultRoot();
            }

            void SceneGraph::Reflect(AZ::ReflectContext* context)
            {
                GraphObjectProxy::Reflect(context);

                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<SceneGraph::NodeIndex>("NodeIndex")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene.graph")
                        ->Constructor<>()
                        ->Constructor<const SceneGraph::NodeIndex&>()
                        ->Method("AsNumber", &SceneGraph::NodeIndex::AsNumber)
                        ->Method("Distance", &SceneGraph::NodeIndex::Distance)
                        ->Method("IsValid", &SceneGraph::NodeIndex::IsValid)
                        ->Method("Equal", &SceneGraph::NodeIndex::operator==)
                            ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                        ->Method("ToString", [](const SceneGraph::NodeIndex& node) { return AZStd::string::format("%u", node.m_value); })
                            ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                        ;

                    behaviorContext->Class<SceneGraph::Name>("SceneGraphName")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene.graph")
                        ->Constructor()
                        ->Method("GetPath", &SceneGraph::Name::GetPath)
                        ->Method("GetName", &SceneGraph::Name::GetName)
                        ->Method("ToString", [](const SceneGraph::Name& self){ return self.GetName(); })
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                        ;

                    behaviorContext->Class<SceneGraph>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene.graph")
                        // static methods
                        ->Method("IsValidName", [](const char* name) { return SceneGraph::IsValidName(name); })
                        ->Method("GetNodeSeperationCharacter", &SceneGraph::GetNodeSeperationCharacter)
                        // instance methods
                        ->Method("GetNodeName", &SceneGraph::GetNodeName)
                        ->Method("GetRoot", &SceneGraph::GetRoot)
                        ->Method("HasNodeContent", &SceneGraph::HasNodeContent)
                        ->Method("HasNodeSibling", &SceneGraph::HasNodeSibling)
                        ->Method("HasNodeChild", &SceneGraph::HasNodeChild)
                        ->Method("HasNodeParent", &SceneGraph::HasNodeParent)
                        ->Method("IsNodeEndPoint", &SceneGraph::IsNodeEndPoint)
                        ->Method("GetNodeParent", [](const SceneGraph& self, NodeIndex node) { return self.GetNodeParent(node); })
                        ->Method("GetNodeSibling", [](const SceneGraph& self, NodeIndex node) { return self.GetNodeSibling(node); })
                        ->Method("GetNodeChild", [](const SceneGraph& self, NodeIndex node) { return self.GetNodeChild(node); })
                        ->Method("GetNodeCount", &SceneGraph::GetNodeCount)
                        ->Method("FindWithPath", [](const SceneGraph& self, const AZStd::string& path)
                        {
                            return self.Find(path);
                        })
                        ->Method("FindWithRootAndPath", [](const SceneGraph& self, NodeIndex root, const AZStd::string& path)
                        {
                            return self.Find(root, path);
                        })
                        ->Method("GetNodeContent", [](const SceneGraph& self, NodeIndex node) -> GraphObjectProxy*
                        {
                            auto graphObject = self.GetNodeContent(node);
                            if (graphObject)
                            {
                                GraphObjectProxy* proxy = aznew GraphObjectProxy(graphObject);
                                return proxy;
                            }
                            return aznew GraphObjectProxy(nullptr);
                        });
                }
            }

            SceneGraph::NodeIndex SceneGraph::Find(const char* path) const
            {
                auto location = FindNameLookupIterator(path);
                return NodeIndex(location != m_nameLookup.end() ? (*location).second : NodeIndex::INVALID_INDEX);
            }

            SceneGraph::NodeIndex SceneGraph::Find(NodeIndex root, const char* name) const
            {
                if (root.m_value < m_names.size())
                {
                    AZStd::string fullname = CombineName(m_names[root.m_value].GetPath(), name);
                    return Find(fullname);
                }
                return NodeIndex();
            }

            const SceneGraph::Name& SceneGraph::GetNodeName(NodeIndex node) const
            {
                if (node.m_value < m_names.size())
                {
                    return m_names[node.m_value];
                }
                else
                {
                    static Name invalidNodeName(AZStd::string("<Invalid>"), 0);
                    return invalidNodeName;
                }
            }

            SceneGraph::NodeIndex SceneGraph::AddChild(NodeIndex parent, const char* name)
            {
                return AddChild(parent, name, AZStd::shared_ptr<DataTypes::IGraphObject>(nullptr));
            }

            SceneGraph::NodeIndex SceneGraph::AddChild(NodeIndex parent, const char* name, const AZStd::shared_ptr<DataTypes::IGraphObject>& content)
            {
                return AddChild(parent, name, AZStd::shared_ptr<DataTypes::IGraphObject>(content));
            }

            SceneGraph::NodeIndex SceneGraph::AddChild(NodeIndex parent, const char* name, AZStd::shared_ptr<DataTypes::IGraphObject>&& content)
            {
                if (parent.m_value < m_hierarchy.size())
                {
                    NodeHeader& parentNode = m_hierarchy[parent.m_value];
                    if (parentNode.HasChild())
                    {
                        return AddSibling(NodeIndex(parentNode.m_childIndex), name, AZStd::move(content));
                    }
                    else
                    {
                        return NodeIndex(AppendChild(parent.m_value, name, AZStd::move(content)));
                    }
                }

                return NodeIndex();
            }

            SceneGraph::NodeIndex SceneGraph::AddSibling(NodeIndex sibling, const char* name)
            {
                return AddSibling(sibling, name, AZStd::shared_ptr<DataTypes::IGraphObject>(nullptr));
            }

            SceneGraph::NodeIndex SceneGraph::AddSibling(NodeIndex sibling, const char* name, const AZStd::shared_ptr<DataTypes::IGraphObject>& content)
            {
                return AddSibling(sibling, name, AZStd::shared_ptr<DataTypes::IGraphObject>(content));
            }

            SceneGraph::NodeIndex SceneGraph::AddSibling(NodeIndex sibling, const char* name, AZStd::shared_ptr<DataTypes::IGraphObject>&& content)
            {
                if (sibling.m_value < m_hierarchy.size())
                {
                    NodeIndex::IndexType siblingIndex = sibling.m_value;
                    while (m_hierarchy[siblingIndex].HasSibling())
                    {
                        siblingIndex = m_hierarchy[siblingIndex].m_siblingIndex;
                    }
                    return NodeIndex(AppendSibling(siblingIndex, name, AZStd::move(content)));
                }

                return NodeIndex();
            }

            bool SceneGraph::SetContent(NodeIndex node, const AZStd::shared_ptr<DataTypes::IGraphObject>& content)
            {
                if (node.m_value < m_content.size())
                {
                    m_content[node.m_value] = content;
                    return true;
                }
                return false;
            }

            bool SceneGraph::SetContent(NodeIndex node, AZStd::shared_ptr<DataTypes::IGraphObject>&& content)
            {
                if (node.m_value < m_content.size())
                {
                    m_content[node.m_value] = AZStd::move(content);
                    return true;
                }
                return false;
            }

            bool SceneGraph::MakeEndPoint(NodeIndex node)
            {
                if (node.m_value < m_hierarchy.size())
                {
                    m_hierarchy[node.m_value].m_isEndPoint = 1;
                    return true;
                }
                return false;
            }

            void SceneGraph::Clear()
            {
                m_nameLookup.clear();
                m_hierarchy.clear();
                m_names.clear();
                m_content.clear();

                AddDefaultRoot();
            }

            bool SceneGraph::IsValidName(const char* name)
            {
                if (!name)
                {
                    return false;
                }

                if (name[0] == 0)
                {
                    return false;
                }

                const char* current = name;
                while (*current)
                {
                    if (*current++ == s_nodeSeperationCharacter)
                    {
                        return false;
                    }
                }
                return true;
            }

            char SceneGraph::GetNodeSeperationCharacter()
            {
                return s_nodeSeperationCharacter;
            }

            SceneGraph::NodeIndex::IndexType SceneGraph::AppendChild(NodeIndex::IndexType parent, const char* name, AZStd::shared_ptr<DataTypes::IGraphObject>&& content)
            {
                if (parent < m_hierarchy.size())
                {
                    NodeHeader parentNode = m_hierarchy[parent];
                    AZ_Assert(!parentNode.HasChild(), "Child '%s' couldn't be added as the target parent already contains a child.", name);
                    AZ_Assert(!parentNode.IsEndPoint(), "Attempting to add a child '%s' to node which is marked as an end point.", name);
                    if (!parentNode.HasChild() && !parentNode.IsEndPoint())
                    {
                        NodeIndex::IndexType nodeIndex = AppendNode(parent, name, AZStd::move(content));
                        m_hierarchy[parent].m_childIndex = nodeIndex;
                        return nodeIndex;
                    }
                }

                return NodeIndex::INVALID_INDEX;
            }

            SceneGraph::NodeIndex::IndexType SceneGraph::AppendSibling(NodeIndex::IndexType sibling, const char* name, AZStd::shared_ptr<DataTypes::IGraphObject>&& content)
            {
                if (sibling < m_hierarchy.size())
                {
                    NodeHeader siblingNode = m_hierarchy[sibling];
                    AZ_Assert(!siblingNode.HasSibling(), "Sibling '%s' couldn't be added as the target node already contains a sibling.", name);
                    if (!siblingNode.HasSibling())
                    {
                        NodeIndex::IndexType nodeIndex = AppendNode(siblingNode.m_parentIndex, name, AZStd::move(content));
                        m_hierarchy[sibling].m_siblingIndex = nodeIndex;
                        return nodeIndex;
                    }
                }

                return NodeIndex::INVALID_INDEX;
            }

            SceneGraph::NodeIndex::IndexType SceneGraph::AppendNode(NodeIndex::IndexType parentIndex, const char* name, AZStd::shared_ptr<DataTypes::IGraphObject>&& content)
            {
                NodeIndex::IndexType nodeIndex = aznumeric_caster(m_hierarchy.size());
                NodeHeader node;
                node.m_parentIndex = parentIndex;
                m_hierarchy.push_back(node);

                AZ_Assert(IsValidName(name), "Name '%s' for SceneGraph sibling contains invalid characters", name);

                AZStd::string fullName;
                size_t nameOffset;
                if (parentIndex != NodeHeader::INVALID_INDEX)
                {
                    const Name& parentName = m_names[parentIndex];
                    fullName = CombineName(parentName.GetPath(), name);
                    nameOffset = parentName.GetPathLength() + (parentName.GetPathLength() != 0 ? 1 : 0);
                }
                else
                {
                    fullName = name;
                    nameOffset = 0;
                }

                StringHash fullNameHash = StringHasher()(fullName);
                AZ_Assert(FindNameLookupIterator(fullNameHash, fullName.c_str()) == m_nameLookup.end(), "Duplicate name found in SceneGraph: %s", fullName.c_str());
                m_nameLookup.insert(NameLookup::value_type(fullNameHash, nodeIndex));
                m_names.emplace_back(AZStd::move(fullName), nameOffset);
                AZ_Assert(m_hierarchy.size() == m_names.size(),
                    "Hierarchy and name lists in SceneGraph have gone out of sync. (%i vs. %i)", m_hierarchy.size(), m_names.size());

                m_content.push_back(AZStd::move(content));
                AZ_Assert(m_hierarchy.size() == m_content.size(),
                    "Hierarchy and data lists in SceneGraph have gone out of sync. (%i vs. %i)", m_hierarchy.size(), m_content.size());

                return nodeIndex;
            }

            SceneGraph::NameLookup::const_iterator SceneGraph::FindNameLookupIterator(const char* name) const
            {
                StringHash hash = StringHasher()(name);
                return FindNameLookupIterator(hash, name);
            }

            SceneGraph::NameLookup::const_iterator SceneGraph::FindNameLookupIterator(StringHash hash, const char* name) const
            {
                auto range = m_nameLookup.equal_range(hash);
                // Always check the name, even if there's only one entry as the hash can be a clash with
                //      the single entry.
                for (auto it = range.first; it != range.second; ++it)
                {
                    if (AzFramework::StringFunc::Equal(m_names[it->second].GetPath(), name, true))
                    {
                        return it;
                    }
                }

                return m_nameLookup.end();
            }

            AZStd::string SceneGraph::CombineName(const char* path, const char* name) const
            {
                AZStd::string result = path;
                if (result.length() > 0)
                {
                    result += s_nodeSeperationCharacter;
                }
                result += name;
                return result;
            }

            void SceneGraph::AddDefaultRoot()
            {
                AZ_Assert(m_hierarchy.size() == 0, "Adding a default root node to a SceneGraph with content.");

                m_hierarchy.push_back(NodeHeader());
                m_nameLookup.insert(NameLookup::value_type(StringHasher()(""), 0));
                m_names.emplace_back("", 0);
                m_content.emplace_back(nullptr);
            }
        } // Containers
    } // SceneAPI
} // AZ

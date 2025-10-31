/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <algorithm>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            size_t SceneNodeSelectionList::GetSelectedNodeCount() const
            {
                return m_selectedNodes.size();
            }

            void SceneNodeSelectionList::AddSelectedNode(const AZStd::string& name)
            {
                if (auto extractedNodeHandle = m_unselectedNodes.extract(name); extractedNodeHandle)
                {
                    m_selectedNodes.insert(AZStd::move(extractedNodeHandle.value()));
                }
                else
                {
                    m_selectedNodes.emplace(name);
                }
            }

            void SceneNodeSelectionList::AddSelectedNode(AZStd::string&& name)
            {
                m_unselectedNodes.erase(name);
                m_selectedNodes.emplace(AZStd::move(name));
            }

            void SceneNodeSelectionList::RemoveSelectedNode(const AZStd::string& name)
            {
                m_selectedNodes.erase(name);
                m_unselectedNodes.emplace(name);
            }

            void SceneNodeSelectionList::ClearSelectedNodes()
            {
                m_selectedNodes.clear();
            }

            void SceneNodeSelectionList::ClearUnselectedNodes()
            {
                m_unselectedNodes.clear();
            }

            bool SceneNodeSelectionList::IsSelectedNode(const AZStd::string& name) const
            {
                return m_selectedNodes.contains(name);
            }

            void SceneNodeSelectionList::EnumerateSelectedNodes(const EnumerateNodesCallback& callback) const
            {
                for (auto& node : m_selectedNodes)
                {
                    if (!callback(node))
                    {
                        break;
                    }
                }
            }

            void SceneNodeSelectionList::EnumerateUnselectedNodes(const EnumerateNodesCallback& callback) const
            {
                for (auto& node : m_unselectedNodes)
                {
                    if (!callback(node))
                    {
                        break;
                    }
                }
            }

            AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList> SceneNodeSelectionList::Copy() const
            {
                return AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList>(new SceneNodeSelectionList(*this));
            }

            void SceneNodeSelectionList::CopyTo(DataTypes::ISceneNodeSelectionList& other) const
            {
                other.ClearSelectedNodes();
                other.ClearUnselectedNodes();

                for (const AZStd::string& selected : m_selectedNodes)
                {
                    other.AddSelectedNode(selected);
                }
                for (const AZStd::string& unselected : m_unselectedNodes)
                {
                    other.RemoveSelectedNode(unselected);
                }
            }

            bool SceneNodeSelectionListVersionConverter(
                AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& classElement)
            {
                // Version 3 - changed selectedNodes/unselectedNodes from vector to unordered_set.
                if (classElement.GetVersion() < 3)
                {
                    // Convert a serialized field from vector<string> to unordered_set<string>
                    auto convertVectorToUnorderedSet = [&serializeContext, &classElement](AZ::Crc32 element) -> bool
                    {
                        int nodesIndex = classElement.FindElement(element);

                        if (nodesIndex < 0)
                        {
                            return false;
                        }

                        AZ::SerializeContext::DataElementNode& nodes = classElement.GetSubElement(nodesIndex);
                        AZStd::vector<AZStd::string> nodesVector;
                        AZStd::unordered_set<AZStd::string> nodesSet;
                        if (!nodes.GetData<AZStd::vector<AZStd::string>>(nodesVector))
                        {
                            return false;
                        }
                        nodesSet.insert(nodesVector.begin(), nodesVector.end());
                        nodes.Convert<AZStd::unordered_set<AZStd::string>>(serializeContext);
                        if (!nodes.SetData<AZStd::unordered_set<AZStd::string>>(serializeContext, nodesSet))
                        {
                            return false;
                        }

                        return true;
                    };

                    // Convert selectedNodes and unselectedNodes from a vector to an unordered_set
                    bool result = convertVectorToUnorderedSet(AZ_CRC_CE("selectedNodes"));
                    result = result && convertVectorToUnorderedSet(AZ_CRC_CE("unselectedNodes"));

                    return result;
                }

                return true;
            }


            void SceneNodeSelectionList::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<SceneNodeSelectionList, DataTypes::ISceneNodeSelectionList>()
                    ->Version(3, &SceneNodeSelectionListVersionConverter)
                    ->Field("selectedNodes", &SceneNodeSelectionList::m_selectedNodes)
                    ->Field("unselectedNodes", &SceneNodeSelectionList::m_unselectedNodes);

                // Explicitly register the AZStd::vector<AZStd::string> type. The version converter needs it to be able to read
                // in the old data, and the type itself only gets registered automatically on-demand through the serializeContext
                // fields. Since the serializeContext no longer contains this type, there's no guarantee it would be created.
                // By explicitly registering it here, we can ensure that it exists.
                serializeContext->RegisterGenericType<AZStd::vector<AZStd::string>>();
            }
        } // SceneData
    } // SceneAPI
} // AZ

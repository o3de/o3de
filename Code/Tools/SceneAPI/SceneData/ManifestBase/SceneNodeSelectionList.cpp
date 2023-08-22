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
                    m_selectedNodes.insert(extractedNodeHandle);
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

            void SceneNodeSelectionList::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<SceneNodeSelectionList, DataTypes::ISceneNodeSelectionList>()->Version(2)
                    ->Field("selectedNodes", &SceneNodeSelectionList::m_selectedNodes)
                    ->Field("unselectedNodes", &SceneNodeSelectionList::m_unselectedNodes);
            }
        } // SceneData
    } // SceneAPI
} // AZ

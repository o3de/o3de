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

            const AZStd::string& SceneNodeSelectionList::GetSelectedNode(size_t index) const
            {
                AZ_Assert(index < m_selectedNodes.size(), "Invalid index %i for selected node in mesh group.", index);
                return m_selectedNodes[index];
            }

            size_t SceneNodeSelectionList::AddSelectedNode(const AZStd::string& name)
            {
                auto unselectEntry = AZStd::find(m_unselectedNodes.begin(), m_unselectedNodes.end(), name);
                if (unselectEntry != m_unselectedNodes.end())
                {
                    m_unselectedNodes.erase(unselectEntry);
                }

                auto entry = AZStd::find(m_selectedNodes.begin(), m_selectedNodes.end(), name);
                if (entry == m_selectedNodes.end())
                {
                    size_t index = m_selectedNodes.size();
                    m_selectedNodes.push_back(name);
                    return index;
                }
                else
                {
                    return entry - m_selectedNodes.begin();
                }
            }

            size_t SceneNodeSelectionList::AddSelectedNode(AZStd::string&& name)
            {
                auto unselectedEntry = AZStd::find(m_unselectedNodes.begin(), m_unselectedNodes.end(), name);
                if (unselectedEntry != m_unselectedNodes.end())
                {
                    m_unselectedNodes.erase(unselectedEntry);
                }

                auto entry = AZStd::find(m_selectedNodes.begin(), m_selectedNodes.end(), name);
                if (entry == m_selectedNodes.end())
                {
                    size_t index = m_selectedNodes.size();
                    m_selectedNodes.push_back(AZStd::move(name));
                    return index;
                }
                else
                {
                    return entry - m_selectedNodes.begin();
                }
            }

            void SceneNodeSelectionList::RemoveSelectedNode(size_t index)
            {
                if (index < m_selectedNodes.size())
                {
                    auto unselectedEntry = AZStd::find(m_unselectedNodes.begin(), m_unselectedNodes.end(), m_selectedNodes[index]);
                    if (unselectedEntry == m_unselectedNodes.end())
                    {
                        m_unselectedNodes.push_back(m_selectedNodes[index]);
                    }
                    m_selectedNodes.erase(m_selectedNodes.begin() + index);
                }
            }

            void SceneNodeSelectionList::RemoveSelectedNode(const AZStd::string& name)
            {
                auto selectEntry = AZStd::find(m_selectedNodes.begin(), m_selectedNodes.end(), name);
                if (selectEntry != m_selectedNodes.end())
                {
                    m_selectedNodes.erase(selectEntry);
                }

                auto entry = AZStd::find(m_unselectedNodes.begin(), m_unselectedNodes.end(), name);
                if (entry == m_unselectedNodes.end())
                {
                    m_unselectedNodes.push_back(name);
                }
            }

            void SceneNodeSelectionList::ClearSelectedNodes()
            {
                m_selectedNodes.clear();
            }



            size_t SceneNodeSelectionList::GetUnselectedNodeCount() const
            {
                return m_unselectedNodes.size();
            }

            const AZStd::string& SceneNodeSelectionList::GetUnselectedNode(size_t index) const
            {
                AZ_Assert(index < m_unselectedNodes.size(), "Invalid index %i for unselected node in mesh group.", index);
                return m_unselectedNodes[index];
            }

            void SceneNodeSelectionList::ClearUnselectedNodes()
            {
                m_unselectedNodes.clear();
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

                serializeContext->Class<SceneNodeSelectionList, DataTypes::ISceneNodeSelectionList>()->Version(1)
                    ->Field("selectedNodes", &SceneNodeSelectionList::m_selectedNodes)
                    ->Field("unselectedNodes", &SceneNodeSelectionList::m_unselectedNodes);
            }
        } // SceneData
    } // SceneAPI
} // AZ

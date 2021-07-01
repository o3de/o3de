/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                template<typename Iterator>
                SceneGraphUpwardsIterator<Iterator>::SceneGraphUpwardsIterator(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator)
                {
                    if (graph.GetHierarchyStorage().end() == graphIterator)
                    {
                        m_index = -1;
                        m_graph = nullptr;
                    }
                    else
                    {
                        m_graph = &graph;
                        m_index = static_cast<int32_t>(AZStd::distance(graph.GetHierarchyStorage().begin(), graphIterator));
                        m_iterator = rootIterator ? AZStd::next(iterator, m_index) : iterator;
                    }
                }

                template<typename Iterator>
                SceneGraphUpwardsIterator<Iterator>::SceneGraphUpwardsIterator()
                    : m_graph(nullptr)
                    , m_index(-1)
                {
                }

                template<typename Iterator>
                auto SceneGraphUpwardsIterator<Iterator>::operator++()->SceneGraphUpwardsIterator &
                {
                    MoveToNext();
                    return *this;
                }

                template<typename Iterator>
                auto SceneGraphUpwardsIterator<Iterator>::operator++(int)->SceneGraphUpwardsIterator
                {
                    SceneGraphUpwardsIterator result = *this;
                    MoveToNext();
                    return result;
                }

                template<typename Iterator>
                bool SceneGraphUpwardsIterator<Iterator>::operator==(const SceneGraphUpwardsIterator& rhs) const
                {
                    return m_graph == rhs.m_graph && m_index == rhs.m_index;
                }

                template<typename Iterator>
                bool SceneGraphUpwardsIterator<Iterator>::operator!=(const SceneGraphUpwardsIterator& rhs) const
                {
                    return m_graph != rhs.m_graph || m_index != rhs.m_index;
                }

                template<typename Iterator>
                auto SceneGraphUpwardsIterator<Iterator>::operator*() const->reference
                {
                    return *m_iterator;
                }

                template<typename Iterator>
                auto SceneGraphUpwardsIterator<Iterator>::GetPointer(AZStd::true_type) const->pointer
                {
                    return m_iterator;
                }

                template<typename Iterator>
                auto SceneGraphUpwardsIterator<Iterator>::GetPointer(AZStd::false_type) const->pointer
                {
                    return m_iterator.operator->();
                }

                template<typename Iterator>
                auto SceneGraphUpwardsIterator<Iterator>::operator->() const->pointer
                {
                    return GetPointer(typename AZStd::is_pointer<Iterator>::type());
                }

                template<typename Iterator>
                SceneGraph::HierarchyStorageConstIterator SceneGraphUpwardsIterator<Iterator>::GetHierarchyIterator() const
                {
                    return AZStd::next(m_graph->GetHierarchyStorage().begin(), m_index);
                }

                template<typename Iterator>
                void SceneGraphUpwardsIterator<Iterator>::MoveToNext()
                {
                    AZ_Assert(m_graph, "Invalid iterator or moved passed end of list.");
                    SceneGraph::NodeHeader header = m_graph->GetHierarchyStorage().begin()[m_index];
                    if (header.HasParent())
                    {
                        AZStd::advance(m_iterator, header.m_parentIndex - m_index);
                        m_index = header.m_parentIndex;
                    }
                    else
                    {
                        m_index = -1;
                        m_graph = nullptr;
                    }
                }

                template<typename Iterator>
                SceneGraphUpwardsIterator<Iterator> MakeSceneGraphUpwardsIterator(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator)
                {
                    return SceneGraphUpwardsIterator<Iterator>(graph, graphIterator, iterator, rootIterator);
                }

                template<typename Iterator>
                SceneGraphUpwardsIterator<Iterator> MakeSceneGraphUpwardsIterator(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator)
                {
                    return SceneGraphUpwardsIterator<Iterator>(graph, graph.ConvertToHierarchyIterator(node), iterator, rootIterator);
                }

                template<typename Iterator>
                View<SceneGraphUpwardsIterator<Iterator> > MakeSceneGraphUpwardsView(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator)
                {
                    return View<SceneGraphUpwardsIterator<Iterator> >(
                        SceneGraphUpwardsIterator<Iterator>(graph, graphIterator, iterator, rootIterator),
                        SceneGraphUpwardsIterator<Iterator>());
                }

                template<typename Iterator>
                View<SceneGraphUpwardsIterator<Iterator> > MakeSceneGraphUpwardsView(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator)
                {
                    return View<SceneGraphUpwardsIterator<Iterator> >(
                        SceneGraphUpwardsIterator<Iterator>(graph, graph.ConvertToHierarchyIterator(node), iterator, rootIterator),
                        SceneGraphUpwardsIterator<Iterator>());
                }
            } // Views
        } // Containers
    } // SceneAPI
} // AZ

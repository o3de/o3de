/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/typetraits/is_pointer.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                template<typename Iterator, typename Traversal>
                SceneGraphDownwardsIterator<Iterator, Traversal>::SceneGraphDownwardsIterator(const SceneGraph& graph, Iterator iterator)
                    : m_graph(&graph)
                    , m_iterator(iterator)
                    , m_index(0)
                    , m_ignoreDescendants(false)
                    , m_firstNode(true)
                {
                }

                template<typename Iterator, typename Traversal>
                SceneGraphDownwardsIterator<Iterator, Traversal>::SceneGraphDownwardsIterator(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator)
                    : m_ignoreDescendants(false)
                    , m_firstNode(true)
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

                template<typename Iterator, typename Traversal>
                SceneGraphDownwardsIterator<Iterator, Traversal>::SceneGraphDownwardsIterator()
                    : m_graph(nullptr)
                    , m_index(-1)
                    , m_ignoreDescendants(false)
                    , m_firstNode(false)
                {
                }

                template<typename Iterator, typename Traversal>
                auto SceneGraphDownwardsIterator<Iterator, Traversal>::operator++()->SceneGraphDownwardsIterator &
                {
                    MoveToNext(Traversal());
                    return *this;
                }

                template<typename Iterator, typename Traversal>
                auto SceneGraphDownwardsIterator<Iterator, Traversal>::operator++(int)->SceneGraphDownwardsIterator
                {
                    SceneGraphDownwardsIterator result = *this;
                    MoveToNext(Traversal());
                    return result;
                }

                template<typename Iterator, typename Traversal>
                bool SceneGraphDownwardsIterator<Iterator, Traversal>::operator==(const SceneGraphDownwardsIterator& rhs) const
                {
                    return m_graph == rhs.m_graph && m_index == rhs.m_index && m_firstNode == rhs.m_firstNode;
                }

                template<typename Iterator, typename Traversal>
                bool SceneGraphDownwardsIterator<Iterator, Traversal>::operator!=(const SceneGraphDownwardsIterator& rhs) const
                {
                    return m_graph != rhs.m_graph || m_index != rhs.m_index || m_firstNode != rhs.m_firstNode;
                }

                template<typename Iterator, typename Traversal>
                auto SceneGraphDownwardsIterator<Iterator, Traversal>::operator*() const->reference
                {
                    return *m_iterator;
                }

                template<typename Iterator, typename Traversal>
                auto SceneGraphDownwardsIterator<Iterator, Traversal>::GetPointer(AZStd::true_type) const->pointer
                {
                    return m_iterator;
                }

                template<typename Iterator, typename Traversal>
                auto SceneGraphDownwardsIterator<Iterator, Traversal>::GetPointer(AZStd::false_type) const->pointer
                {
                    return m_iterator.operator->();
                }

                template<typename Iterator, typename Traversal>
                auto SceneGraphDownwardsIterator<Iterator, Traversal>::operator->() const->pointer
                {
                    return GetPointer(typename AZStd::is_pointer<Iterator>::type());
                }

                template<typename Iterator, typename Traversal>
                SceneGraph::HierarchyStorageConstIterator SceneGraphDownwardsIterator<Iterator, Traversal>::GetHierarchyIterator() const
                {
                    if (m_index >= 0)
                    {
                        return AZStd::next(m_graph->GetHierarchyStorage().begin(), m_index);
                    }
                    else
                    {
                        return SceneGraph::HierarchyStorageConstIterator();
                    }
                }

                template<typename Iterator, typename Traversal>
                void SceneGraphDownwardsIterator<Iterator, Traversal>::IgnoreNodeDescendants()
                {
                    m_ignoreDescendants = -1;
                }

                template<typename Iterator, typename Traversal>
                void SceneGraphDownwardsIterator<Iterator, Traversal>::MoveToNext(DepthFirst)
                {
                    AZ_Assert(m_graph, "Invalid iterator or moved passed end of list.");
                    SceneGraph::NodeHeader header = m_graph->GetHierarchyStorage().begin()[m_index];
                    if (!m_firstNode && header.HasSibling())
                    {
                        m_pending.push_back(static_cast<int32_t>(header.m_siblingIndex));
                    }

                    if (!m_ignoreDescendants && header.HasChild())
                    {
                        JumpTo(header.m_childIndex);
                    }
                    else if (!m_pending.empty())
                    {
                        JumpTo(m_pending.back());
                        m_pending.pop_back();
                    }
                    else
                    {
                        m_index = -1;
                        m_graph = nullptr;
                    }
                    m_ignoreDescendants = false;
                    m_firstNode = false;
                }

                template<typename Iterator, typename Traversal>
                void SceneGraphDownwardsIterator<Iterator, Traversal>::MoveToNext(BreadthFirst)
                {
                    AZ_Assert(m_graph, "Invalid iterator or moved passed end of list.");
                    SceneGraph::NodeHeader header = m_graph->GetHierarchyStorage().begin()[m_index];
                    if (!m_ignoreDescendants && header.HasChild())
                    {
                        m_pending.push_back(static_cast<int32_t>(header.m_childIndex));
                    }

                    if (!m_firstNode && header.HasSibling())
                    {
                        JumpTo(header.m_siblingIndex);
                    }
                    else if (!m_pending.empty())
                    {
                        JumpTo(m_pending.front());
                        m_pending.pop_front();
                    }
                    else
                    {
                        m_index = -1;
                        m_graph = nullptr;
                    }
                    m_ignoreDescendants = false;
                    m_firstNode = false;
                }

                template<typename Iterator, typename Traversal>
                void SceneGraphDownwardsIterator<Iterator, Traversal>::JumpTo(int32_t index)
                {
                    AZStd::advance(m_iterator, index - m_index);
                    m_index = index;
                }

                template<typename Traversal, typename Iterator>
                SceneGraphDownwardsIterator<Iterator, Traversal> MakeSceneGraphDownwardsIterator(const SceneGraph& graph, Iterator iterator)
                {
                    return SceneGraphDownwardsIterator<Iterator, Traversal>(graph, iterator);
                }

                template<typename Traversal, typename Iterator>
                SceneGraphDownwardsIterator<Iterator, Traversal> MakeSceneGraphDownwardsIterator(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator)
                {
                    return SceneGraphDownwardsIterator<Iterator, Traversal>(graph, graphIterator, iterator, rootIterator);
                }

                template<typename Traversal, typename Iterator>
                SceneGraphDownwardsIterator<Iterator, Traversal> MakeSceneGraphDownwardsIterator(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator)
                {
                    return SceneGraphDownwardsIterator<Iterator, Traversal>(graph, graph.ConvertToHierarchyIterator(node), iterator, rootIterator);
                }

                template<typename Traversal, typename Iterator>
                View<SceneGraphDownwardsIterator<Iterator, Traversal> > MakeSceneGraphDownwardsView(const SceneGraph& graph, Iterator iterator)
                {
                    return View<SceneGraphDownwardsIterator<Iterator, Traversal> >(
                        SceneGraphDownwardsIterator<Iterator, Traversal>(graph, iterator),
                        SceneGraphDownwardsIterator<Iterator, Traversal>());
                }

                template<typename Traversal, typename Iterator>
                View<SceneGraphDownwardsIterator<Iterator, Traversal> > MakeSceneGraphDownwardsView(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator)
                {
                    return View<SceneGraphDownwardsIterator<Iterator, Traversal> >(
                        SceneGraphDownwardsIterator<Iterator, Traversal>(graph, graphIterator, iterator, rootIterator),
                        SceneGraphDownwardsIterator<Iterator, Traversal>());
                }

                template<typename Traversal, typename Iterator>
                View<SceneGraphDownwardsIterator<Iterator, Traversal> > MakeSceneGraphDownwardsView(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator)
                {
                    return View<SceneGraphDownwardsIterator<Iterator, Traversal> >(
                        SceneGraphDownwardsIterator<Iterator, Traversal>(graph, graph.ConvertToHierarchyIterator(node), iterator, rootIterator),
                        SceneGraphDownwardsIterator<Iterator, Traversal>());
                }
            } // Views
        } // Containers
    } // SceneAPI
} // AZ

#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <iterator>
#include <type_traits>
#include <AzCore/std/iterator.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <SceneAPI/SceneCore/Containers/Views/View.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                struct FilterAcceptanceTag {};

                // Filter tag to only show regular nodes.
                struct AcceptNodesOnly : public FilterAcceptanceTag {};
                // Filter tag to only show end points.
                struct AcceptEndPointsOnly : public FilterAcceptanceTag {};
                // Filter tag to show all nodes.
                struct AcceptAll : public FilterAcceptanceTag {};

                // Iterator to traverse a SceneGraph from a given node by listing all the direct children. The given node will not be included in the
                //      iteration. By default all children are listed, but optionally only regular nodes or only end points can be returned by 
                //      specifying the appropriate tag (see example below).
                // Example:
                //      auto view = MakeSceneGraphChildView(graph, graph.ConvertToHierarchyIterator(nodeIndex), graph.GetNameStorage().begin(), true);
                //      for (auto& it : view)
                //      {
                //          printf("Node: %s\n", it.c_str());
                //      }
                //
                // Example:
                //      auto view = MakeSceneGraphChildView<AcceptEndPointsOnly>(graph, nodeIndex, graph.GetNameStorage().begin(), true);
                //      for (auto& it : view)
                //      {
                //          printf("End point: %s\n", it.c_str());
                //      }
                template<typename Iterator, typename Filter = AcceptAll>
                class SceneGraphChildIterator
                {
                    static_assert(AZStd::is_base_of<FilterAcceptanceTag, Filter>::value, 
                        "Filter for SceneGraphChildIterator is not a valid tag. Use AcceptNodesOnly, AcceptEndPointsOnly, AcceptAll or leave blank.");
                    static_assert(AZStd::is_base_of<AZStd::bidirectional_iterator_tag, typename AZStd::iterator_traits<Iterator>::iterator_category>::value,
                        "SceneGraphChildIterator needs at least a bidirectional iterator for its data iterator.");

                public:
                    using value_type = typename AZStd::iterator_traits<Iterator>::value_type;
                    using difference_type = AZStd::ptrdiff_t;
                    using pointer = typename AZStd::iterator_traits<Iterator>::pointer;
                    using reference = typename AZStd::iterator_traits<Iterator>::reference;
                    using iterator_category = AZStd::forward_iterator_tag;

                    // Creates an iterator at a specified node in the hierarchy.
                    // graph: SceneGraph that will traversed.
                    // graphIterator: The node to start traversing from.
                    // iterator: The data iterator to be dereferenced from.
                    // rootIterator: If true the data iterator will be the first iterator from the data and
                    //      this iterator will be moved forward to match the graph iterator. If false
                    //      the graph iterator and the data iterator should be pointing to the same relative
                    //      element.
                    SceneGraphChildIterator(const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);
                    SceneGraphChildIterator(const SceneGraphChildIterator&) = default;
                    SceneGraphChildIterator();

                    SceneGraphChildIterator& operator=(const SceneGraphChildIterator&) = default;

                    SceneGraphChildIterator& operator++();
                    SceneGraphChildIterator operator++(int);

                    bool operator==(const SceneGraphChildIterator& rhs) const;
                    bool operator!=(const SceneGraphChildIterator& rhs) const;

                    reference operator*() const;
                    pointer operator->() const;

                    SceneGraph::HierarchyStorageConstIterator GetHierarchyIterator() const;

                private:
                    // For iterators that are raw pointers.
                    pointer GetPointer(AZStd::true_type) const;
                    // For all other types of iterator.
                    pointer GetPointer(AZStd::false_type) const;

                    void MoveToNext();
                    bool ShouldAcceptNode(AcceptNodesOnly) const;
                    bool ShouldAcceptNode(AcceptEndPointsOnly) const;
                    bool ShouldAcceptNode(AcceptAll) const;
                    
                    const SceneGraph* m_graph;
                    Iterator m_iterator;
                    int32_t m_index;
                };

                //
                // Iterator construction
                //

                template<typename Filter, typename Iterator>
                SceneGraphChildIterator<Iterator, Filter> MakeSceneGraphChildIterator(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);

                template<typename Filter, typename Iterator>
                SceneGraphChildIterator<Iterator, Filter> MakeSceneGraphChildIterator(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator);

                template<typename Iterator>
                SceneGraphChildIterator<Iterator> MakeSceneGraphChildIterator(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);

                template<typename Iterator>
                SceneGraphChildIterator<Iterator> MakeSceneGraphChildIterator(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator);


                //
                // View construction
                //

                template<typename Filter, typename Iterator>
                View<SceneGraphChildIterator<Iterator, Filter> > MakeSceneGraphChildView(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);

                template<typename Filter, typename Iterator>
                View<SceneGraphChildIterator<Iterator, Filter> > MakeSceneGraphChildView(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator);

                template<typename Iterator>
                View<SceneGraphChildIterator<Iterator> > MakeSceneGraphChildView(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);

                template<typename Iterator>
                View<SceneGraphChildIterator<Iterator> > MakeSceneGraphChildView(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator);
            } // Views
        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.inl>

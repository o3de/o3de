/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DependencySolver.h"

namespace AZ::ShaderCompiler
{
    namespace detail
    {
        template<typename T, size_t N>
        auto ElasticArray<T,N>::end() -> decltype(Base::end())
        {
            auto realEnd = begin();
            std::advance(realEnd, m_end);
            return realEnd;
        }

        template<typename T, size_t N>
        auto ElasticArray<T,N>::end() const -> decltype(Base::end())
        {
            auto realEnd = begin();
            std::advance(realEnd, m_end);
            return realEnd;
        }

        template<typename T, size_t N>
        auto ElasticArray<T,N>::at(size_t n) -> decltype(Base::at(n))
        {
            if (n >= m_end)
            {
                throw std::runtime_error{"ElasticArray::at out of bounds"};
            }
            return Base::at(n);
        }

        template<typename T, size_t N>
        auto ElasticArray<T,N>::at(size_t n) const -> decltype(Base::at(n))
        {
            if (n >= m_end)
            {
                throw std::runtime_error{"ElasticArray::at out of bounds"};
            }
            return Base::at(n);
        }

        template<typename T, size_t N>
        T& ElasticArray<T,N>::push_back(const T& element)
        {
            ++m_end;
            at(m_end - 1) = element;
            return this->operator[](m_end - 1);
        }

        template< typename ID, EasyToReadMaxDepCountInteger MaxDep >
        void DependencySolverNode<ID, MaxDep>::PushDep(ID d)
        {
            if (!DependsOn(d))  // guarantee no duplicates for strong invariants in other algorithms
            {
                m_dependencies.push_back(d);
            }
        }

        template< typename ID, EasyToReadMaxDepCountInteger MaxDep >
        bool DependencySolverNode<ID, MaxDep>::DependsOn(ID d) const
        {
            return std::find(m_dependencies.begin(), m_dependencies.end(), d) != m_dependencies.end();
        }
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    void DependencySolver<NodeIdT, MaxDepPerNode>::AddDependency(ID node, ID toDependency)
    {
	    m_nodes[node].PushDep(toDependency);
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    void DependencySolver<NodeIdT, MaxDepPerNode>::AddNode(ID node)
    {
        m_order.push_back(node);
	    m_nodes[node];
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    bool DependencySolver<NodeIdT, MaxDepPerNode>::CheckAllDepSatisfied()
    {
	    for (size_t i = 0; i < m_order.size(); ++i)
	    {
		    ID iterNode = m_order[i];
		    Node& n = m_nodes[iterNode];
		    for (size_t j = i + 1; j < m_order.size(); ++j)
		    {
			    if (n.DependsOn(m_order[j]))
                {
				    return false;
                }
		    }
	    }
	    return true;
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    void DependencySolver<NodeIdT, MaxDepPerNode>::Solve()
    {
	    ClearAllTemps();
	    while (ExistsNodesWithoutPermanentMark())
	    {
		    ID n = SelectUnmarked();
		    Visit(n);
	    }
	    assert(m_order.size() == m_result.size());
	    std::copy(m_result.begin(), m_result.end(), m_order.begin());
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    bool DependencySolver<NodeIdT, MaxDepPerNode>::Has(ID node) const
    {
        return m_nodes.find(node) != m_nodes.end();
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    void DependencySolver<NodeIdT, MaxDepPerNode>::Visit(ID curNode)
    {
	    if (HasPermMark(curNode))
	    {
		    return;
	    }
	    if (HasTempMark(curNode)) // not a DAG
	    {
		    throw std::runtime_error{"cycles in the dependencies"};
	    }
	    TempMark(curNode);  // mark curNode with a temporary mark
	    for (ID dependee : m_nodes[curNode].m_dependencies)  // for each node 'dependee' with an edge from 'curNode' to 'dependee'
	    {
		    Visit(dependee);
	    }
	    RemoveTempMark(curNode);
	    PermMark(curNode); // mark curNode with a permanent mark
	    m_result.push_back(curNode);  // add curNode to tail of L
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    void DependencySolver<NodeIdT, MaxDepPerNode>::ClearAllTemps()
    {
	    m_permMarks.clear();
	    m_tempMarks.clear();
	    m_result.clear();
	    m_selectCursor = 0;
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    void DependencySolver<NodeIdT, MaxDepPerNode>::PermMark(ID n)
    {
	    m_permMarks.insert(n);
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    void DependencySolver<NodeIdT, MaxDepPerNode>::TempMark(ID n)
    {
	    m_tempMarks.insert(n);
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    bool DependencySolver<NodeIdT, MaxDepPerNode>::HasPermMark(ID n) const
    {
	    return m_permMarks.find(n) != m_permMarks.end();
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    bool DependencySolver<NodeIdT, MaxDepPerNode>::HasTempMark(ID n) const
    {
	    return m_tempMarks.find(n) != m_tempMarks.end();
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    void DependencySolver<NodeIdT, MaxDepPerNode>::RemoveTempMark(ID n)
    {
	    m_tempMarks.erase(n);
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    bool DependencySolver<NodeIdT, MaxDepPerNode>::ExistsNodesWithoutPermanentMark() const
    {
	    return m_permMarks.size() < m_nodes.size();
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    typename DependencySolver<NodeIdT, MaxDepPerNode>::ID DependencySolver<NodeIdT, MaxDepPerNode>::SelectUnmarked()
    {
        // assumes not empty: Solve() only calls this while ExistsNodesWithoutPermanentMark() holds.
        //
        // The cursor makes this linear over the life of one Solve(). Marks are monotonic -- Visit() only ever adds a
        // permanent mark and nothing clears one mid-solve -- so every node behind the cursor is marked for good and
        // never needs looking at again. A node marked ahead of the cursor by a recursive Visit is skipped when the
        // cursor reaches it. Previously this restarted from m_order.begin() on every call, which made Solve()
        // quadratic in the symbol count: measured at ~380 ms of a ~985 ms AZSLc run on a Material Canvas shader.
        while (m_selectCursor < m_order.size() && HasPermMark(m_order[m_selectCursor]))
        {
            ++m_selectCursor;
        }
        return m_order[m_selectCursor];
    }
}

#ifndef NDEBUG
namespace
{
    using ID = enum I : int {};
    ID operator""_id (unsigned long long i) { return ID(i); }
}

namespace AZ::Tests
{
    inline void DoAsserts6()
    {
		using namespace AZ::ShaderCompiler;
        DependencySolver<ID, 3_maxdep_pernode> ds;

        ds.AddNode(0_id);
        ds.AddDependency(0_id, 4_id);
        ds.AddDependency(0_id, 3_id);
        ds.AddNode(3_id);
        ds.AddNode(2_id);
        ds.AddNode(1_id);
        ds.AddNode(4_id);
        ds.AddNode(5_id);
        ds.AddDependency(5_id, 0_id);
        ds.AddNode(6_id);
        ds.AddDependency(6_id, 5_id);
        ds.AddNode(7_id);
        ds.AddDependency(7_id, 6_id);
        ds.AddDependency(7_id, 5_id);
        ds.AddNode(8_id);
        ds.AddDependency(8_id, 6_id);
        ds.AddNode(9_id);
        ds.AddDependency(9_id, 6_id);
        ds.AddNode(10_id);
        ds.AddDependency(10_id, 8_id);
        ds.AddDependency(10_id, 9_id);
        ds.AddDependency(10_id, 3_id);

        try
        {
            ds.Solve();
            assert(ds.CheckAllDepSatisfied());
        }
        catch (exception&)
        {
            assert(false);
        }
    }
}
#endif

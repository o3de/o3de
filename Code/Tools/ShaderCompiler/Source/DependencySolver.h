/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <iterator>

namespace AZ::ShaderCompiler
{
    enum EasyToReadMaxDepCountInteger : size_t
    {};

    constexpr EasyToReadMaxDepCountInteger operator ""_maxdep_pernode(unsigned long long n)
    {
        return EasyToReadMaxDepCountInteger(n);
    }

    namespace detail
    {
        template< typename T, size_t N >
        struct ElasticArray : array<T, N>
        {
            using Base = array<T, N>;
            using Base::array;
            using Base::operator=;
            using Base::operator[];
            using Base::begin;

            auto end() -> decltype(Base::end());
            auto end() const -> decltype(Base::end());
            auto at(size_t n) -> decltype(Base::at(n));
            auto at(size_t n) const -> decltype(Base::at(n));
            T& push_back(const T& t);

            size_t m_end = 0;
        };

        template< typename ID, EasyToReadMaxDepCountInteger MaxDep >
        struct DependencySolverNode
        {
            void PushDep(ID d);
            bool DependsOn(ID d) const;

            // memory-inlined array to diminish fragmentatioon
            static constexpr size_t MaxDependencies = MaxDep;
            ElasticArray<ID, MaxDependencies> m_dependencies;
        };
    }

    template< typename NodeIdT, EasyToReadMaxDepCountInteger MaxDepPerNode >
    class DependencySolver
    {
    public:
        using ID = NodeIdT;
        using Node = detail::DependencySolverNode<ID, MaxDepPerNode>;

        void AddDependency(ID node, ID toDependency);

        void AddNode(ID node);

        bool CheckAllDepSatisfied();

        void Solve();

        bool Has(ID node) const;

        unordered_map< ID, Node > m_nodes;
        vector< ID > m_order;

    private:
        // Tarjan Topological-Sort 1976
        void Visit(ID curNode);

        void ClearAllTemps();
        void PermMark(ID n);
        void TempMark(ID n);
        bool HasPermMark(ID n) const;
        bool HasTempMark(ID n) const;
        void RemoveTempMark(ID n);
        bool ExistsNodesWithoutPermanentMark() const;
        // get any node that is not marked
        ID SelectUnmarked();

        unordered_set<ID> m_permMarks;
        unordered_set<ID> m_tempMarks;
        vector<ID> m_result; // L
    };
}

#ifndef NDEBUG
namespace AZ::Tests
{
    void DoAsserts6();
}
#endif

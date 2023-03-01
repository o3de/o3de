/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AzToolsFramework
{
    class SliceDependencyBrowserComponent;

    class SliceRelationshipNode
    {
    public:

        friend class SliceDependencyBrowserComponent;

        //////////////////////////////////////////////////////////////////////////
        /*!
        * Hashing and comparison functions for unordered sets of Slice relationship nodes
        */
        struct HashSliceRelationshipNodeKey
        {
            AZStd::size_t operator()(const AZStd::shared_ptr<SliceRelationshipNode>& node) const
            {
                return static_cast<AZStd::size_t>(node->m_relativePathCrc);
            }
        };

        struct HashSliceRelationshipNodeComparator
        {
            bool operator()(const AZStd::shared_ptr<SliceRelationshipNode>& left, const AZStd::shared_ptr<SliceRelationshipNode>& right) const
            {
                return (left->m_relativePathCrc == right->m_relativePathCrc);
            }
        };

        using SliceRelationshipNodeSet = AZStd::unordered_set<AZStd::shared_ptr<SliceRelationshipNode>, HashSliceRelationshipNodeKey, HashSliceRelationshipNodeComparator>;

        SliceRelationshipNode(const AZStd::string& sliceRelativePath);
        SliceRelationshipNode(const AZStd::string& sliceRelativePath, AZ::Crc32 relativePathCrc);

        /**
        * \brief Adds a dependent to this Relationship node
        * \param AZStd::shared_ptr<SliceRelationshipNode> dependent The dependent node
        * \return bool true if a dependent was added false otherwise
        */
        bool AddDependent(const AZStd::shared_ptr<SliceRelationshipNode>& dependent);

        /**
        * \brief Adds a dependency to this Relationship node
        * \param AZStd::shared_ptr<SliceRelationshipNode> dependency The dependency node
        * \return bool true if a dependency was added false otherwise
        */
        bool AddDependency(const AZStd::shared_ptr<SliceRelationshipNode>& dependency);

        /**
        * \brief Returns the set of all dependent slice relationship nodes
        * \return const AzToolsFramework::SliceRelationshipNode::SliceRelationshipNodeSet& The set of all dependent slice relationship nodes
        */
        const SliceRelationshipNodeSet& GetDependents() const;

        /**
        * \brief Returns the set of all slice relationship nodes that this one depends on
        * \return const AzToolsFramework::SliceRelationshipNode::SliceRelationshipNodeSet& The set of all slice relationship nodes that this one depends on
        */
        const SliceRelationshipNodeSet& GetDependencies() const;

        /**
        * \brief Returns the relative path of this slice
        * \return const AZStd::string& Relative path of this slice
        */
        const AZStd::string& GetSliceRelativePath() const;

        /**
        * \brief Returns a Crc to the relative path of this slice
        * \return const AzToolsFramework::AZ::Crc32&
        */
        const AZ::Crc32& GetRelativePathCrc() const;

    private:

        //! Relative path to this slice
        AZStd::string m_sliceRelativePath;

        //! Crc32 of m_sliceRelativePath
        AZ::Crc32 m_relativePathCrc;

        //! Pointers to dependents and dependencies
        SliceRelationshipNodeSet m_dependents;
        SliceRelationshipNodeSet m_dependencies;
    };
}

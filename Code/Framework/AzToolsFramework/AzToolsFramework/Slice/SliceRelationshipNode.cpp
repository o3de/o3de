/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SliceRelationshipNode.h"

namespace AzToolsFramework
{
    SliceRelationshipNode::SliceRelationshipNode(const AZStd::string& sliceRelativePath)
        : m_sliceRelativePath(sliceRelativePath)
        , m_relativePathCrc(AZ::Crc32(sliceRelativePath.c_str(), sliceRelativePath.size(),true))
    {}

    SliceRelationshipNode::SliceRelationshipNode(const AZStd::string& sliceRelativePath, AZ::Crc32 relativePathCrc)
        : m_sliceRelativePath(sliceRelativePath)
        , m_relativePathCrc(relativePathCrc)
    {}

    bool SliceRelationshipNode::AddDependent(const AZStd::shared_ptr<SliceRelationshipNode>& dependent)
    {
        return (m_dependents.insert(dependent)).second;
    }

    bool SliceRelationshipNode::AddDependency(const AZStd::shared_ptr<SliceRelationshipNode>& dependency)
    {
        return (m_dependencies.insert(dependency)).second;
    }

    const SliceRelationshipNode::SliceRelationshipNodeSet& SliceRelationshipNode::GetDependents() const
    {
        return m_dependents;
    }

    const SliceRelationshipNode::SliceRelationshipNodeSet& SliceRelationshipNode::GetDependencies() const
    {
        return m_dependencies;
    }

    const AZStd::string& SliceRelationshipNode::GetSliceRelativePath() const
    {
        return m_sliceRelativePath;
    }

    const AZ::Crc32& SliceRelationshipNode::GetRelativePathCrc() const
    {
        return m_relativePathCrc;
    }

} // namespace AzToolsFramework


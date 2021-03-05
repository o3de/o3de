/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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


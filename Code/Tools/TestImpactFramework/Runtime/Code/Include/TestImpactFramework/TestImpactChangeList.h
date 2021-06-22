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

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Representation of the file CRUD operations of a given set of source changes.
    struct ChangeList
    {
        AZStd::vector<RepoPath> m_createdFiles; //!< Files that were newly created.
        AZStd::vector<RepoPath> m_updatedFiles; //!< Files that were updated.
        AZStd::vector<RepoPath> m_deletedFiles; //!< Files that were deleted.
    };
} // namespace TestImpact

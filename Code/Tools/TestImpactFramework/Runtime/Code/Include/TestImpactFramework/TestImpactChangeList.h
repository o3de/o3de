/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

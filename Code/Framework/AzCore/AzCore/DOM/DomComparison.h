/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPatch.h>

namespace AZ::Dom
{
    //! A set of patches for applying a change and doing the inverse operation.
    struct PatchUndoRedoInfo
    {
        Patch m_forwardPatches;
        Patch m_inversePatches;
    };

    //! Parameters for GenerateHierarchicalDeltaPatch.
    struct DeltaPatchGenerationParameters
    {
        static constexpr size_t NoReplace = AZStd::numeric_limits<size_t>::max();
        static constexpr size_t AlwaysFullReplace = 0;

        //! The threshold of changed values in a node or array which, if exceeded, will cause the generation to create an
        //! entire "replace" oepration instead. If set to NoReplace, no replacement will occur.
        size_t m_replaceThreshold = 3;
    };

    //! Generates a set of patches such that m_forwardPatches.Apply(beforeState) shall produce a document equivalent to afterState, and
    //! a subsequent m_inversePatches.Apply(beforeState) shall produce the original document. This patch generation strategy does a
    //! hierarchical comparison and is not guaranteed to create the minimal set of patches required to transform between the two states.
    PatchUndoRedoInfo GenerateHierarchicalDeltaPatch(const Value& beforeState, const Value& afterState, const DeltaPatchGenerationParameters& params = {});
} // namespace AZ::Dom

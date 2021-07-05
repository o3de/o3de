/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <EMotionFX/Source/AnimGraphNode.h>


namespace CommandSystem
{
    class AnimGraphCopyPasteData
    {
    public:
        /**
         * Get the name for the given node after the copy or cut&paste operation.
         */
        AZStd::string GetNewNodeName(EMotionFX::AnimGraphNode* node, bool cutMode) const;
        AZStd::unordered_map<EMotionFX::AnimGraphNode*, AZStd::string> m_newNamesByCopiedNodes;

        /**
         * Get the connection id for a given connection  after the copy or cut&paste operation.
         * @param[in] connectionId The id of a connection that is present in the anim graph before the copy or cut&paste operation.
         * @result The connection id of the given connection after the copy or cut&paste operation.
         */
        EMotionFX::AnimGraphConnectionId GetNewConnectionId(const EMotionFX::AnimGraphConnectionId& connectionId, bool cutMode);

    private:
        AZStd::unordered_map</*old*/ EMotionFX::AnimGraphConnectionId, /*new*/ EMotionFX::AnimGraphConnectionId> m_newConnectionIdsByOldId;
    };
} // namespace CommandSystem

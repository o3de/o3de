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

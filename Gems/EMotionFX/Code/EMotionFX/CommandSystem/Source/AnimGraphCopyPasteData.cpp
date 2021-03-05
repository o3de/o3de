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

#include <EMotionFX/CommandSystem/Source/AnimGraphCopyPasteData.h>

namespace CommandSystem
{
    AZStd::string AnimGraphCopyPasteData::GetNewNodeName(EMotionFX::AnimGraphNode* node, bool cutMode) const
    {
        if (!node)
        {
            return {};
        }

        if (!cutMode)
        {
            auto itNodeRenamed = m_newNamesByCopiedNodes.find(node);
            if (itNodeRenamed != m_newNamesByCopiedNodes.end())
            {
                return itNodeRenamed->second;
            }
        }

        return node->GetNameString();
    }

    EMotionFX::AnimGraphConnectionId AnimGraphCopyPasteData::GetNewConnectionId(const EMotionFX::AnimGraphConnectionId& connectionId, bool cutMode)
    {
        // Check if the connection id has already been registered and return the new one.
        auto connectionIter = m_newConnectionIdsByOldId.find(connectionId);
        if (connectionIter != m_newConnectionIdsByOldId.end())
        {
            return connectionIter->second;
        }

        EMotionFX::AnimGraphConnectionId newId;
        if (cutMode)
        {
            // Keep the connection id the same when using cut & paste.
            newId = connectionId;
        }
        else
        {
            // Create a new connection id when using copy & paste.
            newId = EMotionFX::AnimGraphConnectionId::Create();
        }

        m_newConnectionIdsByOldId.emplace(connectionId, newId);
        return newId;
    }
} // namespace CommandSystem

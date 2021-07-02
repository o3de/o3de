/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

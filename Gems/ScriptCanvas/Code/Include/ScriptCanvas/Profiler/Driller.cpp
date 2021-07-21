/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    namespace Profiler
    {
        void Driller::OnNodeExecute(Node* node)
        {
            m_output->BeginTag(AZ_CRC("ScriptCanvasGraphDriller", 0xb161ccb2));
            {
                m_output->BeginTag(AZ_CRC("OnNodeExecute", 0x3e51a5eb));
                {
                    m_output->Write(AZ_CRC("NodeName", 0x606d4587), node->GetEntity()->GetName());
                    m_output->Write(AZ_CRC("NodeType", 0xb2906ca8), node->RTTI_GetTypeName());
                }
                m_output->EndTag(AZ_CRC("StringEvent", 0xd1e005df));
            }
            m_output->EndTag(AZ_CRC("ScriptCanvasGraphDriller", 0xb161ccb2));
        }
    }
}

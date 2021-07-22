/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Deprecated/VariableHelpers.h>

#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    namespace Deprecated
    {
        /////////////////
        // VariableInfo
        /////////////////

        void VariableInfo::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<VariableInfo>()
                    ->Version(0)
                    ->Field("ActiveVariableId", &VariableInfo::m_currentVariableId)
                    ->Field("NodeVariableId", &VariableInfo::m_ownedVariableId)
                    ->Field("DataType", &VariableInfo::m_dataType)
                    ;
            }
        }

        VariableInfo::VariableInfo(const VariableId& nodeOwnedVarId)
            : m_currentVariableId(nodeOwnedVarId)
            , m_ownedVariableId(nodeOwnedVarId)
        {
            //VariableRequestBus::EventResult(m_dataType, m_currentVariableId, &VariableRequests::GetType);
        }

        VariableInfo::VariableInfo(const Data::Type& dataType)
            : m_dataType(dataType)
        {
        }
    }
}

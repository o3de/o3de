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
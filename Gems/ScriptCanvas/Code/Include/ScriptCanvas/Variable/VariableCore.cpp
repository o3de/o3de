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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <ScriptCanvas/Variable/VariableCore.h>

namespace ScriptCanvas
{
    VariableId VariableId::MakeVariableId()
    {
        return VariableId(AZ::Uuid::CreateRandom());
    }

    void VariableId::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VariableId>()
                ->Field("m_id", &VariableId::m_id)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<VariableId>("Variable Id", "Uniquely identifies a datum. This Id can be used to address the VariableRequestBus")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Variable Id")
                    ;
            }
        }
    }
}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

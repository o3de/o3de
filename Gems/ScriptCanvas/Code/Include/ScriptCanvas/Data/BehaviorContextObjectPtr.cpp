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

#include "BehaviorContextObjectPtr.h"

#include <AzCore/RTTI/ReflectContext.h>
#include "BehaviorContextObject.h"

namespace ScriptCanvas
{
    void BehaviorContextObjectPtrReflect(AZ::ReflectContext* context)
    {
        BehaviorContextObject::Reflect(context);
        auto behaviorContextObjectPtrGenericClassInfo = AZ::SerializeGenericTypeInfo<BehaviorContextObjectPtr>::GetGenericInfo();
        if (!behaviorContextObjectPtrGenericClassInfo)
        {
            return;
        }

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            behaviorContextObjectPtrGenericClassInfo->Reflect(serializeContext);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BehaviorContextObjectPtr>("BehaviorContextObjectPtr", "Intrusive pointer which keeps a count of ScriptCanvas references to data from the BehaviorContext")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }

        }
    }
}
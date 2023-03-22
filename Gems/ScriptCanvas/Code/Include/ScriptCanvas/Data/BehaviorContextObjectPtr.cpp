/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BehaviorContextObjectPtr.h"

#include <AzCore/Serialization/EditContext.h>
#include <ScriptCanvas/Data/BehaviorContextObject.h>

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

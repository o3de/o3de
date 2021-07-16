/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Data/DataTrait.h>
#include <ScriptCanvas/Data/BehaviorContextObject.h>

namespace ScriptCanvas
{
    namespace Data
    {
        eTraits<eType::BehaviorContextObject>::Type eTraits<eType::BehaviorContextObject>::GetDefault(const Data::Type& scType)
        {
            return BehaviorContextObject::CreateReference(scType.GetAZType());
        }

        bool ScriptCanvas::Data::eTraits<eType::BehaviorContextObject>::IsDefault(const Type& value, const Data::Type&)
        {
            return value->Get() == nullptr;
        }

    } 
}

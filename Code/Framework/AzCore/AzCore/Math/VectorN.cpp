/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/VectorN.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/MathScriptHelpers.h>

namespace AZ
{
    void VectorN::Reflect([[maybe_unused]] ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<VectorN>()
                ->Version(1)
                ->Field("NumElements", &VectorN::m_numElements)
                ->Field("Values", &VectorN::m_values)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<VectorN>("N-Dimensional Vector", "")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        Edit::UIHandlers::Default, &VectorN::m_numElements, "Total Elements", "The total number of elements in the vector")
                            ->Attribute(Edit::Attributes::ChangeNotify, &VectorN::OnSizeChanged)
                    ;
            }
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<VectorN>()->
                Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Common)->
                Attribute(Script::Attributes::Module, "math")->
                Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Constructor<AZStd::size_t>()->
                Constructor<AZStd::size_t, float>()->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)
                ;
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Utils.h>
#include <ScriptCanvas/SystemComponent.h>

#include "ExecutionObjectCloning.h"

namespace ScriptCanvas
{
    namespace Execution
    {
        CloneSource::CloneSource(const AZ::BehaviorClass& bcClass, void* source)
            : m_class(&bcClass)
            , m_source(source)
        {
            AZ_Assert(source, "null source added to clone source");
        }

        CloneSource::Result CloneSource::Clone() const
        {
            void* clone = m_class->Allocate();
            m_class->m_cloner(clone, m_source, m_class->m_userData);
            return { clone, m_class->m_typeId };
        }
    }
}

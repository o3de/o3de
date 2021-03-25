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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Object.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/std/parallel/atomic.h>

namespace AZ
{
    namespace RHI
    {
        void Object::SetName(const Name& name)
        {
            m_name = name;
            SetNameInternal(m_name.GetStringView());
        }

        void Object::SetNameInternal(const AZStd::string_view& name)
        {
            (void)name;
        }

        const Name& Object::GetName() const
        {
            return m_name;
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>

namespace ScriptCanvas
{
    class RuntimeContext
    {
    public:
        RuntimeContext(AZ::EntityId graphId);      
        
        AZ_INLINE AZ::EntityId GetGraphId() const { return m_graphId; }

    protected:
        AZ::EntityId m_graphId;
    };
}

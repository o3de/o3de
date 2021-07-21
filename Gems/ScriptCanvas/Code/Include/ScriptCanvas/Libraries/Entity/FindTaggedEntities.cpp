/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <ScriptCanvas/Libraries/Entity/FindTaggedEntities.h>

#include <LmbrCentral/Scripting/TagComponentBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Entity
        {
            void FindTaggedEntities::OnInputSignal(const SlotId&)
            {
                AZ::Crc32 tagEntity = WhenEntityActiveProperty::GetCrc32(this);
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Entity/FindTaggedEntities.generated.cpp>

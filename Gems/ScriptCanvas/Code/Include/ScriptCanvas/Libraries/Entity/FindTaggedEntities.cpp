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
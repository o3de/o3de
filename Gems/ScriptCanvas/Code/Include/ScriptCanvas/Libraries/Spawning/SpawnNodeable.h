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

#pragma once

#include <Include/ScriptCanvas/Libraries/Spawning/SpawnNodeable.generated.h>

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Spawning
        {
            class SpawnNodeable
                : public ScriptCanvas::Nodeable
            {
                SCRIPTCANVAS_NODE(SpawnNodeable);
            public:
                SpawnNodeable();

                SpawnNodeable(const SpawnNodeable& rhs);

            private:
                AZ::Data::Asset<AzFramework::Spawnable> m_spawnableAsset;
                AzFramework::EntitySpawnTicket m_spawnTicket;
            };
        }
    }
}

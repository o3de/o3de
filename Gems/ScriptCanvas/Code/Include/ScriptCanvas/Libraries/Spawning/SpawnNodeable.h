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

#include <AzCore/Component/TickBus.h>

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    class SpawnNodeable
        : public ScriptCanvas::Nodeable,
            public AZ::TickBus::Handler
    {
        SCRIPTCANVAS_NODE(SpawnNodeable);
    public:
        SpawnNodeable() = default;
        SpawnNodeable(const SpawnNodeable& rhs);
        SpawnNodeable& operator=(SpawnNodeable& rhs);

        void OnInitializeExecutionState() override;
        void OnDeactivate() override;

        //TickBus
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;

        void OnSpawnAssetChanged();

    private:
        AzFramework::EntitySpawnTicket m_spawnTicket;

        AZStd::vector<Data::EntityIDType> m_spawnedEntityList;
        AZStd::vector<size_t> m_spawnBatchSizes;
        AZStd::recursive_mutex m_idBatchMutex;
    };
}

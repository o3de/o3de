// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

// IMPORTANT!
// Copy+paste the fully auto-generated component override code into this header file from Source/AutoGen/${SanitizedCppName}.AutoComponent.h
#include <Source/AutoGen/${SanitizedCppName}.AutoComponent.h>

namespace ${GemName}
{
    class ${SanitizedCppName} : public ${SanitizedCppName}Base
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(${GemName}::${SanitizedCppName}, s_${SanitizedCppNameLowerFirst}ConcreteUuid, ${GemName}::${SanitizedCppName}Base);

        static void Reflect(AZ::ReflectContext* context);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
    };

    class ${SanitizedCppName}Controller : public ${SanitizedCppName}ControllerBase
    {
    public:
        explicit ${SanitizedCppName}Controller(${SanitizedCppName}& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
    };
} // namespace ${GemName}

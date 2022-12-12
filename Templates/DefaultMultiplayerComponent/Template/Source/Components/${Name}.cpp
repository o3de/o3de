// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

// IMPORTANT!
// Copy+paste the fully auto-generated component override code into this cpp file from Source/AutoGen/${SanitizedCppName}.AutoComponent.h

#include <Source/Components/${SanitizedCppName}.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ${GemName}
{
    void ${SanitizedCppName}::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<${SanitizedCppName}, ${SanitizedCppName}Base>()->Version(1);
        }
        ${SanitizedCppName}Base::Reflect(context);
    }

    void ${SanitizedCppName}::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void ${SanitizedCppName}::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    ${SanitizedCppName}Controller::${SanitizedCppName}Controller(${SanitizedCppName}& parent)
        : ${SanitizedCppName}ControllerBase(parent)
    {
    }

    void ${SanitizedCppName}Controller::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void ${SanitizedCppName}Controller::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }
} // namespace ${GemName}

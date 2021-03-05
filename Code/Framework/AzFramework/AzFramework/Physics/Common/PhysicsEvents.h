/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/Event.h>

namespace AzPhysics
{
    struct SystemConfiguration;
    struct SceneConfiguration;

    namespace SystemEvents
    {
        //! Event that triggers when the physics system configuration has been changed.
        //! When triggered the event will send the newly applied SystemConfiguration object.
        using OnConfigurationChangedEvent = AZ::Event<const SystemConfiguration*>;

        //! Event triggers when the physics system has completed initialization.
        //! When triggered the event will send the SystemConfiguration used to initialize the system.
        using OnInitializedEvent = AZ::Event<const SystemConfiguration*>;

        //! Event triggers when the physics system has completed reinitialization.
        using OnReinitializedEvent = AZ::Event<>;

        //! Event triggers when the physics system has completed its shutdown. 
        using OnShutdownEvent = AZ::Event<>;

        //! Event triggers at the beginning of the SystemInterface::Simulate call.
        //! Parameter is the total time that the physics system will run for during the Simulate call.
        using OnPresimulateEvent = AZ::Event<float>;

        //! Event triggers at the end of the SystemInterface::Simulate call.
        using OnPostsimulateEvent = AZ::Event<>;

        //! Event that triggers when the default material library changes.
        //! When triggered the event will send the Asset Id of the new material library.
        using OnDefaultMaterialLibraryChangedEvent = AZ::Event<const AZ::Data::AssetId&>;

        //! Event that triggers when the default scene configuration changes.
        //! When triggered the event will send the new default scene configuration.
        using OnDefaultSceneConfigurationChangedEvent = AZ::Event<const SceneConfiguration*>;
    }
}

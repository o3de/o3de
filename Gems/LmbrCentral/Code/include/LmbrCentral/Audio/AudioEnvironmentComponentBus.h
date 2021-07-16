/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /*!
     * AudioEnvironmentComponentRequests EBus Interface
     * Messages serviced by AudioEnvironmentComponents.
     * Environment = Used for effects, primarily auxillary effects bus sends.
     * See \ref AudioEnvironmentComponent for details.
     */
    class AudioEnvironmentComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //! Set an environment amount on the default assigned environment.
        virtual void SetAmount(float amount) = 0;

        //! Set an environment amount, specify an environment name at run-time (i.e. script).
        virtual void SetEnvironmentAmount(const char* environmentName, float amount) = 0;
    };

    using AudioEnvironmentComponentRequestBus = AZ::EBus<AudioEnvironmentComponentRequests>;

} // namespace LmbrCentral

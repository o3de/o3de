/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace OpenParticle
{
    class ParticleRequest
        : public AZ::ComponentBus
    {
    public:
        //! Triggers the particle system to start playing.
        virtual void Play() = 0;

        //! Pauses the particle system simulation.
        virtual void Pause() = 0;

        //! Stops the particle system simulation.
        virtual void Stop() = 0;

        //! Sets the visibility state of the particle system.
        //! @param visible  If true, the particle system will be rendered. If false, it will remain hidden until visibility is restored.
        virtual void SetVisibility(bool visible) = 0;

        //! Retrieves the current visibility state of the particle system.
        //! \return true if the particle system is visible; false otherwise.
        virtual bool GetVisibility() const = 0;
    };

    using ParticleRequestBus = AZ::EBus<ParticleRequest>;
}

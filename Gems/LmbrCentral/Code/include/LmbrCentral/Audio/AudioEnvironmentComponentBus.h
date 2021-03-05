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

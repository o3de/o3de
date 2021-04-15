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
#include <AzCore/EBus/Event.h>

namespace AZ
{
    class Vector3;

    using NonUniformScaleChangedEvent = AZ::Event<const AZ::Vector3&>;

    //! Requests for working with non-uniform scale.
    class NonUniformScaleRequests
        : public AZ::ComponentBus
    {
    public:
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        static void Reflect(AZ::ReflectContext* context);

        //! Gets the non-uniform scale.
        virtual AZ::Vector3 GetScale() const = 0;

        //! Sets the non-uniform scale.
        virtual void SetScale(const Vector3& scale) = 0;

        //! Registers a handler to be notified when the non-uniform scale is changed.
        virtual void RegisterScaleChangedEvent(NonUniformScaleChangedEvent::Handler& handler) = 0;
    };

    using NonUniformScaleRequestBus = AZ::EBus<NonUniformScaleRequests>;
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace ScriptCanvasEditor
{
    /*!
    System Component for managing the class reflections of Editor introduced types
    */
    class ReflectComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ReflectComponent, "{5F1D37D8-A72A-4C38-B7E7-6BBC90272F92}");

        ReflectComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //// AZ::Component
        void Activate();
        void Deactivate();
        ////
    };
}

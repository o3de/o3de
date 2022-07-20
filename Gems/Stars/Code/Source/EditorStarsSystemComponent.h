/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <StarsSystemComponent.h>

namespace AZ::Render
{
    class EditorStarsSystemComponent
        : public StarsSystemComponent
    {
        using BaseSystemComponent = StarsSystemComponent;
    public:
        AZ_COMPONENT(EditorStarsSystemComponent, "{346580C8-6084-4439-87B6-7DF1F734ED86}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorStarsSystemComponent() = default;
        ~EditorStarsSystemComponent() = default;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
    };
} // namespace Stars

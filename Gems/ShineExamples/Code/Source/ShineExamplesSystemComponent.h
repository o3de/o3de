/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <ShineExamples/ShineExamplesBus.h>
#include <ShineExamplesCppExample.h>
#include "ShineExamplesInternalBus.h"

namespace ShineExamples
{
    class ShineExamplesSystemComponent
        : public AZ::Component
        , protected ShineExamplesRequestBus::Handler
        , protected ShineExamplesInternalBus::Handler
    {
    public:
        AZ_COMPONENT(ShineExamplesSystemComponent, "{045500EA-BB1D-40CE-8811-F1DF6A340557}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ShineExamplesRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ShineExamplesInternalBus interface implementation
        UiDynamicContentDatabase* GetUiDynamicContentDatabase() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private: // data
        UiDynamicContentDatabase* m_uiDynamicContentDatabase = nullptr;
        ShineExamplesCppExample* m_cppExample = nullptr;
    };
}

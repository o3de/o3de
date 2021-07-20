/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <LyShineExamples/LyShineExamplesBus.h>
#include <LyShineExamplesCppExample.h>
#include "LyShineExamplesInternalBus.h"

namespace LyShineExamples
{
    class LyShineExamplesSystemComponent
        : public AZ::Component
        , protected LyShineExamplesRequestBus::Handler
        , protected LyShineExamplesInternalBus::Handler
    {
    public:
        AZ_COMPONENT(LyShineExamplesSystemComponent, "{045500EA-BB1D-40CE-8811-F1DF6A340557}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // LyShineExamplesRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // LyShineExamplesInternalBus interface implementation
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
        LyShineExamplesCppExample* m_cppExample = nullptr;
    };
}

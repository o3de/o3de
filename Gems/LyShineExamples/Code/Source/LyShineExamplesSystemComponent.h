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

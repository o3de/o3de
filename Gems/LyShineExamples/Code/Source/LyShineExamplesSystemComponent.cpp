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

#include "LyShineExamples_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "LyShineExamplesSystemComponent.h"
#include "LyShineExamplesSerialize.h"

#include "UiDynamicContentDatabase.h"
#include "LyShineExamplesCppExample.h"

namespace LyShineExamples
{
    void LyShineExamplesSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        LyShineExamplesSerialize::ReflectTypes(context);
        UiDynamicContentDatabase::Reflect(context);
        LyShineExamplesCppExample::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LyShineExamplesSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LyShineExamplesSystemComponent>("LyShineExamples", "This provides example code using LyShine and code used by sample UI canvases and levels")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "UI")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void LyShineExamplesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LyShineExamplesService"));
    }

    void LyShineExamplesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LyShineExamplesService"));
    }

    void LyShineExamplesSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("LyShineService"));;
    }

    void LyShineExamplesSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    UiDynamicContentDatabase* LyShineExamplesSystemComponent::GetUiDynamicContentDatabase()
    {
        return m_uiDynamicContentDatabase;
    }

    void LyShineExamplesSystemComponent::Init()
    {
    }

    void LyShineExamplesSystemComponent::Activate()
    {
        m_uiDynamicContentDatabase = new UiDynamicContentDatabase();
        m_cppExample = new LyShineExamplesCppExample();

        LyShineExamplesRequestBus::Handler::BusConnect();
        LyShineExamplesInternalBus::Handler::BusConnect();
    }

    void LyShineExamplesSystemComponent::Deactivate()
    {
        LyShineExamplesRequestBus::Handler::BusDisconnect();
        LyShineExamplesInternalBus::Handler::BusDisconnect();

        SAFE_DELETE(m_uiDynamicContentDatabase);
        SAFE_DELETE(m_cppExample);
    }
}

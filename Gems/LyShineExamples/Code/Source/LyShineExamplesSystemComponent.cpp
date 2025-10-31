/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void LyShineExamplesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("LyShineExamplesService"));
    }

    void LyShineExamplesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LyShineExamplesService"));
    }

    void LyShineExamplesSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("LyShineService"));;
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

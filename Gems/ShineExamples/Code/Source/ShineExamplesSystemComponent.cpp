/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "ShineExamplesSystemComponent.h"
#include "ShineExamplesSerialize.h"

#include "UiDynamicContentDatabase.h"
#include "ShineExamplesCppExample.h"

namespace ShineExamples
{
    void ShineExamplesSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ShineExamplesSerialize::ReflectTypes(context);
        UiDynamicContentDatabase::Reflect(context);
        ShineExamplesCppExample::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ShineExamplesSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ShineExamplesSystemComponent>("ShineExamples", "This provides example code using Shine and code used by sample UI canvases and levels")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "UI")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ShineExamplesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShineExamplesService"));
    }

    void ShineExamplesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShineExamplesService"));
    }

    void ShineExamplesSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("ShineService"));;
    }

    void ShineExamplesSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    UiDynamicContentDatabase* ShineExamplesSystemComponent::GetUiDynamicContentDatabase()
    {
        return m_uiDynamicContentDatabase;
    }

    void ShineExamplesSystemComponent::Init()
    {
    }

    void ShineExamplesSystemComponent::Activate()
    {
        m_uiDynamicContentDatabase = new UiDynamicContentDatabase();
        m_cppExample = new ShineExamplesCppExample();

        ShineExamplesRequestBus::Handler::BusConnect();
        ShineExamplesInternalBus::Handler::BusConnect();
    }

    void ShineExamplesSystemComponent::Deactivate()
    {
        ShineExamplesRequestBus::Handler::BusDisconnect();
        ShineExamplesInternalBus::Handler::BusDisconnect();

        SAFE_DELETE(m_uiDynamicContentDatabase);
        SAFE_DELETE(m_cppExample);
    }
}

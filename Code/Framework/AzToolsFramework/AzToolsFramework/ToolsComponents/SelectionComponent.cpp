/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzToolsFramework_precompiled.h"
#include "SelectionComponent.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace AzToolsFramework
{
    namespace Components
    {
        SelectionComponent::SelectionComponent()
        {
            m_currentSelectionFlag = SF_Unselected;
            m_currentSelectionAABB = AZ::Aabb::CreateNull();
        }

        SelectionComponent::~SelectionComponent()
        {
        }

        void SelectionComponent::Init()
        {
        }

        void SelectionComponent::UpdateBounds(const AZ::Aabb& newBounds)
        {
            m_currentSelectionAABB = newBounds;
        }

        void SelectionComponent::Activate()
        {
            SelectionComponentMessages::Bus::Handler::BusConnect(GetEntityId());
            AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        }

        void SelectionComponent::Deactivate()
        {
            AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect(GetEntityId());
            SelectionComponentMessages::Bus::Handler::BusDisconnect(GetEntityId());

            m_currentSelectionFlag = SF_Unselected;
        }

        void SelectionComponent::OnSelected()
        {
            m_currentSelectionFlag = SF_Selected;
        }

        void SelectionComponent::OnDeselected()
        {
            m_currentSelectionFlag = SF_Unselected;
        }

        bool SelectionComponent::IsSelected() const
        {
            return (m_currentSelectionFlag & SF_Selected) != 0;
        }

        bool SelectionComponent::IsPrimarySelection() const
        {
            return ((m_currentSelectionFlag & SF_Selected) != 0) && ((m_currentSelectionFlag & SF_Primary) != 0);
        }
        void SelectionComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

            // reflect data for script, serialization, editing...
            if (serializeContext)
            {
                serializeContext->Class<SelectionComponent, AZ::Component>()
                ;

                AZ::EditContext* ptrEdit = serializeContext->GetEditContext();
                if (ptrEdit)
                {
                    ptrEdit->Class<SelectionComponent>("Selection", "Indicates whether the object is selected or not")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_Hide", 0x32ab90f7))     // no point in showing this in the property inspector, its for internal use.
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true);
                }
            }
        }

        void SelectionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EditorSelectionService", 0x03ef9aae));
        }
    } // namespace Components
} // namespace AzToolsFramework

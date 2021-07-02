/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef SELECTION_COMPONENT_H_INC
#define SELECTION_COMPONENT_H_INC

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Aabb.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponentBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#pragma once

namespace AzToolsFramework
{
    namespace Components
    {
        class SelectionComponent
            : public EditorComponentBase
            , private SelectionComponentMessages::Bus::Handler
            , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        {
        public:

            friend class SelectionComponentFactory;

            AZ_COMPONENT(SelectionComponent, "{73B724FC-43D1-4C75-ACF5-79AA8A3BF89D}", EditorComponentBase)

            SelectionComponent();
            virtual ~SelectionComponent();

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component overrides
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            bool IsSelected() const;
            bool IsPrimarySelection() const;
            void UpdateBounds(const AZ::Aabb& newBounds);

        private:

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void Reflect(AZ::ReflectContext* context);

            AZ::Aabb m_currentSelectionAABB;
            AZ::u32 m_currentSelectionFlag;

            //////////////////////////////////////////////////////////////////////////
            // EntitySelectionEvents::Bus::Handler overrides
            void OnSelected() override;
            void OnDeselected() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // SelectionComponentMessages::Bus
            AZ::Aabb GetSelectionBound() override { return m_currentSelectionAABB; }
            //////////////////////////////////////////////////////////////////////////
        };
    }
} // namespace AzToolsFramework

#endif

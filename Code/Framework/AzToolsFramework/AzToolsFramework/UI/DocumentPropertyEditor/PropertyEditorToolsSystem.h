/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystem.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystemInterface.h>

namespace AzToolsFramework
{
    //! Implementation of PropertyEditorToolsSystemInterface, 
    class PropertyEditorToolsSystem : public PropertyEditorToolsSystemInterface
    {
    public:
        AZ_RTTI(PropertyEditorToolsSystem, "{78954B5C-D147-411F-BBDA-A28D2CA50A3A}", PropertyEditorToolsSystemInterface);
        AZ_CLASS_ALLOCATOR(PropertyEditorToolsSystem, AZ::OSAllocator);

        PropertyEditorToolsSystem();
        ~PropertyEditorToolsSystem() override;

        void RegisterDefaultHandlers();

        PropertyHandlerId GetPropertyHandlerForNode(const AZ::Dom::Value node) override;
        PropertyHandlerInstance CreateHandlerInstance(PropertyHandlerId handlerId) override;
        PropertyHandlerId RegisterHandler(HandlerData handlerData) override;
        void UnregisterHandler(PropertyHandlerId handlerId) override;

    private:
        AZStd::unordered_map<AZ::Name, AZStd::list<HandlerData>> m_registeredHandlers;
        AZStd::vector<PropertyHandlerId> m_defaultHandlers;
        // PropertyEditorSystem contains all non-UI system logic for the DPE, like the DOM schema
        AZ::DocumentPropertyEditor::PropertyEditorSystem m_lowLevelSystem;

        PropertyHandlerId GetPropertyHandlerForType(AZStd::string_view handlerName, const AZ::TypeId& typeId);
    };
} // namespace AzToolsFramework

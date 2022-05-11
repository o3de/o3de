/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>

namespace AzToolsFramework
{
    class PropertyEditorToolsSystemInterface
    {
    public:
        AZ_RTTI(PropertyEditorToolsSystemInterface, "{4E45141B-D612-4DCB-A644-A874EE9A7A52}");

        virtual ~PropertyEditorToolsSystemInterface() = default;

        using PropertyHandlerInstance = AZStd::unique_ptr<PropertyHandlerWidgetInterface>;
        using PropertyHandlerInstanceFactory = AZStd::function<PropertyHandlerInstance()>;

        struct HandlerData
        {
            AZStd::string_view m_name;
            PropertyHandlerInstanceFactory m_factory;
            AZStd::function<bool(const AZ::Dom::Value&)> m_shouldHandleNode;
        };
        using PropertyHandlerId = HandlerData*;
        static constexpr PropertyHandlerId InvalidHandlerId = nullptr;

        virtual PropertyHandlerId GetPropertyHandlerForNode(const AZ::Dom::Value node) = 0;
        virtual PropertyHandlerInstance CreateHandlerInstance(PropertyHandlerId handlerId) = 0;
        virtual PropertyHandlerId RegisterHandler(HandlerData handlerData) = 0;
        virtual void UnregisterHandler(PropertyHandlerId handlerId) = 0;

        template <typename HandlerType>
        void RegisterHandler()
        {
            HandlerData handlerData;
            handlerData.m_name = HandlerType::GetHandlerName();
            handlerData.m_shouldHandleNode = [](const AZ::Dom::Value& node) -> bool
            {
                return HandlerType::ShouldHandleNode(node);
            };
            handlerData.m_factory = []()
            {
                return AZStd::make_unique<HandlerType>();
            };
            RegisterHandler(AZStd::move(handlerData));
        }
    };
}

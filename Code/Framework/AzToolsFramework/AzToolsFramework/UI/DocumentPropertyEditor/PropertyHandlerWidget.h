/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/DOM/DomValue.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystemInterface.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <QPointer>
#include <QWidget>
#endif // defined(Q_MOC_RUN)

namespace AzToolsFramework
{
    class PropertyHandlerWidgetInterface
    {
    public:
        virtual ~PropertyHandlerWidgetInterface() = default;
        virtual QWidget* GetWidget() = 0;
        virtual void SetValueFromDom(const AZ::Dom::Value& node) = 0;

        static bool CanHandleNode([[maybe_unused]] const AZ::Dom::Value& node)
        {
            return true;
        }

        static const AZStd::string_view GetHandlerName()
        {
            return "<undefined handler name>";
        }

        enum class ValueChangeType
        {
            InProgressEdit,
            FinishedEdit,
        };
        using ValueChangedHandler = AZStd::function<void(const AZ::Dom::Value&, ValueChangeType)>;
        void SetValueChangedHandler(ValueChangedHandler handler);

    protected:
        void ValueChangedByUser(const AZ::Dom::Value& newValue, ValueChangeType changeType = ValueChangeType::FinishedEdit);

    private:
        ValueChangedHandler m_handler;
    };

    template<typename BaseWidget>
    class PropertyHandlerWidget
        : public BaseWidget
        , public PropertyHandlerWidgetInterface
    {
    public:
        PropertyHandlerWidget(QWidget* parent = nullptr)
            : BaseWidget(parent)
        {
        }

        QWidget* GetWidget() override
        {
            return this;
        }
    };

    template<typename RpePropertyHandler, typename WrappedType>
    class RpePropertyHandlerWrapper
        : public PropertyHandlerWidgetInterface
        , public PropertyEditorGUIMessages::Bus::Handler
    {
    public:
        RpePropertyHandlerWrapper()
        {
            m_proxyNode.m_classData = &m_proxyClassData;
            m_proxyClassData.m_typeId = azrtti_typeid<WrappedType>();
            m_proxyNode.m_instances.push_back(&m_proxyValue);

            // We're creating a lot of bus instances here
            // If this is slow, we should look into a single handler that does a lookup
            // e.g. by setting a property on the widget created with GetWidget
            PropertyEditorGUIMessages::Bus::Handler::BusConnect();
        }

        ~RpePropertyHandlerWrapper()
        {
            if (m_widget)
            {
                delete m_widget;
            }
        }

        static PropertyHandlerBase& GetRpeHandler()
        {
            static RpePropertyHandler handler;
            return handler;
        }

        QWidget* GetWidget() override
        {
            if (m_widget)
            {
                return m_widget;
            }
            m_widget = GetRpeHandler().CreateGUI(nullptr);
            return m_widget;
        }

        void SetValueFromDom(const AZ::Dom::Value& node)
        {
            // TODO: read attributes into m_proxyNode

            AZ::Dom::Value value = AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Value.ExtractFromDomNode(node).value();
            m_proxyValue = AZ::Dom::Utils::ValueToType<WrappedType>(value).value();
            GetRpeHandler().ReadValuesIntoGUI_Internal(GetWidget(), &m_proxyNode);
        }

        static bool CanHandleNode(const AZ::Dom::Value& node)
        {
            using namespace AZ::DocumentPropertyEditor::Nodes;
            auto typeId = PropertyEditor::ValueType.ExtractFromDomNode(node);
            if (!typeId.has_value())
            {
                AZ::Dom::Value value = PropertyEditor::Value.ExtractFromDomNode(node).value_or({});
                typeId = &AZ::Dom::Utils::GetValueTypeId(value);
            }
            return GetRpeHandler().HandlesType(*typeId.value());
        }

        static const AZStd::string_view GetHandlerName()
        {
            auto propertyEditorSystem = AZ::Interface<AZ::DocumentPropertyEditor::PropertyEditorSystemInterface>::Get();
            AZ_Assert(
                propertyEditorSystem != nullptr,
                "RpePropertyHandlerWrapper::GetHandlerName called with an uninitialized PropertyEditorSystemInterface");
            if (propertyEditorSystem == nullptr)
            {
                return {};
            }
            return propertyEditorSystem->LookupNameFromId(GetRpeHandler().GetHandlerName()).GetStringView();
        }

        void RequestWrite(QWidget* editorGUI)
        {
            if (editorGUI == m_widget)
            {
                ValueChangedByUser(AZ::Dom::Utils::ValueFromType(m_proxyValue), ValueChangeType::InProgressEdit);
            }
        }

        void RequestRefresh(PropertyModificationRefreshLevel)
        {
        }

        void AddElementsToParentContainer(
            QWidget*, size_t, const InstanceDataNode::FillDataClassCallback&)
        {
        }

        void RequestPropertyNotify(QWidget*)
        {
        }

        void OnEditingFinished(QWidget* editorGUI)
        {
            if (editorGUI == m_widget)
            {
                ValueChangedByUser(AZ::Dom::Utils::ValueFromType(m_proxyValue), ValueChangeType::FinishedEdit);
            }
        }

    private:
        QPointer<QWidget> m_widget;
        InstanceDataNode m_proxyNode;
        AZ::SerializeContext::ClassData m_proxyClassData;
        WrappedType m_proxyValue;
    };
} // namespace AzToolsFramework

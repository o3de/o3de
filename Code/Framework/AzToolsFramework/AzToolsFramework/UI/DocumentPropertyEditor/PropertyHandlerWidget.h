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

        static bool ShouldHandleNode([[maybe_unused]] const AZ::Dom::Value& node)
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

        virtual QWidget* GetFirstInTabOrder();
        virtual QWidget* GetLastInTabOrder();

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
            m_proxyNode.m_classElement = &m_proxyClassElement;
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
            using AZ::DocumentPropertyEditor::Nodes::PropertyEditor;

            auto propertyEditorSystem = AZ::Interface<AZ::DocumentPropertyEditor::PropertyEditorSystemInterface>::Get();
            AZ_Assert(
                propertyEditorSystem != nullptr,
                "RpePropertyHandlerWrapper::SetValueFromDom called with an uninitialized PropertyEditorSystemInterface");
            if (propertyEditorSystem == nullptr)
            {
                return;
            }

            m_proxyClassElement.m_attributes.clear();
            for (auto attributeIt = node.MemberBegin(); attributeIt != node.MemberEnd(); ++attributeIt)
            {
                const AZ::Name& name = attributeIt->first;
                if (name == PropertyEditor::Type.GetName() || name == PropertyEditor::Value.GetName() ||
                    name == PropertyEditor::ValueType.GetName())
                {
                    continue;
                }
                AZ::Crc32 attributeId = AZ::Crc32(name.GetStringView());

                const AZ::DocumentPropertyEditor::AttributeDefinitionInterface* attribute =
                    propertyEditorSystem->FindNodeAttribute(name, propertyEditorSystem->FindNode(AZ_NAME_LITERAL(GetHandlerName())));
                AZStd::shared_ptr<AZ::Attribute> marshalledAttribute;
                if (attribute != nullptr)
                {
                    marshalledAttribute = attribute->DomValueToLegacyAttribute(attributeIt->second);
                }
                else
                {
                    marshalledAttribute = AZ::Reflection::WriteDomValueToGenericAttribute(attributeIt->second);
                }

                if (marshalledAttribute)
                {
                    m_proxyClassElement.m_attributes.emplace_back(attributeId, AZStd::move(marshalledAttribute));
                }
            }

            AZ::Dom::Value value = AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Value.ExtractFromDomNode(node).value();
            m_proxyValue = AZ::Dom::Utils::ValueToType<WrappedType>(value).value();
            GetRpeHandler().ReadValuesIntoGUI_Internal(GetWidget(), &m_proxyNode);
            GetRpeHandler().ConsumeAttributes_Internal(GetWidget(), &m_proxyNode);
        }

        static bool ShouldHandleNode(const AZ::Dom::Value& node)
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
                GetRpeHandler().WriteGUIValuesIntoProperty_Internal(GetWidget(), &m_proxyNode);
                ValueChangedByUser(AZ::Dom::Utils::ValueFromType(m_proxyValue), ValueChangeType::InProgressEdit);
            }
        }

        void RequestRefresh(PropertyModificationRefreshLevel)
        {
        }

        void AddElementsToParentContainer(QWidget*, size_t, const InstanceDataNode::FillDataClassCallback&)
        {
        }

        void RequestPropertyNotify(QWidget*)
        {
        }

        void OnEditingFinished(QWidget* editorGUI)
        {
            if (editorGUI == m_widget)
            {
                GetRpeHandler().WriteGUIValuesIntoProperty_Internal(GetWidget(), &m_proxyNode);
                ValueChangedByUser(AZ::Dom::Utils::ValueFromType(m_proxyValue), ValueChangeType::FinishedEdit);
            }
        }

        QWidget* GetFirstInTabOrder() override
        {
            return GetRpeHandler().GetFirstInTabOrder_Internal(GetWidget());
        }

        QWidget* GetLastInTabOrder() override
        {
            return GetRpeHandler().GetLastInTabOrder_Internal(GetWidget());
        }

    private:
        QPointer<QWidget> m_widget;
        InstanceDataNode m_proxyNode;
        AZ::SerializeContext::ClassData m_proxyClassData;
        AZ::SerializeContext::ClassElement m_proxyClassElement;
        WrappedType m_proxyValue;
    };
} // namespace AzToolsFramework

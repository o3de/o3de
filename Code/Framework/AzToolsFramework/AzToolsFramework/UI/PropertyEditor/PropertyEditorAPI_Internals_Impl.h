/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef PROPERTYEDITORAPI_INTERNALS_IMPL_H
#define PROPERTYEDITORAPI_INTERNALS_IMPL_H

#include <QObject>

// this header contains some of the internal template implementation for the
// internal templates. This is required for compilers that evaluate the
// templates before/during translation unit parsing and not at the end (like
// visual studio). This results in PropertyAttributeReader being a partially
// implemented class. This file solves this by being included after
// PropertyAttributeReader is defined.

namespace AzToolsFramework
{
    template <class WidgetType>
    void PropertyHandler_Internal<WidgetType>::ConsumeAttributes_Internal(QWidget* widget, InstanceDataNode* dataNode)
    {
        WidgetType* wid = static_cast<WidgetType*>(widget);
        AZ_Assert(wid, "Invalid class cast - this is not the right kind of widget!");

        BeforeConsumeAttributes(wid, dataNode);

        InstanceDataNode* parent = dataNode->GetParent();

        // Function callbacks require the instance pointer to be the first non-container ancestor.
        while (parent && parent->GetClassMetadata() && parent->GetClassMetadata()->m_container)
        {
            parent = parent->GetParent();
        }

        // If we're a leaf element, we are the instance to invoke on.
        if (parent == nullptr)
        {
            parent = dataNode;
        }

        AZ_Assert(parent->HasInstances(), "No parent instance - there is no parent instance in the parent data node.");
        void* classInstance = parent->FirstInstance(); // pointer to the owner class so we can read member variables and functions

        void* parentClassInstance = nullptr;
        if (InstanceDataNode* parentInstanceDataNode = parent->GetParent())
        {
            parentClassInstance = parentInstanceDataNode->FirstInstance();
        }

        auto consumeAttributes = [this, classInstance, wid](const auto& attributes, const char* name)
        {
            for (size_t i = 0; i < attributes.size(); ++i)
            {
                const auto& attrPair = attributes[i];
                PropertyAttributeReader reader(classInstance, &*attrPair.second);
                ConsumeAttribute(wid, attrPair.first, &reader, name);
            }
        };

        auto consumeParentAttributes = [this, parentClassInstance, wid](const auto& attributes, const char* name)
        {
            if (parentClassInstance)
            {
                for (size_t i = 0; i < attributes.size(); ++i)
                {
                    const auto& attrPair = attributes[i];
                    PropertyAttributeReader reader(parentClassInstance, &*attrPair.second);
                    ConsumeParentAttribute(wid, attrPair.first, &reader, name);
                }
            }
        };

        const AZ::SerializeContext::ClassElement* element = dataNode->GetElementMetadata();
        if (element)
        {
            consumeAttributes(element->m_attributes, element->m_name);

            if (const AZ::Edit::ElementData* elementEdit = dataNode->GetElementEditMetadata();
                elementEdit != nullptr)
            {
                consumeAttributes(elementEdit->m_attributes, elementEdit->m_name);
            }

            const AZ::SerializeContext::ClassElement* parentElement = parent != dataNode ?
                dataNode->GetElementMetadata() :
                nullptr;

            if (parentElement != nullptr)
            {
                // Reuse the current instance element name for the debug name
                consumeParentAttributes(parentElement->m_attributes, element->m_name);

                if (const AZ::Edit::ElementData* elementEdit = parent->GetElementEditMetadata();
                    elementEdit != nullptr)
                {
                    consumeParentAttributes(elementEdit->m_attributes, elementEdit->m_name);
                }
            }
        }
    }
}

#endif

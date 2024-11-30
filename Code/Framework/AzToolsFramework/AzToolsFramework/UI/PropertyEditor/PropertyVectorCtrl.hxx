/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROPERTYEDITOR_PROPERTY_VECTOR3CTRL_H
#define PROPERTYEDITOR_PROPERTY_VECTOR3CTRL_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>

#include "PropertyEditorAPI.h"
#include <AzQtComponents/Components/Widgets/VectorInput.h>
#endif

class QLabel;
class QLayout;

namespace AzQtComponents
{
    class DoubleSpinBox;
}

namespace AzToolsFramework
{
    /*!
     * \class VectorPropertyHandlerCommon
     * \brief Common functionality that is needed by handlers that need to handle a configurable
     * number of floats
     */
    class VectorPropertyHandlerCommon
    {
    public:

        /**
        * @param elementCount Number of elements being managed by this vector control
        * @param elementsPerRow Number of elements in every row
        * @param customLabels A string that has custom labels that are shown in front of elements
        */
        VectorPropertyHandlerCommon(int elementCount, int elementsPerRow = -1, AZStd::string customLabels = "")
            : m_elementCount(elementCount)
            , m_elementsPerRow(elementsPerRow)
            , m_customLabels(customLabels)
        {
        }

        // Constructs a Vector controller GUI attached to the provided parent
        AzQtComponents::VectorInput* ConstructGUI(QWidget* parent) const;

        /**
        * Consumes up to four attributes and configures the GUI
        * [IMPORTANT] Although the GUI and the handler can be configured to use more than four
        * elements, this method only supports attribute consumption up to four. If your handler/gui
        * uses more than four elements, this functionality will need to be augmented
        * @param GUI that will be configured according to the attributes
        * @param The attribute in consideration
        * @param attribute reader
        * @param unused
        */
        void ConsumeAttributes(AzQtComponents::VectorInput* GUI, AZ::u32 attrib,
            PropertyAttributeReader* attrValue, const char* debugName) const;

        int GetElementCount() const
        {
            return m_elementCount;
        }

        int GetElementsPerRow() const
        {
            return m_elementsPerRow;
        }

    private:
        //! stores the number of Vector elements being managed by this property handler
        int m_elementCount;

        //! stores the number of Vector elements per row
        int m_elementsPerRow;

        //! stores the custom labels to be used by controls
        AZStd::string m_customLabels;
    };

    template <typename TypeBeingHandled>
    class VectorPropertyHandlerBase
        : public PropertyHandler < TypeBeingHandled, AzQtComponents::VectorInput >
    {
    protected:
        VectorPropertyHandlerCommon m_common;
    public:
        /**
        * @param elementCount Number of elements being managed by this vector control
        * @param elementsPerRow Number of elements in every row
        * @param customLabels A string that has custom labels that are shown in front of elements
        */
        VectorPropertyHandlerBase(int elementCount, int elementsPerRow = -1, AZStd::string customLabels = "")
            : m_common(elementCount, elementsPerRow, customLabels)
        {
        }

        AZ::u32 GetHandlerName(void) const override
        {
            return AZ_CRC_CE("Vector_Flex_Handler");
        }

        bool IsDefaultHandler() const override
        {
            return true;
        }

        QWidget* GetFirstInTabOrder(AzQtComponents::VectorInput* widget) override
        {
            return widget->GetFirstInTabOrder();
        }

        QWidget* GetLastInTabOrder(AzQtComponents::VectorInput* widget) override
        {
            return widget->GetLastInTabOrder();
        }

        void UpdateWidgetInternalTabbing(AzQtComponents::VectorInput* widget) override
        {
            widget->UpdateTabOrder();
        }

        QWidget* CreateGUI(QWidget* pParent) override
        {
            return m_common.ConstructGUI(pParent);
        }

        void ConsumeAttribute(AzQtComponents::VectorInput* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override
        {
            m_common.ConsumeAttributes(GUI, attrib, attrValue, debugName);
        }

        void WriteGUIValuesIntoProperty(size_t, AzQtComponents::VectorInput* GUI, TypeBeingHandled& instance, InstanceDataNode*) override
        {
            AzQtComponents::VectorElement** elements = GUI->getElements();
            TypeBeingHandled actualValue = instance;
            for (int idx = 0; idx < m_common.GetElementCount(); ++idx)
            {
                actualValue.SetElement(idx, static_cast<float>(elements[idx]->getValue()));
            }
            instance = actualValue;
        }

        bool ReadValuesIntoGUI(size_t, AzQtComponents::VectorInput* GUI, const TypeBeingHandled& instance, InstanceDataNode*) override
        {
            GUI->blockSignals(true);

            for (int idx = 0; idx < m_common.GetElementCount(); ++idx)
            {
                GUI->setValuebyIndex(instance.GetElement(idx), idx);
            }

            GUI->blockSignals(false);
            return false;
        }
    };

    class Vector2PropertyHandler
        : public VectorPropertyHandlerBase<AZ::Vector2>
    {
    public:

        Vector2PropertyHandler()
            : VectorPropertyHandlerBase(2)
        {
        };

        AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::Vector2; }
    };

    class Vector3PropertyHandler
        : public VectorPropertyHandlerBase<AZ::Vector3>
    {
    public:

        Vector3PropertyHandler()
            : VectorPropertyHandlerBase(3)
        {
        };

        AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::Vector3; }
    };

    class Vector4PropertyHandler
        : public VectorPropertyHandlerBase<AZ::Vector4>
    {
    public:

        Vector4PropertyHandler()
            : VectorPropertyHandlerBase(4)
        {
        };

        AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::Vector4; }
    };

    class QuaternionPropertyHandler
        : public VectorPropertyHandlerBase<AZ::Quaternion>
    {
    public:
        QuaternionPropertyHandler()
            : VectorPropertyHandlerBase(3)
        {
        };

        AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::Quaternion; }

        void WriteGUIValuesIntoProperty(size_t index, AzQtComponents::VectorInput* GUI, AZ::Quaternion& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AzQtComponents::VectorInput* GUI, const AZ::Quaternion& instance, InstanceDataNode* node)  override;
    };
}


#endif // PROPERTYEDITOR_PROPERTY_VECTOR3CTRL_H

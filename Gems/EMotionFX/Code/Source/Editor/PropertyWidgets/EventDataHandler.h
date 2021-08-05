/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QComboBox>
#include <QVBoxLayout>
#include <QWidget>

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <EMotionFX/Source/EventData.h>
#endif

namespace EMotionFX
{
    class EventDataTypeSelectionWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EventDataTypeSelectionWidget(QWidget* parent=nullptr);

        const AZ::Uuid GetSelectedClass() const;
        void SetSelectedClass(const AZ::Uuid& classId);

    signals:
        void currentIndexChanged(int);

    private:
        QComboBox* m_comboBox = nullptr;

        class EventDataTypesModel;
        AZStd::shared_ptr<EventDataTypesModel> m_model = nullptr;
    };

    class EventDataHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::shared_ptr<const EMotionFX::EventData>, EventDataTypeSelectionWidget>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;

        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(EventDataTypeSelectionWidget* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, EventDataTypeSelectionWidget* GUI, AZStd::shared_ptr<const EventData>& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, EventDataTypeSelectionWidget* GUI, const AZStd::shared_ptr<const EventData>& instance, AzToolsFramework::InstanceDataNode* node) override;
    };

} // namespace EMotionFX

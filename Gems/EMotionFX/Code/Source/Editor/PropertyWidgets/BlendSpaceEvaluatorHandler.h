/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QComboBox>
#endif


namespace EMotionFX
{
    class BlendSpaceEvaluatorPicker
        : public QComboBox
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendSpaceEvaluatorPicker(QWidget* parent);

        void SetEvaluatorType(AZ::TypeId type);
        AZ::TypeId GetEvaluatorType() const;

    signals:
        void EvaluatorChanged();

    private slots:
        void OnCurrentIndexChanged(int index);

    private:
        AZ::TypeId GetTypeId(int index) const;
    };


    class BlendSpaceEvaluatorHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZ::TypeId, BlendSpaceEvaluatorPicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendSpaceEvaluatorHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(BlendSpaceEvaluatorPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, BlendSpaceEvaluatorPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, BlendSpaceEvaluatorPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
} // namespace EMotionFX

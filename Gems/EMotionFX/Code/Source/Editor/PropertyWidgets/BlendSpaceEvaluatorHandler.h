/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

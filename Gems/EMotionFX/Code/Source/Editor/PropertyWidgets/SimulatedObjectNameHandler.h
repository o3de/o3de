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
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Editor/LineEditValidatable.h>
#endif

namespace EMotionFX
{
    class SimulatedObject;

    class SimulatedObjectNameLineEdit
        : public EMStudio::LineEditValidatable
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        SimulatedObjectNameLineEdit(QWidget* parent);
        ~SimulatedObjectNameLineEdit() = default;

        void SetSimulatedObject(SimulatedObject* simulatedObject);

    private:
        SimulatedObject* m_simulatedObject;
    };

    class SimulatedObjectNameHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, SimulatedObjectNameLineEdit>
    {
        Q_OBJECT // AUTOMOC

        public:
            AZ_CLASS_ALLOCATOR_DECL

        SimulatedObjectNameHandler();
        ~SimulatedObjectNameHandler() = default;

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(SimulatedObjectNameLineEdit* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, SimulatedObjectNameLineEdit* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, SimulatedObjectNameLineEdit* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        SimulatedObject* m_simulatedObject;
    };
} // namespace EMotionFX

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
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <Source/Editor/PropertyWidgets/AnimGraphParameterHandler.h>
#include <QWidget>
#endif


namespace EMotionFX
{
    // Handler to pick paramaters and get input/outputs in a node affected. The node has to implement IParameterPickerRule
    class AnimGraphParameterMaskHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphParameterPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphParameterMaskHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(AnimGraphParameterPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphParameterPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        ObjectAffectedByParameterChanges* m_object;
    };
} // namespace EMotionFX
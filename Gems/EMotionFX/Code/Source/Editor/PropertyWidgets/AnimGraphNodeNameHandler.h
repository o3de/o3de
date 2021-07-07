/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Editor/LineEditValidatable.h>
#endif


namespace EMotionFX
{
    class AnimGraphNode;

    class AnimGraphNodeNameLineEdit
        : public EMStudio::LineEditValidatable
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphNodeNameLineEdit(QWidget* parent);
        ~AnimGraphNodeNameLineEdit() = default;

        void SetNode(AnimGraphNode* node);

    private:
        AnimGraphNode* m_node;
    };


    /**
    * The AnimGraphNodeNameHandler is a custom property handler for the name property of AnimGraphNodes.
    * It validates if the currently entered name candidate is unique and blocks a name change in case it isn't.
    */
    class AnimGraphNodeNameHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, AnimGraphNodeNameLineEdit>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphNodeNameHandler();
        ~AnimGraphNodeNameHandler() = default;

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(AnimGraphNodeNameLineEdit* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphNodeNameLineEdit* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphNodeNameLineEdit* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        AnimGraphNode* m_node;
    };
} // namespace EMotionFX

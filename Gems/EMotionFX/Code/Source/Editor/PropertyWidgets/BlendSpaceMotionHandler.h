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
#include <EMotionFX/Source/BlendSpaceNode.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QComboBox>
#endif


namespace EMotionFX
{
    class BlendSpaceMotionPicker
        : public QComboBox
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendSpaceMotionPicker(QWidget* parent);
        void SetBlendSpaceNode(BlendSpaceNode* blendSpaceNode);

        void ReInit();

        void SetMotionId(AZStd::string motionId);
        AZStd::string GetMotionId() const;

    signals:
        void MotionChanged();

        private slots:
        void OnCurrentIndexChanged(int index);

    private:
        BlendSpaceNode* m_blendSpaceNode;
    };


    class BlendSpaceMotionHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, BlendSpaceMotionPicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendSpaceMotionHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(BlendSpaceMotionPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, BlendSpaceMotionPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, BlendSpaceMotionPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    private:
        BlendSpaceNode* m_blendSpaceNode;
    };
} // namespace EMotionFX
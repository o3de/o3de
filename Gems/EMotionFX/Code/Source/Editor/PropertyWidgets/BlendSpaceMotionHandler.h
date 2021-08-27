/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

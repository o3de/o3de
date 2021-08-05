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
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#endif

QT_FORWARD_DECLARE_CLASS(QGridLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)


namespace EMotionFX
{
    class BlendSpaceMotionWidget
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendSpaceMotionWidget(BlendSpaceNode::BlendSpaceMotion* motion, QGridLayout* layout, int row);

        void UpdateInterface(EMotionFX::BlendSpaceNode* blendSpaceNode, EMotionFX::AnimGraphInstance* animGraphInstance);

        BlendSpaceNode::BlendSpaceMotion*       m_motion;
        QLabel*                                 m_labelMotion;
        AzQtComponents::DoubleSpinBox*          m_spinboxX;
        AzQtComponents::DoubleSpinBox*          m_spinboxY;
        QPushButton*                            m_restoreButton;
        QPushButton*                            m_removeButton;
    };


    class BlendSpaceMotionContainerWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendSpaceMotionContainerWidget(BlendSpaceNode* blendSpaceNode, QWidget* parent);
        void SetBlendSpaceNode(BlendSpaceNode* blendSpaceNode);

        void ReInit();

        void SetMotions(const AZStd::vector<BlendSpaceNode::BlendSpaceMotion>& motions);
        const AZStd::vector<BlendSpaceNode::BlendSpaceMotion>& GetMotions() const;

    signals:
        void MotionsChanged();

    protected slots:
        BlendSpaceMotionWidget* FindWidgetByMotionId(const AZStd::string& motionId) const;
        BlendSpaceMotionWidget* FindWidget(QObject* object);

        void OnAddMotion();
        void OnRemoveMotion(const BlendSpaceNode::BlendSpaceMotion* motion);

        void OnRestorePosition();
        void OnPositionXChanged(double value);
        void OnPositionYChanged(double value);

    private:
        void UpdateMotionPosition(QObject* object, float value, bool updateX, bool updateY);
        void UpdateInterface();
        EMotionFX::AnimGraphInstance* GetSingleSelectedAnimGraphInstance() const;

    private:
        AZStd::vector<BlendSpaceNode::BlendSpaceMotion> m_motions;
        AZStd::vector<BlendSpaceMotionWidget*>          m_motionWidgets;
        BlendSpaceNode*                                 m_blendSpaceNode;

        QWidget*                                        m_containerWidget;
        QLabel*                                         m_addMotionsLabel;
    };


    class BlendSpaceMotionContainerHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<BlendSpaceNode::BlendSpaceMotion>, BlendSpaceMotionContainerWidget>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendSpaceMotionContainerHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(BlendSpaceMotionContainerWidget* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, BlendSpaceMotionContainerWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, BlendSpaceMotionContainerWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    private:
        BlendSpaceNode* m_blendSpaceNode;
    };
} // namespace EMotionFX

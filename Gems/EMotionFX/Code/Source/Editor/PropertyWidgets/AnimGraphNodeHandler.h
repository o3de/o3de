/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <QWidget>
#include <QPushButton>
#endif


namespace EMotionFX
{
    class AnimGraphNodeIdPicker
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphNodeIdPicker(QWidget* parent);
        void SetAnimGraph(AnimGraph* animGraph);

        void SetNodeId(AnimGraphNodeId nodeId);
        AnimGraphNodeId GetNodeId() const;

        void SetShowStatesOnly(bool showStatesOnly);
        void SetNodeTypeFilter(const AZ::TypeId& nodeFilterType);

    signals:
        void SelectionChanged();

    private slots:
        void OnPickClicked();

    private:
        void UpdateInterface();

        AnimGraph*          m_animGraph;
        AnimGraphNodeId     m_nodeId;
        QPushButton*        m_pickButton;
        AZ::TypeId          m_nodeFilterType;
        bool                m_showStatesOnly;
    };


    class AnimGraphNodeIdHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZ::u64, AnimGraphNodeIdPicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphNodeIdHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(AnimGraphNodeIdPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphNodeIdPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphNodeIdPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        AnimGraph*  m_animGraph;
        AZ::TypeId  m_nodeFilterType;
        bool        m_showStatesOnly;
    };


    class AnimGraphMotionNodeIdHandler
        : public AnimGraphNodeIdHandler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphMotionNodeIdHandler();
        AZ::u32 GetHandlerName() const override;
    };


    class AnimGraphStateIdHandler
        : public AnimGraphNodeIdHandler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphStateIdHandler();
        AZ::u32 GetHandlerName() const override;
    };
} // namespace EMotionFX

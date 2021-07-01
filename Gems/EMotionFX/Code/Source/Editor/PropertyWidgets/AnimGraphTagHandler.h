/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Editor/TagSelector.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <QWidget>
#include <QPushButton>
#endif


namespace EMotionFX
{
    class AnimGraphTagSelector
        : public TagSelector
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphTagSelector(QWidget* parent);
        void SetAnimGraph(AnimGraph* animGraph) { m_animGraph = animGraph; }

    private:
        void GetAvailableTags(QVector<QString>& outTags) const override;

        AnimGraph* m_animGraph = nullptr;
    };

    class AnimGraphTagHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphTagSelector>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphTagHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(AnimGraphTagSelector* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphTagSelector* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphTagSelector* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        AnimGraph* m_animGraph;
    };
} // namespace EMotionFX

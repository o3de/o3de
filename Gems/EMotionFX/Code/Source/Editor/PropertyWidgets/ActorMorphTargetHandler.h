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
#include <QWidget>
#include <QPushButton>
#endif


namespace EMotionFX
{
    class ActorMorphTargetPicker
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ActorMorphTargetPicker(bool multiSelection, QWidget* parent);

        void SetMorphTargetNames(const AZStd::vector<AZStd::string>& morphTargetNames);
        const AZStd::vector<AZStd::string>& GetMorphTargetNames() const;

    signals:
        void SelectionChanged();

    private slots:
        void OnPickClicked();
        void OnResetClicked();

    private:
        void UpdateInterface();

        AZStd::vector<AZStd::string>    m_morphTargetNames;
        QPushButton*                    m_pickButton;
        QPushButton*                    m_resetButton;
        bool                            m_multiSelection;
    };


    class ActorSingleMorphTargetHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, ActorMorphTargetPicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        ActorSingleMorphTargetHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(ActorMorphTargetPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorMorphTargetPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorMorphTargetPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        bool m_multiSelection;
    };


    class ActorMultiMorphTargetHandler
        : public ActorSingleMorphTargetHandler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        ActorMultiMorphTargetHandler();
        AZ::u32 GetHandlerName() const override;
    };
} // namespace EMotionFX
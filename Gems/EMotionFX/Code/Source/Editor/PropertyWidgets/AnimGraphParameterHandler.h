/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionFX/Source/AnimGraph.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QWidget>
#include <QPushButton>
#endif


namespace EMotionFX
{
    class ObjectAffectedByParameterChanges;

    // Picker that allows to select one or more parameters (depending on mask mode) and affect the ports of the node
    // This Picker and its handlers are used by the BlendTreeParameterNode and the AnimGraphReferenceNode.
    //
    class AnimGraphParameterPicker
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL
        
        AnimGraphParameterPicker(QWidget* parent,  bool singleSelection = false, bool parameterMaskMode = false);
        void SetFilterTypes(const AZStd::vector<AZ::TypeId>& filterTypes) { m_filterTypes = filterTypes; }
        void SetAnimGraph(AnimGraph* animGraph) { m_animGraph = animGraph; }
        void SetObjectAffectedByParameterChanges(ObjectAffectedByParameterChanges* affectedObject);

        // Called to initialize the parameter names in the UI from values in the object
        void InitializeParameterNames(const AZStd::vector<AZStd::string>& parameterNames);

        // Called when the UI wants to update the parameter names
        void UpdateParameterNames(const AZStd::vector<AZStd::string>& parameterNames);

        const AZStd::vector<AZStd::string>& GetParameterNames() const;

        void SetSingleParameterName(const AZStd::string& parameterName);
        const AZStd::string GetSingleParameterName() const;

    signals:
        void ParametersChanged(const AZStd::vector<AZStd::string>& newParameters);

    private slots:
        void OnPickClicked();
        void OnResetClicked();
        void OnShrinkClicked();

    private:
        void UpdateInterface();

        AnimGraph* m_animGraph;
        ObjectAffectedByParameterChanges* m_affectedByParameterChanges;
        AZStd::vector<AZStd::string> m_parameterNames;
        AZStd::vector<AZ::TypeId> m_filterTypes;
        QPushButton* m_pickButton;
        QPushButton* m_resetButton;
        QPushButton* m_shrinkButton;
        bool m_singleSelection;
        bool m_parameterMaskMode;
    };


    class AnimGraphSingleParameterHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, AnimGraphParameterPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphSingleParameterHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(AnimGraphParameterPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphParameterPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        AnimGraph* m_animGraph;
    };

    class AnimGraphSingleNumberParameterHandler
        : public AnimGraphSingleParameterHandler
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL
        AnimGraphSingleNumberParameterHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
    };

    class AnimGraphSingleVector2ParameterHandler
        : public AnimGraphSingleParameterHandler
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL
        AnimGraphSingleVector2ParameterHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
    };

    class AnimGraphMultipleParameterHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphParameterPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphMultipleParameterHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(AnimGraphParameterPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphParameterPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        AnimGraph* m_animGraph;
    };
} // namespace EMotionFX

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QLineEdit>
#include <QWidget>
#endif


QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace EMotionFX
{
    class ActorJointPicker
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ActorJointPicker(bool singleSelection, const QString& dialogTitle, const QString& dialogDescriptionLabelText, QWidget* parent);

        void AddDefaultFilter(const QString& category, const QString& displayName);

        void SetJointName(const AZStd::string& jointName);
        AZStd::string GetJointName() const;

        void SetJointNames(const AZStd::vector<AZStd::string>& jointNames);
        AZStd::vector<AZStd::string> GetJointNames() const;

        void SetWeightedJointNames(const AZStd::vector<AZStd::pair<AZStd::string, float>>& weightedJointNames);
        AZStd::vector<AZStd::pair<AZStd::string, float>> GetWeightedJointNames() const;

    signals:
        void SelectionChanged();

    private slots:
        void OnPickClicked();
        void OnResetClicked();

    private:
        void UpdateInterface();

        AZStd::vector<AZStd::pair<AZStd::string, float>> m_weightedJointNames;
        AZStd::vector<AZStd::pair<QString, QString>> m_defaultFilters;
        QString m_dialogTitle;
        QString m_dialogDescriptionLabelText;
        QLabel* m_label;
        QPushButton* m_pickButton;
        QPushButton* m_resetButton;
        bool m_singleSelection;
    };

    template<class T>
    class ActorJointElementHandlerImpl
        : public QObject
        , public AzToolsFramework::PropertyHandler<T, QWidget>
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void WriteGUIValuesIntoProperty(size_t index, QWidget* GUI, T& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, QWidget* GUI, const T& instance, AzToolsFramework::InstanceDataNode* node) override;
    };

    using ActorJointElementHandler = ActorJointElementHandlerImpl<AZStd::string>;
    using ActorWeightedJointElementHandler = ActorJointElementHandlerImpl<AZStd::pair<AZStd::string, float>>;


    class ActorSingleJointHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, ActorJointPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(ActorJointPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorJointPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorJointPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };


    class ActorMultiJointHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, ActorJointPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(ActorJointPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorJointPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorJointPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };


    class ActorMultiWeightedJointHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::pair<AZStd::string, float> >, ActorJointPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(ActorJointPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorJointPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorJointPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
} // namespace EMotionFX

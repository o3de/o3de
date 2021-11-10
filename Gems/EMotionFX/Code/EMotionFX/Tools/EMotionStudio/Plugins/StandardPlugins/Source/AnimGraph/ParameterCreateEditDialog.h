/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <MCore/Source/StandardHeaders.h>
#include <QDialog>
#endif

QT_FORWARD_DECLARE_CLASS(QComboBox)

namespace EMotionFX
{
    class Parameter;
}

namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class ValueParameterEditor;

    class ParameterCreateEditDialog
        : public QDialog
        , private AzToolsFramework::IPropertyEditorNotify
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ParameterCreateEditDialog, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        enum
        {
            VALUE_DEFAULT = 0,
            VALUE_MINIMUM = 1,
            VALUE_MAXIMUM = 2
        };

        ParameterCreateEditDialog(AnimGraphPlugin* plugin, QWidget* parent, const EMotionFX::Parameter* editParameter = nullptr);
        ~ParameterCreateEditDialog();

        void Init();

        const AZStd::unique_ptr<EMotionFX::Parameter>& GetParameter() const { return m_parameter; }

        QComboBox* GetValueTypeComboBox() const { return m_valueTypeCombo; }

    protected slots:
        void OnValueTypeChange(int valueType);
        void OnValidate();

    private:
        void InitDynamicInterface(const AZ::TypeId& typeID);

        // AzToolsFramework::IPropertyEditorNotify
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode*) override {}
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode*) override;
        void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode*) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode*) override {}
        void SealUndoStack() override {}

    private:
        AnimGraphPlugin*                    m_plugin;
        QComboBox*                          m_valueTypeCombo;
        QFrame*                             m_previewFrame;
        AzToolsFramework::ReflectedPropertyEditor* m_previewWidget;
        ValueParameterEditor*               m_valueParameterEditor;
        AzToolsFramework::ReflectedPropertyEditor* m_parameterEditorWidget;
        QPushButton*                        m_createButton;

        AZStd::unique_ptr<EMotionFX::Parameter> m_parameter;
        AZStd::string                       m_originalName;

        static int                          s_parameterEditorMinWidth;
    };
} // namespace EMStudio

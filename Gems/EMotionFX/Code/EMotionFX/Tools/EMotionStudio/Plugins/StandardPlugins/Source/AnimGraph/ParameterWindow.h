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
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <QDialog>
#include <QPointer>
#include <QWidget>
#include <QLineEdit>
#include <QTreeWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QScrollArea)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)


namespace EMotionFX
{
    class GroupParameter;
    class ValueParameter;
    class AnimGraph;
    class Parameter;
} // namespace EMotionFX

namespace MCore
{
    class Attribute;
} // namespace MCore

namespace AzQtComponents
{
    class FilteredSearchWidget;
} // namespace AzQtComponents

namespace EMStudio
{
    class AnimGraphPlugin;
    class ValueParameterEditor;
    class ParameterWindowTreeWidget;
    class ParameterCreateEditWidget;

    class ParameterCreateRenameWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ParameterCreateRenameWindow, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        ParameterCreateRenameWindow(const char* windowTitle, const char* topText, const char* defaultName, const char* oldName, const AZStd::vector<AZStd::string>& invalidNames, QWidget* parent);

        AZStd::string GetName() const
        {
            return m_lineEdit->text().toUtf8().data();
        }

    private slots:
        void NameEditChanged(const QString& text);

    private:
        AZStd::string                   m_oldName;
        AZStd::vector<AZStd::string>    m_invalidNames;
        QPushButton*                    m_okButton;
        QPushButton*                    m_cancelButton;
        QLineEdit*                      m_lineEdit;
    };

    class ParameterWindow
        : public QWidget
        , private AzToolsFramework::IPropertyEditorNotify
        , protected EMotionFX::AnimGraphNotificationBus::Handler
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ParameterWindow, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        ParameterWindow(AnimGraphPlugin* plugin);
        ~ParameterWindow();

        void Reinit(bool forceReinit = false);

        // retrieve current selection
        const EMotionFX::Parameter* GetSingleSelectedParameter() const;

        bool GetIsParameterSelected(const AZStd::string& parameterName)
        {
            if (AZStd::find(m_selectedParameterNames.begin(), m_selectedParameterNames.end(), parameterName) == m_selectedParameterNames.end())
            {
                return false;
            }
            return true;
        }

        // select manually
        void SingleSelectGroupParameter(const char* groupName, bool ensureVisibility = false, bool updateInterface = false);
        void SelectParameters(const AZStd::vector<AZStd::string>& parameterNames, bool updateInterface = false);

        void UpdateParameterValues();

        void ClearParameters(bool showConfirmationDialog = true);

        int GetTopLevelItemCount() const;

    signals:
        // AnimGraphNotificationBus
        // callback function when the attribute get changed from trigger actions.
        void OnParameterActionTriggered(const EMotionFX::ValueParameter* valueParameter) override;

    public slots:
        void UpdateParameterValue(const EMotionFX::Parameter* parameter);
        void OnRecorderStateChanged();
        void OnMakeDefaultValue();
        void OnAddParameter();
        void OnAddGroup();
        void OnRemoveSelected();
        void OnEditSelected();

    private slots:
        void UpdateInterface();

        void OnClearButton() { ClearParameters(); }

        void OnGroupCollapsed(QTreeWidgetItem* item);
        void OnGroupExpanded(QTreeWidgetItem* item);

        void OnGroupParameterSelected();

        void OnMoveParameterTo(int idx, const QString& parameter, const QString& parent);
        void OnTextFilterChanged(const QString& text);
        void OnSelectionChanged();

        void OnGamepadControlToggle();

        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);

    private:
        void contextMenuEvent(QContextMenuEvent* event) override;
        void CanMove(bool* outMoveUpPossible, bool* outMoveDownPossible);

        void UpdateSelectionArrays();

        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void AddParameterToInterface(EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, QTreeWidgetItem* parentWidgetItem);

        /**
         * Update the attributes the parameter widgets modify when they are being changed in the interface.
         * Iterate through the selected actor instances and check for the ones that are running the anim graph whose parameters
         * we're showing. This will make sure that e.g. when changing a slider in the parameter window, the corresponding
         * anim graph instances get informed about the changed values.
         */
        void UpdateAttributesForParameterWidgets();

        /**
         * Get the attributes for the given parameter that are influenced by any of the currently selected actor instances
         * that are running the anim graph whose parameters we're showing.
         * @param[in] parameterIndex The parameter index to get the influenced attributes for.
         * @result The list of influenced attributes from the anim graph instances.
         */
        AZStd::vector<MCore::Attribute*> GetAttributesForParameter(size_t parameterIndex) const;

        /**
         * Check if the gamepad control mode is enabled for the given parameter and if its actually being controlled or not.
         */
        void GetGamepadState(EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, bool* outIsActuallyControlled, bool* outIsEnabled);
        void SetGamepadState(EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, bool isEnabled);
        void SetGamepadButtonTooltip(QPushButton* button);

        // IPropertyEditorNotify
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* pNode) override;
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
        void SealUndoStack() override;
        void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint&) override;
        void PropertySelectionChanged(AzToolsFramework::InstanceDataNode *, bool) override;

        EMotionFX::AnimGraph*           m_animGraph;

        // toolbar buttons
        QAction* m_addAction;
        static int s_contextMenuWidth;

        ParameterCreateEditWidget*      m_parameterCreateEditWidget = nullptr;

        AZStd::vector<AZStd::string>    m_selectedParameterNames;
        bool                            m_ensureVisibility;
        bool                            m_lockSelection;

        AZStd::string                   m_filterString;
        AnimGraphPlugin*                m_plugin;
        ParameterWindowTreeWidget*      m_treeWidget;
        AzQtComponents::FilteredSearchWidget* m_searchWidget;
        QVBoxLayout*                    m_verticalLayout;
        AZStd::string                   m_nameString;
        struct ParameterWidget
        {
            AZStd::unique_ptr<ValueParameterEditor> m_valueParameterEditor;
            QPointer<AzToolsFramework::ReflectedPropertyEditor> m_propertyEditor;
        };
        typedef AZStd::unordered_map<const EMotionFX::Parameter*, ParameterWidget> ParameterWidgetByParameter;
        ParameterWidgetByParameter m_parameterWidgets;
    };

    class ParameterWindowTreeWidgetPrivate;
    class ParameterWindowTreeWidget
        : public QTreeWidget
    {
        Q_OBJECT

    public:
        ParameterWindowTreeWidget(QWidget* parent=nullptr);

        void SetAnimGraph(EMotionFX::AnimGraph* animGraph);

    signals:
        void ParameterMoved(int idx, const QString& parameter, const QString& parent);
        void DragEnded();

    protected:
        void startDrag(Qt::DropActions supportedActions) override;
        void dropEvent(QDropEvent* event) override;

    private:
        EMotionFX::AnimGraph* m_animGraph = nullptr;
        QTreeWidgetItem* m_draggedParam = nullptr;
        QTreeWidgetItem* m_draggedParentParam = nullptr;

        Q_DECLARE_PRIVATE(ParameterWindowTreeWidget)
    };
} // namespace EMStudio

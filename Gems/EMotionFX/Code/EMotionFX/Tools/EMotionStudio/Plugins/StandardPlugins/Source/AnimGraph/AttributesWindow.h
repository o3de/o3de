/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <Editor/TypeChoiceButton.h>

#include <QDialog>
#include <QModelIndex>
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QScrollArea)
QT_FORWARD_DECLARE_CLASS(QItemSelection)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QCheckBox)

namespace AzQtComponents
{
    class Card;
}

namespace EMotionFX
{
    class AnimGraphEditor;
    class AnimGraphObject;
    class ObjectEditor;
}

namespace EMStudio
{
    class AttributesWindowRequests
        : public AZ::EBusTraits
    {
    public:
        virtual bool IsLocked() const { return false; }
        virtual QModelIndex GetModelIndex() const { return {}; }
    };

    using AttributesWindowRequestBus = AZ::EBus<AttributesWindowRequests>;

    // forward declarations
    class AnimGraphPlugin;
    class AttributesWindow;

    class AddConditionButton
        : public EMotionFX::TypeChoiceButton
    {
        Q_OBJECT //AUTOMOC

    public:
        AddConditionButton(AnimGraphPlugin* plugin, QWidget* parent);
    };

    class AddActionButton
        : public EMotionFX::TypeChoiceButton
    {
        Q_OBJECT //AUTOMOC

    public:
        AddActionButton(AnimGraphPlugin* plugin, QWidget* parent);
    };

    class PasteConditionsWindow
        : public QDialog
    {
        Q_OBJECT //AUTOMOC
                 MCORE_MEMORYOBJECTCATEGORY(PasteConditionsWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        PasteConditionsWindow(AttributesWindow* attributeWindow);
        virtual ~PasteConditionsWindow();
        bool GetIsConditionSelected(size_t index) const;
    private:
        QPushButton*                m_okButton;
        QPushButton*                m_cancelButton;
        AZStd::vector<QCheckBox*>   m_checkboxes;
    };


    class AttributesWindow
        : public QWidget
        , public AttributesWindowRequestBus::Handler
    {
        Q_OBJECT //AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(AttributesWindow, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        AttributesWindow(AnimGraphPlugin* plugin, QWidget* parent = nullptr);
        ~AttributesWindow();

        // copy & paste
        struct CopyPasteConditionObject
        {
            AZStd::string   m_contents;
            AZStd::string   m_summary;
            AZ::TypeId      m_conditionType;
        };

        struct CopyPasteClipboard
        {
            void Clear();

            AZStd::vector<CopyPasteConditionObject> m_conditions;
            AZ::Outcome<AZStd::string> m_transition;
        };

        const CopyPasteClipboard& GetCopyPasteConditionClipboard() const { return m_copyPasteClipboard; }

        void Init(const QModelIndex& modelIndex = QModelIndex(), bool forceUpdate = false);

        void Lock() { m_isLocked = true; }
        void Unlock() { m_isLocked = false; }
        // AttributesWindowRequestBus overrides
        bool IsLocked() const override { return m_isLocked; }
        QModelIndex GetModelIndex() const override { return m_displayingModelIndex; }
        AddConditionButton* GetAddConditionButton() { return m_addConditionButton; }
        void AddTransitionCopyPasteMenuEntries(QMenu* menu);

        EMotionFX::AnimGraphEditor* GetAnimGraphEditor() const;

    public slots:
        void OnCopy();
        void OnPasteFullTransition();
        void OnPasteConditions();
        void OnPasteConditionsSelective();
        void OnConditionContextMenu(const QPoint& position);

    private slots:
        void AddCondition(const AZ::TypeId& conditionType);
        void OnRemoveCondition();
        

        void OnActionContextMenu(const QPoint& position);

        void AddTransitionAction(const AZ::TypeId& actionType);
        void AddStateAction(const AZ::TypeId& actionType);
        void OnRemoveTransitionAction();
        void OnRemoveStateAction();

        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    private:
        AddConditionButton* m_addConditionButton = nullptr;
        void contextMenuEvent(QContextMenuEvent* event) override;

        void PasteTransition(bool pasteTransitionProperties, bool pasteConditions);

        AnimGraphPlugin*                        m_plugin;
        QScrollArea*                            m_scrollArea;
        QPersistentModelIndex                   m_displayingModelIndex;

        QWidget*                                m_mainReflectedWidget;
        AzQtComponents::Card*                   m_objectCard;
        EMotionFX::AnimGraphEditor*             m_animGraphEditor;
        EMotionFX::ObjectEditor*                m_objectEditor;
        bool                                    m_isLocked = false;

        struct CachedWidgets
        {
            CachedWidgets(AzQtComponents::Card* card, EMotionFX::ObjectEditor* objectEditor)
                : m_card(card)
                , m_objectEditor(objectEditor)
            {}

            AzQtComponents::Card*    m_card;
            EMotionFX::ObjectEditor* m_objectEditor;
        };

        // Condition widgets
        QWidget*                                m_conditionsWidget;
        QLayout*                                m_conditionsLayout;
        AZStd::vector<CachedWidgets>            m_conditionsCachedWidgets;

        // Action widgets
        QWidget*                                m_actionsWidget;
        QLayout*                                m_actionsLayout;
        AZStd::vector<CachedWidgets>            m_actionsCachedWidgets;

        PasteConditionsWindow*                  m_pasteConditionsWindow;

        CopyPasteClipboard                      m_copyPasteClipboard;        

        void UpdateConditions(EMotionFX::AnimGraphObject* object, AZ::SerializeContext* serializeContext, bool forceUpdate = false);
        void UpdateActions(EMotionFX::AnimGraphObject* object, AZ::SerializeContext* serializeContext, bool forceUpdate = false);
    };
} // namespace EMStudio

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
#include "EntityPropertyEditor.hxx"
AZ_POP_DISABLE_WARNING

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/sort.h>

#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/DragAndDrop.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/API/ComponentModeCollectionInterface.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityRuntimeActivationBus.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabComponentAdapter.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabOverrideLabelHandler.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteUtil.hxx>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteWidget.hxx>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditorHeader.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzQtComponents/Components/Widgets/ComboBox.h>
#include <AzQtComponents/Components/Widgets/LineEditRevertHandler.h>
#include <AzQtComponents/Utilities/IconUtilities.h>
#include <AzQtComponents/Utilities/QtViewPaneEffects.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                                    // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
                                                                    // 4800: 'QFlags<QPainter::RenderHint>::Int': forcing value to bool 'true' or 'false' (performance warning)
#include <QContextMenuEvent>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QGraphicsEffect>
#include <QLabel>
#include <QListView>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QStylePainter>
#include <QTimer>
AZ_POP_DISABLE_WARNING

#include <AzToolsFramework/UI/PropertyEditor/ui_EntityPropertyEditor.h>

// This has to live outside of any namespaces due to issues on Linux with calls to Q_INIT_RESOURCE if they are inside a namespace
void initEntityPropertyEditorResources()
{
    Q_INIT_RESOURCE(Icons);
    Q_INIT_RESOURCE(OverrideResources);
}

namespace AzToolsFramework
{
    constexpr const char*  kComponentEditorIndexMimeType = "editor/componentEditorIndices";
    constexpr const char*  kComponentEditorRowWidgetType = "editor/componentEditorRowWidget";

    constexpr const char* kPropertyEditorMenuActionMoveUp("editor/propertyEditorMoveUp");
    constexpr const char* kPropertyEditorMenuActionMoveDown("editor/propertyEditorMoveDown");

    static constexpr const char* enableDPECVarName = "ed_enableDPEInspector";

    AZ_CVAR(
        bool,
        ed_enableDPEInspector,
        true,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "If set, enables experimental Document Property Editor support for the Entity Inspector");

    static constexpr const char* inspectorRefreshDiagnosticsCVarName = "ed_inspectorRefreshDiagnostics";

    AZ_CVAR(
        bool,
        ed_inspectorRefreshDiagnostics,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "If set, logs Entity Inspector full-refresh timing and whether the component composition matches the "
        "previous refresh (I.2 measurement harness for same-composition value-refresh reuse).");

    AZ_CVAR(
        bool,
        ed_inspectorLazyCards,
        true,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "If set (the default), the Entity Inspector builds the rows of the cards in view (and one screen below) at once, and those of "
        "the cards past them and of collapsed cards later, top to bottom: at idle, or as soon as the view or the focus reaches them.");

    //! A lazy cards step builds held cards for about this long (at least one card) before it lets the event loop run
    static constexpr AZ::s64 LazyCardsStepBudgetUs = 10000;

    AZ_CVAR(
        bool,
        ed_inspectorSettleLayout,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "If set, the Entity Inspector also lays its component list out before it paints at the end of a rebuild and of each idle lazy "
        "cards step: a reused card then never shows where the previous selection's card was. It always does after an expand, a "
        "collapse, or cards built as the view reaches them.");

    AZ_CVAR(
        bool,
        ed_inspectorInterruptibleLoad,
        true,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "If set (the default), the Entity Inspector lets a new selection show before it rebuilds, and a newer one interrupt the "
        "rebuild: the rebuild a selection queues paints what waits to be painted first (the outliner's new selection), builds every "
        "card's rows in idle steps, and keeps the inspector's last picture until the cards in view are built, then shows them in one "
        "paint; a new selection drops the steps left.");

    AZ_CVAR(
        int,
        ed_inspectorInterruptibleInlineMs,
        40,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "With ed_inspectorInterruptibleLoad, the rebuild a selection queues builds the cards in view for about this long (ms) before "
        "it lets the event loop run (the engine's tick, other windows' paints), and so does each idle step until they show. 0 builds "
        "every card in idle steps.");

    //! ed_inspectorInterruptibleInlineMs in microseconds
    static AZ::s64 InterruptibleInlineBudgetUs()
    {
        return aznumeric_cast<AZ::s64>(static_cast<int>(ed_inspectorInterruptibleInlineMs)) * 1000;
    }

    static bool ShouldUseDPE()
    {
        bool dpeEnabled = true;
        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue(enableDPECVarName, dpeEnabled);
        }
        return dpeEnabled;
    }

    //since component editors are spaced apart to make room for drop indicator,
    //giving drop logic simple buffer so drops between editors don't go to the bottom
    static const int kComponentEditorDropTargetPrecision = 6;

    //sentinel class to set an attribute on a widget, if need be, and restore the attribute's
    //value back to what it was before in the destructor.
    class AttributeSetterSentinel
    {
    public:
        AttributeSetterSentinel(QWidget* widget, Qt::WidgetAttribute attribute) : m_widget(widget), m_attribute(attribute)
        {
            m_restoreInDestructor = !m_widget->testAttribute(m_attribute);
            if (m_restoreInDestructor)
            {
                m_widget->setAttribute(m_attribute, true);
            }
        }

        ~AttributeSetterSentinel()
        {
            if (m_restoreInDestructor)
            {
                m_widget->setAttribute(m_attribute, false);
            }
        }

    private:
        QWidget* m_widget;
        Qt::WidgetAttribute m_attribute;
        bool m_restoreInDestructor = false;
    };

    //we require an overlay widget to act as a canvas to draw on top of everything in the inspector
    //attaching to inspector rather than component editors so we can draw outside of bounds
    class EntityPropertyEditorOverlay : public QWidget
    {
    public:
        EntityPropertyEditorOverlay(EntityPropertyEditor* editor, QWidget* parent)
            : QWidget(parent)
            , m_editor(editor)
            , m_dropIndicatorOffset(8)
            , m_dropIndicatorRowWidgetOffset(2)
        {
            setPalette(Qt::transparent);
            setWindowFlags(Qt::FramelessWindowHint);
            setAttribute(Qt::WA_NoSystemBackground);
            setAttribute(Qt::WA_TranslucentBackground);
            setAttribute(Qt::WA_TransparentForMouseEvents);
        }

    protected:
        static constexpr int TopMargin = 1;
        static constexpr int RightMargin = 2;
        static constexpr int BottomMargin = 5;
        static constexpr int LeftMargin = 2;
        static constexpr int RowHighlightIndent = 2;

        void paintDraggingRowWidget(QPainter& painter)
        {
            ComponentEditor* rowWidgetEditor = m_editor->GetEditorForCurrentReorderRowWidget();
            PropertyRowWidget* dragRowWidget = m_editor->GetReorderRowWidget();
            PropertyRowWidget* dropTarget = m_editor->GetReorderDropTarget();
            EntityPropertyEditor::DropArea dropArea = m_editor->GetReorderDropArea();

            // The user is dragging a row widget.
            for (auto componentEditor : m_editor->m_componentEditors)
            {
                if (!componentEditor->isVisible())
                {
                    continue;
                }

                if (componentEditor != rowWidgetEditor)
                {
                    continue;
                }

                IPropertyEditor* editor = componentEditor->GetPropertyEditor();
                auto reflectedPropertyEditor = azdynamic_cast<ReflectedPropertyEditor*>(editor);
                if (!reflectedPropertyEditor)
                {
                    continue;
                }
                for (auto& [dataNode, rowWidget] : reflectedPropertyEditor->GetWidgets())
                {
                    if (!rowWidget->isVisible())
                    {
                        continue;
                    }

                    QRect globalRect = m_editor->GetWidgetAndVisibleChildrenGlobalRect(rowWidget);

                    QRect currRect = QRect(
                        QPoint(mapFromGlobal(globalRect.topLeft()) + QPoint(LeftMargin, TopMargin)),
                        QPoint(mapFromGlobal(globalRect.bottomRight()) - QPoint(RightMargin, BottomMargin)));

                    currRect.setLeft(LeftMargin + RowHighlightIndent);
                    currRect.setWidth(rowWidget->GetContainingEditorFrameWidth() - (RightMargin + LeftMargin));

                    if (rowWidget == dragRowWidget)
                    {
                        QStyleOption opt;
                        opt.initFrom(this);
                        opt.rect = currRect;
                        qobject_cast <AzQtComponents::Style*>(style())->drawDragIndicator(&opt, &painter, this);
                    }

                    if (rowWidget == dropTarget)
                    {
                        QRect dropRect = currRect;
                        if (dropArea == EntityPropertyEditor::DropArea::Above)
                        {
                            dropRect.setTop(currRect.top() - m_dropIndicatorRowWidgetOffset);
                        }
                        else
                        {
                            dropRect.setTop(currRect.bottom());
                        }

                        dropRect.setHeight(0);

                        QStyleOption opt;
                        opt.initFrom(this);
                        opt.rect = dropRect;
                        style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, &painter, this);
                    }
                }
            }
        };

        void paintMenuHighlight(QPainter& painter, float alpha)
        {
            // If a RowWidget can be moved up or down, highlight it.
            PropertyRowWidget* dragRowWidget = m_editor->GetReorderRowWidget();
            if (!dragRowWidget)
            {
                return;
            }

            PropertyRowWidget* dropTarget = m_editor->GetReorderDropTarget();
            EntityPropertyEditor::DropArea dropArea = m_editor->GetReorderDropArea();
            QPixmap dragImage = m_editor->GetReorderRowWidgetImage();

            // User has the context menu open with a movable row selected.
            QRect globalRect = m_editor->GetWidgetAndVisibleChildrenGlobalRect(dragRowWidget);

            int top = mapFromGlobal(globalRect.topLeft()).y();
            int imageHeight = static_cast<int>(dragImage.height() / dragImage.devicePixelRatioF());
            int imageWidth = static_cast<int>(dragImage.width() / dragImage.devicePixelRatioF());
            QRect currRect = QRect(QPoint(LeftMargin + 1, top), QPoint(LeftMargin + 1 + imageWidth, top + imageHeight));

            painter.setOpacity(alpha);
            painter.drawPixmap(currRect, dragImage);

            if (dropTarget)
            {
                // A move row menu command is highlighted. Draw an indicator to show where the current row will move to.
                globalRect = m_editor->GetWidgetAndVisibleChildrenGlobalRect(dropTarget);
                QRect dropRect = QRect(
                    QPoint(mapFromGlobal(globalRect.topLeft()) + QPoint(LeftMargin, TopMargin)),
                    QPoint(mapFromGlobal(globalRect.bottomRight()) - QPoint(RightMargin, TopMargin)));

                if (dropArea == EntityPropertyEditor::DropArea::Above)
                {
                    dropRect.setTop(dropRect.top() - m_dropIndicatorRowWidgetOffset);
                }
                else
                {
                    dropRect.setTop(dropRect.bottom());
                }
                dropRect.setHeight(0);

                painter.setOpacity(alpha);
                QStyleOption lineOpt;
                lineOpt.initFrom(this);
                lineOpt.rect = dropRect;
                style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &lineOpt, &painter, this);
                painter.setOpacity(1.0f);
            }
        };

        void paintDraggingComponent(QPainter& painter)
        {
            bool drag = false;
            bool drop = false;
            QRect currRect;

            // Check for a component editor being dragged.
            for (auto componentEditor : m_editor->m_componentEditors)
            {
                if (!componentEditor->isVisible())
                {
                    continue;
                }

                QRect globalRect = m_editor->GetWidgetGlobalRect(componentEditor);

                currRect = QRect(
                    QPoint(mapFromGlobal(globalRect.topLeft()) + QPoint(LeftMargin, TopMargin)),
                    QPoint(mapFromGlobal(globalRect.bottomRight()) - QPoint(RightMargin, BottomMargin)));

                currRect.setWidth(currRect.width() - 1);
                currRect.setHeight(currRect.height() - 1);

                if (componentEditor->IsDragged())
                {
                    QStyleOption opt;
                    opt.initFrom(this);
                    opt.rect = currRect;
                    static_cast<AzQtComponents::Style*>(style())->drawDragIndicator(&opt, &painter, this);
                    drag = true;
                }

                if (componentEditor->IsDropTarget())
                {
                    QRect dropRect = currRect;
                    dropRect.setTop(currRect.top() - m_dropIndicatorOffset);
                    dropRect.setHeight(0);

                    QStyleOption opt;
                    opt.initFrom(this);
                    opt.rect = dropRect;
                    style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, &painter, this);

                    drop = true;
                }
            }

            if (drag && !drop)
            {
                QRect dropRect = currRect;
                dropRect.setTop(currRect.top() - m_dropIndicatorOffset);
                dropRect.setHeight(0);

                QStyleOption opt;
                opt.initFrom(this);
                opt.rect = dropRect;
                style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, &painter, this);
            }
        };

        void paintMovedRow(QPainter& painter, float alpha)
        {
            // After a move has been carried out, briefly highlight the moved row.
            PropertyRowWidget* rowWidget = m_editor->GetRowToHighlight();

            if (!rowWidget)
            {
                return;
            }

            QRect globalRect = m_editor->GetWidgetAndVisibleChildrenGlobalRect(rowWidget);
            QRect currRect = QRect(
                QPoint(mapFromGlobal(globalRect.topLeft()) + QPoint(LeftMargin, TopMargin)),
                QPoint(mapFromGlobal(globalRect.bottomRight()) - QPoint(RightMargin, BottomMargin)));

            currRect.setLeft(LeftMargin + 2);
            currRect.setWidth(rowWidget->GetContainingEditorFrameWidth() - (RightMargin + LeftMargin));

            painter.setOpacity(alpha);

            QPen pen;
            QColor drawColor = Qt::white;
            drawColor.setAlphaF(alpha);
            pen.setColor(drawColor);
            pen.setWidth(1);
            painter.setPen(pen);
            painter.drawRect(currRect);
        }

        void paintEvent(QPaintEvent* event) override
        {
            QWidget::paintEvent(event);

            QPainter painter(this);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

            EntityPropertyEditor::ReorderState currentState = m_editor->GetReorderState();
            float indicatorAlpha = m_editor->GetMoveIndicatorAlpha();

            switch (currentState)
            {
            case EntityPropertyEditor::ReorderState::DraggingRowWidget:
                paintDraggingRowWidget(painter);
                break;
            case EntityPropertyEditor::ReorderState::UsingMenu:
                paintMenuHighlight(painter, 1.0f);
                break;
            case EntityPropertyEditor::ReorderState::MenuOperationInProgress:
                paintMenuHighlight(painter, indicatorAlpha);
                break;
            case EntityPropertyEditor::ReorderState::DraggingComponent:
                paintDraggingComponent(painter);
                break;
            case EntityPropertyEditor::ReorderState::HighlightMovedRow:
                paintMovedRow(painter, indicatorAlpha);
                break;
            default:
                break;
            }
        }

        bool event(QEvent* ev) override
        {
            // Apparently, despite the Qt::WA_TransparentForMouseEvents flag being set,
            // this overlay widget can still get mouse events (including mouse press and release).
            // I couldn't determine how/when/or why, and it's only on certain systems (QA only found one
            // in the entire studio) but it is an issue.
            // To fix it, we resolve the widget that this event should go to and forward the event to that instead.
            switch (ev->type())
            {
                case QEvent::MouseButtonPress:
                case QEvent::MouseButtonRelease:
                case QEvent::MouseButtonDblClick:
                case QEvent::MouseMove:
                {
                    QMouseEvent* originalMouseEvent = static_cast<QMouseEvent*>(ev);

                    // ensure that the TransparentForMouseEvents flag is set, so that
                    // the widget under the mouse gets properly detected, otherwise, "this" will get returned by childAt()
                    AttributeSetterSentinel attributeSetterSentinel(this, Qt::WA_TransparentForMouseEvents);

                    QWidget* newWidget = m_editor->childAt(m_editor->mapFromGlobal(originalMouseEvent->globalPosition()).toPoint());

                    if ((newWidget != this) && (newWidget != nullptr))
                    {
                        QPoint newLocal = newWidget->mapFromGlobal(originalMouseEvent->globalPosition().toPoint());
                        QMouseEvent newMouseEvent(
                            ev->type(),
                            newLocal,
                            originalMouseEvent->scenePosition(),
                            originalMouseEvent->globalPosition(),
                            originalMouseEvent->button(),
                            originalMouseEvent->buttons(),
                            originalMouseEvent->modifiers(),
                            originalMouseEvent->source());

                        bool handled = QApplication::sendEvent(newWidget, &newMouseEvent);

                        ev->setAccepted(handled);

                        return handled;
                    }
                }
                break;
            }

            return QWidget::event(ev);
        }


    private:
        EntityPropertyEditor* m_editor;
        int m_dropIndicatorOffset;
        int m_dropIndicatorRowWidgetOffset;
    };

    EntityPropertyEditor::SharedComponentInfo::SharedComponentInfo(AZ::Component* component)
    {
        m_instances.push_back(component);
    }

    ComponentFilter GetDefaultComponentFilter()
    {
        return AppearsInGameComponentMenu;
    }

    EntityPropertyEditor::EntityPropertyEditor(QWidget* pParent, Qt::WindowFlags flags, bool isLevelEntityEditor)
        : QWidget(pParent, flags)
        , m_propertyEditBusy(0)
        , m_componentFilter(GetDefaultComponentFilter())
        , m_componentPalette(nullptr)
        , m_autoScrollCount(0)
        , m_autoScrollMargin(16)
        , m_autoScrollQueued(false)
        , m_isSystemEntityEditor(false)
        , m_isLevelEntityEditor(isLevelEntityEditor)
        , m_commandInvokedHandler(
              [this](AZStd::string_view command, const AZ::ConsoleCommandContainer&, AZ::ConsoleFunctorFlags, AZ::ConsoleInvokedFrom)
              {
                  if (command == enableDPECVarName)
                  {
                      ClearInstances();
                      for (auto componentEditor : m_componentEditors)
                      {
                          componentEditor->deleteLater();
                      }
                      m_componentEditors.clear();
                      UpdateContents();
                  }
              })
    {
        m_commandInvokedHandler.Connect(AZ::Interface<AZ::IConsole>::Get()->GetConsoleCommandInvokedEvent());
        initEntityPropertyEditorResources();

        if (Prefab::IsInspectorOverrideManagementEnabled())
        {
            auto* propertyEditorToolsSystemInterface = AZ::Interface<PropertyEditorToolsSystemInterface>::Get();
            AZ_Assert(propertyEditorToolsSystemInterface != nullptr, "EntityPropertyEditor - "
                "PropertyEditorToolsSystemInterface is not available for registering prefab override label handler.");
            Prefab::RegisterPrefabOverrideLabelHandler(*propertyEditorToolsSystemInterface);
        }

        m_prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
        AZ_Assert(m_prefabPublicInterface != nullptr, "EntityPropertyEditor requires a PrefabPublicInterface instance on Initialize.");

        m_instanceUpdateExecutorInterface = AZ::Interface<Prefab::InstanceUpdateExecutorInterface>::Get();
        AZ_Assert(m_instanceUpdateExecutorInterface, "EntityPropertyEditor - Could not retrieve instance of InstanceUpdateExecutorInterface");

        m_readOnlyEntityPublicInterface = AZ::Interface<ReadOnlyEntityPublicInterface>::Get();
        AZ_Assert(m_readOnlyEntityPublicInterface != nullptr, "EntityPropertyEditor requires a ReadOnlyEntityPublicInterface instance on Initialize.");

        m_focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get();
        AZ_Assert(m_focusModeInterface != nullptr, "EntityPropertyEditor requires a FocusModeInterface instance on Initialize.");

        m_containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();

        setObjectName("EntityPropertyEditor");
        setAcceptDrops(true);

        m_isAlreadyQueuedRefresh = false;
        m_shouldScrollToNewComponents = false;
        m_shouldScrollToNewComponentsQueued = false;
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
        m_selectionEventAccepted = false;

        m_componentEditorLastSelectedIndex = -1;
        m_componentEditorsUsed = 0;
        m_componentEditors.clear();
        m_componentToEditorMap.clear();

        m_gui = new Ui::EntityPropertyEditorUI();
        m_gui->setupUi(this);
        if (AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            // The form embeds an orange (255,153,0) WindowText palette on these labels that
            // the app-wide `* { color: white }` rule always overrode; without the sheet the
            // palette would show. Reset them to the inherited (white) text colour.
            // TOKENIZE: InspectorHeaderTextColor
            for (QLabel* label : { static_cast<QLabel*>(m_gui->m_entityNameLabel), static_cast<QLabel*>(m_gui->m_entityIdLabel),
                                   static_cast<QLabel*>(m_gui->m_entityIdText), static_cast<QLabel*>(m_gui->m_prefabContainerLabel),
                                   static_cast<QLabel*>(m_gui->m_prefabContainerText), static_cast<QLabel*>(m_gui->m_statusLabel) })
            {
                label->setPalette(QPalette());
            }
        }
        m_gui->m_entityNameEditor->setReadOnly(false);
        m_gui->m_entityDetailsLabel->setObjectName("LabelEntityDetails");
        m_gui->m_entitySearchBox->setReadOnly(false);
        m_gui->m_entitySearchBox->setContextMenuPolicy(Qt::CustomContextMenu);
        m_gui->m_entitySearchBox->setClearButtonEnabled(true);
        AzQtComponents::LineEdit::applySearchStyle(m_gui->m_entitySearchBox);

        m_itemNames = QStringList{tr("Start active"), tr("Start inactive"), tr("Editor only")};
        const int itemNameCount = aznumeric_cast<int>(m_itemNames.size());
        QStandardItemModel* model = new QStandardItemModel(itemNameCount, 1);
        for (int row = 0; row < itemNameCount; ++row)
        {
            m_comboItems[row] = new QStandardItem(m_itemNames[row]);
            m_comboItems[row]->setCheckable(true);
            model->setItem(row, 0, m_comboItems[row]);
        }
        m_gui->m_statusComboBox->setModel(model);
        AzQtComponents::ComboBox::addCustomCheckStateStyle(m_gui->m_statusComboBox);
        EnableEditor(true);
        m_sceneIsNew = true;

        m_gui->m_prefabContainerText->setElideMode(Qt::ElideLeft);

        connect(m_gui->m_entityIcon, &QPushButton::clicked, this, &EntityPropertyEditor::BuildEntityIconMenu);
        connect(m_gui->m_addComponentButton, &QPushButton::clicked, this, &EntityPropertyEditor::OnAddComponent);
        connect(m_gui->m_entitySearchBox, &QLineEdit::textChanged, this, &EntityPropertyEditor::OnSearchTextChanged);
        connect(m_gui->m_entitySearchBox, &QWidget::customContextMenuRequested, this, &EntityPropertyEditor::OnSearchContextMenu);
        connect(m_gui->m_pinButton, &QToolButton::clicked, this, &EntityPropertyEditor::OpenPinnedInspector);
        connect(m_gui->m_collapseAllButton, &QToolButton::clicked, this, &EntityPropertyEditor::OnCollapseAll);

        m_componentPalette = new ComponentPaletteWidget(this, true);
        connect(m_componentPalette, &ComponentPaletteWidget::OnAddComponentEnd, this, [this]()
        {
            QueuePropertyRefresh();
            m_shouldScrollToNewComponents = true;
        });

        HideComponentPalette();

        m_overlay = new EntityPropertyEditorOverlay(this, m_gui->m_componentListContents);
        UpdateOverlay();

        // The list grows and shrinks below the cards in view: its resize paints only the area it exposes, not every card in view
        m_gui->m_componentListContents->setAttribute(Qt::WA_StaticContents);

        ToolsApplicationEvents::Bus::Handler::BusConnect();
        AZ::EntitySystemBus::Handler::BusConnect();
        EntityPropertyEditorNotificationBus::Handler::BusConnect();
        EntityPropertyEditorRequestBus::Handler::BusConnect();
        EditorWindowUIRequestBus::Handler::BusConnect();

        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);
        ReadOnlyEntityPublicNotificationBus::Handler::BusConnect(editorEntityContextId);

        m_spacer = nullptr;

        m_emptyIcon = QIcon();
        m_clearIcon = QIcon(":/AssetBrowser/Resources/close.png");

        m_dragIcon = QIcon(QStringLiteral(":/Cursors/Grabbing.svg"));
        m_dragCursor = QCursor(m_dragIcon.pixmap(16), 10, 5);

        m_serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(m_serializeContext, "Failed to acquire application serialize context.");

        m_isBuildingProperties = false;

        setTabOrder(m_gui->m_entityNameEditor, m_gui->m_statusComboBox);
        setTabOrder(m_gui->m_statusComboBox, m_gui->m_addComponentButton);

        // the world editor has a fixed id:

        connect(m_gui->m_entityNameEditor,
            SIGNAL(editingFinished()),
            this,
            SLOT(OnEntityNameChanged()));

        m_gui->m_entityNameEditor->installEventFilter(this);
        new AzQtComponents::LineEditRevertHandler(m_gui->m_entityNameEditor);

        connect(m_gui->m_statusComboBox,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(OnStatusChanged(int)));

        CreateActions();
        UpdateContents();

        EditorEntityContextNotificationBus::Handler::BusConnect();

        //forced to register global event filter with application for selection
        //mousePressEvent and other handlers won't trigger if event is accepted by a child widget like a QLineEdit
        //TODO explore other options to avoid referencing qApp and filtering all events even though research says
        //this is the way to do it without overriding or registering with all child widgets
        qApp->installEventFilter(this);

        // Lazy cards: the view reaching held cards builds them (a viewport resize is caught by eventFilter)
        connect(m_gui->m_componentList->verticalScrollBar(), &QScrollBar::valueChanged, this, &EntityPropertyEditor::ReleaseLazyCardsInView);

        ComponentModeFramework::EditorComponentModeNotificationBus::Handler::BusConnect(
            GetEntityContextId());
        ViewportEditorModeNotificationsBus::Handler::BusConnect(GetEntityContextId());

        m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();

        // Assign this widget to the Editor Entity Property Editor Action Context.
        AssignWidgetToActionContextHelper(
                EditorIdentifiers::EditorEntityPropertyEditorActionContextIdentifier, this);
    }

    EntityPropertyEditor::~EntityPropertyEditor()
    {
        RemoveWidgetFromActionContextHelper(
                EditorIdentifiers::EditorEntityPropertyEditorActionContextIdentifier, this);

        qApp->removeEventFilter(this);

        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        ComponentModeFramework::EditorComponentModeNotificationBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        ReadOnlyEntityPublicNotificationBus::Handler::BusDisconnect();
        EditorWindowUIRequestBus::Handler::BusDisconnect();
        EntityPropertyEditorRequestBus::Handler::BusDisconnect();
        EntityPropertyEditorNotificationBus::Handler::BusDisconnect();
        AZ::EntitySystemBus::Handler::BusDisconnect();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();

        for (auto& entityId : m_overrideSelectedEntityIds)
        {
            DisconnectFromEntityBuses(entityId);
        }

        delete m_gui;
    }

    void EntityPropertyEditor::OnSearchContextMenu(const QPoint& pos)
    {
        QMenu* menu = m_gui->m_entitySearchBox->createStandardContextMenu();
        menu->setStyleSheet("background-color: #333333");
        menu->exec(m_gui->m_entitySearchBox->mapToGlobal(pos));
        delete menu;
    }

    void EntityPropertyEditor::GetSelectedAndPinnedEntities(EntityIdList& selectedEntityIds)
    {
        // When run from tests we do not have a window and we require the function to always run.
        if (DoesOwnFocus())
        {
            // GetSelected will return either the selected entities or if a pinned inspector is selected
            // it will return the entities relevant to that inspector
            GetSelectedEntities(selectedEntityIds);
        }
    }

    void EntityPropertyEditor::GetSelectedEntities(EntityIdList& selectedEntityIds)
    {
        if (IsLockedToSpecificEntities())
        {
            // Only include entities that currently exist
            selectedEntityIds.clear();
            selectedEntityIds.reserve(m_overrideSelectedEntityIds.size());
            for (AZ::EntityId entityId : m_overrideSelectedEntityIds)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
                bool addEntity = (entity != nullptr);

                // If this is a Level Inspector, the level entity is only considered valid if we've
                // finished creating or loading a level.  It should *not* be considered valid when
                // you open the Editor without creating or loading a level, even though it exists.
                // It also should not be considered valid during the creation / loading process,
                // only when the process is finished.
                if (m_isLevelEntityEditor)
                {
                    bool levelOpen = false;
                    AzToolsFramework::EditorRequestBus::BroadcastResult(levelOpen, &AzToolsFramework::EditorRequests::IsLevelDocumentOpen);
                    addEntity = addEntity && levelOpen;
                }

                if (addEntity)
                {
                    selectedEntityIds.emplace_back(entityId);
                }
            }
        }
        else
        {
            ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);
        }
    }

    void EntityPropertyEditor::GetSelectedComponents(AZStd::unordered_set<AZ::EntityComponentIdPair>& selectedComponentEntityIds)
    {
        selectedComponentEntityIds.insert(m_selectedEntityComponentIds.begin(), m_selectedEntityComponentIds.end());
    }

    void EntityPropertyEditor::SetNewComponentId(AZ::ComponentId componentId)
    {
        m_newComponentId = componentId;
    }

    void EntityPropertyEditor::VisitComponentEditors(const VisitComponentEditorsCallback& callback) const
    {
        for (const ComponentEditor* componentEditor : m_componentEditors)
        {
            if (!componentEditor)
            {
                continue;
            }

            if (!callback(componentEditor))
            {
                return;
            }
        }
    }

    void EntityPropertyEditor::OnComponentSelectionChanged(
        EntityPropertyEditor* entityPropertyEditor,
        [[maybe_unused]] const AZStd::unordered_set<AZ::EntityComponentIdPair>& selectedEntityComponentIds)
    {
        if (entityPropertyEditor != this)
        {
            ClearComponentEditorSelection();
        }
    }

    void EntityPropertyEditor::SetOverrideEntityIds(const AzToolsFramework::EntityIdSet& entities)
    {
        m_overrideSelectedEntityIds = entities;

        for (auto& entityId : m_overrideSelectedEntityIds)
        {
            ConnectToEntityBuses(entityId);
        }

        m_gui->m_pinButton->setVisible(m_overrideSelectedEntityIds.empty() && !m_isSystemEntityEditor && !m_isLevelEntityEditor);

        UpdateContents();
    }

    void EntityPropertyEditor::BeforeEntitySelectionChanged()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        if (IsLockedToSpecificEntities())
        {
            return;
        }

        ClearComponentEditorDragging();
        // BeforeEntitySelectionChanged is called during undo/redo operations.
        // This is because the Entity gets completely destroyed and recreated
        // (Steps = deselect => destroy => create => select). In ComponentMode
        // we know a selection change event cannot happen for any other reason
        // than this, so ensure we do not refresh the Component Editor (this would
        // lose the current selection highlight of the active entity)
        if (!AzToolsFramework::ComponentModeFramework::InComponentMode())
        {
            ClearComponentEditorSelection();
            ClearComponentEditorState();
        }

        // Clear the focus of the current widget in order to trigger editing
        // completion before the selected entity list is modified. Otherwise,
        // the entity will not automatically be marked as dirty since it will
        // no longer be selected when receiving the BeforePropertyModified event
        if (DoesOwnFocus())
        {
            QWidget* widget = QApplication::focusWidget();
            if (this != widget)
            {
                widget->clearFocus();
            }
        }
    }

    void EntityPropertyEditor::AfterEntitySelectionChanged(
        const AzToolsFramework::EntityIdList& newlySelectedEntities,
        const AzToolsFramework::EntityIdList& newlyDeselectedEntities)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        if (IsLockedToSpecificEntities())
        {
            // ensure we refresh all entity property editors when
            // selection changes if they are pinned as certain operations
            // may be prohibited when an entity is not selected
            InvalidatePropertyDisplay(Refresh_AttributesAndValues);
            return;
        }

        for (AZ::EntityId newlySelectedEntity : newlySelectedEntities)
        {
            ConnectToEntityBuses(newlySelectedEntity);
        }

        for (AZ::EntityId newlyDeselectedEntity : newlyDeselectedEntities)
        {
            DisconnectFromEntityBuses(newlyDeselectedEntity);
        }

        if (!newlySelectedEntities.empty() || !newlyDeselectedEntities.empty())
        {
            ClearSearchFilter();
        }

        if (newlySelectedEntities.empty())
        {
            bool areAnyEntitiesSelected = false;
            ToolsApplicationRequests::Bus::BroadcastResult(areAnyEntitiesSelected, &ToolsApplicationRequests::AreAnyEntitiesSelected);
            if (!areAnyEntitiesSelected)
            {
                // Ensure a prompt refresh when all entities have been removed/deselected.
                ClearInstances(false); // nothing is selected, make sure all editors and instances are committed and cleared before updating
                UpdateContents();
                return;
            }
        }

        // when entity selection changed, we need to repopulate our GUI
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::OnEntityInitialized(const AZ::EntityId& entityId)
    {
        if (IsLockedToSpecificEntities())
        {
            // During slice reloading, an entity can be deregistered, then registered again.
            // Refresh if the editor had been locked to that entity.
            if (AZStd::find(m_overrideSelectedEntityIds.begin(), m_overrideSelectedEntityIds.end(), entityId) != m_overrideSelectedEntityIds.end())
            {
                QueuePropertyRefresh();
            }
        }
    }

    void EntityPropertyEditor::OnEntityDestroyed(const AZ::EntityId& entityId)
    {
        if (IsEntitySelected(entityId))
        {
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::OnEntityActivated(const AZ::EntityId& entityId)
    {
        if (IsEntitySelected(entityId))
        {
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        if (IsEntitySelected(entityId))
        {
            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name)
    {
        if (IsSingleEntitySelected(entityId))
        {
            m_gui->m_entityNameEditor->setText(QString(name.c_str()));
            SelectedEntityNameChanged(entityId, name);
        }
    }

    void EntityPropertyEditor::OnEntityStartStatusChanged(const AZ::EntityId& entityId)
    {
        if (IsEntitySelected(entityId))
        {
            UpdateStatusComboBox();
        }
    }

    void EntityPropertyEditor::OnComponentOverrideContextMenu(const QPoint& position)
    {
        m_menuManagerInterface->DisplayMenuAtScreenPosition(EditorIdentifiers::InspectorEntityComponentContextMenuIdentifier, position);
    }

    bool EntityPropertyEditor::IsEntitySelected(const AZ::EntityId& id) const
    {
        return AZStd::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), id) != m_selectedEntityIds.end();
    }

    bool EntityPropertyEditor::IsSingleEntitySelected(const AZ::EntityId& id) const
    {
        return m_selectedEntityIds.size() == 1 && IsEntitySelected(id);
    }

    void EntityPropertyEditor::OnStartPlayInEditor()
    {
        setEnabled(false);
    }

    void EntityPropertyEditor::OnStopPlayInEditor()
    {
        setEnabled(true);
    }

    void EntityPropertyEditor::UpdateEntityIcon()
    {
        AZStd::string iconSourcePath;
        AzToolsFramework::EditorRequestBus::BroadcastResult(iconSourcePath, &AzToolsFramework::EditorRequestBus::Events::GetDefaultEntityIcon);

        if (!m_selectedEntityIds.empty())
        {
            //Test if every entity has the same icon, if so use that icon, otherwise use the default one.
            AZStd::string firstIconPath;
            EditorEntityIconComponentRequestBus::EventResult(firstIconPath, m_selectedEntityIds.front(), &EditorEntityIconComponentRequestBus::Events::GetEntityIconPath);

            bool haveSameIcon = true;
            for (AZ::EntityId entityId : m_selectedEntityIds)
            {
                AZStd::string iconPath;
                EditorEntityIconComponentRequestBus::EventResult(iconPath, entityId, &EditorEntityIconComponentRequestBus::Events::GetEntityIconPath);
                if (iconPath != firstIconPath)
                {
                    haveSameIcon = false;
                    break;
                }
            }

            if (haveSameIcon && !firstIconPath.empty())
            {
                iconSourcePath = AZStd::move(firstIconPath);
            }
        }

        // setIcon schedules the icon's paint. No repaint(): its only caller is UpdateContents, whose updates are disabled, and there
        // Qt 6 still painted and flushed the whole window mid-rebuild; setUpdatesEnabled(true) at the rebuild's end repaints it anyway.
        m_gui->m_entityIcon->setIcon(AzQtComponents::CachedIcon(QString(iconSourcePath.c_str())));
    }

    EntityPropertyEditor::InspectorLayout EntityPropertyEditor::GetCurrentInspectorLayout() const
    {
        // Prefabs layout logic

        // If this is the container entity for the root instance, treat it like a level entity.
        AZ::EntityId levelContainerEntityId = m_prefabPublicInterface->GetLevelInstanceContainerEntityId();
        if (AZStd::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), levelContainerEntityId) != m_selectedEntityIds.end())
        {
            if (m_selectedEntityIds.size() > 1)
            {
                return InspectorLayout::Invalid;
            }
            else
            {
                return InspectorLayout::Level;
            }
        }
        else
        {
            // If this is the container entity for the currently focused prefab, utilize a separate layout.
            if (auto prefabFocusPublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabFocusPublicInterface>::Get())
            {
                AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
                EditorEntityContextRequestBus::BroadcastResult(
                    editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

                AZ::EntityId focusedPrefabContainerEntityId =
                    prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(editorEntityContextId);
                if (AZStd::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), focusedPrefabContainerEntityId) !=
                    m_selectedEntityIds.end())
                {
                    if (m_selectedEntityIds.size() > 1)
                    {
                        return InspectorLayout::Invalid; // we have somehow selected both the container and something else.
                    }
                    else
                    {
                        return InspectorLayout::ContainerEntityOfFocusedPrefab;
                    }
                }
                else
                {
                    // we have not selected THE container entity of the focused container,
                    // but we could potentially have selected one or more container entities.
                    bool anyContainerEntitySelected = false;
                    bool anyNonContainerEntitySelected = false;
                    for (const auto& selectedEntityId : m_selectedEntityIds)
                    {
                        if (m_prefabPublicInterface->IsInstanceContainerEntity(selectedEntityId))
                        {
                            anyContainerEntitySelected = true;
                        }
                        else
                        {
                            anyNonContainerEntitySelected = true;
                        }
                    }
                    if (anyContainerEntitySelected && anyNonContainerEntitySelected)
                    {
                        return InspectorLayout::Invalid; // we have somehow selected both a container and something else.
                    }
                    else if (anyContainerEntitySelected && !anyNonContainerEntitySelected)
                    {
                        return InspectorLayout::ContainerEntity;
                    }
                }
            }
        }

        return InspectorLayout::Entity;
    }

    void EntityPropertyEditor::UpdateEntityDisplay()
    {
        UpdateStatusComboBox();

        if (m_selectedEntityIds.size() > 1)
        {
            // Generic text for multiple entities selected
            m_gui->m_entityDetailsLabel->setVisible(true);
            m_gui->m_entityDetailsLabel->setText(tr("Common components shown"));
            m_gui->m_entityNameEditor->setText(tr("%n entities selected", "", static_cast<int>(m_selectedEntityIds.size())));
            m_gui->m_entityNameEditor->setReadOnly(true);

            m_gui->m_entityIdText->setText(tr("multiple selected"));
            m_gui->m_prefabContainerText->setText(tr("multiple prefab containers selected"));
        }
        else if (!m_selectedEntityIds.empty())
        {
            auto entityId = m_selectedEntityIds.front();

            // No entity details for one entity
            m_gui->m_entityDetailsLabel->setVisible(false);

            // If we're in edit mode, make the name field editable.
            m_gui->m_entityNameEditor->setReadOnly(!m_gui->m_componentListContents->isEnabled() || m_selectionContainsReadOnlyEntity);

            // get the name of the entity.
            auto entity = GetSelectedEntityById(entityId);
            m_gui->m_entityNameEditor->setText(entity ? entity->GetName().data() : "Entity Not Found");

            m_gui->m_entityIdText->setText(QString::number(static_cast<AZ::u64>(entityId)));

            if (m_prefabPublicInterface->IsInstanceContainerEntity(entityId))
            {
                // If the entity is a container entity, display the owning prefab path.
                AZ::IO::Path prefabPath = m_prefabPublicInterface->GetOwningInstancePrefabPath(entityId);
                m_gui->m_prefabContainerText->setText(QString::fromUtf8(prefabPath.c_str()));
            }
            else
            {
                m_gui->m_prefabContainerText->setText("");
            }
        }
    }

    EntityPropertyEditor::SelectionEntityTypeInfo EntityPropertyEditor::GetSelectionEntityTypeInfo(const EntityIdList& selection) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        SelectionEntityTypeInfo result = SelectionEntityTypeInfo::None;

        InspectorLayout layout = GetCurrentInspectorLayout();

        if (layout == InspectorLayout::Level)
        {
            // The Level Inspector should only have a list of selectable components after the
            // level entity itself is valid (i.e. "selected").
            return selection.empty() ? SelectionEntityTypeInfo::None : SelectionEntityTypeInfo::LevelEntity;
        }

        if (layout == InspectorLayout::ContainerEntityOfFocusedPrefab)
        {
            return selection.empty() ? SelectionEntityTypeInfo::None : SelectionEntityTypeInfo::ContainerEntityOfFocusedPrefab;
        }

        if (layout == InspectorLayout::Invalid)
        {
            return SelectionEntityTypeInfo::Mixed;
        }

        if (layout == InspectorLayout::ContainerEntity)
        {
            return SelectionEntityTypeInfo::OnlyPrefabEntities;
        }

        for (AZ::EntityId selectedEntityId : selection)
        {
            bool isPrefabEntity = (m_prefabPublicInterface) ? m_prefabPublicInterface->IsInstanceContainerEntity(selectedEntityId) : false;

            if (isPrefabEntity)
            {
                if (result == SelectionEntityTypeInfo::None)
                {
                    result = SelectionEntityTypeInfo::OnlyPrefabEntities;
                }
                else if (result == SelectionEntityTypeInfo::OnlyStandardEntities)
                {
                    result = SelectionEntityTypeInfo::Mixed;
                    // An entity of both prefab and non-prefab type have been found, so break out of the loop.
                    break;
                }
            }
            else
            {
                if (result == SelectionEntityTypeInfo::None)
                {
                    result = SelectionEntityTypeInfo::OnlyStandardEntities;
                }
                else if (result == SelectionEntityTypeInfo::OnlyLayerEntities || result == SelectionEntityTypeInfo::OnlyPrefabEntities)
                {
                    result = SelectionEntityTypeInfo::Mixed;
                    // An entity of both layer and non-layer type have been found, so break out of the loop.
                    break;
                }
            }
        }
        return result;
    }

    bool EntityPropertyEditor::CanAddComponentsToSelection(const SelectionEntityTypeInfo& selectionEntityTypeInfo) const
    {
        if (m_selectionContainsReadOnlyEntity)
        {
            // Can't add components if there is a read only entity in the selection
            return false;
        }

        if (selectionEntityTypeInfo == SelectionEntityTypeInfo::Mixed ||
            selectionEntityTypeInfo == SelectionEntityTypeInfo::None ||
            selectionEntityTypeInfo == SelectionEntityTypeInfo::OnlyPrefabEntities ||
            selectionEntityTypeInfo == SelectionEntityTypeInfo::ContainerEntityOfFocusedPrefab)
        {
            // Can't add components in mixed selection, or if nothing is selected, or if in prefab containers
            return false;
        }

        // This follows the pattern in OnAddComponent, which also uses an empty filter.
        AZ::ComponentDescriptor::DependencyArrayType serviceFilter;
        // Components can't be added if there are none available to be added.
        return ComponentPaletteUtil::ContainsEditableComponents(m_serializeContext, m_componentFilter, serviceFilter);
    }

    QString EntityPropertyEditor::GetEntityDetailsLabelText() const
    {
        QString entityDetailsLabelText("");
        if (IsLockedToSpecificEntities())
        {
            entityDetailsLabelText = QObject::tr("The entity this inspector was pinned to has been deleted.");
        }
        else
        {
            entityDetailsLabelText = QObject::tr("Select an entity to show its properties in the inspector.");
        }

        if (IsLockedToSpecificEntities())
        {
            if (m_isLevelEntityEditor)
            {
                entityDetailsLabelText = tr("Create or load a level to enable the Level Inspector.");
            }
            else
            {
                entityDetailsLabelText = tr("The entity this inspector was pinned to has been deleted.");
            }
        }
        else
        {
            entityDetailsLabelText = tr("Select an entity to show its properties in the inspector.");
        }

        return entityDetailsLabelText;
    }

    void EntityPropertyEditor::UpdateContents()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        const AZStd::chrono::steady_clock::time_point refreshStartTime = AZStd::chrono::steady_clock::now();
        m_lazyCardsStartUs = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(refreshStartTime.time_since_epoch()).count();
        // D1b sub-phase timing (ed_inspectorRefreshDiagnostics): localize where UpdateContents spends its time.
        // Default each boundary to the start so skipped phases (e.g. no entity selected) report 0ms.
        AZStd::chrono::steady_clock::time_point tAfterClear = refreshStartTime;
        AZStd::chrono::steady_clock::time_point tBeforeBuildArray = refreshStartTime;
        AZStd::chrono::steady_clock::time_point tAfterBuildArray = refreshStartTime;
        AZStd::chrono::steady_clock::time_point tAfterBuildUI = refreshStartTime;
        AZStd::chrono::steady_clock::time_point tBeforeEntityIcon = refreshStartTime;   // the header updates that end buildUI
        AZStd::chrono::steady_clock::time_point tAfterEntityIcon = refreshStartTime;

        // D1c: read the diagnostics cvar once for this rebuild and reset the per-component accumulators.
        m_inspectorDiagnosticsActive = false;
        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue(inspectorRefreshDiagnosticsCVarName, m_inspectorDiagnosticsActive);
        }
        m_diagCreateEditorUs = 0;
        m_diagAddInstanceUs = 0;
        m_diagOverrideVizUs = 0;
        m_diagNotificationsUs = 0;
        m_diagInvalidateUs = 0;
        m_diagShowUs = 0;
        m_diagClearHideUs = 0;
        m_diagClearContentUs = 0;
        m_diagClearPreventUs = 0;

        // The adapters' and property editors' own accumulators restart here, so the report below covers this rebuild only
        AZ::DocumentPropertyEditor::ReflectionAdapter::TakeGenerateContentsMicroseconds();
        AZ::u64 ignoredRowsUs = 0;
        AZ::u64 ignoredDirtyHandlersUs = 0;
        DocumentPropertyEditor::TakeResetTimings(ignoredRowsUs, ignoredDirtyHandlersUs);
        AZ::u64 ignoredGenerations = 0;
        AZ::u64 ignoredMismatches = 0;
        AZ::DocumentPropertyEditor::ReflectionAdapter::TakeVerifyCounts(ignoredGenerations, ignoredMismatches);

        // A freeze of the previous rebuild ends with this one: its updates stay disabled until this one's end
        setUpdatesEnabled(false);
        m_viewFrozen = false;

        m_isBuildingProperties = true;
        m_isAlreadyQueuedRefresh = false;

        HideComponentPalette();

        ClearInstances(false);
        const AZStd::chrono::steady_clock::time_point tAfterClearInstances = AZStd::chrono::steady_clock::now();
        ClearComponentEditorDragging();
        tAfterClear = AZStd::chrono::steady_clock::now();

        m_selectedEntityIds.clear();
        GetSelectedEntities(m_selectedEntityIds);

        // Check if any of the selected entities are marked as read only
        m_selectionContainsReadOnlyEntity = false;
        for (const auto& entityId : m_selectedEntityIds)
        {
            if (m_readOnlyEntityPublicInterface->IsReadOnly(entityId))
            {
                m_selectionContainsReadOnlyEntity = true;
                break;
            }
        }

        SourceControlFileInfo scFileInfo;
        ToolsApplicationRequests::Bus::BroadcastResult(scFileInfo, &ToolsApplicationRequests::GetSceneSourceControlInfo);

        if (!m_spacer)
        {
            m_spacer = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);
        }

        // Hide the entity stuff and add component button if no entities are displayed
        const bool hasEntitiesDisplayed = !m_selectedEntityIds.empty();

        SelectionEntityTypeInfo selectionEntityTypeInfo = GetSelectionEntityTypeInfo(m_selectedEntityIds);

        QString entityDetailsLabelText("");
        bool entityDetailsVisible = false;
        if (!hasEntitiesDisplayed)
        {
            entityDetailsVisible = true;
            entityDetailsLabelText = GetEntityDetailsLabelText();
        }
        else if (selectionEntityTypeInfo == SelectionEntityTypeInfo::OnlyLayerEntities)
        {
            // If a customer filter is not already in use, only show layer components.
            if (!m_customFilterSet)
            {
                // Don't call SetAddComponentMenuFilter because it will set the custom filter flag.
                m_componentFilter = AZStd::move(AppearsInLayerComponentMenu);
            }
        }
        else if (selectionEntityTypeInfo == SelectionEntityTypeInfo::OnlyStandardEntities)
        {
            // If a customer filter is not already in use, reset to default in case we were previously only showing layers.
            if (!m_customFilterSet)
            {
                // Don't call SetAddComponentMenuFilter because it will set the custom filter flag.
                m_componentFilter = GetDefaultComponentFilter();
            }
        }
        else if (selectionEntityTypeInfo == SelectionEntityTypeInfo::LevelEntity)
        {
            if (!m_customFilterSet)
            {
                m_componentFilter = AZStd::move(AppearsInLevelComponentMenu);
            }
        }

        InspectorLayout currentLayout = GetCurrentInspectorLayout();

        bool isLevelLayout = currentLayout == InspectorLayout::Level;
        bool isContainerOfFocusedPrefabLayout = currentLayout == InspectorLayout::ContainerEntityOfFocusedPrefab;
        bool isPrefabLayout = currentLayout == InspectorLayout::ContainerEntity || isContainerOfFocusedPrefabLayout;

        m_gui->m_entityDetailsLabel->setText(entityDetailsLabelText);
        m_gui->m_entityDetailsLabel->setVisible(entityDetailsVisible);
        m_gui->m_entityNameEditor->setVisible(hasEntitiesDisplayed);
        m_gui->m_entityNameLabel->setVisible(hasEntitiesDisplayed);
        m_gui->m_entityIcon->setVisible(hasEntitiesDisplayed);
        m_gui->m_pinButton->setVisible(m_overrideSelectedEntityIds.empty() && hasEntitiesDisplayed && !m_isSystemEntityEditor);
        m_gui->m_statusLabel->setVisible(
            hasEntitiesDisplayed && !m_isSystemEntityEditor && !isLevelLayout);
        m_gui->m_statusComboBox->setVisible(
            hasEntitiesDisplayed && !m_isSystemEntityEditor && !isLevelLayout);
        m_gui->m_entityIdLabel->setVisible(
            hasEntitiesDisplayed && !m_isSystemEntityEditor && !isLevelLayout);
        m_gui->m_entityIdText->setVisible(
            hasEntitiesDisplayed && !m_isSystemEntityEditor && !isLevelLayout);

        bool displayComponentSearchBox = hasEntitiesDisplayed;
        if (hasEntitiesDisplayed)
        {
            tBeforeBuildArray = AZStd::chrono::steady_clock::now();
            // Build up components to display
            SharedComponentArray sharedComponentArray;
            BuildSharedComponentArray(sharedComponentArray,
                !(selectionEntityTypeInfo == SelectionEntityTypeInfo::OnlyStandardEntities ||
                    selectionEntityTypeInfo == SelectionEntityTypeInfo::OnlyPrefabEntities) ||
                selectionEntityTypeInfo == SelectionEntityTypeInfo::ContainerEntityOfFocusedPrefab);
            tAfterBuildArray = AZStd::chrono::steady_clock::now();

            if (sharedComponentArray.size() == 0)
            {
                // Don't display the search box if there were no common components.
                displayComponentSearchBox = false;
            }
            else
            {
                BuildSharedComponentUI(sharedComponentArray);
            }


            tBeforeEntityIcon = AZStd::chrono::steady_clock::now();
            UpdateEntityIcon();
            tAfterEntityIcon = AZStd::chrono::steady_clock::now();
            UpdateEntityDisplay();
            tAfterBuildUI = AZStd::chrono::steady_clock::now();
        }

        m_gui->m_darkBox->setVisible(displayComponentSearchBox && !m_isSystemEntityEditor && !isLevelLayout || isPrefabLayout);
        m_gui->m_entitySearchBox->setVisible(displayComponentSearchBox);
        m_gui->m_collapseAllButton->setVisible(displayComponentSearchBox);

        bool isEditingPrefabContainer = isContainerOfFocusedPrefabLayout;

        m_gui->m_entityIdWidget->setVisible(!isEditingPrefabContainer);
        m_gui->m_prefabContainerWidget->setVisible(isPrefabLayout);

        bool displayAddComponentMenu = !isPrefabLayout && CanAddComponentsToSelection(selectionEntityTypeInfo);
        m_gui->m_addComponentButton->setVisible(displayAddComponentMenu);

        if (m_shouldScrollToNewComponents)
        {
            QueueScrollToNewComponent();
        }
        else if (!m_shouldScrollToNewComponentsQueued)
        {
            // if we are not going to scroll to new components, then
            // immediately execute a scroll to restore the saved scroll position.
            QTimer::singleShot(0, this, &EntityPropertyEditor::ScrollToNewComponent);
            m_shouldScrollToNewComponentsQueued = true;
        }

        LoadComponentEditorState();
        UpdateInternalState();

        layout()->update();
        layout()->activate();

        // with ed_inspectorSettleLayout, the cards where they stay before the inspector paints them (a reused card is still where the
        // previous selection's was)
        SettleComponentList(SettleOrigin::Build);

        m_isBuildingProperties = false;

        // An interruptible rebuild builds the cards in view for a while at once, then keeps the inspector's last picture until the idle
        // steps have built the rest of them
        if (m_interruptibleRebuild && InterruptibleInlineBudgetUs() > 0)
        {
            BuildViewCards(InterruptibleInlineBudgetUs());
        }
        AZ::s64 viewShownUs = 0;    // the view shown by this rebuild itself (reported after its own line)
        if (m_interruptibleRebuild && !ViewCardsBuilt())
        {
            StartViewFreeze();
        }
        else
        {
            setUpdatesEnabled(true);
            if (m_interruptibleRebuild)
            {
                viewShownUs =
                    AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now().time_since_epoch()).count();
            }
        }

        // re-enable all actions so that things like "delete" or "cut" work again
        QList<QAction*> actionList = actions();
        for (QAction* action : actionList)
        {
            action->setEnabled(true);
        }

        // Make sure that prefab instance propagation is resumed after a property tree refresh.
        // Note: this is a sanity check in case IPropertyEditorNotify::BeforePropertyModified
        // is called, but a IPropertyEditorNotify::SetPropertyEditingComplete isn't called
        m_instanceUpdateExecutorInterface->SetShouldPauseInstancePropagation(false);

        // D1/D1b measurement harness. Reports sub-phase timing of UpdateContents and, crucially, PER-COMPONENT reuse
        // opportunity (reusableComps) rather than whole-entity composition match. ComponentEditors/adapters are
        // per-component, so the reusable unit is the component TYPE, not the whole entity: most selections share
        // common types (Transform, Mesh, ...) even when the entity as a whole differs. Whole-entity match almost
        // never happens and is the wrong thing to optimize for.
        if (m_inspectorDiagnosticsActive)
        {
            AZStd::vector<AZ::Uuid> currentSignature;
            currentSignature.reserve(m_componentEditorsUsed);
            for (AZ::s32 componentEditorIndex = 0; componentEditorIndex < m_componentEditorsUsed; ++componentEditorIndex)
            {
                currentSignature.push_back(m_componentEditors[componentEditorIndex]->GetComponentType());
            }

            // Per-component reuse vs the previous selection: multiset intersection of component types.
            // Counts how many of this selection's component editors COULD have been kept-and-re-pointed
            // instead of cleared and rebuilt.
            int reusableComponents = 0;
            AZStd::vector<bool> prevConsumed(m_lastBuiltComponentSignature.size(), false);
            for (const AZ::Uuid& typeId : currentSignature)
            {
                for (size_t prevIndex = 0; prevIndex < m_lastBuiltComponentSignature.size(); ++prevIndex)
                {
                    if (!prevConsumed[prevIndex] && m_lastBuiltComponentSignature[prevIndex] == typeId)
                    {
                        prevConsumed[prevIndex] = true;
                        ++reusableComponents;
                        break;
                    }
                }
            }

            const auto toMs = [](AZStd::chrono::steady_clock::duration d)
            {
                return aznumeric_cast<double>(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(d).count()) / 1000.0;
            };
            const AZStd::chrono::steady_clock::time_point tEnd = AZStd::chrono::steady_clock::now();
            AZ_Printf(
                "EntityInspector",
                "UpdateContents: entities=%zu components=%d total=%.2fms "
                "[clear=%.2f preamble=%.2f buildArray=%.2f buildUI=%.2f finalize=%.2f] reusableComps=%d/%d",
                m_selectedEntityIds.size(),
                static_cast<int>(m_componentEditorsUsed),
                toMs(tEnd - refreshStartTime),
                toMs(tAfterClear - refreshStartTime),
                toMs(tBeforeBuildArray - tAfterClear),
                toMs(tAfterBuildArray - tBeforeBuildArray),
                toMs(tAfterBuildUI - tAfterBuildArray),
                toMs(tEnd - tAfterBuildUI),
                reusableComponents,
                static_cast<int>(currentSignature.size()));

            // D1c: per-component breakdown of buildUI + the split of the clear phase.
            AZ_Printf(
                "EntityInspector",
                "  buildUI: createEditor=%.2f addInstance=%.2f overrideViz=%.2f notifications=%.2f invalidate=%.2f show=%.2f"
                " | clear: instances=%.2f dragging=%.2f [hide=%.2f content=%.2f prevent=%.2f] (ms)",
                aznumeric_cast<double>(m_diagCreateEditorUs) / 1000.0,
                aznumeric_cast<double>(m_diagAddInstanceUs) / 1000.0,
                aznumeric_cast<double>(m_diagOverrideVizUs) / 1000.0,
                aznumeric_cast<double>(m_diagNotificationsUs) / 1000.0,
                aznumeric_cast<double>(m_diagInvalidateUs) / 1000.0,
                aznumeric_cast<double>(m_diagShowUs) / 1000.0,
                toMs(tAfterClearInstances - refreshStartTime),
                toMs(tAfterClear - tAfterClearInstances),
                aznumeric_cast<double>(m_diagClearHideUs) / 1000.0,
                aznumeric_cast<double>(m_diagClearContentUs) / 1000.0,
                aznumeric_cast<double>(m_diagClearPreventUs) / 1000.0);

            // The DPE parts of addInstance + invalidate, over all component editors: the adapters generating their contents (the
            // DOM), the property editors building the rows from it, and refreshing the new handlers
            AZ::u64 resetRowsUs = 0;
            AZ::u64 resetDirtyHandlersUs = 0;
            DocumentPropertyEditor::TakeResetTimings(resetRowsUs, resetDirtyHandlersUs);
            const AZ::u64 generateContentsUs = AZ::DocumentPropertyEditor::ReflectionAdapter::TakeGenerateContentsMicroseconds();
            AZ_Printf(
                "EntityInspector",
                "  dpe: dom=%.2f rows=%.2f dirtyHandlers=%.2f (ms)",
                aznumeric_cast<double>(generateContentsUs) / 1000.0,
                aznumeric_cast<double>(resetRowsUs) / 1000.0,
                aznumeric_cast<double>(resetDirtyHandlersUs) / 1000.0);

            // The header updates after the component editors, the rest of buildUI
            AZ_Printf(
                "EntityInspector",
                "  header: entityIcon=%.2f entityDisplay=%.2f (ms)",
                toMs(tAfterEntityIcon - tBeforeEntityIcon),
                toMs(tAfterBuildUI - tAfterEntityIcon));

            // With ed_dpeVerifyContentCaches: the contents generated twice in this rebuild, and how many differed (must be 0)
            AZ::u64 verifiedGenerations = 0;
            AZ::u64 mismatchedGenerations = 0;
            AZ::DocumentPropertyEditor::ReflectionAdapter::TakeVerifyCounts(verifiedGenerations, mismatchedGenerations);
            if (verifiedGenerations > 0)
            {
                AZ_Printf(
                    "EntityInspector",
                    "  dpeVerify: generations=%llu mismatches=%llu",
                    static_cast<unsigned long long>(verifiedGenerations),
                    static_cast<unsigned long long>(mismatchedGenerations));
            }

            m_lastBuiltComponentSignature = AZStd::move(currentSignature);
            m_lastBuiltSelectionCount = m_selectedEntityIds.size();

            if (viewShownUs != 0)
            {
                ReportFirstView(viewShownUs);
            }
        }
        else
        {
            // Forget the previous selection so a re-enabled report never compares against a stale one.
            m_lastBuiltComponentSignature.clear();
        }
    }

    void EntityPropertyEditor::GetAllComponentsForEntityInOrder(
        const AZ::Entity* entity, AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        componentsOnEntity.clear();

        if (entity)
        {
            const AZ::EntityId entityId = entity->GetId();

            // get all components related to the entity in sorted and fixed order buckets
            GetAllComponentsForEntity(entity, componentsOnEntity);

            RemoveHiddenComponents(componentsOnEntity);
            SortComponentsByOrder(entityId, componentsOnEntity);
            SortComponentsByPriority(componentsOnEntity);
        }
    }

    void SaveComponentOrder(const AZ::EntityId entityId, AZStd::span<AZ::Component* const> componentsInOrder)
    {
        ComponentOrderArray componentOrder;
        componentOrder.clear();
        componentOrder.reserve(componentsInOrder.size());

        for (auto component : componentsInOrder)
        {
            if (component && component->GetEntityId() == entityId)
            {
                componentOrder.push_back(component->GetId());
            }
        }

        EditorInspectorComponentRequestBus::Event(
            entityId, &EditorInspectorComponentRequests::SetComponentOrderArray, componentOrder);
    }

    bool EntityPropertyEditor::DoesComponentPassFilter(const AZ::Component* component, const ComponentFilter& filter)
    {
        auto componentClassData = component ? GetComponentClassData(component) : nullptr;
        return componentClassData && filter(*componentClassData);
    }

   bool EntityPropertyEditor::AreComponentsRemovable(AZStd::span<AZ::Component* const> components) const
    {
        for (auto component : components)
        {
            if (!IsComponentRemovable(component))
            {
                return false;
            }
        }
        return true;
    }


    bool EntityPropertyEditor::AllowAnyComponentModification() const
    {
        const InspectorLayout currentLayout = GetCurrentInspectorLayout();
        return currentLayout != InspectorLayout::ContainerEntityOfFocusedPrefab &&
               currentLayout != InspectorLayout::ContainerEntity;
    }

    bool EntityPropertyEditor::AreComponentsDraggable(AZStd::span<AZ::Component* const> components) const
    {
        if (!AllowAnyComponentModification())
        {
            return false;
        }

        return AZStd::all_of(
            components.begin(), components.end(), [](AZ::Component* component) { return IsComponentDraggable(component); });
    }

    bool EntityPropertyEditor::AreComponentsCopyable(AZStd::span<AZ::Component* const> components) const
    {
        return AreComponentsCopyable(components, m_componentFilter);
    }

    bool EntityPropertyEditor::AreComponentsCopyable(AZStd::span<AZ::Component* const> components, const ComponentFilter& filter)
    {
        for (auto component : components)
        {
            if (!DoesComponentPassFilter(component, filter))
            {
                auto editorComponentDescriptor = GetEditorComponentDescriptor(component);
                if (!editorComponentDescriptor || !editorComponentDescriptor->SupportsPasteOver())
                {
                    return false;
                }
            }
        }
        return true;
    }

    AZ::Component* EntityPropertyEditor::ExtractMatchingComponent(AZ::Component* referenceComponent, AZ::Entity::ComponentArrayType& availableComponents)
    {
        for (auto availableComponentIterator = availableComponents.begin(); availableComponentIterator != availableComponents.end(); ++availableComponentIterator)
        {
            auto component = *availableComponentIterator;
            auto editorComponent = GetEditorComponent(component);
            auto referenceEditorComponent = GetEditorComponent(referenceComponent);

            // Early out on class data not matching, we require components to be the same class
            if (GetComponentClassData(referenceComponent) != GetComponentClassData(component))
            {
                continue;
            }

            auto referenceEditorComponentDescriptor = GetEditorComponentDescriptor(referenceComponent);
            // If not AZ_EDITOR_COMPONENT, then as long as they match type (checked above) it is considered matching
            // Otherwise, use custom component matching available via AZ_EDITOR_COMPONENT
            if (!referenceEditorComponentDescriptor || referenceEditorComponentDescriptor->DoComponentsMatch(referenceEditorComponent, editorComponent))
            {
                // Remove the component we found from the available set
                availableComponents.erase(availableComponentIterator);
                return component;
            }
        }

        return nullptr;
    }

    void EntityPropertyEditor::BuildSharedComponentArray(SharedComponentArray& sharedComponentArray, bool containsLayerEntity)
    {
        AZ_Assert(!m_selectedEntityIds.empty(), "BuildSharedComponentArray should only be called if there are entities being displayed");

        AZ::EntityId entityId = m_selectedEntityIds.front();
        AZ::Entity* entity = GetSelectedEntityById(entityId);

        // Skip building sharedComponentArray if a runtime entity isn't already activated
        if (!entity || (!m_isSystemEntityEditor && entity->GetState() != AZ::Entity::State::Active))
        {
            return;
        }

        // Gather initial list of eligible display components from the first entity
        AZ::Entity::ComponentArrayType entityComponents;
        GetAllComponentsForEntityInOrder(entity, entityComponents);

        for (auto component : entityComponents)
        {
            // Skip components that we should not display
            // Only need to do this for the initial set, components without edit data on other entities should not match these
            if (!ShouldInspectorShowComponent(component))
            {
                continue;
            }
            // Filters for non-layer entities aren't setup in the same way as layer entities,
            // inclusion in the add components menu does not match the list of what components will display.
            // Layers are setup in this way.
            if (containsLayerEntity)
            {
                const AZ::SerializeContext::ClassData* componentClassData = GetComponentClassData(component);
                if (componentClassData && !m_componentFilter(*componentClassData))
                {
                    continue;
                }
            }

            sharedComponentArray.push_back(SharedComponentInfo(component));
        }

        // Now loop over the other entities
        for (size_t entityIndex = 1; entityIndex < m_selectedEntityIds.size(); ++entityIndex)
        {
            entity = GetSelectedEntityById(m_selectedEntityIds[entityIndex]);
            if (!entity)
            {
                continue;
            }

            entityComponents.clear();
            GetAllComponentsForEntityInOrder(entity, entityComponents);

            // Loop over the known components on all entities (so far)
            // Check to see if they are also on this entity
            // If not, remove them from the list
            for (auto sharedComponentInfoIterator = sharedComponentArray.begin(); sharedComponentInfoIterator != sharedComponentArray.end(); )
            {
                auto firstComponent = sharedComponentInfoIterator->m_instances[0];

                auto matchingComponent = ExtractMatchingComponent(firstComponent, entityComponents);
                if (!matchingComponent)
                {
                    // Remove this as it isn't on all entities so don't bother continuing with it
                    sharedComponentInfoIterator = sharedComponentArray.erase(sharedComponentInfoIterator);
                    continue;
                }

                // Add the matching component and continue
                sharedComponentInfoIterator->m_instances.push_back(matchingComponent);
                ++sharedComponentInfoIterator;
            }
        }
    }

    bool EntityPropertyEditor::ComponentMatchesCurrentFilter(SharedComponentInfo& sharedComponentInfo) const
    {
        if (m_filterString.size() == 0)
        {
            return true;
        }

        if (sharedComponentInfo.m_instances.front())
        {
            AZStd::string componentName = GetFriendlyComponentName(sharedComponentInfo.m_instances.front());

            if (componentName.size() == 0 || AzFramework::StringFunc::Find(componentName.c_str(), m_filterString.c_str()) != AZStd::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    void EntityPropertyEditor::BuildSharedComponentUI(SharedComponentArray& sharedComponentArray)
    {
        // last-known widget, for the sake of tab-ordering
        QWidget* lastTabWidget = m_gui->m_addComponentButton;

        // D1c per-component timing: attribute the buildUI cost to each sub-call. markUs() advances the marker and
        // accumulates the elapsed microseconds into the given bucket; a no-op when diagnostics are off.
        const bool diag = m_inspectorDiagnosticsActive;
        auto markUs = [diag](AZStd::chrono::steady_clock::time_point& marker, AZ::s64& accumulatorUs)
        {
            if (!diag)
            {
                return;
            }
            const AZStd::chrono::steady_clock::time_point now = AZStd::chrono::steady_clock::now();
            accumulatorUs += AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(now - marker).count();
            marker = now;
        };

        // Lazy cards: the cards from the top build their rows until they cover the view as it will be shown (the scroll position the
        // rebuild restores, the view, and one screen below); the cards after them, and collapsed ones, hold theirs. Not with a filter
        // (whether a card shows at all depends on its rows) or when the view goes to a new component.
        // An interruptible rebuild holds every card: the idle steps build the view's first.
        const bool lazyCards = ed_inspectorLazyCards && ShouldUseDPE() && m_filterString.empty() && !m_shouldScrollToNewComponents;
        const int lazyCardsCover =
            m_interruptibleRebuild ? 0 : AZStd::max(m_savedVerticalScroll, 0) + 2 * m_gui->m_componentList->viewport()->height();
        const int cardSpacing = AZStd::max(m_gui->m_componentListContents->layout()->spacing(), 0);
        int cardsHeight = 0;

        for (auto& sharedComponentInfo : sharedComponentArray)
        {
            bool componentInFilter = ComponentMatchesCurrentFilter(sharedComponentInfo);

            AZStd::chrono::steady_clock::time_point mark =
                diag ? AZStd::chrono::steady_clock::now() : AZStd::chrono::steady_clock::time_point{};

            auto componentEditor = CreateComponentEditor();
            markUs(mark, m_diagCreateEditorUs);

            // A card past the cover holds its rows before its instances are added, so that the resets they queue build nothing
            if (lazyCards && cardsHeight >= lazyCardsCover)
            {
                componentEditor->HoldContentBuild();
            }

            // Add instances to componentEditor
            auto& componentInstances = sharedComponentInfo.m_instances;
            for (AZ::Component* componentInstance : componentInstances)
            {
                // non-first instances are aggregated under the first instance
                AZ::Component* aggregateInstance = componentInstance != componentInstances.front() ? componentInstances.front() : nullptr;

                componentEditor->AddInstance(componentInstance, aggregateInstance, nullptr);
            }
            markUs(mark, m_diagAddInstanceUs);

            // Set up other entity property editor customization
            if (ShouldUseDPE() && Prefab::IsInspectorOverrideManagementEnabled())
            {
                // Set up visualization for overrides on the component
                UpdateOverrideVisualization(*componentEditor);
            }
            markUs(mark, m_diagOverrideVizUs);

            // Set tab order for editor
            setTabOrder(lastTabWidget, componentEditor);
            lastTabWidget = componentEditor;

            // Refresh editor
            componentEditor->AddNotifications();
            markUs(mark, m_diagNotificationsUs);
            componentEditor->UpdateExpandability();
            // A collapsed card builds its rows when it is expanded (or at idle); a held card's InvalidateAll builds nothing
            if (lazyCards && !componentEditor->IsExpanded())
            {
                componentEditor->HoldContentBuild();
            }
            componentEditor->InvalidateAll(!componentInFilter ? m_filterString.c_str() : nullptr);
            markUs(mark, m_diagInvalidateUs);

            // If we are in read only mode, then show the components as disabled
            if (m_selectionContainsReadOnlyEntity)
            {
                componentEditor->mockDisabledState(true);
            }

            if (componentEditor->HasContents())
            {
                for (AZ::Component* componentInstance : componentInstances)
                {
                    // Map the component to the editor
                    m_componentToEditorMap[componentInstance] = componentEditor;
                }
                componentEditor->show();
                if (lazyCards)
                {
                    cardsHeight += componentEditor->sizeHint().height() + cardSpacing;
                }
            }
            else
            {
                componentEditor->hide();
                componentEditor->ClearInstances(true);
            }
            markUs(mark, m_diagShowUs);
        }

        // The held cards build at idle, one step at a time
        m_lazyCardsHeldAtBuild = GetLazyCardsPending();
        m_lazyCardsSteps = 0;
        if (m_lazyCardsHeldAtBuild > 0)
        {
            m_lazyCardsDiagnostics = diag;
            QueueLazyCardsStep();
        }
        UpdateLazyCardsPendingProperty();
    }

    // ----------------------------------------------------------------------------------
    // Component list layout
    // ----------------------------------------------------------------------------------

    //! Counts the layout requests the application delivers while it filters the application's events
    class LayoutRequestCounter : public QObject
    {
    public:
        bool eventFilter(QObject* watched, QEvent* event) override
        {
            m_delivered += event->type() == QEvent::LayoutRequest ? 1 : 0;
            return QObject::eventFilter(watched, event);
        }

        int m_delivered = 0;
    };

    void EntityPropertyEditor::SettleComponentList(SettleOrigin origin)
    {
        // A layout that changes asks the layout above it to follow with a posted layout request: a card's rows, the card, the list, and
        // the scroll area that sizes the list. The event loop sends the requests posted before it started, then paints the inspector,
        // and sends the requests those posted on a later turn: after a rebuild a reused card showed where the previous selection's
        // card was (over the card above it) and moved on the next turn; an expanded row's rows showed before they were placed. The
        // requests are sent here round after round until none is left.
        // A rebuild and an idle lazy cards step settle only with ed_inspectorSettleLayout; an interaction always does, or the card the
        // user acts on bounces.
        if (m_componentListSettling || (origin == SettleOrigin::Build && !ed_inspectorSettleLayout))
        {
            return;
        }
        m_componentListSettling = true;

        LayoutRequestCounter counter;
        QCoreApplication::instance()->installEventFilter(&counter);
        int rounds = 0;
        while (rounds < SettleRoundsMax)
        {
            counter.m_delivered = 0;
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            if (counter.m_delivered == 0)
            {
                break;
            }
            ++rounds;
        }
        QCoreApplication::instance()->removeEventFilter(&counter);

        m_componentListSettling = false;
    }

    // ----------------------------------------------------------------------------------
    // Interruptible load (ed_inspectorInterruptibleLoad)
    // ----------------------------------------------------------------------------------

    void EntityPropertyEditor::RunQueuedRefresh()
    {
        // A selection queues this rebuild ahead of the paints it causes (Qt posts paints at low priority), so the outliner showed the
        // new selection only once the inspector was rebuilt. The pending paints go first.
        if (ed_inspectorInterruptibleLoad)
        {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::UpdateRequest);
        }

        m_interruptibleRebuild = ed_inspectorInterruptibleLoad;
        UpdateContents();
        m_interruptibleRebuild = false;
    }

    int EntityPropertyEditor::FirstViewCardHeld() const
    {
        // the bottom of the view the rebuild restores (the saved scroll position), and the cards' tops at their heights now (a held
        // card has its header's)
        const int viewBottom = AZStd::max(m_savedVerticalScroll, 0) + m_gui->m_componentList->viewport()->height();
        const QLayout* listLayout = m_gui->m_componentListContents->layout();
        const int spacing = AZStd::max(listLayout->spacing(), 0);
        int top = listLayout->contentsMargins().top();
        for (AZ::s32 index = 0; index < m_componentEditorsUsed && top < viewBottom; ++index)
        {
            const ComponentEditor* componentEditor = m_componentEditors[index];
            if (!componentEditor->isVisible())
            {
                continue;
            }
            if (componentEditor->IsContentBuildHeld() && componentEditor->IsExpanded())
            {
                return index;
            }
            top += componentEditor->sizeHint().height() + spacing;
        }
        return -1;
    }

    void EntityPropertyEditor::BuildViewCards(AZ::s64 budgetUs)
    {
        const AZStd::chrono::steady_clock::time_point start = AZStd::chrono::steady_clock::now();
        for (int held = FirstViewCardHeld(); held >= 0; held = FirstViewCardHeld())
        {
            ReleaseLazyCardsThrough(held, SettleOrigin::Build);

            const AZ::s64 elapsedUs =
                AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - start).count();
            if (elapsedUs >= budgetUs)
            {
                return;
            }
        }
    }

    void EntityPropertyEditor::StartViewFreeze()
    {
        // the updates stay disabled (UpdateContents disabled them) and the mouse is ignored until EndViewFreeze
        m_viewFrozen = true;
        const AZ::u64 generation = ++m_viewFreezeGeneration;
        QTimer::singleShot(ViewFreezeMaxMs, this, [this, generation]()
            {
                // the steps did not get there (paused, or a rebuild is queued and will end the freeze): build the view now
                if (generation != m_viewFreezeGeneration || !m_viewFrozen || m_isAlreadyQueuedRefresh || m_isBuildingProperties)
                {
                    return;
                }
                BuildViewCards(AZStd::numeric_limits<AZ::s64>::max());
                EndViewFreeze();
                UpdateLazyCardsPendingProperty();
            });
    }

    void EntityPropertyEditor::EndViewFreeze()
    {
        if (!m_viewFrozen)
        {
            return;
        }
        m_viewFrozen = false;

        // The cards in view at their heights before the one paint that shows them (a step leaves the layout to the event loop),
        // and the scroll position restored again: when the rebuild restored it, the held cards were too short to reach it
        SettleComponentList(SettleOrigin::Interaction);
        if (m_savedVerticalScroll >= 0)
        {
            m_gui->m_componentList->verticalScrollBar()->setValue(m_savedVerticalScroll);
        }
        setUpdatesEnabled(true);
        ReportFirstView(
            AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now().time_since_epoch()).count());
    }

    void EntityPropertyEditor::ReportFirstView(AZ::s64 shownUs) const
    {
        if (m_inspectorDiagnosticsActive)
        {
            AZ_Printf(
                "EntityInspector", "  firstView: ms=%.2f steps=%d (ms since the rebuild started)",
                aznumeric_cast<double>(shownUs - m_lazyCardsStartUs) / 1000.0, m_lazyCardsSteps);
        }
    }

    bool EntityPropertyEditor::IgnoredWhileFrozen(const QObject* object, const QEvent* event) const
    {
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
        case QEvent::ContextMenu:
            return object->isWidgetType() && (object == this || isAncestorOf(static_cast<const QWidget*>(object)));
        default:
            return false;
        }
    }

    // ----------------------------------------------------------------------------------
    // Lazy cards (ed_inspectorLazyCards)
    // ----------------------------------------------------------------------------------

    int EntityPropertyEditor::GetLazyCardsPending() const
    {
        int held = 0;
        for (AZ::s32 index = 0; index < m_componentEditorsUsed; ++index)
        {
            held += m_componentEditors[index]->IsContentBuildHeld() ? 1 : 0;
        }
        return held;
    }

    void EntityPropertyEditor::ReleaseLazyCardsThrough(int lastIndex, SettleOrigin settleOrigin, AZ::s64 budgetUs)
    {
        if (m_lazyCardsReleasing)
        {
            return;
        }
        m_lazyCardsReleasing = true;

        const AZStd::chrono::steady_clock::time_point releaseStart = AZStd::chrono::steady_clock::now();
        QScrollBar* verticalScroll = m_gui->m_componentList->verticalScrollBar();
        bool released = false;
        for (int index = 0; index <= lastIndex && index < m_componentEditorsUsed; ++index)
        {
            ComponentEditor* componentEditor = m_componentEditors[index];
            if (!componentEditor->IsContentBuildHeld())
            {
                continue;
            }

            const bool aboveView = componentEditor->isVisible() && componentEditor->geometry().bottom() < verticalScroll->value();
            const int heightBefore = componentEditor->height();
            componentEditor->ReleaseContentBuild();
            released = true;

            // A card above the view resizes the list now, so that the view keeps what it shows (the height it gains is only known
            // once the list is laid out, so this settles whatever the origin)
            if (aboveView)
            {
                SettleComponentList(SettleOrigin::Interaction);
                verticalScroll->setValue(verticalScroll->value() + componentEditor->height() - heightBefore);
            }

            // an idle step stops when its budget is spent (after one card at least)
            const AZ::s64 elapsedUs =
                AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - releaseStart).count();
            if (budgetUs > 0 && elapsedUs >= budgetUs)
            {
                break;
            }
        }

        // the built cards at their new heights, and the cards below them moved, before the inspector paints
        if (released)
        {
            SettleComponentList(settleOrigin);
        }

        m_lazyCardsReleasing = false;
    }

    bool EntityPropertyEditor::CanReleaseLazyCards() const
    {
        // not inside a release, during a rebuild, or with one queued (the held cards' components may be going)
        return !m_lazyCardsReleasing && !m_isBuildingProperties && !m_isAlreadyQueuedRefresh && GetLazyCardsPending() > 0;
    }

    void EntityPropertyEditor::ReleaseLazyCardsInView()
    {
        // While the view is frozen the steps build it: the scroll bar that shows as the list grows (a viewport resize) and the scroll
        // restore must not build the whole view at once
        if (m_viewFrozen || !CanReleaseLazyCards())
        {
            return;
        }

        // the last card that starts in the view or in the screen below it
        const int viewBottom = m_gui->m_componentList->verticalScrollBar()->value() + 2 * m_gui->m_componentList->viewport()->height();
        int lastInView = -1;
        for (AZ::s32 index = 0; index < m_componentEditorsUsed; ++index)
        {
            const ComponentEditor* componentEditor = m_componentEditors[index];
            if (componentEditor->isVisible() && componentEditor->geometry().top() <= viewBottom)
            {
                lastInView = index;
            }
        }
        ReleaseLazyCardsThrough(lastInView, SettleOrigin::Interaction);
        UpdateLazyCardsPendingProperty();
    }

    void EntityPropertyEditor::QueueLazyCardsStep(int delayMs)
    {
        const AZ::u64 generation = m_lazyCardsGeneration;
        QTimer::singleShot(delayMs, this, [this, generation]()
            {
                RunLazyCardsStep(generation);
            });
    }

    void EntityPropertyEditor::RunLazyCardsStep(AZ::u64 generation)
    {
        // A rebuild queued or ran since the cards were held: it holds its own
        if (generation != m_lazyCardsGeneration || m_isAlreadyQueuedRefresh || m_isBuildingProperties)
        {
            return;
        }
        // Not while a card or a row is being dragged around: building moves the cards under it
        if (m_currentReorderState != ReorderState::Inactive)
        {
            QueueLazyCardsStep(50);
            return;
        }

        // The next held cards from the top, for about one step's budget, at least one card; while the inspector keeps its last
        // picture, the cards in view, for the rebuild's budget: nothing shows before they are built
        if (m_viewFrozen)
        {
            BuildViewCards(AZStd::max(InterruptibleInlineBudgetUs(), LazyCardsStepBudgetUs));
        }
        else
        {
            ReleaseLazyCardsThrough(m_componentEditorsUsed - 1, SettleOrigin::Build, LazyCardsStepBudgetUs);
        }
        ++m_lazyCardsSteps;

        // the cards in view built: the frozen inspector shows them
        if (m_viewFrozen && ViewCardsBuilt())
        {
            EndViewFreeze();
        }

        if (GetLazyCardsPending() > 0)
        {
            QueueLazyCardsStep();
        }
        else
        {
            FinishLazyCards();
        }
        UpdateLazyCardsPendingProperty();
    }

    void EntityPropertyEditor::CompleteLazyCards()
    {
        if (GetLazyCardsPending() == 0)
        {
            return;
        }
        ReleaseLazyCardsThrough(m_componentEditorsUsed - 1, SettleOrigin::Interaction);
        FinishLazyCards();
        UpdateLazyCardsPendingProperty();
    }

    void EntityPropertyEditor::FinishLazyCards()
    {
        // Every card is built: a freeze cannot outlast it
        EndViewFreeze();

        // The generation ends here: a step still queued for it does nothing
        ++m_lazyCardsGeneration;
        if (!m_lazyCardsDiagnostics)
        {
            return;
        }
        m_lazyCardsDiagnostics = false;

        // The time from the start of the rebuild that held the cards to the last card built, and what the held cards' builds cost
        const AZ::s64 nowUs =
            AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now().time_since_epoch()).count();
        AZ::u64 resetRowsUs = 0;
        AZ::u64 resetDirtyHandlersUs = 0;
        DocumentPropertyEditor::TakeResetTimings(resetRowsUs, resetDirtyHandlersUs);
        const AZ::u64 generateContentsUs = AZ::DocumentPropertyEditor::ReflectionAdapter::TakeGenerateContentsMicroseconds();
        AZ_Printf(
            "EntityInspector",
            "  lazyCards: held=%d steps=%d completeMs=%.2f dom=%.2f rows=%.2f dirtyHandlers=%.2f (ms)",
            m_lazyCardsHeldAtBuild,
            m_lazyCardsSteps,
            aznumeric_cast<double>(nowUs - m_lazyCardsStartUs) / 1000.0,
            aznumeric_cast<double>(generateContentsUs) / 1000.0,
            aznumeric_cast<double>(resetRowsUs) / 1000.0,
            aznumeric_cast<double>(resetDirtyHandlersUs) / 1000.0);
    }

    void EntityPropertyEditor::UpdateLazyCardsPendingProperty()
    {
        // read by the PerfBench kit, which waits for every card's rows before it reads the inspector
        const int pending = GetLazyCardsPending();
        if (property("lazyCardsPending").toInt() != pending)
        {
            setProperty("lazyCardsPending", pending);
        }
    }

    void EntityPropertyEditor::UpdateOverrideVisualization(ComponentEditor& componentEditor)
    {
        if (auto prefabOverridePublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabOverridePublicInterface>::Get())
        {
            AZStd::span<AZ::Component* const> components = componentEditor.GetComponents();
            AZ_Assert(!components.empty() && components[0], "ComponentEditor should have at least one component.");

            // Overrides on container entities are for internal use only and should not be exposed
            AZ::EntityId entityId = components[0]->GetEntityId();
            if (m_prefabPublicInterface->IsInstanceContainerEntity(entityId))
            {
                return;
            }

            // Get icon path based on current component override state
            AZStd::string iconOverlayPath;
            AZ::EntityComponentIdPair entityComponentIdPair(components[0]->GetEntityId(), components[0]->GetId());
            if (auto overrideType = prefabOverridePublicInterface->GetComponentOverrideType(entityComponentIdPair);
                overrideType.has_value())
            {
                if (overrideType == AzToolsFramework::Prefab::OverrideType::AddComponent)
                {
                    iconOverlayPath = ":/OverrideResources/component_added_as_override.svg";
                }
                else if (overrideType == AzToolsFramework::Prefab::OverrideType::EditComponent)
                {
                    iconOverlayPath = ":/OverrideResources/component_modified_as_override.svg";
                }
            }

            AZStd::string prevIconOverlayPath(componentEditor.property("IconOverlayPath").toString().toUtf8());
            if (prevIconOverlayPath != iconOverlayPath)
            {
                componentEditor.setProperty("IconOverlayPath", iconOverlayPath.c_str());
                componentEditor.GetHeader()->SetIconOverlay(QIcon(iconOverlayPath.c_str()));
                componentEditor.GetHeader()->SetComponentIconClickable(!iconOverlayPath.empty());
            }
        }
    }

    ComponentEditor* EntityPropertyEditor::CreateComponentEditor()
    {
        //caching allocated component editors for reuse and to preserve order
        if (m_componentEditorsUsed >= m_componentEditors.size())
        {
            // create a new component editor since cache has been exceeded

            /* create a factory for component adapters
             * depending on the preferences state, create the correct type of adapter, or none at all */
            ComponentEditor::ComponentAdapterFactory adapterFactory =
                [&]() -> AZStd::shared_ptr<AZ::DocumentPropertyEditor::ComponentAdapter>
                {
                    if (ShouldUseDPE())
                    {
                        return (
                            Prefab::IsInspectorOverrideManagementEnabled()
                            ? AZStd::make_shared<Prefab::PrefabComponentAdapter>()
                            : AZStd::make_shared<AZ::DocumentPropertyEditor::ComponentAdapter>());
                    }
                    return nullptr;
                };
            auto componentEditor = new ComponentEditor(m_serializeContext, this, this, adapterFactory);
            componentEditor->setAcceptDrops(true);

            connect(componentEditor, &ComponentEditor::OnExpansionContractionDone, this, [this]()
                {
                    setUpdatesEnabled(false);
                    layout()->update();
                    layout()->activate();
                    setUpdatesEnabled(true);
                });

            // force Card resize when expansion state changes or when explicitly requested by OnSizeUpdateRequested (o3de #15945:
            // cards kept the size of the previous component): a size recompute, not a restyle (Card.cpp repolishes the card when its
            // own properties change). The list is then laid out before the inspector paints; a rebuild and a lazy release settle it
            // once, at their end.
            auto resizeCard = [this, componentEditor]()
            {
                if (QWidget* content = componentEditor->contentWidget())
                {
                    content->updateGeometry();
                }
                if (QLayout* cardLayout = componentEditor->layout())
                {
                    cardLayout->invalidate();
                }
                componentEditor->updateGeometry();
                if (!m_isBuildingProperties && !m_lazyCardsReleasing)
                {
                    SettleComponentList(SettleOrigin::Interaction);
                }
            };
            connect(componentEditor, &ComponentEditor::OnExpansionContractionDone, componentEditor, resizeCard);
            connect(componentEditor, &ComponentEditor::OnSizeUpdateRequested, componentEditor, resizeCard);

            // Lazy cards: a held card the user expands builds its rows (the cards above it first)
            connect(componentEditor, &AzQtComponents::Card::expandStateChanged, this, [this, componentEditor](bool expanded)
                {
                    if (expanded && componentEditor->IsContentBuildHeld() && CanReleaseLazyCards())
                    {
                        const auto used = m_componentEditors.begin() + m_componentEditorsUsed;
                        if (const auto card = AZStd::find(m_componentEditors.begin(), used, componentEditor); card != used)
                        {
                            ReleaseLazyCardsThrough(aznumeric_cast<int>(card - m_componentEditors.begin()), SettleOrigin::Interaction);
                            UpdateLazyCardsPendingProperty();
                        }
                    }
                });

            connect(componentEditor, &ComponentEditor::OnDisplayComponentEditorMenu, this, &EntityPropertyEditor::OnDisplayComponentEditorMenu);
            connect(componentEditor, &ComponentEditor::OnRequestRequiredComponents, this, &EntityPropertyEditor::OnRequestRequiredComponents);
            connect(componentEditor, &ComponentEditor::OnRequestRemoveComponents, this, [this](AZStd::span<AZ::Component* const> components) {DeleteComponents(components); });
            connect(componentEditor, &ComponentEditor::OnRequestDisableComponents, this, [this](AZStd::span<AZ::Component* const> components) {DisableComponents(components); });
            componentEditor->GetPropertyEditor()->SetReadOnlyQueryFunction([this](const InstanceDataNode* node) { return QueryInstanceDataNodeReadOnlyStatus(node); });
            componentEditor->GetPropertyEditor()->SetHiddenQueryFunction([this](const InstanceDataNode* node) { return QueryInstanceDataNodeHiddenStatus(node); });
            componentEditor->GetPropertyEditor()->SetIndicatorQueryFunction([this](const InstanceDataNode* node) { return GetAppropriateIndicator(node); });

            if (ShouldUseDPE() && Prefab::IsInspectorOverrideManagementEnabled())
            {
                // Connect to the component icon's click event to display override context menu
                connect(
                    componentEditor,
                    &ComponentEditor::OnComponentIconClicked,
                    this,
                    &EntityPropertyEditor::OnComponentOverrideContextMenu);

                // Subscribe to DPE property changes to keep the component icon updated based on override state
                auto propertyChanged =
                    [this, componentEditor](const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& changeInfo)
                    {
                        // Check if the component editor is currently in use
                        if (componentEditor->isVisible() &&
                            changeInfo.changeType == AZ::DocumentPropertyEditor::Nodes::ValueChangeType::FinishedEdit)
                        {
                            UpdateOverrideVisualization(*componentEditor);
                        }
                    };
                componentEditor->ConnectPropertyChangeHandler(propertyChanged);
            }

            //move spacer to bottom of editors
            m_gui->m_componentListContents->layout()->removeItem(m_spacer);
            m_gui->m_componentListContents->layout()->addWidget(componentEditor);
            m_gui->m_componentListContents->layout()->addItem(m_spacer);
            m_gui->m_componentListContents->layout()->update();

            //add new editor to cache
            m_componentEditors.push_back(componentEditor);
        }

        //return editor at current index from cache
        return m_componentEditors[m_componentEditorsUsed++];
    }

    void EntityPropertyEditor::InvalidatePropertyDisplay(PropertyModificationRefreshLevel level)
    {
        if (level == Refresh_None)
        {
            return;
        }

        if (level == Refresh_EntireTree)
        {
            QueuePropertyRefresh();
            return;
        }

        if (level == Refresh_EntireTree_NewContent)
        {
             // this should ONLY happen for when new content is added to the bottom of the view, like when Add Component occurs.
            QueuePropertyRefresh();
            m_shouldScrollToNewComponents = true;
            return;
        }

        // Only queue invalidation refreshes on all the component editors if a full refresh hasn't been queued.
        // If a full refresh *is* already queued, there's no value in the editors queueing to refresh themselves
        // a second time immediately afterwards.
        if (!m_isAlreadyQueuedRefresh)
        {
            for (auto componentEditor : m_componentEditors)
            {
                if (componentEditor->isVisible())
                {
                    componentEditor->QueuePropertyEditorInvalidation(level);
                }
            }
        }
    }

    void EntityPropertyEditor::InvalidatePropertyDisplayForComponent(AZ::EntityComponentIdPair entityComponentIdPair, PropertyModificationRefreshLevel level)
    {
        if (level == Refresh_None)
        {
            return;
        }

        if (m_isAlreadyQueuedRefresh)
        {
            return;
        }

        if (m_selectedEntityComponentIds.find(entityComponentIdPair) == m_selectedEntityComponentIds.end())
        {
            return;
        }

        for (auto componentEditor : m_componentEditors)
        {
            if (componentEditor->isVisible())
            {
                componentEditor->QueuePropertyEditorInvalidationForComponent(entityComponentIdPair, level);
            }
        }
    }

    void EntityPropertyEditor::BeforeUndoRedo()
    {
        m_currentUndoOperation = nullptr;
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::AfterUndoRedo()
    {
        m_currentUndoOperation = nullptr;
    }

    // implementation of IPropertyEditorNotify:
    void EntityPropertyEditor::BeforePropertyModified(InstanceDataNode* pNode)
    {
        if (m_isBuildingProperties)
        {
            AZ_Error(
                "PropertyEditor", !m_isBuildingProperties,
                "A property is being modified while we're building properties - make sure you "
                "do not call RequestWrite except in response to user action. An undo will not "
                "be recorded for this change.");

            return;
        }

        EntityIdList selectedEntityIds;
        GetSelectedEntities(selectedEntityIds);

        bool allSelectedEntitiesEditable = true;
        ToolsApplicationRequests::Bus::BroadcastResult(allSelectedEntitiesEditable, &ToolsApplicationRequests::AreEntitiesEditable, selectedEntityIds);
        if (!allSelectedEntitiesEditable)
        {
            return;
        }

        for (AZ::EntityId entityId : selectedEntityIds)
        {
            ToolsApplicationRequests::Bus::BroadcastResult(allSelectedEntitiesEditable, &ToolsApplicationRequests::IsEntityEditable, entityId);
            if (!allSelectedEntitiesEditable)
            {
                return;
            }
        }

        if ((m_currentUndoNode == pNode) && (m_currentUndoOperation))
        {
            // attempt to resume the last undo operation:
            ToolsApplicationRequests::Bus::BroadcastResult(m_currentUndoOperation, &ToolsApplicationRequests::ResumeUndoBatch, m_currentUndoOperation, "Modify Entity Property");
        }
        else
        {
            ToolsApplicationRequests::Bus::BroadcastResult(m_currentUndoOperation, &ToolsApplicationRequests::BeginUndoBatch, "Modify Entity Property");
            m_currentUndoNode = pNode;
        }

        // mark the ones' we're editing as dirty, so the user does not have to.
        for (auto entityShownId : selectedEntityIds)
        {
            ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::AddDirtyEntity, entityShownId);
        }

        // Notify prefab system to pause instance propagation during editing.
        // This is done here in the BeforePropertyModified handler and not in SetPropertyEditingActive
        // because SetPropertyEditingActive is not hooked up to be called from anywhere and never gets hit.
        // BeforePropertyModified/AfterPropertyModified are called multiple times while a property is being
        // modified (ex. while moving a slider handle), and SetPropertyEditingComplete is called once when
        // editing is complete (ex. doing a mouse up on the slider handle)
        m_instanceUpdateExecutorInterface->SetShouldPauseInstancePropagation(true);
    }

    void EntityPropertyEditor::AfterPropertyModified(InstanceDataNode* pNode)
    {
        if (m_isBuildingProperties)
        {
            AZ_Error(
                "PropertyEditor", !m_isBuildingProperties,
                "A property was modified while building properties - make sure you do not "
                "call RequestWrite except in response to user action. An undo will not be "
                "recorded for this change.");

            return;
        }

        AZ_Assert(m_currentUndoOperation, "Invalid undo operation in AfterPropertyModified");
        AZ_Assert(m_currentUndoNode == pNode, "Invalid undo operation in AfterPropertyModified - node doesn't match!");

        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::EndUndoBatch);

        InstanceDataNode* componentNode = pNode;
        while (componentNode->GetParent())
        {
            componentNode = componentNode->GetParent();
        }

        if (!componentNode)
        {
            AZ_Warning("Property Editor", false, "Failed to locate component data associated with the affected field. External change event cannot be sent.");
            return;
        }

        // Set our flag to not refresh values based on our own update broadcasts
        m_initiatingPropertyChangeNotification = true;

        // Send EBus notification for the affected entities, along with information about the affected component.
        for (size_t instanceIdx = 0; instanceIdx < componentNode->GetNumInstances(); ++instanceIdx)
        {
            AZ::Component* componentInstance = m_serializeContext->Cast<AZ::Component*>(
                componentNode->GetInstance(instanceIdx), componentNode->GetClassMetadata()->m_typeId);
            if (componentInstance)
            {
                PropertyEditorEntityChangeNotificationBus::Event(
                    componentInstance->GetEntityId(),
                    &PropertyEditorEntityChangeNotifications::OnEntityComponentPropertyChanged,
                    componentInstance->GetId());
            }
        }

        // notify listeners when a component (type) is modified
        PropertyEditorChangeNotificationBus::Broadcast(
            &PropertyEditorChangeNotifications::OnComponentPropertyChanged,
            componentNode->GetClassMetadata()->m_typeId);

        m_initiatingPropertyChangeNotification = false;
    }

    void EntityPropertyEditor::SetPropertyEditingActive(InstanceDataNode* /*pNode*/)
    {
        // Note: This signal doesn't actually get called. It doesn't seem to be hooked up to anything
        MarkPropertyEditorBusyStart();
    }

    void EntityPropertyEditor::SetPropertyEditingComplete(InstanceDataNode* /*pNode*/)
    {
        MarkPropertyEditorBusyEnd();

        // Notify prefab system to resume instance propagation now that editing is complete
        m_instanceUpdateExecutorInterface->SetShouldPauseInstancePropagation(false);
    }

    void EntityPropertyEditor::OnPropertyRefreshRequired()
    {
        MarkPropertyEditorBusyEnd();
    }

    void EntityPropertyEditor::Reflect(AZ::ReflectContext* /*context*/)
    {
    }

    void EntityPropertyEditor::SetAllowRename(bool allowRename)
    {
        m_gui->m_entityNameEditor->setReadOnly(!allowRename);
    }

    void EntityPropertyEditor::ClearInstances(bool invalidateImmediately)
    {
        // Per-substep timing to localize the pool-wide teardown cost (hide vs content-clear vs prevent-refresh).
        const bool diag = m_inspectorDiagnosticsActive;
        const auto clearMarkUs = [diag](AZStd::chrono::steady_clock::time_point& marker, AZ::s64& accumulatorUs)
        {
            if (!diag)
            {
                return;
            }
            const AZStd::chrono::steady_clock::time_point now = AZStd::chrono::steady_clock::now();
            accumulatorUs += AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(now - marker).count();
            marker = now;
        };

        // Lazy cards: the cards are cleared, so the steps left of the last build do nothing and no hold outlives it
        ++m_lazyCardsGeneration;

        for (auto componentEditor : m_componentEditors)
        {
            AZStd::chrono::steady_clock::time_point clearMark =
                diag ? AZStd::chrono::steady_clock::now() : AZStd::chrono::steady_clock::time_point{};

            componentEditor->hide();
            clearMarkUs(clearMark, m_diagClearHideUs);

            componentEditor->DiscardContentBuild();
            componentEditor->ClearInstances(invalidateImmediately);
            clearMarkUs(clearMark, m_diagClearContentUs);

            // Re-enable RPE-level refresh calls.  Since we're clearing out the associated RPE, there's no longer a danger
            // that they will get a partial refresh while in an invalid state.
            // (RPE refreshes were prevented in QueuePropertyRefresh() to ensure that no RPEs tried a partial refresh in-between
            // the time an EPE full refresh was requested and when it executed.)
            componentEditor->PreventRefresh(false);
            clearMarkUs(clearMark, m_diagClearPreventUs);
        }

        m_componentEditorsUsed = 0;
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
        m_selectedEntityIds.clear();
        m_componentToEditorMap.clear();
    }

    void EntityPropertyEditor::MarkPropertyEditorBusyStart()
    {
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
        m_propertyEditBusy++;
    }

    void EntityPropertyEditor::MarkPropertyEditorBusyEnd()
    {
        // Not all property handlers emit SetPropertyEditingActive before
        // calling SetPropertyEditingComplete. For instance, the CheckBox
        // handler just calls SetPropertyEditingComplete when its state is
        // toggled. Only process the calls where SetPropertyEditingActive was
        // called first.
        m_propertyEditBusy--;
        if (m_propertyEditBusy == 0)
        {
            QueuePropertyRefresh();
        }
        else if (m_propertyEditBusy < 0)
        {
            m_propertyEditBusy = 0;
        }
    }

    void EntityPropertyEditor::SealUndoStack()
    {
        m_currentUndoNode = nullptr;
        m_currentUndoOperation = nullptr;
    }

    void EntityPropertyEditor::QueuePropertyRefresh()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        if (!m_isAlreadyQueuedRefresh)
        {
            m_isAlreadyQueuedRefresh = true;

            // disable all actions until queue refresh is done, so that long queues of events like
            // click - delete - click - delete - click .... dont all occur while the state is invalid.
            // note that this happens when the data is already in an invalid state before this function is called...
            // pointers are bad, components have been deleted, etc,
            QList<QAction*> actionList = actions();
            for (QAction* action : actionList)
            {
                action->setEnabled(false);
            }

            // Make sure that all component editors stop trying to refresh themselves until after the full refresh
            // occurs, since a full refresh request means that the existing RPEs have invalid data in them, and accessing
            // that data via a partial refresh could cause a crash.
            // If a "lower-level" refresh (i.e. InvalidateAttributesAndValues) is already queued, it could
            // potentially reference the invalid data before the EPE's full refresh executes.  We also need to prevent any
            // RPE refreshes from getting queued between now and the full refresh, because "Cancel" doesn't actually cancel
            // the queued request, it just clears the refresh state.  New queued requests could potentially restore the
            // refresh state and effectively restore the previously-queued request, which would still execute before the
            // EPE's full refresh.
            // (In UpdateContents(), after the full refresh occurs, we stop preventing RPE refreshes from getting queued)
            bool ownedFocus = DoesOwnFocus();
            for (auto componentEditor : m_componentEditors)
            {
                // Cancel any refreshes that were queued prior to this point, since they are no longer guaranteed to
                // be valid requests.
                componentEditor->CancelQueuedRefresh();
                // Prevent any future refreshes from getting queued until we've completed the UpdateContents() call.
                componentEditor->PreventRefresh(true);
            }

            // If the EntityPropertyEditor or any of its children were the application's focused widget before
            // the above call to: componentEditor->PreventRefresh(true);
            // Then that call could result in that widget being disabled, which will clear the focus and leave
            // the application with no focused widget, which means it won't be able to receive key/shortcut events.
            // So if we owned the focus and the focusWidget is now nullptr, then reset the focus to the EntityPropertyEditor.
            // This happens anytime QueuePropertyRefresh is invoked (e.g. when switching between component modes, or triggering a full tree
            // rebuild)
            if (ownedFocus && !QApplication::focusWidget())
            {
                setFocus();
            }

            // Refresh the properties using a singleShot
            // this makes sure that the properties aren't refreshed while processing
            // other events, and instead runs after the current events are processed

            // note that if we're getting here, we're actually clearing and rebuilding the entire
            // tree, not just refreshing values (that is taken care of elsewhere).
            // Sometimes, these events cause it to repeatedly refresh, so instead, give it a delay of more than 0 so that
            // tick can happen first.
            QTimer::singleShot(0, this, &EntityPropertyEditor::RunQueuedRefresh);

            //saving state any time refresh gets queued because requires valid components
            //attempting to call directly anywhere state needed to be preserved always occurred with QueuePropertyRefresh
            SaveComponentEditorState();

            QScrollBar* verticalScroll = m_gui->m_componentList->verticalScrollBar();
            QScrollBar* horizontalScroll = m_gui->m_componentList->horizontalScrollBar();
            m_savedVerticalScroll = verticalScroll->value();
            m_savedHorizontalScroll = horizontalScroll->value();
        }
    }

    void EntityPropertyEditor::OnAddComponent()
    {
        AZ::ComponentDescriptor::DependencyArrayType serviceFilter;
        AZ::ComponentDescriptor::DependencyArrayType incompatibleServiceFilter;
        QRect globalRect = GetWidgetGlobalRect(m_gui->m_addComponentButton) | GetWidgetGlobalRect(m_gui->m_componentList);
        ShowComponentPalette(m_componentPalette, globalRect.topLeft(), globalRect.size(), serviceFilter, incompatibleServiceFilter);
    }

    void EntityPropertyEditor::GotSceneSourceControlStatus(SourceControlFileInfo& fileInfo)
    {
        m_sceneIsNew = false;
        EnableEditor(!fileInfo.IsReadOnly());
    }

    void EntityPropertyEditor::PerformActionsBasedOnSceneStatus(bool sceneIsNew, bool readOnly)
    {
        m_sceneIsNew = sceneIsNew;
        EnableEditor(!readOnly);
    }

    void EntityPropertyEditor::EnableEditor(bool enabled)
    {
        m_gui->m_entityNameEditor->setReadOnly(!enabled);
        m_gui->m_statusComboBox->setEnabled(enabled);
        m_gui->m_addComponentButton->setVisible(enabled);
        m_gui->m_componentListContents->setEnabled(enabled);
    }

    void EntityPropertyEditor::OnEntityNameChanged()
    {
        if (m_gui->m_entityNameEditor->isReadOnly())
        {
            return;
        }

        QByteArray entityNameByteArray = m_gui->m_entityNameEditor->text().toUtf8();

        const AZStd::string entityName(entityNameByteArray.data());

        if (entityName.size() == 0)
        {
            // Do not allow them to set an entity name to an empty string
            // If they try to, just refresh the properties to get the correct name back into the field
            QueuePropertyRefresh();
            return;
        }

        EntityIdList selectedEntityIds;
        GetSelectedEntities(selectedEntityIds);

        if (selectedEntityIds.size() == 1)
        {
            AZStd::vector<AZ::Entity*> entityList;

            for (AZ::EntityId entityId : selectedEntityIds)
            {
                AZ::Entity* entity = GetSelectedEntityById(entityId);

                if (entity && entityName != entity->GetName())
                {
                    entityList.push_back(entity);
                }
            }

            if (entityList.size())
            {
                ScopedUndoBatch undoBatch("Modify Entity Name");

                for (AZ::Entity* entity : entityList)
                {
                    entity->SetName(entityName);
                    ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::AddDirtyEntity, entity->GetId());
                }
            }
        }
    }

    void EntityPropertyEditor::RequestPropertyContextMenu(InstanceDataNode* node, const QPoint& position)
    {
        AZ_Assert(node, "Invalid node passed to context menu callback.");

        if (!node || m_disabled)
        {
            return;
        }

        // Don't show if a move operation is pending.
        if (m_currentReorderState != ReorderState::Inactive)
        {
            return;
        }

        // Locate the owning component and class data corresponding to the clicked node.
        InstanceDataNode* componentNode = node;
        while (componentNode->GetParent())
        {
            componentNode = componentNode->GetParent();
        }

        // Get inner class data for the clicked component.
        AZ::Component* component = m_serializeContext->Cast<AZ::Component*>(
            componentNode->FirstInstance(), componentNode->GetClassMetadata()->m_typeId);
        if (component)
        {
            auto componentClassData = GetComponentClassData(component);
            if (componentClassData)
            {
                QMenu menu;

                // Populate and show the context menu.
                AddMenuOptionsForComponents(menu, position);

                if (!menu.actions().empty())
                {
                    menu.addSeparator();
                }

                AddMenuOptionsForFields(node, componentNode, componentClassData, menu);

                AzToolsFramework::Components::EditorComponentBase* editorComponent = static_cast<AzToolsFramework::Components::EditorComponentBase*>(component);

                if (editorComponent)
                {
                    editorComponent->AddContextMenuActions(&menu);
                }

                if (!menu.actions().empty())
                {
                    m_currentReorderState = EntityPropertyEditor::ReorderState::UsingMenu;
                    menu.exec(position);
                    if (m_currentReorderState != EntityPropertyEditor::ReorderState::MenuOperationInProgress)
                    {
                        m_currentReorderState = EntityPropertyEditor::ReorderState::Inactive;
                    }
                }
            }
        }
    }

    void EntityPropertyEditor::AddMenuOptionsForComponents(QMenu& menu, const QPoint& /*position*/)
    {
        UpdateInternalState();
        menu.addActions(actions());
    }

    void EntityPropertyEditor::AddMenuOptionsForFields(
        InstanceDataNode* fieldNode,
        InstanceDataNode* componentNode,
        const AZ::SerializeContext::ClassData* componentClassData,
        QMenu& menu)
    {
        if (!fieldNode)
        {
            return;
        }

        // For items marked as new (container elements), we'll synchronize the parent node.
        if (fieldNode->IsNewVersusComparison())
        {
            fieldNode = fieldNode->GetParent();
            AZ_Assert(fieldNode && fieldNode->GetClassMetadata() && fieldNode->GetClassMetadata()->m_container, "New element should be a child of a container.");
        }

        AZ::Component* componentInstance =
            m_serializeContext->Cast<AZ::Component*>(componentNode->FirstInstance(), componentClassData->m_typeId);
        AZ_Assert(componentInstance, "Failed to cast component instance.");

        m_reorderRowWidget = nullptr;
        // Add move up/down actions if appropriate
        auto componentEditorIterator = m_componentToEditorMap.find(componentInstance);
        AZ_Assert(componentEditorIterator != m_componentToEditorMap.end(), "Unable to find a component editor for the given component");
        if (componentEditorIterator != m_componentToEditorMap.end())
        {
            m_reorderRowWidgetEditor = componentEditorIterator->second;
            PropertyRowWidget* widget = componentEditorIterator->second->GetPropertyEditor()->GetWidgetFromNode(fieldNode);
            if (widget->CanBeReordered())
            {
                m_reorderRowWidget = widget;
                SetRowWidgetHighlighted(widget);

                QAction* moveUpAction = menu.addAction(tr("Move %1 Up").arg(widget->GetNameLabel()->text()));
                moveUpAction->setEnabled(false);
                moveUpAction->setData(kPropertyEditorMenuActionMoveUp);

                if (widget->CanMoveUp())
                {
                    moveUpAction->setEnabled(true);
                    connect(
                        moveUpAction, &QAction::triggered, this,
                        [this, widget]
                        {
                            ContextMenuActionMoveItemUp(m_reorderRowWidgetEditor, widget);
                        });
                }

                QAction* moveDownAction = menu.addAction(tr("Move %1 Down").arg(widget->GetNameLabel()->text()));
                moveDownAction->setEnabled(false);
                moveDownAction->setData(kPropertyEditorMenuActionMoveDown);
                if (widget->CanMoveDown())
                {
                    moveDownAction->setEnabled(true);
                    connect(
                        moveDownAction, &QAction::triggered, this,
                        [this, widget]
                        {
                            ContextMenuActionMoveItemDown(m_reorderRowWidgetEditor, widget);
                        });
                }

                menu.addSeparator();
            }
        }
    }

    bool EntityPropertyEditor::HasAnyVisibleElements(const InstanceDataNode& node)
    {
        if (CalculateNodeDisplayVisibility(node, false) == AzToolsFramework::NodeDisplayVisibility::Visible)
        {
            return true;
        }

        for (const InstanceDataNode& childNode : node.GetChildren())
        {
            if (HasAnyVisibleElements(childNode))
            {
                return true;
            }
        }


        return false;
    }

    void EntityPropertyEditor::BeginMoveRowWidgetFade()
    {
        // Fade out the highlights and indicator bar for two seconds before moving.
        m_moveFadeSecondsRemaining = MoveFadeSeconds;
        m_currentReorderState = EntityPropertyEditor::ReorderState::MenuOperationInProgress;
        AZ::TickBus::Handler::BusConnect();
    }

    void EntityPropertyEditor::HighlightMovedRowWidget()
    {
        if (m_currentReorderState != ReorderState::WaitForRedraw)
        {
            return;
        }
        UpdateOverlay();

        m_currentReorderState = ReorderState::HighlightMovedRow;
        m_moveFadeSecondsRemaining = MoveFadeSeconds;

        AZ::TickBus::Handler::BusConnect();
        m_overlay->setVisible(true);
    }

    void EntityPropertyEditor::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        m_moveFadeSecondsRemaining -= deltaTime;
        m_overlay->setVisible(true);
        if (m_moveFadeSecondsRemaining <= 0.0f)
        {
            m_moveFadeSecondsRemaining = 0.0f;

            if (m_currentReorderState == ReorderState::MenuOperationInProgress)
            {
                m_reorderRowWidgetEditor->GetPropertyEditor()->MoveNodeToIndex(m_nodeToMove, m_indexMapOfMovedRow[0]);

                // Ensure the highlight gets drawn once the RPE is updated.
                m_currentReorderState = ReorderState::WaitForRedraw;
                AZ::TickBus::Handler::BusDisconnect();
                ScrollToNewComponent();
            }
            else
            {
                m_currentReorderState = ReorderState::Inactive;
                AZ::TickBus::Handler::BusDisconnect();
                m_overlay->setVisible(false);
            }
        }

        // Force a repaint to show the fade.
        repaint(0, 0, -1, -1);
    }

    void EntityPropertyEditor::GenerateRowWidgetIndexMapToChildIndex(PropertyRowWidget* parent, int destIndex)
    {
        m_indexMapOfMovedRow.clear();
        m_indexMapOfMovedRow.push_back(destIndex);

        while (parent)
        {
            int index = parent->GetIndexInParent();
            if (index < 0)
            {
                // Top level widget.
                break;
            }
            m_indexMapOfMovedRow.push_back(parent->GetIndexInParent());
            parent = parent->GetParentRow();
        }
    }

    void EntityPropertyEditor::ContextMenuActionMoveItemUp(ComponentEditor* componentEditor, PropertyRowWidget* rowWidget)
    {
        // After the RPE is rebuilt, there'll be no way to work out which is the moved RowWidget.
        // Generate a map of the child indices up to the root.
        PropertyRowWidget* parent = rowWidget->GetParentRow();
        GenerateRowWidgetIndexMapToChildIndex(parent, rowWidget->GetIndexInParent() - 1);

        m_reorderRowWidgetEditor = componentEditor;
        m_nodeToMove = rowWidget->GetNode();
        BeginMoveRowWidgetFade();
    }

    void EntityPropertyEditor::ContextMenuActionMoveItemDown(ComponentEditor* componentEditor, PropertyRowWidget* rowWidget)
    {
        PropertyRowWidget* parent = rowWidget->GetParentRow();
        GenerateRowWidgetIndexMapToChildIndex(parent, rowWidget->GetIndexInParent() + 1);

        m_reorderRowWidgetEditor = componentEditor;
        m_nodeToMove = rowWidget->GetNode();
        BeginMoveRowWidgetFade();
    }

    void EntityPropertyEditor::CalculateAndAdjustNodeAddress(const InstanceDataNode& componentFieldNode, AddressRootType rootType, InstanceDataNode::Address& outAddress) const
    {
        outAddress = componentFieldNode.ComputeAddress();

        InstanceDataHierarchy::InstanceDataNode* componentRootNode = componentFieldNode.GetRoot();
        AZ::Component* component = m_serializeContext->Cast<AZ::Component*>(
            componentRootNode->FirstInstance(), componentRootNode->GetClassMetadata()->m_typeId);

        if (component)
        {
            // Make sure address uses the component's persistent Id so it can be addressed under the entity or components container.
            // Property data is populated per component, so the value isn't using the persistent Id required for addressing
            // under containers.
            outAddress.back() = component->GetId();

            switch (rootType)
            {
            case AddressRootType::RootAtComponentsContainer:
            {
                // Insert a node to represent the Components container.
                outAddress.push_back(AZ_CRC_CE("Components"));
            }
            break;
            case AddressRootType::RootAtEntity:
            {
                // Insert a node to represent the Components container, and another to represent the entity root.
                outAddress.push_back(AZ_CRC_CE("Components"));
                outAddress.push_back(AZ_CRC_CE("AZ::Entity"));
            }
            break;
            }
        }
    }

    bool EntityPropertyEditor::GetEntityDataPatchAddress(const InstanceDataNode* componentFieldNode, AZ::DataPatch::AddressType& dataPatchAddressOut, AZ::EntityId* entityIdOut) const
    {
        if (InstanceDataNode* componentNode = componentFieldNode ? componentFieldNode->GetRoot() : nullptr)
        {
            AZ::Component* component = m_serializeContext->Cast<AZ::Component*>(
                componentNode->FirstInstance(), componentNode->GetClassMetadata()->m_typeId);
            if (component)
            {
                AZ::EntityId entityId = component->GetEntityId();
                if (entityId.IsValid())
                {
                    // Data patch address is compatible with the InstanceDataHierarchy::Address, with one key difference:
                    // It's stored in reverse order. InstanceDataHierarchy::Address is leaf-first. DataPatch::Address is root-first.

                    // Additionally, we generate a ReflectedPropertyEditor for each component, so all instance data nodes from
                    // one of those property grids are component-relative. To be addressable in a data patch or at the slice level,
                    // the component crc needs to be converted to its persistent Id (component id), and additional parent address
                    // levels must be added. See \ref CalculateAndAdjustNodeAddress for implementation details.

                    InstanceDataNode::Address nodeAddress;
                    CalculateAndAdjustNodeAddress(*componentFieldNode, AddressRootType::RootAtComponentsContainer, nodeAddress);

                    // DataPatch::Address is root-first, while InstanceDataHierarchy::Address is leaf-first, so read
                    // node address in reverse to convert to DataPatch::Address.
                    dataPatchAddressOut.clear();
                    dataPatchAddressOut.reserve(nodeAddress.size());
                    for (auto reverseIterator = nodeAddress.rbegin(); reverseIterator != nodeAddress.rend(); ++reverseIterator)
                    {
                        dataPatchAddressOut.push_back(*reverseIterator);
                    }

                    if (entityIdOut)
                    {
                        *entityIdOut = entityId;
                    }

                    return true;
                }
            }
        }

        return false;
    }

    bool EntityPropertyEditor::QueryInstanceDataNodeReadOnlyStatus(const InstanceDataNode* node)
    {
        return QueryInstanceDataNodeEffectStatus(node, AZ::DataPatch::Flag::PreventOverrideEffect);
    }

    bool EntityPropertyEditor::QueryInstanceDataNodeReadOnlySetStatus(const InstanceDataNode* node)
    {
        return QueryInstanceDataNodeSetStatus(node, AZ::DataPatch::Flag::PreventOverrideSet);
    }

    bool EntityPropertyEditor::QueryInstanceDataNodeHiddenStatus(const InstanceDataNode* node)
    {
        return QueryInstanceDataNodeEffectStatus(node, AZ::DataPatch::Flag::HidePropertyEffect);
    }

    bool EntityPropertyEditor::QueryInstanceDataNodeHiddenSetStatus(const InstanceDataNode* node)
    {
        return QueryInstanceDataNodeSetStatus(node, AZ::DataPatch::Flag::HidePropertySet);
    }

    const char* EntityPropertyEditor::GetAppropriateIndicator(const InstanceDataNode* node)
    {
        if (QueryInstanceDataNodeHiddenSetStatus(node))
        {
            return ":/PropertyEditor/Resources/hidden.png";
        }
        else if (QueryInstanceDataNodeReadOnlySetStatus(node))
        {
            return ":/PropertyEditor/Resources/locked.png";
        }

        return "";
    }

    bool EntityPropertyEditor::QueryInstanceDataNodeSetStatus(const InstanceDataNode* node, AZ::DataPatch::Flag testFlag)
    {
        // If target node is affected by HideProperty flag, it should be read-only.
        AZ::EntityId entityId;
        if (GetEntityDataPatchAddress(node, m_dataPatchAddressBuffer, &entityId))
        {
            if (const AZ::SliceComponent* rootSlice = GetEntityRootSlice(entityId))
            {
                AZ::DataPatch::Flags flags = rootSlice->GetEntityDataFlagsAtAddress(entityId, m_dataPatchAddressBuffer);
                if (flags & testFlag)
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool EntityPropertyEditor::QueryInstanceDataNodeEffectStatus(const InstanceDataNode* node, AZ::DataPatch::Flag testFlag)
    {
        // If target node is affected by HideProperty flag, it should be read-only.
        AZ::EntityId entityId;
        if (GetEntityDataPatchAddress(node, m_dataPatchAddressBuffer, &entityId))
        {
            if (const AZ::SliceComponent* rootSlice = GetEntityRootSlice(entityId))
            {
                AZ::DataPatch::Flags flags = rootSlice->GetEffectOfEntityDataFlagsAtAddress(entityId, m_dataPatchAddressBuffer);
                if (flags & testFlag)
                {
                    return true;
                }
            }
        }

        return false;
    }

    void EntityPropertyEditor::UpdateStatusComboBox()
    {
        QSignalBlocker noSignals(m_gui->m_statusComboBox);

        if (m_selectedEntityIds.empty() || m_isLevelEntityEditor)
        {
            m_gui->m_statusComboBox->setVisible(false);
            return;
        }

        size_t comboItemCount = StatusTypeToIndex(StatusItems);
        for (size_t i = 0; i < comboItemCount; ++i)
        {
            m_comboItems[i]->setCheckState(Qt::Unchecked);
        }

        bool allActive = true;
        bool allInactive = true;
        bool allEditorOnly = true;
        bool someActive = false;
        bool someInactive = false;
        bool someEditorOnly = false;

        for (AZ::EntityId id : m_selectedEntityIds)
        {
            bool isEditorOnly = false;
            EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, id, &EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);

            if (isEditorOnly)
            {
                bool isSliceRoot = false;
                EditorEntityInfoRequestBus::EventResult(isSliceRoot, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceRoot);

                allEditorOnly &= true;
                someEditorOnly = true;
                allActive = false;
                allInactive = false;
            }
            else
            {

                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);

                const bool isInitiallyActive = entity ? entity->IsRuntimeActiveByDefault() : true;
                allActive &= isInitiallyActive;
                allInactive &= !isInitiallyActive;
                allEditorOnly = false;
                someActive |= isInitiallyActive;
                someInactive |= !isInitiallyActive;
            }
        }

        m_gui->m_statusComboBox->setItalic(false);
        if (allActive)
        {
            m_gui->m_statusComboBox->setHeaderOverride(m_itemNames[static_cast<int>(StatusTypeToIndex(StatusType::StatusStartActive))]);
            m_gui->m_statusComboBox->setCurrentIndex(static_cast<int>(StatusTypeToIndex(StatusType::StatusStartActive)));
            m_comboItems[StatusTypeToIndex(StatusType::StatusStartActive)]->setCheckState(Qt::Checked);
        }
        else
            if (allInactive)
            {
                m_gui->m_statusComboBox->setHeaderOverride(m_itemNames[static_cast<int>(StatusTypeToIndex(StatusType::StatusStartInactive))]);
                m_gui->m_statusComboBox->setCurrentIndex(static_cast<int>(StatusTypeToIndex(StatusType::StatusStartInactive)));
                m_comboItems[StatusTypeToIndex(StatusType::StatusStartInactive)]->setCheckState(Qt::Checked);
            }
            else
                if (allEditorOnly)
                {
                    m_gui->m_statusComboBox->setHeaderOverride(m_itemNames[static_cast<int>(StatusTypeToIndex(StatusType::StatusEditorOnly))]);
                    m_gui->m_statusComboBox->setCurrentIndex(static_cast<int>(StatusTypeToIndex(StatusType::StatusEditorOnly)));
                    m_comboItems[StatusTypeToIndex(StatusType::StatusEditorOnly)]->setCheckState(Qt::Checked);
                }
                else // Some marked active, some not
                {
                    m_gui->m_statusComboBox->setItalic(true);
                    m_gui->m_statusComboBox->setHeaderOverride("- Multiple selected -");
                    if (someActive)
                    {
                        m_comboItems[StatusTypeToIndex(StatusType::StatusStartActive)]->setCheckState(Qt::PartiallyChecked);
                    }
                    if (someInactive)
                    {
                        m_comboItems[StatusTypeToIndex(StatusType::StatusStartInactive)]->setCheckState(Qt::PartiallyChecked);
                    }
                    if (someEditorOnly)
                    {
                        m_comboItems[StatusTypeToIndex(StatusType::StatusEditorOnly)]->setCheckState(Qt::PartiallyChecked);
                    }
                }

        m_gui->m_statusComboBox->setDisabled(m_selectionContainsReadOnlyEntity);
        m_gui->m_statusComboBox->setVisible(!m_isSystemEntityEditor && !m_isLevelEntityEditor);
        m_gui->m_statusComboBox->style()->unpolish(m_gui->m_statusComboBox);
        m_gui->m_statusComboBox->style()->polish(m_gui->m_statusComboBox);
    }

    size_t EntityPropertyEditor::StatusTypeToIndex(StatusType statusType) const
    {
        switch (statusType)
        {
        case StatusStartActive:
            return 0;
        case StatusStartInactive:
            return 1;
        case StatusEditorOnly:
            return 2;
        case StatusItems:
            return 3;
        default:
            AZ_Assert(false, "StatusType for EntityPropertyEditor is out of bounds.");
            return 1;
        }
    }

    EntityPropertyEditor::StatusType EntityPropertyEditor::IndexToStatusType(size_t index) const
    {
        switch (index)
        {
        case 0:
            return StatusStartActive;
        case 1:
            return StatusStartInactive;
        case 2:
            return StatusEditorOnly;
        default:
            AZ_Assert(index < StatusType::StatusItems, "Index for EntityPropertyEditor::IndexToStatusType is out of bounds");
            return StatusEditorOnly;
        }
    }

    void EntityPropertyEditor::OnDisplayComponentEditorMenu(const QPoint& position)
    {
        if (m_disabled)
        {
            return;
        }

        QMenu menu;

        AddMenuOptionsForComponents(menu, position);

        if (!menu.actions().empty())
        {
            menu.addSeparator();
        }

        const auto& componentsToEdit = GetSelectedComponents();
        if (componentsToEdit.size() > 0)
        {
            AzToolsFramework::Components::EditorComponentBase* editorComponent = static_cast<AzToolsFramework::Components::EditorComponentBase*>(componentsToEdit.front());

            if (editorComponent)
            {
                editorComponent->AddContextMenuActions(&menu);
            }
        }

        if (!menu.actions().empty())
        {
            menu.exec(position);
        }
    }

    void EntityPropertyEditor::OnRequestRequiredComponents(
        const QPoint& position,
        const QSize& size,
        AZStd::span<const AZ::ComponentServiceType> services,
        AZStd::span<const AZ::ComponentServiceType> incompatibleServices)
    {
        ShowComponentPalette(m_componentPalette, position, size, services, incompatibleServices);
    }

    void EntityPropertyEditor::HideComponentPalette()
    {
        m_componentPalette->hide();
        m_componentPalette->setGeometry(QRect(QPoint(0, 0), QSize(0, 0)));
    }

    void EntityPropertyEditor::ShowComponentPalette(
        ComponentPaletteWidget* componentPalette,
        const QPoint& position,
        const QSize& size,
        AZStd::span<const AZ::ComponentServiceType> serviceFilter,
        AZStd::span<const AZ::ComponentServiceType> incompatibleServiceFilter)
    {
        HideComponentPalette();

        bool areEntitiesEditable = true;
        ToolsApplicationRequests::Bus::BroadcastResult(areEntitiesEditable, &ToolsApplicationRequests::AreEntitiesEditable, m_selectedEntityIds);
        if (areEntitiesEditable)
        {
            componentPalette->setGeometry(QRect(position, size));
            componentPalette->Populate(m_serializeContext, m_selectedEntityIds, m_componentFilter, serviceFilter, incompatibleServiceFilter);
        }
    }

    void EntityPropertyEditor::SetAddComponentMenuFilter(ComponentFilter componentFilter)
    {
        m_componentFilter = AZStd::move(componentFilter);
        m_customFilterSet = true;
    }

    void EntityPropertyEditor::CreateActions()
    {
        m_actionToAddComponents = new QAction(tr("Add component"), this);
        m_actionToAddComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToAddComponents, &QAction::triggered, this, &EntityPropertyEditor::OnAddComponent);
        addAction(m_actionToAddComponents);
        m_entityComponentActions.push_back(m_actionToAddComponents);

        m_actionToDeleteComponents = new QAction(tr("Delete component"), this);
        m_actionToDeleteComponents->setShortcut(QKeySequence::Delete);
        m_actionToDeleteComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToDeleteComponents, &QAction::triggered, this, [this]() { DeleteComponents(); });
        addAction(m_actionToDeleteComponents);
        m_entityComponentActions.push_back(m_actionToDeleteComponents);

        auto* separator1 = new QAction(this);
        separator1->setSeparator(true);
        addAction(separator1);

        m_actionToCutComponents = new QAction(tr("Cut component"), this);
        m_actionToCutComponents->setShortcut(QKeySequence::Cut);
        m_actionToCutComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToCutComponents, &QAction::triggered, this, &EntityPropertyEditor::CutComponents);
        addAction(m_actionToCutComponents);
        m_entityComponentActions.push_back(m_actionToCutComponents);

        m_actionToCopyComponents = new QAction(tr("Copy component"), this);
        m_actionToCopyComponents->setShortcut(QKeySequence::Copy);
        m_actionToCopyComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToCopyComponents, &QAction::triggered, this, &EntityPropertyEditor::CopyComponents);
        addAction(m_actionToCopyComponents);
        m_entityComponentActions.push_back(m_actionToCopyComponents);

        m_actionToPasteComponents = new QAction(tr("Paste component"), this);
        m_actionToPasteComponents->setShortcut(QKeySequence::Paste);
        m_actionToPasteComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToPasteComponents, &QAction::triggered, this, &EntityPropertyEditor::PasteComponents);
        addAction(m_actionToPasteComponents);
        m_entityComponentActions.push_back(m_actionToPasteComponents);

        m_actionToDuplicateComponents = new QAction(tr("Duplicate component"), this);
        m_actionToDuplicateComponents->setShortcut(QKeySequence("Ctrl+D"));
        m_actionToDuplicateComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToDuplicateComponents, &QAction::triggered, this, [this]()
        {
            DuplicateComponents();
        });
        addAction(m_actionToDuplicateComponents);
        m_entityComponentActions.push_back(m_actionToDuplicateComponents);

        auto* separator2 = new QAction(this);
        separator2->setSeparator(true);
        addAction(separator2);

        m_actionToEnableComponents = new QAction(tr("Enable component"), this);
        m_actionToEnableComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToEnableComponents, &QAction::triggered, this, [this]() {EnableComponents(); });
        addAction(m_actionToEnableComponents);
        m_entityComponentActions.push_back(m_actionToEnableComponents);

        m_actionToDisableComponents = new QAction(tr("Disable component"), this);
        m_actionToDisableComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToDisableComponents, &QAction::triggered, this, [this]() {DisableComponents(); });
        addAction(m_actionToDisableComponents);
        m_entityComponentActions.push_back(m_actionToDisableComponents);

        m_actionToMoveComponentsUp = new QAction(tr("Move component up"), this);
        m_actionToMoveComponentsUp->setShortcut(QKeySequence::MoveToPreviousPage);
        m_actionToMoveComponentsUp->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToMoveComponentsUp, &QAction::triggered, this, &EntityPropertyEditor::MoveComponentsUp);
        addAction(m_actionToMoveComponentsUp);
        m_entityComponentActions.push_back(m_actionToMoveComponentsUp);

        m_actionToMoveComponentsDown = new QAction(tr("Move component down"), this);
        m_actionToMoveComponentsDown->setShortcut(QKeySequence::MoveToNextPage);
        m_actionToMoveComponentsDown->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToMoveComponentsDown, &QAction::triggered, this, &EntityPropertyEditor::MoveComponentsDown);
        addAction(m_actionToMoveComponentsDown);
        m_entityComponentActions.push_back(m_actionToMoveComponentsDown);

        m_actionToMoveComponentsTop = new QAction(tr("Move component to top"), this);
        m_actionToMoveComponentsTop->setShortcut(Qt::Key_Home);
        m_actionToMoveComponentsTop->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToMoveComponentsTop, &QAction::triggered, this, &EntityPropertyEditor::MoveComponentsTop);
        addAction(m_actionToMoveComponentsTop);
        m_entityComponentActions.push_back(m_actionToMoveComponentsTop);

        m_actionToMoveComponentsBottom = new QAction(tr("Move component to bottom"), this);
        m_actionToMoveComponentsBottom->setShortcut(Qt::Key_End);
        m_actionToMoveComponentsBottom->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToMoveComponentsBottom, &QAction::triggered, this, &EntityPropertyEditor::MoveComponentsBottom);
        addAction(m_actionToMoveComponentsBottom);
        m_entityComponentActions.push_back(m_actionToMoveComponentsBottom);

        auto* separator3 = new QAction(this);
        separator3->setSeparator(true);
        addAction(separator3);

        m_actionToCollapseAll = new QAction(tr("Collapse All"), this);
        connect(m_actionToCollapseAll, &QAction::triggered, this, &EntityPropertyEditor::OnCollapseAll);
        addAction(m_actionToCollapseAll);
        m_entityComponentActions.push_back(m_actionToCollapseAll);

        UpdateInternalState();
    }

    void EntityPropertyEditor::UpdateActions()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_disabled)
        {
            return;
        }

        const auto& componentsToEdit = GetSelectedComponents();

        // you can use this to veto any ability to modify components present:
        const bool allowAnyComponentModification = AllowAnyComponentModification();

        const bool hasComponents = !m_selectedEntityIds.empty() && !componentsToEdit.empty();
        // Don't allow components to be removed/cut/enabled/disabled if read only
        const bool allowRemove = allowAnyComponentModification && hasComponents && AreComponentsRemovable(componentsToEdit) && !m_selectionContainsReadOnlyEntity;
        const bool allowCopy = allowAnyComponentModification && hasComponents && AreComponentsCopyable(componentsToEdit);

        m_actionToDeleteComponents->setEnabled(allowRemove);
        m_actionToCutComponents->setEnabled(allowRemove && allowCopy);
        m_actionToCopyComponents->setEnabled(allowCopy);
        m_actionToPasteComponents->setEnabled(allowAnyComponentModification && !m_selectedEntityIds.empty() && CanPasteComponentsOnSelectedEntities());
        m_actionToDuplicateComponents->setEnabled(allowCopy && allowAnyComponentModification && !m_selectedEntityIds.empty());
        m_actionToMoveComponentsUp->setEnabled(allowRemove && IsMoveComponentsUpAllowed());
        m_actionToMoveComponentsDown->setEnabled(allowRemove && IsMoveComponentsDownAllowed());
        m_actionToMoveComponentsTop->setEnabled(allowRemove && IsMoveComponentsUpAllowed());
        m_actionToMoveComponentsBottom->setEnabled(allowRemove && IsMoveComponentsDownAllowed());

        bool allowEnable = false;
        bool allowDisable = false;

        AZ::Entity::ComponentArrayType disabledComponents;

        //build a working set of all components selected for edit
        AZStd::unordered_set<AZ::Component*> enabledComponents(componentsToEdit.begin(), componentsToEdit.end());
        for (const auto& entityId : m_selectedEntityIds)
        {
            disabledComponents.clear();
            EditorDisabledCompositionRequestBus::Event(entityId, &EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);

            //remove all disabled components from the set of enabled components
            for (auto disabledComponent : disabledComponents)
            {
                if (enabledComponents.erase(disabledComponent) > 0)
                {
                    //if any disabled components were found/removed from the selected set, assume they can be enabled
                    allowEnable = true;
                }
            }
        }

        // Even though this causes two loops on the selected entity list, calling GetSelectionEntityTypeInfo avoids duplicating code.
        SelectionEntityTypeInfo selectionTypeInfo;
        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "EntityPropertyEditor::UpdateActions GetSelectionEntityTypeInfo");
            selectionTypeInfo = GetSelectionEntityTypeInfo(m_selectedEntityIds);
        }
        m_actionToAddComponents->setEnabled(CanAddComponentsToSelection(selectionTypeInfo));

        //if any components remain in the selected set, assume they can be disabled
        allowDisable = !enabledComponents.empty();

        //disable actions when not allowed
        m_actionToEnableComponents->setEnabled(allowRemove && allowEnable);
        m_actionToDisableComponents->setEnabled(allowRemove && allowDisable);

        //additional request to hide actions when not allowed so enable and disable aren't shown at the same time
        m_actionToEnableComponents->setVisible(allowRemove && allowEnable);
        m_actionToDisableComponents->setVisible(allowRemove && allowDisable);

        if (hasComponents)
        {
            bool anyExpanded = false;
            for (auto componentEditor : m_componentEditors)
            {
                if (componentEditor->IsExpanded())
                {
                    anyExpanded = true;
                    break;
                }
            }

            m_actionToCollapseAll->setEnabled(anyExpanded);
        }
        else
        {
            m_actionToCollapseAll->setEnabled(false);
        }

        m_actionToCollapseAll->setVisible(hasComponents);
    }

    bool EntityPropertyEditor::CanPasteComponentsOnSelectedEntities() const
    {
        return CanPasteComponentsOnSelectedEntitiesFromMimeData(ComponentMimeData::GetComponentMimeDataFromClipboard());
    }

    bool EntityPropertyEditor::CanPasteComponentsOnSelectedEntitiesFromMimeData(const QMimeData* mimeData) const
    {
        if (!AllowAnyComponentModification())
        {
            return false;
        }

        if (m_selectedEntityIds.empty())
        {
            return false;
        }

        if (m_selectionContainsReadOnlyEntity)
        {
            // Can't paste components if there is a read only entity in the selection
            return false;
        }

        SelectionEntityTypeInfo selectionTypeInfo = GetSelectionEntityTypeInfo(m_selectedEntityIds);
        if (!CanAddComponentsToSelection(selectionTypeInfo))
        {
            // Can't paste components if there is a mixed selection or read only entities
            return false;
        }

        if (!mimeData)
        {
            return false;
        }

        // Create class data from mime data
        ComponentTypeMimeData::ClassDataContainer classDataForComponentsToPaste;
        ComponentTypeMimeData::Get(mimeData, classDataForComponentsToPaste);

        if (classDataForComponentsToPaste.empty())
        {
            return false;
        }

        bool canPaste = true;

        // Check if all components from mime data can be pasted onto all selected entities
        for (const AZ::EntityId& entityId : m_selectedEntityIds)
        {
            auto entity = GetEntityById(entityId);
            if (!entity || !CanPasteComponentsOnEntity(classDataForComponentsToPaste, entity))
            {
                canPaste = false;
                break;
            }
        }

        return canPaste;
    }

    bool EntityPropertyEditor::CanPasteComponentsOnEntity(const ComponentTypeMimeData::ClassDataContainer& classDataForComponentsToPaste, const AZ::Entity* entity) const
    {
        if (!AllowAnyComponentModification())
        {
            return false;
        }

        if (!entity || classDataForComponentsToPaste.empty())
        {
            return false;
        }

        bool canPaste = true;

        for (auto componentClassData : classDataForComponentsToPaste)
        {
            if (componentClassData)
            {
                // A component can be pasted onto an entity if it appears in the add component menu or if it already exists on the entity
                auto existingComponent = entity->FindComponent(componentClassData->m_typeId);
                if (!existingComponent && (!m_componentFilter || !m_componentFilter(*componentClassData)))
                {
                    canPaste = false;
                    break;
                }
            }
        }

        return canPaste;
    }

    AZ::Entity::ComponentArrayType EntityPropertyEditor::GetCopyableComponents() const
    {
        const auto& selectedComponents = GetSelectedComponents();
        AZ::Entity::ComponentArrayType copyableComponents;
        copyableComponents.reserve(selectedComponents.size());
        for (auto component : selectedComponents)
        {
            const AZ::Entity* entity = component ? component->GetEntity() : nullptr;
            const AZ::EntityId entityId = entity ? entity->GetId() : AZ::EntityId();
            if (entityId == m_selectedEntityIds.front())
            {
                copyableComponents.push_back(component);
            }
        }
        return copyableComponents;
    }

    void EntityPropertyEditor::DeleteComponents(AZStd::span<AZ::Component* const> components)
    {
        if (!components.empty() && AreComponentsRemovable(components))
        {
            // Need to queue an update for all inspectors in case multiples are viewing the same entity and the removal of a component internally triggers an invalidate call
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);

            ScopedUndoBatch undoBatch("Remove Component(s)");

            AzToolsFramework::RemoveComponents(components);
        }
    }

    void EntityPropertyEditor::DeleteComponents()
    {
        DeleteComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::CutComponents()
    {
        if (!AllowAnyComponentModification())
        {
            return;
        }

        const auto& componentsToEdit = GetSelectedComponents();
        if (!componentsToEdit.empty() && AreComponentsRemovable(componentsToEdit))
        {
            QueuePropertyRefresh();

            //intentionally not using EntityCompositionRequests::CutComponents because we only want to copy components from first selected entity
            ScopedUndoBatch undoBatch("Cut Component(s)");
            CopyComponents();
            DeleteComponents();
        }
    }

    void EntityPropertyEditor::CopyComponents()
    {
        const auto& componentsToEdit = GetCopyableComponents();
        if (!componentsToEdit.empty() && AreComponentsCopyable(componentsToEdit))
        {
            EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::CopyComponents, componentsToEdit);
        }
    }

    void EntityPropertyEditor::DuplicateComponents()
    {
        const auto& componentsToEdit = GetCopyableComponents();
        if (componentsToEdit.empty() || !AreComponentsCopyable(componentsToEdit))
        {
            return;
        }

        // Build mime data directly and paste it, so duplicate doesn't overwrite the user's clipboard.
        AZStd::unique_ptr<QMimeData> mimeData = ComponentMimeData::Create(componentsToEdit);
        PasteComponentsFromMimeData(mimeData.get());
    }

    void EntityPropertyEditor::PasteComponents()
    {
        PasteComponentsFromMimeData(ComponentMimeData::GetComponentMimeDataFromClipboard());
    }

    void EntityPropertyEditor::PasteComponentsFromMimeData(const QMimeData* mimeData)
    {
        if (!m_selectedEntityIds.empty() && CanPasteComponentsOnSelectedEntitiesFromMimeData(mimeData))
        {
            ScopedUndoBatch undoBatch("Paste Component(s)");

            AZStd::span<AZ::Component* const> selectedComponents = GetSelectedComponents();

            AZ::Entity::ComponentArrayType componentsAfterPaste;
            AZ::Entity::ComponentArrayType componentsInOrder;

            //paste to all selected entities
            for (const AZ::EntityId& entityId : m_selectedEntityIds)
            {
                //get components and order before pasting so we can insert new components above the first selected one
                componentsInOrder.clear();
                GetAllComponentsForEntityInOrder(GetEntity(entityId), componentsInOrder);

                //perform the paste operation which should add new components to the entity or pending list
                EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::PasteComponentsToEntityFromMimeData, entityId, mimeData);

                //get the post-paste set of components, which should include all prior components plus new ones
                componentsAfterPaste.clear();
                GetAllComponentsForEntity(entityId, componentsAfterPaste);

                //remove pre-existing components from the post-paste set
                for (auto component : componentsInOrder)
                {
                    componentsAfterPaste.erase(AZStd::remove(componentsAfterPaste.begin(), componentsAfterPaste.end(), component), componentsAfterPaste.end());
                }

                for (auto component : componentsAfterPaste)
                {
                    //add the remaining new components before the first selected one
                    auto insertItr = AZStd::find_first_of(componentsInOrder.begin(), componentsInOrder.end(), selectedComponents.begin(), selectedComponents.end());
                    componentsInOrder.insert(insertItr, component);

                    //only scroll to the bottom if added to the back of the first entity
                    if (insertItr == componentsInOrder.end() && entityId == m_selectedEntityIds.front())
                    {
                        m_shouldScrollToNewComponents = true;
                    }
                }

                SortComponentsByPriority(componentsInOrder);
                SaveComponentOrder(entityId, componentsInOrder);
            }

            QueuePropertyRefresh();
        }
    }

    void EntityPropertyEditor::EnableComponents(AZStd::span<AZ::Component* const> components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::EnableComponents, components);
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::EnableComponents()
    {
        EnableComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::DisableComponents(AZStd::span<AZ::Component* const> components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::DisableComponents, components);
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::DisableComponents()
    {
        DisableComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::MoveComponentsUp()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!AreComponentsRemovable(componentsToEdit))
        {
            return;
        }

        ScopedUndoBatch undoBatch("Move Component(s) Up");

        ComponentEditorVector componentEditors = m_componentEditors;
        for (size_t sourceComponentEditorIndex = 1; sourceComponentEditorIndex < componentEditors.size(); ++sourceComponentEditorIndex)
        {
            size_t targetComponentEditorIndex = sourceComponentEditorIndex - 1;
            if (IsMoveAllowed(componentEditors, sourceComponentEditorIndex, targetComponentEditorIndex))
            {
                auto& sourceComponentEditor = componentEditors[sourceComponentEditorIndex];
                auto& targetComponentEditor = componentEditors[targetComponentEditorIndex];
                auto sourceComponents = sourceComponentEditor->GetComponents();
                auto targetComponents = targetComponentEditor->GetComponents();
                MoveComponentRowBefore(sourceComponents, targetComponents, undoBatch);
                AZStd::swap(targetComponentEditor, sourceComponentEditor);
            }
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::MoveComponentsDown()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!AreComponentsRemovable(componentsToEdit))
        {
            return;
        }

        ScopedUndoBatch undoBatch("Move Component(s) Down");

        ComponentEditorVector componentEditors = m_componentEditors;
        for (size_t targetComponentEditorIndex = componentEditors.size() - 1; targetComponentEditorIndex > 0; --targetComponentEditorIndex)
        {
            size_t sourceComponentEditorIndex = targetComponentEditorIndex - 1;
            if (IsMoveAllowed(componentEditors, sourceComponentEditorIndex, targetComponentEditorIndex))
            {
                auto& sourceComponentEditor = componentEditors[sourceComponentEditorIndex];
                auto& targetComponentEditor = componentEditors[targetComponentEditorIndex];
                auto sourceComponents = sourceComponentEditor->GetComponents();
                auto targetComponents = targetComponentEditor->GetComponents();
                MoveComponentRowAfter(sourceComponents, targetComponents, undoBatch);
                AZStd::swap(targetComponentEditor, sourceComponentEditor);
            }
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::MoveComponentsTop()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!AreComponentsRemovable(componentsToEdit))
        {
            return;
        }

        ScopedUndoBatch undoBatch("Move Component(s) to Top");

        AZ::Entity::ComponentArrayType componentsOnEntity;

        for (const auto& entityId : m_selectedEntityIds)
        {
            const auto componentsToEditByEntityIdItr = GetSelectedComponentsByEntityId().find(entityId);
            if (componentsToEditByEntityIdItr != GetSelectedComponentsByEntityId().end())
            {
                undoBatch.MarkEntityDirty(entityId);

                //TODO cache components on entities on inspector update
                componentsOnEntity.clear();
                GetAllComponentsForEntityInOrder(AzToolsFramework::GetEntity(entityId), componentsOnEntity);

                AZStd::span<AZ::Component* const> componentsToMove = componentsToEditByEntityIdItr->second;
                AZStd::sort(
                    componentsOnEntity.begin(),
                    componentsOnEntity.end(),
                    [&componentsToMove](const AZ::Component* component1, const AZ::Component* component2)
                    {
                        return
                        AZStd::find(componentsToMove.begin(), componentsToMove.end(), component1) != componentsToMove.end() &&
                        AZStd::find(componentsToMove.begin(), componentsToMove.end(), component2) == componentsToMove.end();
                    });

                SortComponentsByPriority(componentsOnEntity);
                SaveComponentOrder(entityId, componentsOnEntity);
            }
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::MoveComponentsBottom()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!AreComponentsRemovable(componentsToEdit))
        {
            return;
        }

        ScopedUndoBatch undoBatch("Move Component(s) to Bottom");

        AZ::Entity::ComponentArrayType componentsOnEntity;

        for (const auto& entityId : m_selectedEntityIds)
        {
            const auto componentsToEditByEntityIdItr = GetSelectedComponentsByEntityId().find(entityId);
            if (componentsToEditByEntityIdItr != GetSelectedComponentsByEntityId().end())
            {
                undoBatch.MarkEntityDirty(entityId);

                //TODO cache components on entities on inspector update
                componentsOnEntity.clear();
                GetAllComponentsForEntityInOrder(AzToolsFramework::GetEntity(entityId), componentsOnEntity);

                AZStd::span<AZ::Component* const> componentsToMove = componentsToEditByEntityIdItr->second;
                AZStd::sort(
                    componentsOnEntity.begin(),
                    componentsOnEntity.end(),
                    [&componentsToMove](const AZ::Component* component1, const AZ::Component* component2)
                    {
                        return
                        AZStd::find(componentsToMove.begin(), componentsToMove.end(), component2) != componentsToMove.end() &&
                        AZStd::find(componentsToMove.begin(), componentsToMove.end(), component1) == componentsToMove.end();
                    });

                SortComponentsByPriority(componentsOnEntity);
                SaveComponentOrder(entityId, componentsOnEntity);
            }
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::MoveComponentBefore(const AZ::Component* sourceComponent, const AZ::Component* targetComponent, ScopedUndoBatch& undo)
    {
        (void)undo;
        if (sourceComponent && sourceComponent != targetComponent)
        {
            const AZ::Entity* entity = sourceComponent->GetEntity();
            const AZ::EntityId entityId = entity->GetId();
            undo.MarkEntityDirty(entityId);

            AZ::Entity::ComponentArrayType componentsOnEntity;
            GetAllComponentsForEntityInOrder(entity, componentsOnEntity);
            componentsOnEntity.erase(AZStd::remove(componentsOnEntity.begin(), componentsOnEntity.end(), sourceComponent), componentsOnEntity.end());
            auto location = AZStd::find(componentsOnEntity.begin(), componentsOnEntity.end(), targetComponent);
            componentsOnEntity.insert(location, const_cast<AZ::Component*>(sourceComponent));

            SortComponentsByPriority(componentsOnEntity);
            SaveComponentOrder(entityId, componentsOnEntity);
        }
    }

    void EntityPropertyEditor::MoveComponentAfter(const AZ::Component* sourceComponent, const AZ::Component* targetComponent, ScopedUndoBatch& undo)
    {
        (void)undo;
        if (sourceComponent && sourceComponent != targetComponent)
        {
            const AZ::Entity* entity = sourceComponent->GetEntity();
            const AZ::EntityId entityId = entity->GetId();
            undo.MarkEntityDirty(entityId);

            AZ::Entity::ComponentArrayType componentsOnEntity;
            GetAllComponentsForEntityInOrder(entity, componentsOnEntity);
            componentsOnEntity.erase(AZStd::remove(componentsOnEntity.begin(), componentsOnEntity.end(), sourceComponent), componentsOnEntity.end());
            auto location = AZStd::find(componentsOnEntity.begin(), componentsOnEntity.end(), targetComponent);
            if (location != componentsOnEntity.end())
            {
                ++location;
            }

            componentsOnEntity.insert(location, const_cast<AZ::Component*>(sourceComponent));

            SortComponentsByPriority(componentsOnEntity);
            SaveComponentOrder(entityId, componentsOnEntity);
        }
    }

    void EntityPropertyEditor::MoveComponentRowBefore(AZStd::span<AZ::Component* const> sourceComponents, AZStd::span<AZ::Component* const> targetComponents, ScopedUndoBatch& undo)
    {
        //determine if both sets of components have the same number of elements
        //this should always be the case because component editors are only shown for components with equivalents on all selected entities
        //the only time they should be out of alignment is when dragging to space above or below all the component editors
        const bool targetComponentsAlign = sourceComponents.size() == targetComponents.size();

        //move all source components above target
        //this will account for multiple selected entities and components
        //but we may disallow the operation for multi-select or force it to only apply to the first selected entity
        for (size_t componentIndex = 0; componentIndex < sourceComponents.size(); ++componentIndex)
        {
            const AZ::Component* sourceComponent = sourceComponents[componentIndex];
            const AZ::Component* targetComponent = targetComponentsAlign ? targetComponents[componentIndex] : nullptr;
            MoveComponentBefore(sourceComponent, targetComponent, undo);
        }
    }

    void EntityPropertyEditor::MoveComponentRowAfter(AZStd::span<AZ::Component* const> sourceComponents, AZStd::span<AZ::Component* const> targetComponents, ScopedUndoBatch& undo)
    {
        //determine if both sets of components have the same number of elements
        //this should always be the case because component editors are only shown for components with equivalents on all selected entities
        //the only time they should be out of alignment is when dragging to space above or below all the component editors
        const bool targetComponentsAlign = sourceComponents.size() == targetComponents.size();

        //move all source components below target
        //this will account for multiple selected entities and components
        //but we may disallow the operation for multi-select or force it to only apply to the first selected entity
        for (size_t componentIndex = 0; componentIndex < sourceComponents.size(); ++componentIndex)
        {
            const AZ::Component* sourceComponent = sourceComponents[componentIndex];
            const AZ::Component* targetComponent = targetComponentsAlign ? targetComponents[componentIndex] : nullptr;
            MoveComponentAfter(sourceComponent, targetComponent, undo);
        }
    }

    bool EntityPropertyEditor::IsMoveAllowed(const ComponentEditorVector& componentEditors) const
    {
        for (size_t sourceComponentEditorIndex = 1; sourceComponentEditorIndex < componentEditors.size(); ++sourceComponentEditorIndex)
        {
            size_t targetComponentEditorIndex = sourceComponentEditorIndex - 1;
            if (IsMoveAllowed(componentEditors, sourceComponentEditorIndex, targetComponentEditorIndex))
            {
                return true;
            }
        }
        return false;
    }

    bool EntityPropertyEditor::IsMoveAllowed(const ComponentEditorVector& componentEditors, size_t sourceComponentEditorIndex, size_t targetComponentEditorIndex) const
    {
        const auto sourceComponentEditor = componentEditors[sourceComponentEditorIndex];
        const auto targetComponentEditor = componentEditors[targetComponentEditorIndex];
        const auto& sourceComponents = sourceComponentEditor->GetComponents();
        const auto& targetComponents = targetComponentEditor->GetComponents();
        return
            sourceComponentEditor->isVisible() &&
            targetComponentEditor->isVisible() &&
            sourceComponentEditor->IsSelected() &&
            !targetComponentEditor->IsSelected() &&
            sourceComponents.size() == m_selectedEntityIds.size() &&
            targetComponents.size() == m_selectedEntityIds.size() &&
            AreComponentsRemovable(sourceComponents) &&
            AreComponentsRemovable(targetComponents) &&
            AreComponentsDraggable(sourceComponents) &&
            AreComponentsDraggable(targetComponents);
    }

    bool EntityPropertyEditor::IsMoveComponentsUpAllowed() const
    {
        return IsMoveAllowed(m_componentEditors);
    }

    bool EntityPropertyEditor::IsMoveComponentsDownAllowed() const
    {
        ComponentEditorVector componentEditors = m_componentEditors;
        AZStd::reverse(componentEditors.begin(), componentEditors.end());
        return IsMoveAllowed(componentEditors);
    }

    bool EntityPropertyEditor::DoesOwnFocus() const
    {
        QWidget* widget = QApplication::focusWidget();
        return this == widget || isAncestorOf(widget);
    }

    AZ::u32 EntityPropertyEditor::GetHeightOfRowAndVisibleChildren(const PropertyRowWidget* row) const
    {
        if (!row->isVisible())
        {
            return 0;
        }

        QRect rect = QRect(row->mapToGlobal(row->rect().topLeft()), row->mapToGlobal(row->rect().bottomRight()));

        AZ::u32 height = rect.height() + 1;

        for (AZ::u32 childIndex = 0; childIndex < row->GetChildRowCount(); childIndex++)
        {
            PropertyRowWidget* childRow = row->GetChildRowByIndex(childIndex);
            if (childRow->isVisible())
            {
                height += GetHeightOfRowAndVisibleChildren(childRow);
            }
        }

        return height;
    }

    QRect EntityPropertyEditor::GetWidgetAndVisibleChildrenGlobalRect(const PropertyRowWidget* widget) const
    {
        QRect rect = QRect(widget->mapToGlobal(widget->rect().topLeft()), widget->mapToGlobal(widget->rect().bottomRight()));
        rect.setHeight(GetHeightOfRowAndVisibleChildren(widget));

        return rect;
    }

    PropertyRowWidget* EntityPropertyEditor::GetRowWidgetAtSameLevelAfter(PropertyRowWidget* widget) const
    {
        PropertyRowWidget* parent = widget->GetParentRow();

        bool found = false;
        for (PropertyRowWidget* child : parent->GetChildrenRows())
        {
            if (found)
            {
                return child;
            }

            if (child == widget)
            {
                found = true;
            }
        }

        return nullptr;
    }

    PropertyRowWidget* EntityPropertyEditor::GetRowWidgetAtSameLevelBefore(PropertyRowWidget* widget) const
    {
        PropertyRowWidget* parent = widget->GetParentRow();

        PropertyRowWidget* previous = nullptr;
        for (PropertyRowWidget* child : parent->GetChildrenRows())
        {
            if (child == widget)
            {
                return previous;
            }

            previous = child;
        }

        return nullptr;
    }

    QRect EntityPropertyEditor::GetWidgetGlobalRect(const QWidget* widget) const
    {
        return QRect(
            widget->mapToGlobal(widget->rect().topLeft()),
            widget->mapToGlobal(widget->rect().bottomRight()));
    }

    bool EntityPropertyEditor::DoesIntersectWidget(const QRect& globalRect, const QWidget* widget) const
    {
        return widget->isVisible() && globalRect.intersects(GetWidgetGlobalRect(widget));
    }

    bool EntityPropertyEditor::DoesIntersectSelectedComponentEditor(const QRect& globalRect) const
    {
        for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
        {
            if (componentEditor->IsSelected())
            {
                return true;
            }
        }
        return false;
    }

    bool EntityPropertyEditor::DoesIntersectNonSelectedComponentEditor(const QRect& globalRect) const
    {
        for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
        {
            if (!componentEditor->IsSelected())
            {
                return true;
            }
        }
        return false;
    }

    void EntityPropertyEditor::ClearComponentEditorDragging()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->SetDragged(false);
            componentEditor->SetDropTarget(false);
        }
        UpdateOverlay();
    }

    void EntityPropertyEditor::ClearComponentEditorSelection()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->SetSelected(false);
        }
        SaveComponentEditorState();
        UpdateInternalState();
    }

    void EntityPropertyEditor::SelectRangeOfComponentEditors(const AZ::s32 index1, const AZ::s32 index2, bool selected)
    {
        if (index1 >= 0 && index2 >= 0)
        {
            const AZ::s32 min = AZStd::min(index1, index2);
            const AZ::s32 max = AZStd::max(index1, index2);
            for (AZ::s32 index = min; index <= max; ++index)
            {
                m_componentEditors[index]->SetSelected(selected);
            }
            m_componentEditorLastSelectedIndex = index2;
        }
        SaveComponentEditorState();
        UpdateInternalState();
        NotifySelectionChanges();
    }

    void EntityPropertyEditor::SelectIntersectingComponentEditors(const QRect& globalRect, bool selected)
    {
        for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
        {
            componentEditor->SetSelected(selected);
            m_componentEditorLastSelectedIndex = GetComponentEditorIndex(componentEditor);
        }
        SaveComponentEditorState();
        UpdateInternalState();
        NotifySelectionChanges();
    }

    bool EntityPropertyEditor::SelectIntersectingComponentEditorsSafe(const QRect& globalRect)
    {
        bool selectedChanged = false; // check if selection actually changed (component wasn't disabled)
        for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
        {
            // only want to allow selection when component editor is enabled
            if (componentEditor->isEnabled())
            {
                componentEditor->SetSelected(true);
                m_componentEditorLastSelectedIndex = GetComponentEditorIndex(componentEditor);
                selectedChanged = true;
            }
        }
        SaveComponentEditorState();
        UpdateInternalState();
        NotifySelectionChanges();

        return selectedChanged;
    }

    void EntityPropertyEditor::ToggleIntersectingComponentEditors(const QRect& globalRect)
    {
        for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
        {
            componentEditor->SetSelected(!componentEditor->IsSelected());
            m_componentEditorLastSelectedIndex = GetComponentEditorIndex(componentEditor);
        }
        SaveComponentEditorState();
        UpdateInternalState();
        NotifySelectionChanges();
    }

    AZ::s32 EntityPropertyEditor::GetComponentEditorIndex(const ComponentEditor* componentEditor) const
    {
        AZ::s32 index = 0;
        for (auto componentEditorToCompare : m_componentEditors)
        {
            if (componentEditorToCompare == componentEditor)
            {
                return index;
            }
            ++index;
        }
        return -1;
    }

    AZ::s32 EntityPropertyEditor::GetComponentEditorIndexFromType(const AZ::Uuid& componentType) const
    {
        AZ::s32 index = 0;
        for (auto componentEditorToCompare : m_componentEditors)
        {
            if (componentEditorToCompare->GetComponentType() == componentType)
            {
                return index;
            }
            ++index;
        }
        return -1;
    }

    ComponentEditorVector EntityPropertyEditor::GetIntersectingComponentEditors(const QRect& globalRect) const
    {
        ComponentEditorVector intersectingComponentEditors;
        intersectingComponentEditors.reserve(m_componentEditors.size());
        for (auto componentEditor : m_componentEditors)
        {
            if (DoesIntersectWidget(globalRect, componentEditor))
            {
                intersectingComponentEditors.push_back(componentEditor);
            }
        }
        return intersectingComponentEditors;
    }

    const ComponentEditorVector& EntityPropertyEditor::GetSelectedComponentEditors() const
    {
        return m_selectedComponentEditors;
    }

    AZStd::span<AZ::Component* const> EntityPropertyEditor::GetSelectedComponents() const
    {
        return m_selectedComponents;
    }

    const AZStd::unordered_map<AZ::EntityId, AZ::Entity::ComponentArrayType>& EntityPropertyEditor::GetSelectedComponentsByEntityId() const
    {
        return m_selectedComponentsByEntityId;
    }

    void EntityPropertyEditor::UpdateSelectionCache()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        m_selectedComponentEditors.clear();
        m_selectedComponentEditors.reserve(m_componentEditors.size());
        for (auto componentEditor : m_componentEditors)
        {
            if (componentEditor->isVisible() && componentEditor->IsSelected())
            {
                m_selectedComponentEditors.push_back(componentEditor);
            }
        }

        m_selectedComponents.clear();
        for (auto componentEditor : m_selectedComponentEditors)
        {
            const auto& components = componentEditor->GetComponents();
            m_selectedComponents.insert(m_selectedComponents.end(), components.begin(), components.end());
        }

        m_selectedEntityComponentIds.clear();
        m_selectedComponentsByEntityId.clear();
        for (auto component : m_selectedComponents)
        {
            m_selectedComponentsByEntityId[component->GetEntityId()].push_back(component);
            m_selectedEntityComponentIds.insert(AZ::EntityComponentIdPair(component->GetEntityId(), component->GetId()));
        }
    }

    void EntityPropertyEditor::NotifySelectionChanges()
    {
        EntityPropertyEditorNotificationBus::Broadcast(
            &EntityPropertyEditorNotifications::OnComponentSelectionChanged, this, m_selectedEntityComponentIds);
    }

    void EntityPropertyEditor::SaveComponentEditorState()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // SaveComponentEditorState can be called when adding or removing a
        // component, the components list stored by the component editor
        // will be empty. In this state we do not want to save the editor
        // state as we'll lose what was the currently selected entity and
        // what components were collapsed - only save if we know the component
        // editor has valid components
        bool canSave = false;
        for (auto componentEditor : m_componentEditors)
        {
            if (!componentEditor->GetComponents().empty())
            {
                canSave = true;
                break;
            }
        }

        if (canSave)
        {
            m_componentEditorSaveStateTable.clear();
            for (auto componentEditor : m_componentEditors)
            {
                SaveComponentEditorState(componentEditor);
            }
        }
    }

    void EntityPropertyEditor::SaveComponentEditorState(ComponentEditor* componentEditor)
    {
        for (auto component : componentEditor->GetComponents())
        {
            AZ::ComponentId componentId = component->GetId();
            ComponentEditorSaveState& state = m_componentEditorSaveStateTable[componentId];
            state.m_selected = componentEditor->IsSelected();
        }
    }

    void EntityPropertyEditor::LoadComponentEditorState()
    {
        for (auto componentEditor : m_componentEditors)
        {
            ComponentEditorSaveState state;
            if (!componentEditor->GetComponents().empty())
            {
                AZ::ComponentId componentId = componentEditor->GetComponents().front()->GetId();
                state = m_componentEditorSaveStateTable[componentId];
            }

            componentEditor->SetSelected(state.m_selected);
        }
        UpdateOverlay();
    }

    void EntityPropertyEditor::ClearComponentEditorState()
    {
        m_componentEditorSaveStateTable.clear();
    }

    void EntityPropertyEditor::ScrollToNewComponent()
    {
        m_shouldScrollToNewComponentsQueued = false;

        if (!m_shouldScrollToNewComponents)
        {
            // if we get here, we should just restore the scroll.
            if (m_savedVerticalScroll >= 0)
            {
                m_gui->m_componentList->verticalScrollBar()->setValue(m_savedVerticalScroll);
            }

            if (m_savedHorizontalScroll >= 0)
            {
                m_gui->m_componentList->horizontalScrollBar()->setValue(m_savedHorizontalScroll);
            }
            return;
        }
        m_shouldScrollToNewComponents = false;

        // force new components to be visible
        // if no component has been explicitly set at the most recently added,
        // assume new components are added to the end of the list and layout
        AZ::s32 newComponentIndex = m_componentEditorsUsed - 1;

        // if there is a component id explicitly set as the most recently added, try to find it and make sure it is visible
        if (m_newComponentId.has_value() && m_newComponentId.value() != AZ::InvalidComponentId)
        {
            AZ::ComponentId newComponentId = m_newComponentId.value();
            for (AZ::s32 componentIndex = 0; componentIndex < m_componentEditorsUsed; ++componentIndex)
            {
                if (m_componentEditors[componentIndex])
                {
                    for (const auto component : m_componentEditors[componentIndex]->GetComponents())
                    {
                        if (component->GetId() == newComponentId)
                        {
                            newComponentIndex = componentIndex;
                        }
                    }
                }
            }
        }

        auto componentEditor = GetComponentEditorsFromIndex(newComponentIndex);
        if (componentEditor)
        {
            m_gui->m_componentList->ensureWidgetVisible(componentEditor);
        }
        m_newComponentId.reset();

        HighlightMovedRowWidget();
    }

    void EntityPropertyEditor::QueueScrollToNewComponent()
    {
        if (!m_shouldScrollToNewComponentsQueued)
        {
            // this delay cannot be zero but still should be as short as possible to avoid jiggling in the display
            // it cannot be zero, because other immediate updates are likely scheduled
            // and it cannot be 1 since other cascading updates are likely to be scheduled.
            // In windows, a timer of less than about 20ms will actually turn into a busy loop, so 30ms is a good value
            QTimer::singleShot(30, this, &EntityPropertyEditor::ScrollToNewComponent);
            m_shouldScrollToNewComponentsQueued = true;
        }
    }

    void EntityPropertyEditor::BuildEntityIconMenu()
    {
        QMenu menu(this);
        menu.setToolTipsVisible(true);

        QAction* action = nullptr;
        action = menu.addAction(QObject::tr("Set default icon"));
        action->setToolTip(tr("Sets the icon to the first component icon after the transform or uses the transform icon"));
        QObject::connect(action, &QAction::triggered, this, &EntityPropertyEditor::SetEntityIconToDefault);

        action = menu.addAction(QObject::tr("Set custom icon"));
        action->setToolTip(tr("Choose a custom icon for this entity"));
        QObject::connect(action, &QAction::triggered, this, &EntityPropertyEditor::PopupAssetBrowserForEntityIcon);

        menu.exec(GetWidgetGlobalRect(m_gui->m_entityIcon).bottomLeft());
    }

    void EntityPropertyEditor::SetEntityIconToDefault()
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Change Entity Icon");

        AZ::Data::AssetId invalidAssetId;
        for (AZ::EntityId entityId : m_selectedEntityIds)
        {
            EditorEntityIconComponentRequestBus::Event(entityId, &EditorEntityIconComponentRequestBus::Events::SetEntityIconAsset, invalidAssetId);
        }

        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::PopupAssetBrowserForEntityIcon()
    {
        /*
        * Request the AssetBrowser Dialog and set a type filter
        */

        if (m_selectedEntityIds.empty())
        {
            return;
        }

        AZ::Data::AssetType entityIconAssetType("{3436C30E-E2C5-4C3B-A7B9-66C94A28701B}");
        AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection(entityIconAssetType);

        AZ::Data::AssetId currentEntityIconAssetId;
        EditorEntityIconComponentRequestBus::EventResult(currentEntityIconAssetId, m_selectedEntityIds.front(), &EditorEntityIconComponentRequestBus::Events::GetEntityIconAssetId);
        selection.SetSelectedAssetId(currentEntityIconAssetId);
        EditorRequests::Bus::Broadcast(&EditorRequests::BrowseForAssets, selection);

        if (selection.IsValid())
        {
            auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
            if (product)
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Change Entity Icon");

                AZ::Data::AssetId iconAssetId = product->GetAssetId();
                for (AZ::EntityId entityId : m_selectedEntityIds)
                {
                    EditorEntityIconComponentRequestBus::Event(entityId, &EditorEntityIconComponentRequestBus::Events::SetEntityIconAsset, iconAssetId);
                }

                QueuePropertyRefresh();
            }
            else
            {
                AZ_Warning("Property Editor", false, "Incorrect asset selected. Expected entity icon asset.");
            }
        }
    }

    void EntityPropertyEditor::OnStatusChanged(int index)
    {
        {
            ScopedUndoBatch undo("Change Status");

            switch (IndexToStatusType(index))
            {
                case StatusType::StatusStartActive:
                case StatusType::StatusStartInactive:
                {
                    bool entityIsStartingActive = index == StatusTypeToIndex(StatusType::StatusStartActive);

                    for (AZ::EntityId entityId : m_selectedEntityIds)
                    {
                        AZ::Entity* entity = nullptr;
                        AZ::ComponentApplicationBus::BroadcastResult(
                            entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

                        EditorOnlyEntityComponentRequestBus::Event(
                            entityId, &EditorOnlyEntityComponentRequests::SetIsEditorOnlyEntity,
                            false);
                        if (entity)
                        {
                            EditorEntityRuntimeActivationChangeNotificationBus::Broadcast(
                                &EditorEntityRuntimeActivationChangeNotificationBus::Events::
                                OnEntityRuntimeActivationChanged,
                                entityId, entityIsStartingActive);

                            entity->SetRuntimeActiveByDefault(entityIsStartingActive);
                            undo.MarkEntityDirty(entityId);
                        }
                    }
                    break;
                }
                case StatusType::StatusEditorOnly:
                    for (AZ::EntityId entityId : m_selectedEntityIds)
                    {
                        EditorOnlyEntityComponentRequestBus::Event(entityId, &EditorOnlyEntityComponentRequests::SetIsEditorOnlyEntity, true);
                    }
                    break;
            }
        }

        // This notification will trigger the update of the actual combo box, so no need to call that explicitly
        for (AZ::EntityId entityId : m_selectedEntityIds)
        {
            AZ::EntitySystemBus::Broadcast(&AZ::EntitySystemBus::Events::OnEntityStartStatusChanged, entityId);
        }
    }

    void EntityPropertyEditor::UpdateOverlay()
    {
        m_overlay->setVisible(true);
        m_overlay->setGeometry(m_gui->m_componentListContents->rect());
        m_overlay->raise();
        m_overlay->update();
    }

    void EntityPropertyEditor::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        UpdateOverlay();
    }

    void EntityPropertyEditor::contextMenuEvent(QContextMenuEvent* event)
    {
        OnDisplayComponentEditorMenu(event->globalPos());
        event->accept();
    }

    bool EntityPropertyEditor::HandleMenuEvent(QObject* /*object*/, QEvent* event)
    {
        QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!menu)
        {
            return false;
        }

        PropertyRowWidget* lastReorderDropTarget = m_reorderDropTarget;

        switch (event->type())
        {
        case QEvent::Leave:
            m_reorderDropTarget = nullptr;
            break;
        case QEvent::Enter:
            // Drop through.
        case QEvent::MouseMove:
            QMouseEvent* originalMouseEvent = static_cast<QMouseEvent*>(event);
            QAction* action = menu->actionAt(originalMouseEvent->pos());
            if (!action)
            {
                m_reorderDropTarget = nullptr;
                break;
            }

            if (action->data() == kPropertyEditorMenuActionMoveUp)
            {
                m_reorderDropTarget = GetRowWidgetAtSameLevelBefore(m_reorderRowWidget);

                m_reorderDropArea = EntityPropertyEditor::DropArea::Above;
            }
            else if (action->data() == kPropertyEditorMenuActionMoveDown)
            {
                m_reorderDropTarget = GetRowWidgetAtSameLevelAfter(m_reorderRowWidget);

                if (m_reorderDropTarget)
                {
                    m_reorderDropArea = EntityPropertyEditor::DropArea::Below;
                }
            }

            break;
        }

        if (lastReorderDropTarget != m_reorderDropTarget)
        {
            // Force a redraw as the menu is preventing automatic updates.
            repaint(0, 0, -1, -1);
        }

        return false;
    }

    //overridden to intercept application level mouse events for component editor selection
    bool EntityPropertyEditor::eventFilter(QObject* object, QEvent* event)
    {
        // Interruptible load: the frozen inspector shows its last picture, which the mouse must not act on
        if (m_viewFrozen && IgnoredWhileFrozen(object, event))
        {
            return true;
        }

        HandleMenuEvent(object, event);
        HandleSelectionEvents(object, event);

        // Lazy cards: a resized view builds the held cards it reaches. (Tab needs nothing more: the list scrolls to the focused widget,
        // and the scroll builds the cards down to one screen below it before the focus gets there.)
        if (event->type() == QEvent::Resize && object == m_gui->m_componentList->viewport())
        {
            ReleaseLazyCardsInView();
        }
        return false;
    }

    void EntityPropertyEditor::mousePressEvent(QMouseEvent* event)
    {
        ResetDrag(event);

        PropertyRowWidget* rowWidget = FindPropertyRowWidgetAt(event->globalPosition().toPoint());
        if (rowWidget && rowWidget->CanBeReordered() && event->buttons() & Qt::LeftButton)
        {
            QApplication::setOverrideCursor(m_dragCursor);
        }
    }

    void EntityPropertyEditor::mouseReleaseEvent(QMouseEvent* event)
    {
        ResetDrag(event);

        if (QApplication::overrideCursor() && !(event->buttons() & Qt::LeftButton))
        {
            QApplication::restoreOverrideCursor();
        }
    }

    void EntityPropertyEditor::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_isAlreadyQueuedRefresh)
        {
            // not allowed to start operations when we are rebuilding the tree
            return;
        }
        StartDrag(event);
    }

    void EntityPropertyEditor::dragEnterEvent(QDragEnterEvent* event)
    {
        if (m_selectionContainsReadOnlyEntity)
        {
            event->ignore();
            return;
        }

        if (UpdateDrag(event->position().toPoint(), event->buttons(), event->mimeData()))
        {
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }

    void EntityPropertyEditor::dragMoveEvent(QDragMoveEvent* event)
    {
        if (m_selectionContainsReadOnlyEntity)
        {
            event->ignore();
            return;
        }

        if (UpdateDrag(event->position().toPoint(), event->buttons(), event->mimeData()))
        {
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }

    void EntityPropertyEditor::dropEvent(QDropEvent* event)
    {
        HandleDrop(event);

        if (QApplication::overrideCursor())
        {
            QApplication::restoreOverrideCursor();
        }
    }

    void EntityPropertyEditor::DragStopped()
    {
        if (QApplication::overrideCursor())
        {
            QApplication::restoreOverrideCursor();
        }

        EndRowWidgetReorder();
    }

    bool EntityPropertyEditor::HandleSelectionEvents(QObject* object, QEvent* event)
    {
        // if we're in the middle of a tree rebuild, we can't afford to touch the internals
        if (m_isAlreadyQueuedRefresh)
        {
            return false;
        }

        (void)object;
        if (m_dragStarted || m_selectedEntityIds.empty())
        {
            return false;
        }

        if (event->type() != QEvent::MouseButtonPress &&
            event->type() != QEvent::MouseButtonDblClick &&
            event->type() != QEvent::MouseButtonRelease)
        {
            return false;
        }

        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        //selection now occurs on mouse released
        //reset selection flag when mouse is clicked to allow additional selection changes
        if (event->type() == QEvent::MouseButtonPress)
        {
            m_selectionEventAccepted = false;
            return false;
        }

        //reject input if selection already occurred for this click
        if (m_selectionEventAccepted)
        {
            return false;
        }

        //reject input if a popup or modal window is active
        //this does not apply to menues, because then we can not select an item
        //while right clicking on it (context menu pops up first, selection won't happen)
        if (QApplication::activeModalWidget() != nullptr ||
            (QApplication::activePopupWidget() != nullptr &&
             qobject_cast<QMenu*>(QApplication::activePopupWidget()) == nullptr) ||
            QApplication::focusWidget() == m_componentPalette)
        {
            return false;
        }

        // if we were clicking on a menu (just has been opened) remove the event filter
        // as long as it is opened, otherwise clicking on the menu might result in a change
        // of selection before the action is fired
        if (QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget()))
        {
            qApp->removeEventFilter(this);
            connect(menu, &QMenu::aboutToHide, this, [this]() {
                qApp->installEventFilter(this);
            });
            return false; // also get out of here without eating this specific event.
        }

        const QRect globalRect(mouseEvent->globalPosition().toPoint(), mouseEvent->globalPosition().toPoint());

        //reject input outside of the inspector's component list
        if (!DoesOwnFocus() ||
            !DoesIntersectWidget(globalRect, m_gui->m_componentList))
        {
            return false;
        }

        //reject input from other buttons
        if ((mouseEvent->button() != Qt::LeftButton) &&
            (mouseEvent->button() != Qt::RightButton))
        {
            return false;
        }

        if (AzToolsFramework::ComponentModeFramework::InComponentMode())
        {
            AZ::s32 currentSelected = m_componentEditorLastSelectedIndex;
            // change selection here
            if (SelectIntersectingComponentEditorsSafe(globalRect))
            {
                // notify active component mode has changed
                bool changed = false;
                AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::BroadcastResult(
                    changed, &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::SelectActiveComponentMode,
                    m_componentEditors[m_componentEditorLastSelectedIndex]->GetComponentType());

                // deselect the component we were just on if selection changed
                if (    changed
                    &&  currentSelected >= 0
                    &&  currentSelected < m_componentEditors.size()
                    &&  currentSelected != m_componentEditorLastSelectedIndex)
                {
                    m_componentEditors[currentSelected]->SetSelected(false);
                }
            }
        }
        else
        {
            //right click is allowed if the component editor under the mouse is not selected
            if (mouseEvent->button() == Qt::RightButton)
            {
                if (DoesIntersectSelectedComponentEditor(globalRect))
                {
                    return false;
                }

                ClearComponentEditorSelection();
                SelectIntersectingComponentEditors(globalRect, true);
            }
            else if (mouseEvent->button() == Qt::LeftButton)
            {
                //if shift or control is pressed this is a multi=-select operation, otherwise reset the selection
                if (mouseEvent->modifiers() & Qt::ControlModifier)
                {
                    ToggleIntersectingComponentEditors(globalRect);
                }
                else if (mouseEvent->modifiers() & Qt::ShiftModifier)
                {
                    ComponentEditorVector intersections = GetIntersectingComponentEditors(globalRect);
                    if (!intersections.empty())
                    {
                        SelectRangeOfComponentEditors(
                            m_componentEditorLastSelectedIndex, GetComponentEditorIndex(intersections.front()), true);
                    }
                }
                else
                {
                    ClearComponentEditorSelection();
                    SelectIntersectingComponentEditors(globalRect, true);
                }
            }
        }

        UpdateInternalState();

        //ensure selection logic executes only once per click since eventFilter may execute multiple times for a single click
        m_selectionEventAccepted = true;
        return true;
    }


    QRect EntityPropertyEditor::GetInflatedRectFromPoint(const QPoint& point, int radius) const
    {
        return QRect(point - QPoint(radius, radius), point + QPoint(radius, radius));
    }

    bool EntityPropertyEditor::GetComponentsAtDropEventPosition(QDropEvent* event, AZ::Entity::ComponentArrayType& targetComponents)
    {
        const QPoint globalPos(mapToGlobal(event->position().toPoint()));

        //get component editor(s) where drop will occur
        ComponentEditor* targetComponentEditor = GetReorderDropTarget(
            GetInflatedRectFromPoint(globalPos, kComponentEditorDropTargetPrecision));
        if (targetComponentEditor)
        {
            targetComponents = targetComponentEditor->GetComponents();
            return true;
        }

        return false;
    }

    bool EntityPropertyEditor::IsDragAllowed(const ComponentEditorVector& componentEditors) const
    {
        if (!AllowAnyComponentModification())
        {
            return false;
        }

        if (m_dragStarted || m_selectedEntityIds.empty() || componentEditors.empty() || !DoesOwnFocus())
        {
            return false;
        }

        for (auto componentEditor : componentEditors)
        {
            if (!componentEditor ||
                !componentEditor->isVisible() ||
                !AreComponentsRemovable(componentEditor->GetComponents()) ||
                !AreComponentsDraggable(componentEditor->GetComponents()))
            {
                return false;
            }
        }

        return true;
    }

    // this helper function checks what can be spawned in mimedata and only calls the callback for the creatable ones.
    void EntityPropertyEditor::GetCreatableAssetEntriesFromMimeData(const QMimeData* mimeData, ProductCallback callbackFunction) const
    {
        if (mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            // the usual case - we only allow asset browser drops of assets that have actually been associated with a kind of component.
            AssetBrowser::AssetBrowserEntry::ForEachEntryInMimeData<AssetBrowser::ProductAssetBrowserEntry>(mimeData, [&](const AssetBrowser::ProductAssetBrowserEntry* product)
            {
                bool canCreateComponent = false;
                AZ::AssetTypeInfoBus::EventResult(canCreateComponent, product->GetAssetType(), &AZ::AssetTypeInfo::CanCreateComponent, product->GetAssetId());

                AZ::Uuid componentTypeId = AZ::Uuid::CreateNull();
                AZ::AssetTypeInfoBus::EventResult(componentTypeId, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);

                if (canCreateComponent && !componentTypeId.IsNull())
                {
                    callbackFunction(product);
                }
            });
        }
    }

    bool EntityPropertyEditor::IsDropAllowed(const QMimeData* mimeData, const QPoint& globalPos) const
    {
        if (m_isAlreadyQueuedRefresh)
        {
            return false; // can't drop while tree is rebuilding itself!
        }

        if (m_selectionContainsReadOnlyEntity)
        {
            return false;
        }

        const QRect globalRect(globalPos, globalPos);
        if (!mimeData || m_selectedEntityIds.empty() || !DoesIntersectWidget(globalRect, this))
        {
            return false;
        }

        if (mimeData->hasFormat(kComponentEditorIndexMimeType))
        {
            return IsDropAllowedForComponentReorder(mimeData, globalPos);
        }

        if (!mimeData->hasFormat(ComponentTypeMimeData::GetMimeType()) &&
            !mimeData->hasFormat(ComponentAssetMimeDataContainer::GetMimeType()) &&
            !mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            return false;
        }

        // In a majority of cases, only one of these will actually check anything and the others will just return true (default return value for the methods)
        // Regardless of how many mime types there are, if any mime data would result in an invalid type of component for this inspector type, we will disallow the entire drop.
        if (!CanDropForComponentTypes(mimeData) || !CanDropForComponentAssets(mimeData) || !CanDropForAssetBrowserEntries(mimeData))
        {
            return false;
        }

        if (mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            bool hasAny = false;
            GetCreatableAssetEntriesFromMimeData(mimeData, [&](const AssetBrowser::ProductAssetBrowserEntry* /*entry*/)
            {
                hasAny = true;
            });

            if (!hasAny)
            {
                return false;
            }
        }
        return true;
    }

    bool EntityPropertyEditor::IsDropAllowedForComponentReorder(const QMimeData* mimeData, const QPoint& globalPos) const
    {
        //drop requires a buffer for intersection tests due to spacing between editors
        const QRect globalRect(GetInflatedRectFromPoint(globalPos, kComponentEditorDropTargetPrecision));
        if (!mimeData || !mimeData->hasFormat(kComponentEditorIndexMimeType) || !DoesIntersectWidget(globalRect, this))
        {
            return false;
        }

        return true;
    }

    bool EntityPropertyEditor::CreateComponentWithAsset(const AZ::Uuid& componentType, const AZ::Data::AssetId& assetId, AZ::Entity::ComponentArrayType& createdComponents)
    {
        if (m_selectedEntityIds.empty() || componentType.IsNull())
        {
            return false;
        }

        EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome = AZ::Failure(AZStd::string("AddComponentsToEntities is not handled"));
        EntityCompositionRequestBus::BroadcastResult(addComponentsOutcome, &EntityCompositionRequests::AddComponentsToEntities, m_selectedEntityIds, AZ::ComponentTypeList({ componentType }));
        if (!addComponentsOutcome)
        {
            return false;
        }

        for (auto entityId : m_selectedEntityIds)
        {
            auto addedComponent = addComponentsOutcome.GetValue()[entityId].m_componentsAdded[0];
            AZ_Assert(GetComponentTypeId(addedComponent) == componentType, "Added component returned was not the type requested to add");

            createdComponents.push_back(addedComponent);

            auto editorComponent = GetEditorComponent(addedComponent);
            if (editorComponent)
            {
                editorComponent->SetPrimaryAsset(assetId);
            }
        }

        QueuePropertyRefresh();
        return true;
    }

    ComponentEditor* EntityPropertyEditor::GetReorderDropTarget(const QRect& globalRect) const
    {
        const ComponentEditorVector& targetComponentEditors = GetIntersectingComponentEditors(globalRect);
        ComponentEditor* targetComponentEditor = !targetComponentEditors.empty() ? targetComponentEditors.back() : nullptr;

        //continue to move to the next editor if the current target is being dragged, not removable, or we overlap its lower half
        auto targetItr = AZStd::find(m_componentEditors.begin(), m_componentEditors.end(), targetComponentEditor);
        while (targetComponentEditor
            && (targetComponentEditor->IsDragged()
                || !AreComponentsRemovable(targetComponentEditor->GetComponents())
                || !AreComponentsDraggable(targetComponentEditor->GetComponents())
                || (globalRect.center().y() > GetWidgetGlobalRect(targetComponentEditor).center().y())))
        {
            if (targetItr == m_componentEditors.end() || targetComponentEditor == m_componentEditors.back() || !targetComponentEditor->isVisible())
            {
                targetComponentEditor = nullptr;
                break;
            }

            targetComponentEditor = *(++targetItr);
        }
        return targetComponentEditor;
    }

    bool EntityPropertyEditor::ResetDrag(QMouseEvent* event)
    {
        const QPoint globalPos(event->globalPosition().toPoint());
        const QRect globalRect(globalPos, globalPos);

        //additional checks since handling is done in event filter
        m_dragStartPosition = globalPos;
        m_dragStarted = false;
        ClearComponentEditorDragging();
        ResetAutoScroll();
        return true;
    }

    bool EntityPropertyEditor::FindAllowedRowWidgetReorderDropTarget(const QPoint& globalPos)
    {
        const QRect globalRect(globalPos, globalPos);

        AZ_Assert(m_reorderRowWidgetEditor, "Missing editor for row widget drag.");

        IPropertyEditor* editor = m_reorderRowWidgetEditor->GetPropertyEditor();
        if (auto reflectedPropertyEditor = azdynamic_cast<ReflectedPropertyEditor*>(editor))
        {
            AzToolsFramework::ReflectedPropertyEditor::WidgetList widgets = reflectedPropertyEditor->GetWidgets();
            for (auto widgetPair : widgets)
            {
                PropertyRowWidget* widget = widgetPair.second;
                if (!widget)
                {
                    continue;
                }

                if (DoesIntersectWidget(globalRect, reinterpret_cast<QWidget*>(widget)))
                {
                    if (widget->CanBeReordered() && widget->GetParentRow() == m_reorderRowWidget->GetParentRow())
                    {
                        m_reorderDropTarget = widget;

                        QRect widgetRect = GetWidgetAndVisibleChildrenGlobalRect(widget);
                        if (globalPos.y() < widgetRect.center().y())
                        {
                            m_reorderDropArea = EntityPropertyEditor::DropArea::Above;
                        }
                        else
                        {
                            m_reorderDropArea = EntityPropertyEditor::DropArea::Below;
                        }

                        return true;
                    }

                    // We're hovering over a child of a reorderable ancestor, use the ancestor as the drop target.
                    PropertyRowWidget* parent = widget->GetParentRow();
                    while (parent)
                    {
                        if (parent->CanBeReordered() && parent->GetParentRow() == m_reorderRowWidget->GetParentRow())
                        {
                            m_reorderDropTarget = parent;

                            QRect widgetRect = GetWidgetAndVisibleChildrenGlobalRect(parent);
                            if (globalPos.y() < widgetRect.center().y())
                            {
                                m_reorderDropArea = EntityPropertyEditor::DropArea::Above;
                            }
                            else
                            {
                                m_reorderDropArea = EntityPropertyEditor::DropArea::Below;
                            }

                            return true;
                        }
                        parent = parent->GetParentRow();
                    }
                }
            }
        }

        return false;
    }

    bool EntityPropertyEditor::UpdateRowWidgetDrag(const QPoint& localPos, Qt::MouseButtons mouseButtons, const QMimeData* /*mimeData*/)
    {
        const QPoint globalPos(mapToGlobal(localPos));
        const QRect globalRect(globalPos, globalPos);

        if (!m_reorderRowWidget)
        {
            return false;
        }

        if (m_reorderDropTarget)
        {
            m_reorderDropTarget = nullptr;
        }

        UpdateOverlay();

        // additional checks since handling is done in event filter
        if ((mouseButtons & Qt::LeftButton) && DoesIntersectWidget(globalRect, this))
        {
            FindAllowedRowWidgetReorderDropTarget(globalPos);
            {
                UpdateOverlay();
                return true;
            }
        }
        return false;
    }

    bool EntityPropertyEditor::UpdateDrag(const QPoint& localPos, Qt::MouseButtons mouseButtons, const QMimeData* mimeData)
    {
        const QPoint globalPos(mapToGlobal(localPos));
        const QRect globalRect(globalPos, globalPos);

        if (m_reorderRowWidget)
        {
            UpdateRowWidgetDrag(localPos, mouseButtons, mimeData);
            QueueAutoScroll();
            UpdateOverlay();
            return true;
        }

        //reset drop indicators
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->SetDropTarget(false);
        }
        UpdateOverlay();

        //additional checks since handling is done in event filter
        if ((mouseButtons& Qt::LeftButton) && DoesIntersectWidget(globalRect, this))
        {
            if (IsDropAllowed(mimeData, globalPos))
            {
                QueueAutoScroll();

                //update drop indicators for detected drop targets
                if (IsDropAllowedForComponentReorder(mimeData, globalPos))
                {
                    ComponentEditor* targetComponentEditor = GetReorderDropTarget(
                        GetInflatedRectFromPoint(globalPos, kComponentEditorDropTargetPrecision));

                    if (targetComponentEditor)
                    {
                        targetComponentEditor->SetDropTarget(true);
                        UpdateOverlay();
                    }
                }

                return true;
            }
        }
        return false;
    }

    PropertyRowWidget* EntityPropertyEditor::FindPropertyRowWidgetAt(QPoint globalPos)
    {
        const QRect globalRect(globalPos, globalPos);

        const bool dragSelected = DoesIntersectSelectedComponentEditor(globalRect);
        const auto& componentEditors = dragSelected ? GetSelectedComponentEditors() : GetIntersectingComponentEditors(globalRect);

        for (auto componentEditor : componentEditors)
        {
            IPropertyEditor* editor = componentEditor->GetPropertyEditor();
            if (auto reflectedPropertyEditor = azdynamic_cast<ReflectedPropertyEditor*>(editor))
            {
                AzToolsFramework::ReflectedPropertyEditor::WidgetList widgets = reflectedPropertyEditor->GetWidgets();
                for (auto& [dataNode, rowWidget] : widgets)
                {
                    if (DoesIntersectWidget(globalRect, reinterpret_cast<QWidget*>(rowWidget)) && rowWidget->CanBeReordered())
                    {
                        return rowWidget;
                    }
                }
            }
        }

        return nullptr;
    }

    bool EntityPropertyEditor::StartDrag(QMouseEvent* event)
    {
        // do not initiate a drag if property editor is disabled
        if (m_disabled)
        {
            return false;
        }

        const QPoint globalPos(event->globalPosition().toPoint());
        const QRect globalRect(globalPos, globalPos);

        //additional checks since handling is done in event filter
        if (m_dragStarted || !(event->buttons() & Qt::LeftButton) || !DoesIntersectWidget(globalRect, this))
        {
            return false;
        }

        //determine if the mouse moved enough to register as a drag
        if ((globalPos - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
        {
            return false;
        }

        //if dragging a selected editor, drag all selected
        //otherwise, only drag the one under the cursor
        const QRect dragRect(m_dragStartPosition, m_dragStartPosition);
        const bool dragSelected = DoesIntersectSelectedComponentEditor(dragRect);
        const auto& componentEditors = dragSelected ? GetSelectedComponentEditors() : GetIntersectingComponentEditors(dragRect);
        if (!IsDragAllowed(componentEditors))
        {
            return false;
        }

        //drag must be initiated from a component header since property editor may have conflicting widgets
        bool intersectsHeader = false;
        for (auto componentEditor : componentEditors)
        {
            if (DoesIntersectWidget(dragRect, reinterpret_cast<QWidget*>(componentEditor->GetHeader())))
            {
                intersectsHeader = true;
            }
        }

        if (!intersectsHeader)
        {
            for (auto componentEditor : componentEditors)
            {
                IPropertyEditor* editor = componentEditor->GetPropertyEditor();
                if (auto reflectedPropertyEditor = azdynamic_cast<ReflectedPropertyEditor*>(editor))
                {
                    AzToolsFramework::ReflectedPropertyEditor::WidgetList widgets = reflectedPropertyEditor->GetWidgets();
                    for (AZStd::pair<InstanceDataNode*, PropertyRowWidget*> w : widgets)
                    {
                        if (w.second)
                        {
                            if (DoesIntersectWidget(dragRect, reinterpret_cast<QWidget*>(w.second)) && w.second->CanBeReordered())
                            {
                                m_currentReorderState = EntityPropertyEditor::ReorderState::DraggingRowWidget;
                                m_reorderRowWidget = w.second;
                                m_reorderRowWidgetEditor = componentEditor;
                                if (m_reorderDropTarget)
                                {
                                    m_reorderDropTarget = nullptr;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        m_dragStarted = true;

        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData();

        QRect dragImageRect;

        if (m_reorderRowWidget)
        {
            // We're dragging a PropertyRowWidget, grab the image from that.
            mimeData->setData(kComponentEditorRowWidgetType, QByteArray());

            drag->setMimeData(mimeData);
            drag->setPixmap(m_reorderRowWidget->createDragImage(
                QColor("#8E863E"), QColor("#EAECAA"), 0.5f, PropertyRowWidget::DragImageType::SingleRow));
            drag->setHotSpot(m_dragStartPosition - GetWidgetGlobalRect(m_reorderRowWidget).topLeft());
            drag->setDragCursor(m_dragIcon.pixmap(32), Qt::DropAction::MoveAction);
            // Ensure we can tidy up if the drop happens elsewhere.
            connect(drag, &QObject::destroyed, this, &EntityPropertyEditor::DragStopped);
            drag->exec(Qt::MoveAction, Qt::MoveAction);
        }
        else
        {
            AZStd::vector<AZ::s32> componentEditorIndices;
            componentEditorIndices.reserve(componentEditors.size());
            for (auto componentEditor : componentEditors)
            {
                // compute the drag image size
                if (componentEditorIndices.empty())
                {
                    dragImageRect = componentEditor->rect();
                }
                else
                {
                    dragImageRect.setHeight(dragImageRect.height() + componentEditor->rect().height());
                }

                // add component editor index to drag data
                auto componentEditorIndex = GetComponentEditorIndex(componentEditor);
                if (componentEditorIndex >= 0)
                {
                    componentEditorIndices.push_back(GetComponentEditorIndex(componentEditor));
                }
            }

            // build image from dragged editor UI
            QImage dragImage(dragImageRect.size(), QImage::Format_ARGB32_Premultiplied);
            QPainter painter(&dragImage);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(dragImageRect, Qt::transparent);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.setOpacity(0.5f);

            // render a vertical stack of component editors, may change to render just the headers
            QPoint dragImageOffset(0, 0);
            for (AZ::s32 index : componentEditorIndices)
            {
                auto componentEditor = GetComponentEditorsFromIndex(index);
                if (componentEditor)
                {
                    if (DoesIntersectWidget(dragRect, componentEditor))
                    {
                        // offset drag image from the drag start position
                        drag->setHotSpot(dragImageOffset + (m_dragStartPosition - GetWidgetGlobalRect(componentEditor).topLeft()));
                    }

                    // render the component editor to the drag image
                    componentEditor->render(&painter, dragImageOffset);

                    // update the render offset by the component editor height
                    dragImageOffset.setY(dragImageOffset.y() + componentEditor->rect().height());
                }
            }
            painter.end();

            // mark dragged components after drag initiated to draw indicators
            for (AZ::s32 index : componentEditorIndices)
            {
                auto componentEditor = GetComponentEditorsFromIndex(index);
                if (componentEditor)
                {
                    componentEditor->SetDragged(true);
                }
            }
            UpdateOverlay();

            // encode component editor indices as internal drag data
            mimeData->setData(
                kComponentEditorIndexMimeType,
                QByteArray(
                    reinterpret_cast<char*>(componentEditorIndices.data()),
                    static_cast<int>(componentEditorIndices.size() * sizeof(AZ::s32))));

            drag->setMimeData(mimeData);
            drag->setPixmap(QPixmap::fromImage(dragImage));
            drag->exec(Qt::MoveAction, Qt::MoveAction);

            // mark dragged components after drag completed to stop drawing indicators
            for (AZ::s32 index : componentEditorIndices)
            {
                auto componentEditor = GetComponentEditorsFromIndex(index);
                if (componentEditor)
                {
                    componentEditor->SetDragged(false);
                }
            }
        }

        UpdateOverlay();
        return true;
    }

    void EntityPropertyEditor::SetRowWidgetHighlighted(PropertyRowWidget* rowWidget)
    {
        m_reorderRowWidget = rowWidget;
        m_reorderRowImage = rowWidget->createDragImage(
            QColor("#8E863E"), QColor("#EAECAA"), 0.5f, PropertyRowWidget::DragImageType::IncludeVisibleChildren);
    }

    void EntityPropertyEditor::EndRowWidgetReorder()
    {
        m_reorderDropTarget = nullptr;
        m_reorderRowWidget = nullptr;
        m_reorderDropTarget = nullptr;
        m_currentReorderState = EntityPropertyEditor::ReorderState::Inactive;
        m_overlay->setVisible(false);
    }

    bool EntityPropertyEditor::HandleDrop(QDropEvent* event)
    {
        const QPoint globalPos(mapToGlobal(event->position().toPoint()));
        const QMimeData* mimeData = event->mimeData();

        if (m_currentReorderState == EntityPropertyEditor::ReorderState::DraggingRowWidget)
        {
            if (FindAllowedRowWidgetReorderDropTarget(globalPos))
            {
                if (m_reorderDropArea == EntityPropertyEditor::DropArea::Above)
                {
                    m_reorderRowWidgetEditor->GetPropertyEditor()->MoveNodeBefore(
                        m_reorderRowWidget->GetNode(), m_reorderDropTarget->GetNode());
                }
                else
                {
                    m_reorderRowWidgetEditor->GetPropertyEditor()->MoveNodeAfter(
                        m_reorderRowWidget->GetNode(), m_reorderDropTarget->GetNode());
                }
            }

            event->acceptProposedAction();

            EndRowWidgetReorder();
        }
        else
        {
            if (IsDropAllowed(mimeData, globalPos))
            {
                // handle drop for supported mime types
                HandleDropForComponentTypes(event);
                HandleDropForComponentAssets(event);
                HandleDropForAssetBrowserEntries(event);
                HandleDropForComponentReorder(event);
                event->acceptProposedAction();
                return true;
            }
        }

        return false;
    }

    bool EntityPropertyEditor::CanDropForComponentTypes(const QMimeData* mimeData) const
    {
        if (m_isLevelEntityEditor)
        {
            if (mimeData && mimeData->hasFormat(ComponentTypeMimeData::GetMimeType()))
            {
                ComponentTypeMimeData::ClassDataContainer classDataContainer;
                ComponentTypeMimeData::Get(mimeData, classDataContainer);

                for (auto componentClass : classDataContainer)
                {
                    if (componentClass)
                    {
                        if (!AppearsInLevelComponentMenu(*componentClass))
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool EntityPropertyEditor::HandleDropForComponentTypes(QDropEvent* event)
    {
        const QMimeData* mimeData = event->mimeData();
        if (mimeData && mimeData->hasFormat(ComponentTypeMimeData::GetMimeType()))
        {
            ComponentTypeMimeData::ClassDataContainer classDataContainer;
            ComponentTypeMimeData::Get(mimeData, classDataContainer);

            ScopedUndoBatch undo("Add component with type");

            AZ::Entity::ComponentArrayType targetComponents = AZ::Entity::ComponentArrayType();
            GetComponentsAtDropEventPosition(event, targetComponents);

            AZ::Entity::ComponentArrayType newComponents;
            //create new components from dropped component types
            for (auto componentClass : classDataContainer)
            {
                if (componentClass)
                {
                    CreateComponentWithAsset(componentClass->m_typeId, AZ::Data::AssetId(), newComponents);
                }
            }

            MoveComponentRowBefore(newComponents, targetComponents, undo);

            return true;
        }
        return false;
    }

    bool EntityPropertyEditor::CanDropForComponentAssets(const QMimeData* mimeData) const
    {
        if (m_isLevelEntityEditor)
        {
            if (mimeData && mimeData->hasFormat(ComponentAssetMimeDataContainer::GetMimeType()))
            {
                ComponentAssetMimeDataContainer mimeContainer;
                if (mimeContainer.FromMimeData(mimeData))
                {
                    for (const auto& asset : mimeContainer.m_assets)
                    {
                        auto componentClassData = GetComponentClassDataForType(asset.m_classId);
                        if (!AppearsInLevelComponentMenu(*componentClassData))
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool EntityPropertyEditor::HandleDropForComponentAssets(QDropEvent* event)
    {
        const QMimeData* mimeData = event->mimeData();
        if (mimeData && mimeData->hasFormat(ComponentAssetMimeDataContainer::GetMimeType()))
        {
            ComponentAssetMimeDataContainer mimeContainer;
            if (mimeContainer.FromMimeData(event->mimeData()))
            {
                AZ::Entity::ComponentArrayType targetComponents = AZ::Entity::ComponentArrayType();
                GetComponentsAtDropEventPosition(event, targetComponents);

                AZ::Entity::ComponentArrayType newComponents;

                ScopedUndoBatch undo("Add component with asset");

                //create new components from dropped assets
                for (const auto& asset : mimeContainer.m_assets)
                {
                    CreateComponentWithAsset(asset.m_classId, asset.m_assetId, newComponents);
                }
                MoveComponentRowBefore(newComponents, targetComponents, undo);
            }
            return true;
        }
        return false;
    }

    bool EntityPropertyEditor::CanDropForAssetBrowserEntries(const QMimeData* mimeData) const
    {
        bool canDrop = true;

        if (m_isLevelEntityEditor)
        {
            if (mimeData && mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
            {
                GetCreatableAssetEntriesFromMimeData(mimeData,
                    [&](const AssetBrowser::ProductAssetBrowserEntry* product)
                {
                    AZ::Uuid componentType = AZ::Uuid::CreateNull();
                    AZ::AssetTypeInfoBus::EventResult(componentType, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);
                    auto componentClassData = GetComponentClassDataForType(componentType);

                    canDrop &= AppearsInLevelComponentMenu(*componentClassData);
                });
            }
        }

        return canDrop;
    }

    bool EntityPropertyEditor::HandleDropForAssetBrowserEntries(QDropEvent* event)
    {
        // do we need to create an undo?
        bool createdAny = false;
        const QMimeData* mimeData = event->mimeData();
        if ((!mimeData) || (!mimeData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType())))
        {
            return false;
        }

        // before we start, find out whether any in the mimedata are even appropriate so we can execute an undo
        // note that the product pointers are only valid during this callback, so there is no point in caching them here.
        GetCreatableAssetEntriesFromMimeData(mimeData,
            [&](const AssetBrowser::ProductAssetBrowserEntry* /*product*/)
            {
                createdAny = true;
            });

        if (createdAny)
        {
            AZ::Entity::ComponentArrayType targetComponents = AZ::Entity::ComponentArrayType();
            GetComponentsAtDropEventPosition(event, targetComponents);

            // create one undo to wrap the entire operation since we detected a valid asset.
            ScopedUndoBatch undo("Add component from asset browser");
            GetCreatableAssetEntriesFromMimeData(mimeData,
                [&](const AssetBrowser::ProductAssetBrowserEntry* product)
                {
                    AZ::Entity::ComponentArrayType newComponents;
                    AZ::Uuid componentType;
                    AZ::AssetTypeInfoBus::EventResult(componentType, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);
                    CreateComponentWithAsset(componentType, product->GetAssetId(), newComponents);
                    MoveComponentRowBefore(newComponents, targetComponents, undo);
                });

            return true;
        }

        return false;
    }

    bool EntityPropertyEditor::HandleDropForComponentReorder(QDropEvent* event)
    {
        const QMimeData* mimeData = event->mimeData();
        if (mimeData && mimeData->hasFormat(kComponentEditorIndexMimeType))
        {
            ScopedUndoBatch undo("Reorder components");

            //get component editor(s) to drop on targets, moving the associated components above the target
            const ComponentEditorVector& sourceComponentEditors = GetComponentEditorsFromIndices(ExtractComponentEditorIndicesFromMimeData(mimeData));

            AZ::Entity::ComponentArrayType targetComponents = AZ::Entity::ComponentArrayType();
            GetComponentsAtDropEventPosition(event, targetComponents);

            for (const ComponentEditor* sourceComponentEditor : sourceComponentEditors)
            {
                AZStd::span<AZ::Component* const> sourceComponents = sourceComponentEditor->GetComponents();
                MoveComponentRowBefore(sourceComponents, targetComponents, undo);
            }

            QueuePropertyRefresh();
            return true;
        }
        return false;
    }

    AZStd::vector<AZ::s32> EntityPropertyEditor::ExtractComponentEditorIndicesFromMimeData(const QMimeData* mimeData) const
    {
        //build vector of indices from mime data
        const QByteArray sourceComponentEditorData = mimeData->data(kComponentEditorIndexMimeType);
        const auto indexCount = sourceComponentEditorData.size() / sizeof(AZ::s32);
        const auto indexStart = reinterpret_cast<const AZ::s32*>(sourceComponentEditorData.data());
        const auto indexEnd = indexStart + indexCount;
        return AZStd::vector<AZ::s32>(indexStart, indexEnd);
    }

    ComponentEditorVector EntityPropertyEditor::GetComponentEditorsFromIndices(const AZStd::vector<AZ::s32>& indices) const
    {
        ComponentEditorVector componentEditors;
        componentEditors.reserve(indices.size());
        for (auto index : indices)
        {
            auto componentEditor = GetComponentEditorsFromIndex(index);
            if (componentEditor)
            {
                componentEditors.push_back(componentEditor);
            }
        }
        return componentEditors;
    }

    ComponentEditor* EntityPropertyEditor::GetComponentEditorsFromIndex(const AZ::s32 index) const
    {
        return index >= 0 && index < m_componentEditors.size() ? m_componentEditors[index] : nullptr;
    }

    void EntityPropertyEditor::ResetAutoScroll()
    {
        m_autoScrollCount = 0;
        m_autoScrollQueued = false;
    }

    void EntityPropertyEditor::QueueAutoScroll()
    {
        if (!m_autoScrollQueued)
        {
            m_autoScrollQueued = true;
            QTimer::singleShot(0, this, &EntityPropertyEditor::UpdateAutoScroll);
        }
    }

    void EntityPropertyEditor::UpdateAutoScroll()
    {
        if (m_autoScrollQueued)
        {
            m_autoScrollQueued = false;

            // find how much we should scroll with
            QScrollBar* verticalScroll = m_gui->m_componentList->verticalScrollBar();
            QScrollBar* horizontalScroll = m_gui->m_componentList->horizontalScrollBar();

            int verticalStep = verticalScroll->pageStep();
            int horizontalStep = horizontalScroll->pageStep();
            if (m_autoScrollCount < qMax(verticalStep, horizontalStep))
            {
                ++m_autoScrollCount;
            }

            int verticalValue = verticalScroll->value();
            int horizontalValue = horizontalScroll->value();

            QPoint pos = QCursor::pos();
            QRect area = GetWidgetGlobalRect(m_gui->m_componentList);

            // do the scrolling if we are in the scroll margins
            if (pos.y() - area.top() < m_autoScrollMargin)
            {
                verticalScroll->setValue(verticalValue - m_autoScrollCount);
            }
            else if (area.bottom() - pos.y() < m_autoScrollMargin)
            {
                verticalScroll->setValue(verticalValue + m_autoScrollCount);
            }
            if (pos.x() - area.left() < m_autoScrollMargin)
            {
                horizontalScroll->setValue(horizontalValue - m_autoScrollCount);
            }
            else if (area.right() - pos.x() < m_autoScrollMargin)
            {
                horizontalScroll->setValue(horizontalValue + m_autoScrollCount);
            }

            // if nothing changed, stop scrolling
            if (verticalValue != verticalScroll->value() || horizontalValue != horizontalScroll->value())
            {
                UpdateOverlay();
                QueueAutoScroll();
            }
            else
            {
                ResetAutoScroll();
            }
        }
    }

    void EntityPropertyEditor::UpdateInternalState()
    {
        UpdateSelectionCache();
        UpdateActions();
        UpdateOverlay();
    }

    void EntityPropertyEditor::OnSearchTextChanged()
    {
        m_filterString = m_gui->m_entitySearchBox->text().toLatin1().data();

        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->SetFilterString(m_filterString);
        }

        UpdateContents();
    }

    void EntityPropertyEditor::ClearSearchFilter()
    {
        m_gui->m_entitySearchBox->setText("");
    }

    void EntityPropertyEditor::OpenPinnedInspector()
    {
        AzToolsFramework::EntityIdList selectedEntities;
        GetSelectedEntities(selectedEntities);

        AzToolsFramework::EntityIdSet pinnedEntities(selectedEntities.begin(), selectedEntities.end());
        AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::OpenPinnedInspector, pinnedEntities);
    }

    void EntityPropertyEditor::OnCollapseAll()
    {
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->SetExpanded(false);
        }
    }

    void EntityPropertyEditor::OnPrepareForContextReset()
    {
        if (IsLockedToSpecificEntities() && !m_isLevelEntityEditor)
        {
            CloseInspectorWindow();
        }
    }

    void EntityPropertyEditor::OnContextReset()
    {
        ClearInstances(true);
    }

    void EntityPropertyEditor::CloseInspectorWindow()
    {
        for (auto& entityId : m_overrideSelectedEntityIds)
        {
            DisconnectFromEntityBuses(entityId);
        }
        AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::Bus::Events::ClosePinnedInspector, this);
    }

    void EntityPropertyEditor::SetSystemEntityEditor(bool isSystemEntityEditor)
    {
        m_isSystemEntityEditor = isSystemEntityEditor;
        UpdateContents();
    }

    void EntityPropertyEditor::OnEntityComponentPropertyChanged(AZ::ComponentId componentId)
    {
        // When m_initiatingPropertyChangeNotification is true, it means this EntityPropertyEditor was
        // the one that is broadcasting this property change, so no need to refresh our values
        if (m_initiatingPropertyChangeNotification)
        {
            return;
        }

        for (auto componentEditor : m_componentEditors)
        {
            if (componentEditor->isVisible() && componentEditor->HasComponentWithId(componentId))
            {
                componentEditor->QueuePropertyEditorInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values);
                break;
            }
        }
    }

    void EntityPropertyEditor::OnComponentOrderChanged()
    {
        if (m_isBuildingProperties)
        {
            return;
        }

        QueuePropertyRefresh();
        m_shouldScrollToNewComponents = true;
    }

    void EntityPropertyEditor::ConnectToEntityBuses(const AZ::EntityId& entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AzToolsFramework::EditorInspectorComponentNotificationBus::MultiHandler::BusConnect(entityId);
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(entityId);
        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            editorEntityContextId, &EditorEntityContextRequestBus::Events::GetEditorEntityContextId);
        AzToolsFramework::FocusModeNotificationBus::Handler::BusConnect(editorEntityContextId);
    }

    void EntityPropertyEditor::DisconnectFromEntityBuses(const AZ::EntityId& entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AzToolsFramework::FocusModeNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorInspectorComponentNotificationBus::MultiHandler::BusDisconnect(entityId);
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(entityId);
    }

    void EntityPropertyEditor::OnEditorFocusChanged(
        [[maybe_unused]] AZ::EntityId previousFocusEntityId, [[maybe_unused]] AZ::EntityId newFocusEntityId)
    {
        // We need to wait for the Container Entity System to update before checking
        QTimer::singleShot( 1, this, [this]()
            {
                bool enable = true;
                for (AZ::EntityId entityId : m_overrideSelectedEntityIds)
                {
                    if (!m_focusModeInterface->IsInFocusSubTree(entityId) ||
                        m_containerEntityInterface->IsUnderClosedContainerEntity(entityId))
                    {
                        enable = false;
                        break;
                    }
                }
                SetEditorUiEnabled(enable);
            });
    }

    static void SetPropertyEditorState(Ui::EntityPropertyEditorUI* propertyEditorUi, const bool on)
    {
        // enable/disable all widgets relating to the entity inspector that should
        // be deactivated in ComponentMode
        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_entityIcon, on);
        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_entityNameLabel, on);
        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_entityNameEditor, on);
        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_pinButton, on);

        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_statusLabel, on);
        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_statusComboBox, on);

        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_entityIdLabel, on);
        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_entityIdText, on);
        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_prefabContainerText, on);

        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_addComponentButton, on);

        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_entitySearchBox, on);
    }

    AZ::Entity* EntityPropertyEditor::GetSelectedEntityById(AZ::EntityId& entityId) const
    {
        return GetEntityById(entityId);
    }

    // add/remove (enable/disable) all component related actions
    // that can be performed on an entity from the inspector
    static void EnableDisableComponentActions(
        QWidget* widget, const QVector<QAction*>& actions, const bool enable)
    {
        if (enable)
        {
            for (QAction* action : actions)
            {
                widget->addAction(action);
            }
        }
        else
        {
            for (QAction* action : actions)
            {
                widget->removeAction(action);
            }
        }
    }

    static void EnableComponentActions(
        QWidget* widget, const QVector<QAction*>& actions)
    {
        EnableDisableComponentActions(widget, actions, true);
    }

    static void DisableComponentActions(
        QWidget* widget, const QVector<QAction*>& actions)
    {
        EnableDisableComponentActions(widget, actions, false);
    }

    void EntityPropertyEditor::SetEditorUiEnabled(bool enable)
    {
        if (!m_selectedEntityIds.empty())
        {
            if (enable)
            {
                EnableComponentActions(this, m_entityComponentActions);
            }
            else
            {
                DisableComponentActions(this, m_entityComponentActions);
            }
            SetPropertyEditorState(m_gui, enable);

            for (auto componentEditor : m_componentEditors)
            {
                AzQtComponents::SetWidgetInteractEnabled(componentEditor, enable);
            }
        }
        else
        {
            AzQtComponents::SetWidgetInteractEnabled(m_gui->m_entityDetailsLabel, enable);
        }
        m_disabled = !enable;

        // record the selected state after entering/leaving component mode
        SaveComponentEditorState();
    }

    void EntityPropertyEditor::OnReadOnlyEntityStatusChanged(const AZ::EntityId& entityId, [[maybe_unused]] bool readOnly)
    {
        if (IsEntitySelected(entityId))
        {
            UpdateContents();
        }
    }

    void EntityPropertyEditor::OnEditorModeActivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Component)
        {
            DisableComponentActions(this, m_entityComponentActions);
            SetPropertyEditorState(m_gui, false);
            m_disabled = true;

            // note: ComponentModeCollectionInterface cannot be cached as it may change during the lifetime of the application
            const auto componentModeTypes = AZ::Interface<ComponentModeCollectionInterface>::Get()->GetComponentTypes();


            if (!componentModeTypes.empty())
            {
                m_componentEditorLastSelectedIndex = GetComponentEditorIndexFromType(componentModeTypes.front());
            }

            for (auto componentEditor : m_componentEditors)
            {
                componentEditor->EnteredComponentMode(componentModeTypes);

                // if this component editor is active and editable during component mode
                if (componentEditor->IsSelected())
                {
                    // scroll to the relevant component card
                    m_gui->m_componentList->ensureWidgetVisible(componentEditor);
                }
            }

            // record the selected state after entering component mode
            SaveComponentEditorState();
        }
    }

    void EntityPropertyEditor::OnEditorModeDeactivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Component)
        {
            EnableComponentActions(this, m_entityComponentActions);
            SetPropertyEditorState(m_gui, true);
            const auto componentModeTypes = AZ::Interface<ComponentModeCollectionInterface>::Get()->GetComponentTypes();
            m_disabled = false;

            for (auto componentEditor : m_componentEditors)
            {
                componentEditor->LeftComponentMode(componentModeTypes);
            }

            // record the selected state after leaving component mode
            SaveComponentEditorState();
        }
    }

    void EntityPropertyEditor::ActiveComponentModeChanged(const AZ::Uuid& componentType)
    {
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->ActiveComponentModeChanged(componentType);

            // if this component editor has been changed to during component mode
            if (componentEditor->IsSelected())
            {
                // scroll to the relevant component card
                m_gui->m_componentList->ensureWidgetVisible(componentEditor);
            }
        }
    }

    EntityPropertyEditor::ReorderState EntityPropertyEditor::GetReorderState() const
    {
        return m_currentReorderState;
    }

    ComponentEditor* EntityPropertyEditor::GetEditorForCurrentReorderRowWidget() const
    {
        return m_reorderRowWidgetEditor;
    }

    PropertyRowWidget* EntityPropertyEditor::GetReorderRowWidget() const
    {
        return m_reorderRowWidget;
    }

    PropertyRowWidget* EntityPropertyEditor::GetReorderDropTarget() const
    {
        return m_reorderDropTarget;
    }

    EntityPropertyEditor::DropArea EntityPropertyEditor::GetReorderDropArea() const
    {
        return m_reorderDropArea;
    }

    QPixmap EntityPropertyEditor::GetReorderRowWidgetImage() const
    {
        return m_reorderRowImage;
    }

    float EntityPropertyEditor::GetMoveIndicatorAlpha() const
    {
        if (m_currentReorderState != ReorderState::MenuOperationInProgress)
        {
            return 1.0f;
        }

        return m_moveFadeSecondsRemaining / MoveFadeSeconds;
    }

    PropertyRowWidget* EntityPropertyEditor::GetRowToHighlight()
    {
        // Use the pregenerated map to find the RowWidget that's in the new position.
        QSet<PropertyRowWidget*> rowWidgets = m_reorderRowWidgetEditor->GetPropertyEditor()->GetTopLevelWidgets();
        if (rowWidgets.isEmpty())
        {
            return nullptr;
        }

        PropertyRowWidget* highlightRow = *rowWidgets.begin();

        int mapIndex = static_cast<int>(m_indexMapOfMovedRow.size() - 1);

        while (mapIndex >= 0)
        {
            int mapEntry = m_indexMapOfMovedRow[mapIndex];
            highlightRow = highlightRow->GetChildrenRows()[mapEntry];
            mapIndex--;
        }

        return highlightRow;
    }
}

StatusComboBox::StatusComboBox(QWidget* parent)
    : QComboBox(parent)
{
}

void StatusComboBox::paintEvent(QPaintEvent* event)
{
    if (m_headerOverride.size())
    {
        QStylePainter painter(this);

        QStyleOptionComboBox opt;
        this->initStyleOption(&opt);

        opt.currentText = m_headerOverride;

        painter.drawComplexControl(QStyle::CC_ComboBox, opt);

        if (m_italic)
        {
            QFont fnt = painter.font();
            fnt.setItalic(true);
            painter.setFont(fnt);
        }
        painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
    }
    else
    {
        QComboBox::paintEvent(event);
    }
}

void StatusComboBox::setHeaderOverride(QString overrideString)
{
    m_headerOverride = overrideString;
}

void StatusComboBox::setItalic(bool italic)
{
    m_italic = italic;
}

void StatusComboBox::showPopup()
{
    QComboBox::showPopup();
}

// We need to prevent the comboBox reacting to the mouse wheel
void StatusComboBox::wheelEvent(QWheelEvent* e)
{
    if (hasFocus())
    {
        QComboBox::wheelEvent(e);
    }
}


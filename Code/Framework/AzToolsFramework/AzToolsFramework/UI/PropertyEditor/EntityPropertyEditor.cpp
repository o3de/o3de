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
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/sort.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/DragAndDrop.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzToolsFramework/API/ComponentModeCollectionInterface.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityRuntimeActivationBus.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Slice/SliceDataFlagsCommand.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
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
#include <AzToolsFramework/ToolsComponents/EditorLayerComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteUtil.hxx>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditorHeader.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzQtComponents/Utilities/QtViewPaneEffects.h>
#include <AzQtComponents/Components/Widgets/ComboBox.h>

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
}

namespace AzToolsFramework
{
    constexpr const char*  kComponentEditorIndexMimeType = "editor/componentEditorIndices";
    constexpr const char*  kComponentEditorRowWidgetType = "editor/componentEditorRowWidget";

    constexpr const char* kPropertyEditorMenuActionMoveUp("editor/propertyEditorMoveUp");
    constexpr const char* kPropertyEditorMenuActionMoveDown("editor/propertyEditorMoveDown");

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

                for (auto& [dataNode, rowWidget] : componentEditor->GetPropertyEditor()->GetWidgets())
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
                        opt.init(this);
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
                        opt.init(this);
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
                lineOpt.init(this);
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
                    opt.init(this);
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
                    opt.init(this);
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
                opt.init(this);
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

                    QWidget* newWidget = m_editor->childAt(m_editor->mapFromGlobal(originalMouseEvent->globalPos()));

                    if ((newWidget != this) && (newWidget != nullptr))
                    {
                        QPoint newLocal = newWidget->mapFromGlobal(originalMouseEvent->globalPos());
                        QMouseEvent newMouseEvent(
                            ev->type(),
                            newLocal,
                            originalMouseEvent->windowPos(),
                            originalMouseEvent->screenPos(),
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

    EntityPropertyEditor::SharedComponentInfo::SharedComponentInfo(AZ::Component* component, AZ::Component* sliceReferenceComponent)
        : m_sliceReferenceComponent(sliceReferenceComponent)
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
    {

        initEntityPropertyEditorResources();

        m_componentModeCollection = AZ::Interface<ComponentModeCollectionInterface>::Get();
        AZ_Assert(m_componentModeCollection, "Could not retrieve component mode collection.");

        m_prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
        AZ_Assert(m_prefabPublicInterface != nullptr, "EntityPropertyEditor requires a PrefabPublicInterface instance on Initialize.");

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
        m_gui->m_entityNameEditor->setReadOnly(false);
        m_gui->m_entityDetailsLabel->setObjectName("LabelEntityDetails");
        m_gui->m_entitySearchBox->setReadOnly(false);
        m_gui->m_entitySearchBox->setContextMenuPolicy(Qt::CustomContextMenu);
        m_gui->m_entitySearchBox->setClearButtonEnabled(true);
        AzQtComponents::LineEdit::applySearchStyle(m_gui->m_entitySearchBox);

        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            m_prefabsAreEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

        m_itemNames =
            m_prefabsAreEnabled ? QStringList{"Universal", "Editor only"} : QStringList{"Start active", "Start inactive", "Editor only"};
        int itemNameCount = m_itemNames.size();
        QStandardItemModel* model = new QStandardItemModel(itemNameCount, 1);
        for (int row = 0; row < itemNameCount; ++row)
        {
            m_comboItems[row] = new QStandardItem(m_itemNames[row]);
            m_comboItems[row]->setCheckable(true);
            model->setItem(row, 0, m_comboItems[row]);
        }
        m_gui->m_statusComboBox->setModel(model);
        m_gui->m_statusComboBox->setStyleSheet("QComboBox {border: 0px; border-radius:3px; background-color:#555555; color:white}"
            "QComboBox:on {background-color:#e9e9e9; color:black; border:0px}"
            "QComboBox::down-arrow:on {image: url(:/stylesheet/img/dropdowns/black_down_arrow.png)}"
            "QComboBox::drop-down {border-radius: 3p}");
        AzQtComponents::ComboBox::addCustomCheckStateStyle(m_gui->m_statusComboBox);
        EnableEditor(true);
        m_sceneIsNew = true;

        connect(m_gui->m_entityIcon, &QPushButton::clicked, this, &EntityPropertyEditor::BuildEntityIconMenu);
        connect(m_gui->m_addComponentButton, &QPushButton::clicked, this, &EntityPropertyEditor::OnAddComponent);
        connect(m_gui->m_entitySearchBox, &QLineEdit::textChanged, this, &EntityPropertyEditor::OnSearchTextChanged);
        connect(m_gui->m_entitySearchBox, &QWidget::customContextMenuRequested, this, &EntityPropertyEditor::OnSearchContextMenu);
        connect(m_gui->m_pinButton, &QToolButton::clicked, this, &EntityPropertyEditor::OpenPinnedInspector);

        m_componentPalette = new ComponentPaletteWidget(this, true);
        connect(m_componentPalette, &ComponentPaletteWidget::OnAddComponentEnd, this, [this]()
        {
            QueuePropertyRefresh();
            m_shouldScrollToNewComponents = true;
        });

        HideComponentPalette();

        m_overlay = new EntityPropertyEditorOverlay(this, m_gui->m_componentListContents);
        UpdateOverlay();

        ToolsApplicationEvents::Bus::Handler::BusConnect();
        AZ::EntitySystemBus::Handler::BusConnect();
        EntityPropertyEditorRequestBus::Handler::BusConnect();
        EditorWindowUIRequestBus::Handler::BusConnect();
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

        AzToolsFramework::ComponentModeFramework::EditorComponentModeNotificationBus::Handler::BusConnect(
            AzToolsFramework::GetEntityContextId());
        ViewportEditorModeNotificationsBus::Handler::BusConnect(GetEntityContextId());
    }

    EntityPropertyEditor::~EntityPropertyEditor()
    {
        qApp->removeEventFilter(this);

        EditorWindowUIRequestBus::Handler::BusDisconnect();
        EntityPropertyEditorRequestBus::Handler::BusDisconnect();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        AZ::EntitySystemBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ComponentModeFramework::EditorComponentModeNotificationBus::Handler::BusDisconnect();
        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        
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

    void EntityPropertyEditor::SetNewComponentId(AZ::ComponentId componentId)
    {
        m_newComponentId = componentId;
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
            UpdateContents(); // immediately refresh
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
        bool isLayerEntity = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isLayerEntity,
            entityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
        if (isLayerEntity)
        {
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
                entityId,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetOverwriteFlag, true);
        }
    }

    void EntityPropertyEditor::OnEntityStartStatusChanged(const AZ::EntityId& entityId)
    {
        if (IsEntitySelected(entityId))
        {
            UpdateStatusComboBox();
        }
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

        m_gui->m_entityIcon->setIcon(QIcon(iconSourcePath.c_str()));
        m_gui->m_entityIcon->repaint();
    }

    EntityPropertyEditor::InspectorLayout EntityPropertyEditor::GetCurrentInspectorLayout() const
    {
        if (!m_prefabsAreEnabled)
        {
            return m_isLevelEntityEditor ? InspectorLayout::Level : InspectorLayout::Entity;
        }

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
                        return InspectorLayout::Invalid;
                    }
                    else
                    {
                        return InspectorLayout::ContainerEntityOfFocusedPrefab;
                    }
                }
            }
        }

        return InspectorLayout::Entity;
    }

    void EntityPropertyEditor::UpdateEntityDisplay()
    {
        UpdateStatusComboBox();

        InspectorLayout layout = GetCurrentInspectorLayout();

        if (!m_prefabsAreEnabled && layout == InspectorLayout::Level)
        {
            AZStd::string levelName;
            AzToolsFramework::EditorRequestBus::BroadcastResult(levelName, &AzToolsFramework::EditorRequests::GetLevelName);
            m_gui->m_entityNameEditor->setText(levelName.c_str());
            m_gui->m_entityNameEditor->setReadOnly(true);
        }
        else if (m_selectedEntityIds.size() > 1)
        {
            // Generic text for multiple entities selected
            m_gui->m_entityDetailsLabel->setVisible(true);
            m_gui->m_entityDetailsLabel->setText(tr("Common components shown"));
            m_gui->m_entityNameEditor->setText(tr("%n entities selected", "", static_cast<int>(m_selectedEntityIds.size())));
            m_gui->m_entityNameEditor->setReadOnly(true);

            m_gui->m_entityIdText->setText(tr("multiple selected"));
        }
        else if (!m_selectedEntityIds.empty())
        {
            auto entityId = m_selectedEntityIds.front();

            // No entity details for one entity
            m_gui->m_entityDetailsLabel->setVisible(false);

            // If we're in edit mode, make the name field editable.
            m_gui->m_entityNameEditor->setReadOnly(!m_gui->m_componentListContents->isEnabled());

            // get the name of the entity.
            auto entity = GetSelectedEntityById(entityId);
            m_gui->m_entityNameEditor->setText(entity ? entity->GetName().data() : "Entity Not Found");

            m_gui->m_entityIdText->setText(QString::number(static_cast<AZ::u64>(entityId)));
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

        for (AZ::EntityId selectedEntityId : selection)
        {
            bool isLayerEntity = false;
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                isLayerEntity,
                selectedEntityId,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

            bool isPrefabEntity = (m_prefabPublicInterface) ? m_prefabPublicInterface->IsInstanceContainerEntity(selectedEntityId) : false;

            if (isLayerEntity)
            {
                if (result == SelectionEntityTypeInfo::None)
                {
                    result = SelectionEntityTypeInfo::OnlyLayerEntities;
                }
                else if (result == SelectionEntityTypeInfo::OnlyStandardEntities)
                {
                    result = SelectionEntityTypeInfo::Mixed;
                    // An entity of both layer and non-layer type have been found, so break out of the loop.
                    break;
                }
            }
            else if (isPrefabEntity)
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
        if (selectionEntityTypeInfo == SelectionEntityTypeInfo::Mixed ||
            selectionEntityTypeInfo == SelectionEntityTypeInfo::None)
        {
            // Can't add components in mixed selection, or if nothing is selected.
            return false;
        }

        // This follows the pattern in OnAddComponent, which also uses an empty filter.
        AZStd::vector<AZ::ComponentServiceType> serviceFilter;
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
        setUpdatesEnabled(false);

        m_isBuildingProperties = true;
        m_sliceCompareToEntity.reset();

        // Wait to clear this until after the reset(), just in case any components try to call RefreshTree when we clear
        // the m_sliceCompareToEntity entity.
        m_isAlreadyQueuedRefresh = false;

        HideComponentPalette();

        ClearInstances(false);
        ClearComponentEditorDragging();

        m_selectedEntityIds.clear();
        GetSelectedEntities(m_selectedEntityIds);

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
        else if (selectionEntityTypeInfo == SelectionEntityTypeInfo::OnlyLayerEntities || selectionEntityTypeInfo == SelectionEntityTypeInfo::OnlyPrefabEntities)
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
                m_componentFilter = AZStd::move(GetDefaultComponentFilter());
            }
        }
        else if (selectionEntityTypeInfo == SelectionEntityTypeInfo::LevelEntity)
        {
            if (!m_customFilterSet)
            {
                m_componentFilter = AZStd::move(AppearsInLevelComponentMenu);
            }
        }

        bool isLevelLayout = GetCurrentInspectorLayout() == InspectorLayout::Level;
        bool isContainerOfFocusedPrefabLayout = GetCurrentInspectorLayout() == InspectorLayout::ContainerEntityOfFocusedPrefab;

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
            // Build up components to display
            SharedComponentArray sharedComponentArray;
            BuildSharedComponentArray(sharedComponentArray,
                !(selectionEntityTypeInfo == SelectionEntityTypeInfo::OnlyStandardEntities ||
                  selectionEntityTypeInfo == SelectionEntityTypeInfo::OnlyPrefabEntities) ||
                  selectionEntityTypeInfo == SelectionEntityTypeInfo::ContainerEntityOfFocusedPrefab);

            if (sharedComponentArray.size() == 0)
            {
                // Don't display the search box if there were no common components.
                displayComponentSearchBox = false;
            }
            else
            {
                BuildSharedComponentUI(sharedComponentArray);
            }

            UpdateEntityIcon();
            UpdateEntityDisplay();
        }

        m_gui->m_darkBox->setVisible(
            displayComponentSearchBox && !m_isSystemEntityEditor && !isLevelLayout && !isContainerOfFocusedPrefabLayout);
        m_gui->m_entitySearchBox->setVisible(displayComponentSearchBox);

        bool displayAddComponentMenu = CanAddComponentsToSelection(selectionEntityTypeInfo);
        m_gui->m_addComponentButton->setVisible(displayAddComponentMenu);

        QueueScrollToNewComponent();
        LoadComponentEditorState();
        UpdateInternalState();

        layout()->update();
        layout()->activate();

        m_isBuildingProperties = false;

        setUpdatesEnabled(true);

        // re-enable all actions so that things like "delete" or "cut" work again
        QList<QAction*> actionList = actions();
        for (QAction* action : actionList)
        {
            action->setEnabled(true);
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
            SaveComponentOrder(entityId, componentsOnEntity);
        }
    }

    void EntityPropertyEditor::SortComponentsByPriority(AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        // Create list of components with their current order. AZStd::sort isn't guaranteed to maintain order for equivalent entities.

        AZStd::vector< OrderedSortComponentEntry> sortedComponents;
        int index = 0;
        for (AZ::Component* component : componentsOnEntity)
        {
            sortedComponents.push_back(OrderedSortComponentEntry(component, index++));
        }

        // shuffle immovable components back to the front
        AZStd::sort(
            sortedComponents.begin(),
            sortedComponents.end(),
            [=](const OrderedSortComponentEntry& component1, const OrderedSortComponentEntry& component2)
            {
                AZStd::optional<int> fixedComponentListIndex1 = GetFixedComponentListIndex(component1.m_component);
                AZStd::optional<int> fixedComponentListIndex2 = GetFixedComponentListIndex(component2.m_component);

                // If both components have fixed list indices, sort based on those indices
                if (fixedComponentListIndex1.has_value() && fixedComponentListIndex2.has_value())
                {
                    return fixedComponentListIndex1.value() < fixedComponentListIndex2.value();
                }

                // If component 1 has a fixed list index, sort it first
                if (fixedComponentListIndex1.has_value())
                {
                    return true;
                }

                // If component 2 has a fixed list index, component 1 should not be sorted before it
                if (fixedComponentListIndex2.has_value())
                {
                    return false;
                }

                if (!IsComponentRemovable(component1.m_component) && IsComponentRemovable(component2.m_component))
                {
                    return true;
                }

                if (IsComponentRemovable(component1.m_component) && !IsComponentRemovable(component2.m_component))
                {
                    return false;
                }

                //maintain original order if they don't need swapping
                return component1.m_originalOrder < component2.m_originalOrder;
            });

        //create new order array from sorted structure
        componentsOnEntity.clear();
        for (OrderedSortComponentEntry& component : sortedComponents)
        {
            componentsOnEntity.push_back(component.m_component);
        }
    }

    void SortComponentsByOrder(const AZ::EntityId entityId, AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        // sort by component order, shuffling anything not found in component order to the end
        ComponentOrderArray componentOrder;
        EditorInspectorComponentRequestBus::EventResult(
            componentOrder, entityId, &EditorInspectorComponentRequests::GetComponentOrderArray);

        AZStd::sort(
            componentsOnEntity.begin(),
            componentsOnEntity.end(),
            [&componentOrder](const AZ::Component* component1, const AZ::Component* component2)
            {
                return
                    AZStd::find(componentOrder.begin(), componentOrder.end(), component1->GetId()) <
                    AZStd::find(componentOrder.begin(), componentOrder.end(), component2->GetId());
            });
    }

    void SaveComponentOrder(const AZ::EntityId entityId, const AZ::Entity::ComponentArrayType& componentsInOrder)
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

    bool EntityPropertyEditor::IsComponentRemovable(const AZ::Component* component)
    {
        // Determine if this component can be removed.
        auto componentClassData = component ? GetComponentClassData(component) : nullptr;
        if (componentClassData && componentClassData->m_editData)
        {
            if (auto editorDataElement = componentClassData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::RemoveableByUser))
                {
                    if (auto attributeData = azdynamic_cast<AZ::Edit::AttributeData<bool>*>(attribute))
                    {
                        return attributeData->Get(nullptr);
                    }
                }
            }
        }

        if (componentClassData && AppearsInAnyComponentMenu(*componentClassData))
        {
            return true;
        }

        // If this is a GenericComponentWrapper which wraps a nullptr, let the user remove it
        if (auto genericComponentWrapper = azrtti_cast<const Components::GenericComponentWrapper*>(component))
        {
            if (!genericComponentWrapper->GetTemplate())
            {
                return true;
            }
        }

        return false;
    }

    bool EntityPropertyEditor::AreComponentsRemovable(const AZ::Entity::ComponentArrayType& components) const
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

    AZStd::optional<int> EntityPropertyEditor::GetFixedComponentListIndex(const AZ::Component* component)
    {
        auto componentClassData = component ? GetComponentClassData(component) : nullptr;
        if (componentClassData && componentClassData->m_editData)
        {
            if (auto editorDataElement = componentClassData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::FixedComponentListIndex))
                {
                    if (auto attributeData = azdynamic_cast<AZ::Edit::AttributeData<int>*>(attribute))
                    {
                        return { attributeData->Get(nullptr) };
                    }
                }
            }
        }
        return {};
    }

    bool EntityPropertyEditor::IsComponentDraggable(const AZ::Component* component)
    {
        return !GetFixedComponentListIndex(component).has_value();
    }

    bool EntityPropertyEditor::AreComponentsDraggable(const AZ::Entity::ComponentArrayType& components) const
    {
        return AZStd::all_of(
            components.begin(), components.end(), [](AZ::Component* component) { return IsComponentDraggable(component); });
    }

    bool EntityPropertyEditor::AreComponentsCopyable(const AZ::Entity::ComponentArrayType& components) const
    {
        return AreComponentsCopyable(components, m_componentFilter);
    }

    bool EntityPropertyEditor::AreComponentsCopyable(const AZ::Entity::ComponentArrayType& components, const ComponentFilter& filter)
    {
        for (auto component : components)
        {
            if (component->RTTI_IsTypeOf(Layers::EditorLayerComponent::RTTI_Type()))
            {
                return false;
            }
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

    bool EntityPropertyEditor::SelectedEntitiesAreFromSameSourceSliceEntity() const
    {
        AZ::EntityId commonAncestorId;

        for (AZ::EntityId id : m_selectedEntityIds)
        {
            AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
            AzFramework::SliceEntityRequestBus::EventResult(sliceInstanceAddress, id,
                &AzFramework::SliceEntityRequests::GetOwningSlice);
            auto sliceReference = sliceInstanceAddress.GetReference();
            if (!sliceReference)
            {
                return false;
            }
            else
            {
                AZ::SliceComponent::EntityAncestorList ancestors;
                sliceReference->GetInstanceEntityAncestry(id, ancestors, 1);
                if ( ancestors.empty() || !ancestors[0].m_entity)
                {
                    return false;
                }

                if (!commonAncestorId.IsValid())
                {
                    commonAncestorId = ancestors[0].m_entity->GetId();
                }
                else if (ancestors[0].m_entity->GetId() != commonAncestorId)
                {
                    return false;
                }
            }
        }

        return true;
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

        // For single selection of a slice-instanced entity, gather the direct slice ancestor
        // so we can visualize per-component differences.
        m_sliceCompareToEntity.reset();
        if (SelectedEntitiesAreFromSameSourceSliceEntity())
        {
            AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;

            AzFramework::SliceEntityRequestBus::EventResult(sliceInstanceAddress, entityId,
                &AzFramework::SliceEntityRequests::GetOwningSlice);
            auto sliceReference = sliceInstanceAddress.GetReference();
            auto sliceInstance = sliceInstanceAddress.GetInstance();
            if (sliceReference)
            {
                AZ::SliceComponent::EntityAncestorList ancestors;
                sliceReference->GetInstanceEntityAncestry(entityId, ancestors, 1);

                if (!ancestors.empty())
                {
                    m_sliceCompareToEntity = SliceUtilities::CloneSliceEntityForComparison(*ancestors[0].m_entity, *sliceInstance, *m_serializeContext);
                }
            }
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

            // Grab the slice reference component, if we have a slice compare entity
            auto sliceReferenceComponent = m_sliceCompareToEntity ? m_sliceCompareToEntity->FindComponent(component->GetId()) : nullptr;

            sharedComponentArray.push_back(SharedComponentInfo(component, sliceReferenceComponent));
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

        for (auto& sharedComponentInfo : sharedComponentArray)
        {
            bool componentInFilter = ComponentMatchesCurrentFilter(sharedComponentInfo);

            auto componentEditor = CreateComponentEditor();

            // Add instances to componentEditor
            auto& componentInstances = sharedComponentInfo.m_instances;
            for (AZ::Component* componentInstance : componentInstances)
            {
                // non-first instances are aggregated under the first instance
                AZ::Component* aggregateInstance = componentInstance != componentInstances.front() ? componentInstances.front() : nullptr;

                // Reference the slice entity if we are a slice so we can indicate differences from base
                AZ::Component* referenceComponentInstance = sharedComponentInfo.m_sliceReferenceComponent;
                componentEditor->AddInstance(componentInstance, aggregateInstance, referenceComponentInstance);
            }

            // Set tab order for editor
            setTabOrder(lastTabWidget, componentEditor);
            lastTabWidget = componentEditor;

            // Refresh editor
            componentEditor->AddNotifications();
            componentEditor->UpdateExpandability();
            componentEditor->InvalidateAll(!componentInFilter ? m_filterString.c_str() : nullptr);

            if (!componentEditor->GetPropertyEditor()->HasFilteredOutNodes() || componentEditor->GetPropertyEditor()->HasVisibleNodes())
            {
                for (AZ::Component* componentInstance : componentInstances)
                {
                    // Map the component to the editor
                    m_componentToEditorMap[componentInstance] = componentEditor;
                }
                componentEditor->show();
            }
            else
            {
                componentEditor->hide();
                componentEditor->ClearInstances(true);
            }
        }
    }

    ComponentEditor* EntityPropertyEditor::CreateComponentEditor()
    {
        //caching allocated component editors for reuse and to preserve order
        if (m_componentEditorsUsed >= m_componentEditors.size())
        {
            //create a new component editor since cache has been exceeded
            auto componentEditor = new ComponentEditor(m_serializeContext, this, this);
            componentEditor->setAcceptDrops(true);

            connect(componentEditor, &ComponentEditor::OnExpansionContractionDone, this, [this]()
                {
                    setUpdatesEnabled(false);
                    layout()->update();
                    layout()->activate();
                    setUpdatesEnabled(true);
                });
            connect(componentEditor, &ComponentEditor::OnDisplayComponentEditorMenu, this, &EntityPropertyEditor::OnDisplayComponentEditorMenu);
            connect(componentEditor, &ComponentEditor::OnRequestRequiredComponents, this, &EntityPropertyEditor::OnRequestRequiredComponents);
            connect(componentEditor, &ComponentEditor::OnRequestRemoveComponents, this, [this](const AZ::Entity::ComponentArrayType& components) {DeleteComponents(components); });
            connect(componentEditor, &ComponentEditor::OnRequestDisableComponents, this, [this](const AZ::Entity::ComponentArrayType& components) {DisableComponents(components); });
            componentEditor->GetPropertyEditor()->SetValueComparisonFunction([this](const InstanceDataNode* source, const InstanceDataNode* target) { return InstanceNodeValueHasNoPushableChange(source, target); });
            componentEditor->GetPropertyEditor()->SetReadOnlyQueryFunction([this](const InstanceDataNode* node) { return QueryInstanceDataNodeReadOnlyStatus(node); });
            componentEditor->GetPropertyEditor()->SetHiddenQueryFunction([this](const InstanceDataNode* node) { return QueryInstanceDataNodeHiddenStatus(node); });
            componentEditor->GetPropertyEditor()->SetIndicatorQueryFunction([this](const InstanceDataNode* node) { return GetAppropriateIndicator(node); });

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
        MarkPropertyEditorBusyStart();
    }

    void EntityPropertyEditor::SetPropertyEditingComplete(InstanceDataNode* /*pNode*/)
    {
        MarkPropertyEditorBusyEnd();
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
        for (auto componentEditor : m_componentEditors)
        {
            componentEditor->hide();
            componentEditor->ClearInstances(invalidateImmediately);

            // Re-enable RPE-level refresh calls.  Since we're clearing out the associated RPE, there's no longer a danger
            // that they will get a partial refresh while in an invalid state.
            // (RPE refreshes were prevented in QueuePropertyRefresh() to ensure that no RPEs tried a partial refresh in-between
            // the time an EPE full refresh was requested and when it executed.)
            componentEditor->PreventRefresh(false);
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
            for (auto componentEditor : m_componentEditors)
            {
                // Cancel any refreshes that were queued prior to this point, since they are no longer guaranteed to
                // be valid requests.
                componentEditor->CancelQueuedRefresh();
                // Prevent any future refreshes from getting queued until we've completed the UpdateContents() call.
                componentEditor->PreventRefresh(true);
            }

            // Refresh the properties using a singleShot
            // this makes sure that the properties aren't refreshed while processing
            // other events, and instead runs after the current events are processed
            QTimer::singleShot(0, this, &EntityPropertyEditor::UpdateContents);

            //saving state any time refresh gets queued because requires valid components
            //attempting to call directly anywhere state needed to be preserved always occurred with QueuePropertyRefresh
            SaveComponentEditorState();
        }
    }

    void EntityPropertyEditor::OnAddComponent()
    {
        AZStd::vector<AZ::ComponentServiceType> serviceFilter;
        AZStd::vector<AZ::ComponentServiceType> incompatibleServiceFilter;
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
                ScopedUndoBatch undoBatch("ModifyEntityName");

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

                AddMenuOptionsForRevert(node, componentNode, componentClassData, menu);

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

        // Generate slice data push/pull options.
        AZ::Component* componentInstance = m_serializeContext->Cast<AZ::Component*>(
                componentNode->FirstInstance(), componentClassData->m_typeId);
        AZ_Assert(componentInstance, "Failed to cast component instance.");

        // With the entity the property ultimately belongs to, we can look up slice ancestry for push/pull options.
        AZ::Entity* entity = componentInstance->GetEntity();

        AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityIdContextQueryBus::EventResult(contextId, entity->GetId(), &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);
        if (contextId.IsNull())
        {
            AZ_Error("PropertyEditor", false, "Entity \"%s\" does not belong to any context.", entity->GetName().c_str());
            return;
        }

        // If prefabs are enabled, there will be no root slice so bail out here since we don't need
        // to show any slice options in the menu
        AZ::SliceComponent* rootSlice = nullptr;
        AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSlice, contextId,
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
        if (rootSlice)
        {
            AZ::SliceComponent::SliceInstanceAddress address;
            AzFramework::SliceEntityRequestBus::EventResult(address, entity->GetId(), &AzFramework::SliceEntityRequests::GetOwningSlice);
            AZ::SliceComponent::SliceReference* sliceReference = address.GetReference();
            if (sliceReference)
            {
                // This entity is instanced from a slice, so show data push/pull options
                AZ::SliceComponent::EntityAncestorList ancestors;
                sliceReference->GetInstanceEntityAncestry(entity->GetId(), ancestors);

                AZ_Error(
                    "PropertyEditor", !ancestors.empty(), "Entity \"%s\" belongs to a slice, but its source entity could not be located.",
                    entity->GetName().c_str());
                if (!ancestors.empty())
                {
                    menu.addSeparator();

                    // Populate slice push options.
                    // Address should start with the fully-addressable component Id to resolve within the target entity.
                    InstanceDataHierarchy::Address pushFieldAddress;
                    CalculateAndAdjustNodeAddress(*fieldNode, AddressRootType::RootAtEntity, pushFieldAddress);
                    if (!pushFieldAddress.empty())
                    {
                        SliceUtilities::PopulateQuickPushMenu(
                            menu, entity->GetId(), pushFieldAddress,
                            SliceUtilities::QuickPushMenuOptions(
                                "Save field override",
                                SliceUtilities::QuickPushMenuOverrideDisplayCount::ShowOverrideCountOnlyWhenMultiple));
                    }
                }
            }

            menu.addSeparator();

            // by leaf node, we mean a visual leaf node in the property editor (ie, we do not have any visible children)
            bool isLeafNode = !fieldNode->GetClassMetadata() || !fieldNode->GetClassMetadata()->m_container;

            if (isLeafNode)
            {
                for (const InstanceDataNode& childNode : fieldNode->GetChildren())
                {
                    if (HasAnyVisibleElements(childNode))
                    {
                        // If we have any visible children, we must not be a leaf node
                        isLeafNode = false;
                        break;
                    }
                }
            }

#ifdef ENABLE_SLICE_EDITOR
            // Show PreventOverride & HideProperty options
            if (GetEntityDataPatchAddress(fieldNode, m_dataPatchAddressBuffer))
            {
                AZ::DataPatch::Flags nodeFlags = rootSlice->GetEntityDataFlagsAtAddress(entity->GetId(), m_dataPatchAddressBuffer);

                if (nodeFlags & AZ::DataPatch::Flag::PreventOverrideSet)
                {
                    QAction* PreventOverrideAction = menu.addAction(tr("Allow property override"));
                    PreventOverrideAction->setEnabled(isLeafNode);
                    connect(
                        PreventOverrideAction, &QAction::triggered, this,
                        [this, fieldNode]
                        {
                            ContextMenuActionSetDataFlag(fieldNode, AZ::DataPatch::Flag::PreventOverrideSet, false);
                            InvalidatePropertyDisplay(Refresh_AttributesAndValues);
                        });
                }
                else
                {
                    QAction* PreventOverrideAction = menu.addAction(tr("Prevent property override"));
                    PreventOverrideAction->setEnabled(isLeafNode);
                    connect(
                        PreventOverrideAction, &QAction::triggered, this,
                        [this, fieldNode]
                        {
                            ContextMenuActionSetDataFlag(fieldNode, AZ::DataPatch::Flag::PreventOverrideSet, true);
                            InvalidatePropertyDisplay(Refresh_AttributesAndValues);
                        });
                }

                if (nodeFlags & AZ::DataPatch::Flag::HidePropertySet)
                {
                    QAction* HideProperyAction = menu.addAction(tr("Show property on instances"));
                    HideProperyAction->setEnabled(isLeafNode);
                    connect(
                        HideProperyAction, &QAction::triggered, this,
                        [this, fieldNode]
                        {
                            ContextMenuActionSetDataFlag(fieldNode, AZ::DataPatch::Flag::HidePropertySet, false);
                            InvalidatePropertyDisplay(Refresh_AttributesAndValues);
                        });
                }
                else
                {
                    QAction* HideProperyAction = menu.addAction(tr("Hide property on instances"));
                    HideProperyAction->setEnabled(isLeafNode);
                    connect(
                        HideProperyAction, &QAction::triggered, this,
                        [this, fieldNode]
                        {
                            ContextMenuActionSetDataFlag(fieldNode, AZ::DataPatch::Flag::HidePropertySet, true);
                            InvalidatePropertyDisplay(Refresh_AttributesAndValues);
                        });
                }
            }
#endif

            if (sliceReference)
            {
                // This entity is referenced from a slice, so show property override options
                bool hasChanges = fieldNode->HasChangesVersusComparison(false);

                if (!hasChanges && isLeafNode)
                {
                    // Add an option to set the ForceOverride flag for this field
                    menu.setToolTipsVisible(true);
                    QAction* forceOverrideAction = menu.addAction(tr("Force property override"));
                    forceOverrideAction->setToolTip(tr("Prevents a property from inheriting from its source slice"));
                    connect(
                        forceOverrideAction, &QAction::triggered, this,
                        [this, fieldNode]()
                        {
                            ContextMenuActionSetDataFlag(fieldNode, AZ::DataPatch::Flag::ForceOverrideSet, true);
                        });
                }
            }
        }

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

    void EntityPropertyEditor::AddMenuOptionsForRevert(InstanceDataNode* fieldNode, InstanceDataNode* componentNode, const AZ::SerializeContext::ClassData* componentClassData, QMenu& menu)
    {
        QMenu* revertMenu = nullptr;

        auto addRevertMenu = [&menu]()
        {
            QMenu* revertOverridesMenu = menu.addMenu(tr("Revert overrides"));
            revertOverridesMenu->setToolTipsVisible(true);
            return revertOverridesMenu;
        };

        //check for changes on selected property
        if (componentClassData)
        {
            AZ::SliceComponent::SliceInstanceAddress address;

            AZ::Component* componentInstance = m_serializeContext->Cast<AZ::Component*>(
                componentNode->FirstInstance(), componentClassData->m_typeId);
            AZ_Assert(componentInstance, "Failed to cast component instance.");
            AZ::Entity* entity = componentInstance->GetEntity();

            AzFramework::SliceEntityRequestBus::EventResult(address, entity->GetId(),
                &AzFramework::SliceEntityRequests::GetOwningSlice);
            AZ::SliceComponent::SliceReference* sliceReference = address.GetReference();

            if (!sliceReference)
            {
                return;
            }

            // Only add the "Revert overrides" menu option if it belongs to a slice
            revertMenu = addRevertMenu();
            revertMenu->setEnabled(false);

            if (fieldNode)
            {
                bool hasChanges = fieldNode->HasChangesVersusComparison(false);

                if (hasChanges)
                {
                    bool isLeafNode = !fieldNode->GetClassMetadata()->m_container;

                    if (isLeafNode)
                    {
                        for (const InstanceDataNode& childNode : fieldNode->GetChildren())
                        {
                            if (HasAnyVisibleElements(childNode))
                            {
                                // If we have any visible children, we must not be a leaf node
                                isLeafNode = false;
                                break;
                            }
                        }

                        if (isLeafNode)
                        {
                            revertMenu->setEnabled(true);

                            // Add an option to pull data from the first level of the slice (clear the override).
                            QAction* revertAction = revertMenu->addAction(tr("Property"));
                            revertAction->setToolTip(tr("Revert the value for this property to the last saved state."));
                            revertAction->setEnabled(true);
                            connect(revertAction, &QAction::triggered, this, [this, componentInstance, fieldNode]()
                            {
                                ContextMenuActionPullFieldData(componentInstance, fieldNode);
                            }
                            );
                        }
                    }
                }
            }
        }

        //check for changes on selected component(s)
        bool hasSliceChanges = false;
        bool isPartOfSlice = false;

        const auto& componentsToEdit = GetSelectedComponents();

        for (auto component : componentsToEdit)
        {
            AZ_Assert(component, "Parent component is invalid.");
            auto componentEditorIterator = m_componentToEditorMap.find(component);
            AZ_Assert(componentEditorIterator != m_componentToEditorMap.end(), "Unable to find a component editor for the given component");
            if (componentEditorIterator != m_componentToEditorMap.end())
            {
                auto componentEditor = componentEditorIterator->second;
                componentEditor->GetPropertyEditor()->EnumerateInstances(
                    [&hasSliceChanges, &isPartOfSlice](InstanceDataHierarchy& hierarchy)
                    {
                        InstanceDataNode* root = hierarchy.GetRootNode();
                        if (root && root->GetComparisonNode())
                        {
                            isPartOfSlice = true;
                            if (root->HasChangesVersusComparison(true))
                            {
                                hasSliceChanges = true;
                            }
                        }
                    });
            }
        }

        if (isPartOfSlice && hasSliceChanges)
        {
            if (!revertMenu)
            {
                revertMenu = addRevertMenu();
            }
            revertMenu->setEnabled(true);

            QAction* revertComponentAction = revertMenu->addAction(tr("Component"));
            revertComponentAction->setToolTip(tr("Revert all properties for this component to their last saved state."));
            connect(revertComponentAction, &QAction::triggered, this, [this]()
            {
                ResetToSlice();
            });
        }

        //check for changes on selected entities
        EntityIdList selectedEntityIds;
        GetSelectedEntities(selectedEntityIds);

        bool canRevert = false;

        for (AZ::EntityId id : m_selectedEntityIds)
        {
            bool entityHasOverrides = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(entityHasOverrides, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::HasSliceEntityOverrides);
            if (entityHasOverrides)
            {
                canRevert = true;
                break;
            }
        }

        if (canRevert && !selectedEntityIds.empty())
        {
            //check for changes in the slice
            EntityIdSet relevantEntitiesSet;
            ToolsApplicationRequestBus::BroadcastResult(relevantEntitiesSet, &ToolsApplicationRequestBus::Events::GatherEntitiesAndAllDescendents, selectedEntityIds);

            EntityIdList relevantEntities;
            relevantEntities.reserve(relevantEntitiesSet.size());
            for (AZ::EntityId& id : relevantEntitiesSet)
            {
                relevantEntities.push_back(id);
            }

            if (!revertMenu)
            {
                revertMenu = addRevertMenu();
            }
            revertMenu->setEnabled(true);
            QAction* revertAction = revertMenu->addAction(QObject::tr("Entity"));
            revertAction->setToolTip(QObject::tr("This will revert all component properties on this entity to the last saved."));

            QObject::connect(revertAction, &QAction::triggered, [relevantEntities]
            {
                SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
                    &SliceEditorEntityOwnershipServiceRequests::ResetEntitiesToSliceDefaults, relevantEntities);
            });
        }
    }


    void EntityPropertyEditor::ContextMenuActionPullFieldData(AZ::Component* parentComponent, InstanceDataNode* fieldNode)
    {
        AZ_Assert(fieldNode && fieldNode->GetComparisonNode(), "Invalid node for slice data pull.");
        if (!fieldNode->GetComparisonNode())
        {
            return;
        }

        BeforePropertyModified(fieldNode);

        // Clear any slice data flags for this field
        if (GetEntityDataPatchAddress(fieldNode, m_dataPatchAddressBuffer))
        {
            auto clearDataFlagsCommand = aznew ClearSliceDataFlagsBelowAddressCommand(parentComponent->GetEntityId(), m_dataPatchAddressBuffer, "Clear data flags");
            clearDataFlagsCommand->SetParent(m_currentUndoOperation);
        }

        AZStd::unordered_set<InstanceDataNode*> targetContainerNodesWithNewElements;
        InstanceDataHierarchy::CopyInstanceData(fieldNode->GetComparisonNode(), fieldNode, m_serializeContext,
            // Target container child node being removed callback
            nullptr,

            // Source container child node being created callback
            [&targetContainerNodesWithNewElements](const InstanceDataNode* sourceNode, InstanceDataNode* targetNodeParent)
            {
                (void)sourceNode;

                targetContainerNodesWithNewElements.insert(targetNodeParent);
            }
            );

        // Invoke Add notifiers
        for (InstanceDataNode* targetContainerNode : targetContainerNodesWithNewElements)
        {
            AZ_Assert(targetContainerNode->GetClassMetadata()->m_container, "Target container node isn't a container!");
            for (const AZ::Edit::AttributePair& attribute : targetContainerNode->GetElementEditMetadata()->m_attributes)
            {
                if (attribute.first == AZ::Edit::Attributes::AddNotify)
                {
                    AZ::Edit::AttributeFunction<void()>* func = azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(attribute.second);
                    if (func)
                    {
                        InstanceDataNode* parentNode = targetContainerNode->GetParent();
                        if (parentNode)
                        {
                            for (size_t idx = 0; idx < parentNode->GetNumInstances(); ++idx)
                            {
                                func->Invoke(parentNode->GetInstance(idx));
                            }
                        }
                    }
                }
            }
        }

        // Make sure change notifiers are invoked for the change across any changed nodes in sub-hierarchy
        // under the affected node.
        AZ_Assert(parentComponent, "Parent component is invalid.");
        auto componentEditorIterator = m_componentToEditorMap.find(parentComponent);
        AZ_Assert(componentEditorIterator != m_componentToEditorMap.end(), "Unable to find a component editor for the given component");
        if (componentEditorIterator != m_componentToEditorMap.end())
        {
            AZStd::vector<PropertyRowWidget*> widgetStack;
            AZStd::vector<PropertyRowWidget*> widgetsToNotify;
            widgetStack.reserve(64);
            widgetsToNotify.reserve(64);

            auto componentEditor = componentEditorIterator->second;
            PropertyRowWidget* widget = componentEditor->GetPropertyEditor()->GetWidgetFromNode(fieldNode);
            if (widget)
            {
                widgetStack.push_back(widget);

                while (!widgetStack.empty())
                {
                    PropertyRowWidget* top = widgetStack.back();
                    widgetStack.pop_back();

                    InstanceDataNode* topWidgetNode = top->GetNode();
                    if (topWidgetNode && topWidgetNode->HasChangesVersusComparison(false))
                    {
                        widgetsToNotify.push_back(top);
                    }

                    // Only add children widgets for notification if the newly updated parent has children
                    // otherwise the instance data nodes will have disappeared out from under the widgets
                    // They will be cleaned up when we refresh the entire tree after the notifications go out
                    if (!topWidgetNode || topWidgetNode->GetChildren().size() > 0)
                    {
                        for (auto* childWidget : top->GetChildrenRows())
                        {
                            widgetStack.push_back(childWidget);
                        }
                    }
                }

                // Issue property notifications, starting with leaf widgets first.
                for (auto iter = widgetsToNotify.rbegin(); iter != widgetsToNotify.rend(); ++iter)
                {
                    (*iter)->DoPropertyNotify();
                }
            }
        }

        AfterPropertyModified(fieldNode);

        // We must refresh the entire tree, because restoring previous entity state may've had major structural effects.
        InvalidatePropertyDisplay(Refresh_EntireTree);
    }

    void EntityPropertyEditor::ContextMenuActionSetDataFlag(InstanceDataNode* node, AZ::DataPatch::Flag flag, bool additive)
    {
        // Attempt to get Entity and DataPatch address from this node.
        AZ::EntityId entityId;
        if (GetEntityDataPatchAddress(node, m_dataPatchAddressBuffer, &entityId))
        {
            BeforePropertyModified(node);

            auto command = aznew SliceDataFlagsCommand(entityId, m_dataPatchAddressBuffer, flag, additive, "Set slice data flag");
            command->SetParent(m_currentUndoOperation);

            AfterPropertyModified(node);
            InvalidatePropertyDisplay(Refresh_AttributesAndValues);
        }
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
                    outAddress.push_back(AZ_CRC("Components", 0xee48f5fd));
                }
                break;
                case AddressRootType::RootAtEntity:
                {
                    // Insert a node to represent the Components container, and another to represent the entity root.
                    outAddress.push_back(AZ_CRC("Components", 0xee48f5fd));
                    outAddress.push_back(AZ_CRC("AZ::Entity", 0x1fd61e11));
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

    bool EntityPropertyEditor::InstanceNodeValueHasNoPushableChange(const InstanceDataNode* sourceNode, const InstanceDataNode* targetNode)
    {
        if (targetNode == nullptr)
        {
            return sourceNode == nullptr;
        }
        // If target node is affected by ForceOverride flag, consider it different from source.
        AZ::EntityId entityId;
        if (GetEntityDataPatchAddress(targetNode, m_dataPatchAddressBuffer, &entityId))
        {
            if (AZ::SliceComponent* rootSlice = GetEntityRootSlice(entityId))
            {
                AZ::DataPatch::Flags flags = rootSlice->GetEntityDataFlagsAtAddress(entityId, m_dataPatchAddressBuffer);
                if (flags & AZ::DataPatch::Flag::ForceOverrideSet)
                {
                    return false;
                }
            }
        }

        // Verify that this is a field that can be pushed into the slice.
        if (InstanceDataNode* componentNode = targetNode->GetRoot())
        {
            AZ::Component* component = m_serializeContext->Cast<AZ::Component*>(
                componentNode->FirstInstance(), componentNode->GetClassMetadata()->m_typeId);
            if (component)
            {
                AZ::Entity* entity = component->GetEntity();
                bool isRootEntity = entity ? SliceUtilities::IsRootEntity(*entity) : false;
                if (!SliceUtilities::IsNodePushable(*targetNode, isRootEntity))
                {
                    // If this field is one that can't be pushed into the slice, then
                    // return true to signify that here is no pushable change on this field.
                    return true;
                }
            }
        }

        // Otherwise, do the default value comparison.
        return InstanceDataHierarchy::DefaultValueComparisonFunction(sourceNode, targetNode);
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
        for (size_t i=0; i<comboItemCount; ++i)
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

        if (m_prefabsAreEnabled)
        {
            if (allInactive)
            {
                AZ_Warning("Prefab", false, "All entities found to be inactive. This is an option that's not supported with Prefabs.");
                allInactive = false;
                allEditorOnly = true;
            }
            if (someInactive)
            {
                AZ_Warning("Prefab", false, "Some inactive entities found. This is an option that's not supported with Prefabs.");
                someInactive = false;
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

        m_gui->m_statusComboBox->setVisible(!m_isSystemEntityEditor && !m_isLevelEntityEditor);
        m_gui->m_statusComboBox->style()->unpolish(m_gui->m_statusComboBox);
        m_gui->m_statusComboBox->style()->polish(m_gui->m_statusComboBox);
    }

    size_t EntityPropertyEditor::StatusTypeToIndex(StatusType statusType) const
    {
        if (m_prefabsAreEnabled)
        {
            switch (statusType)
            {
            case StatusStartActive:
                return 0;
            case StatusStartInactive:
                AZ_Assert(false, "StatusStartInactive is not supported when Prefabs are enabled.");
                return 0;
            case StatusEditorOnly:
                return 1;
            case StatusItems:
                return 2;
            default:
                AZ_Assert(false, "StatusType for EntityPropertyEditor is out of bounds.");
                return 1;
            }
        }
        else
        {
            return statusType;
        }
    }

    EntityPropertyEditor::StatusType EntityPropertyEditor::IndexToStatusType(size_t index) const
    {
        if (m_prefabsAreEnabled)
        {
            switch (index)
            {
            case 0:
                return StatusStartActive;
            case 1:
                return StatusEditorOnly;
            default:
                AZ_Assert(index < StatusType::StatusItems, "Index for EntityPropertyEditor::IndexToStatusType is out of bounds");
                return StatusEditorOnly;
            }
        }
        else
        {
            AZ_Assert(index < StatusType::StatusItems, "Index for EntityPropertyEditor::IndexToStatusType is out of bounds");
            return aznumeric_cast<StatusType>(index);
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

        AddMenuOptionsForRevert(nullptr, nullptr, nullptr, menu);

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
        const AZStd::vector<AZ::ComponentServiceType>& services, 
        const AZStd::vector<AZ::ComponentServiceType>& incompatibleServices)
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
        const AZStd::vector<AZ::ComponentServiceType>& serviceFilter,
        const AZStd::vector<AZ::ComponentServiceType>& incompatibleServiceFilter)
    {
        HideComponentPalette();

        bool areEntitiesEditable = true;
        ToolsApplicationRequests::Bus::BroadcastResult(areEntitiesEditable, &ToolsApplicationRequests::AreEntitiesEditable, m_selectedEntityIds);
        if (areEntitiesEditable)
        {
            componentPalette->Populate(m_serializeContext, m_selectedEntityIds, m_componentFilter, serviceFilter, incompatibleServiceFilter);
            componentPalette->Present();
            componentPalette->setGeometry(QRect(position, size));
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

        QAction* seperator1 = new QAction(this);
        seperator1->setSeparator(true);
        addAction(seperator1);

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

        QAction* seperator2 = new QAction(this);
        seperator2->setSeparator(true);
        addAction(seperator2);

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

        const bool hasComponents = !m_selectedEntityIds.empty() && !componentsToEdit.empty();
        const bool allowRemove = hasComponents && AreComponentsRemovable(componentsToEdit);
        const bool allowCopy = hasComponents && AreComponentsCopyable(componentsToEdit);

        m_actionToDeleteComponents->setEnabled(allowRemove);
        m_actionToCutComponents->setEnabled(allowRemove && allowCopy);
        m_actionToCopyComponents->setEnabled(allowCopy);
        m_actionToPasteComponents->setEnabled(!m_selectedEntityIds.empty() && CanPasteComponentsOnSelectedEntities());
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
    }

    bool EntityPropertyEditor::CanPasteComponentsOnSelectedEntities() const
    {
        if (m_selectedEntityIds.empty())
        {
            return false;
        }

        // Grab component data from clipboard, if exists
        const QMimeData* mimeData = ComponentMimeData::GetComponentMimeDataFromClipboard();

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
        if (!entity || classDataForComponentsToPaste.empty())
        {
            return false;
        }

        bool canPaste = true;

        for (auto componentClassData : classDataForComponentsToPaste)
        {
            if (componentClassData)
            {
                // A component can be pasted onto an entity if it appears in the game component menu or if it already exists on the entity
                auto existingComponent = entity->FindComponent(componentClassData->m_typeId);
                if (!existingComponent && !AppearsInGameComponentMenu(*componentClassData))
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

    void EntityPropertyEditor::DeleteComponents(const AZ::Entity::ComponentArrayType& components)
    {
        if (!components.empty() && AreComponentsRemovable(components))
        {
            // Need to queue an update for all inspectors in case multiples are viewing the same entity and the removal of a component internally triggers an invalidate call
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);

            ScopedUndoBatch undoBatch("Removing components.");

            AzToolsFramework::RemoveComponents(components);
        }
    }

    void EntityPropertyEditor::DeleteComponents()
    {
        DeleteComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::CutComponents()
    {
        const auto& componentsToEdit = GetSelectedComponents();
        if (!componentsToEdit.empty() && AreComponentsRemovable(componentsToEdit))
        {
            QueuePropertyRefresh();

            //intentionally not using EntityCompositionRequests::CutComponents because we only want to copy components from first selected entity
            ScopedUndoBatch undoBatch("Cut components.");
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

    void EntityPropertyEditor::PasteComponents()
    {
        if (!m_selectedEntityIds.empty() && CanPasteComponentsOnSelectedEntities())
        {
            ScopedUndoBatch undoBatch("Paste components.");

            AZ::Entity::ComponentArrayType selectedComponents = GetSelectedComponents();

            AZ::Entity::ComponentArrayType componentsAfterPaste;
            AZ::Entity::ComponentArrayType componentsInOrder;

            //paste to all selected entities
            for (const AZ::EntityId& entityId : m_selectedEntityIds)
            {
                //get components and order before pasting so we can insert new components above the first selected one
                componentsInOrder.clear();
                GetAllComponentsForEntityInOrder(GetEntity(entityId), componentsInOrder);

                //perform the paste operation which should add new components to the entity or pending list
                EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::PasteComponentsToEntity, entityId);

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

    void EntityPropertyEditor::EnableComponents(const AZ::Entity::ComponentArrayType& components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::EnableComponents, components);
        QueuePropertyRefresh();
    }

    void EntityPropertyEditor::EnableComponents()
    {
        EnableComponents(GetSelectedComponents());
    }

    void EntityPropertyEditor::DisableComponents(const AZ::Entity::ComponentArrayType& components)
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

        ScopedUndoBatch undoBatch("Move components up.");

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

        ScopedUndoBatch undoBatch("Move components down.");

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

        ScopedUndoBatch undoBatch("Move components to top.");

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

                const AZ::Entity::ComponentArrayType& componentsToMove = componentsToEditByEntityIdItr->second;
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

        ScopedUndoBatch undoBatch("Move components to bottom.");

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

                const AZ::Entity::ComponentArrayType& componentsToMove = componentsToEditByEntityIdItr->second;
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

    void EntityPropertyEditor::MoveComponentRowBefore(const AZ::Entity::ComponentArrayType& sourceComponents, const AZ::Entity::ComponentArrayType& targetComponents, ScopedUndoBatch& undo)
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

    void EntityPropertyEditor::MoveComponentRowAfter(const AZ::Entity::ComponentArrayType& sourceComponents, const AZ::Entity::ComponentArrayType& targetComponents, ScopedUndoBatch& undo)
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

    void EntityPropertyEditor::ResetToSlice()
    {
        // Reset selected components to slice values
        const auto& componentsToEdit = GetSelectedComponents();

        if (!componentsToEdit.empty())
        {
            ScopedUndoBatch undoBatch("Resetting components to slice defaults.");

            for (auto component : componentsToEdit)
            {
                AZ_Assert(component, "Component is invalid.");
                auto componentEditorIterator = m_componentToEditorMap.find(component);
                AZ_Assert(componentEditorIterator != m_componentToEditorMap.end(), "Unable to find a component editor for the given component");
                if (componentEditorIterator != m_componentToEditorMap.end())
                {
                    auto componentEditor = componentEditorIterator->second;
                    componentEditor->GetPropertyEditor()->EnumerateInstances(
                        [this, &component](InstanceDataHierarchy& hierarchy)
                        {
                            InstanceDataNode* root = hierarchy.GetRootNode();
                            ContextMenuActionPullFieldData(component, root);
                        }
                        );
                }
            }

            QueuePropertyRefresh();
        }
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

    const AZ::Entity::ComponentArrayType& EntityPropertyEditor::GetSelectedComponents() const
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

        m_selectedComponentsByEntityId.clear();
        for (auto component : m_selectedComponents)
        {
            m_selectedComponentsByEntityId[component->GetEntityId()].push_back(component);
        }
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
        m_shouldScrollToNewComponents = false;
        m_shouldScrollToNewComponentsQueued = false;
        m_newComponentId.reset();

        HighlightMovedRowWidget();
    }

    void EntityPropertyEditor::QueueScrollToNewComponent()
    {
        if (m_shouldScrollToNewComponents && !m_shouldScrollToNewComponentsQueued)
        {
            QTimer::singleShot(100, this, &EntityPropertyEditor::ScrollToNewComponent);
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
        HandleMenuEvent(object, event);
        HandleSelectionEvents(object, event);
        return false;
    }

    void EntityPropertyEditor::mousePressEvent(QMouseEvent* event)
    {
        ResetDrag(event);

        PropertyRowWidget* rowWidget = FindPropertyRowWidgetAt(event->globalPos());
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
        if (UpdateDrag(event->pos(), event->mouseButtons(), event->mimeData()))
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
        if (UpdateDrag(event->pos(), event->mouseButtons(), event->mimeData()))
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

        const QRect globalRect(mouseEvent->globalPos(), mouseEvent->globalPos());

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
        const QPoint globalPos(mapToGlobal(event->pos()));

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
        const QPoint globalPos(event->globalPos());
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

        AzToolsFramework::ReflectedPropertyEditor::WidgetList widgets = m_reorderRowWidgetEditor->GetPropertyEditor()->GetWidgets();
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
            AzToolsFramework::ReflectedPropertyEditor::WidgetList widgets = componentEditor->GetPropertyEditor()->GetWidgets();
            for (auto& [dataNode, rowWidget] : widgets)
            {
                if (DoesIntersectWidget(globalRect, reinterpret_cast<QWidget*>(rowWidget)) && rowWidget->CanBeReordered())
                {
                    return rowWidget;
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

        const QPoint globalPos(event->globalPos());
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
                AzToolsFramework::ReflectedPropertyEditor::WidgetList widgets = componentEditor->GetPropertyEditor()->GetWidgets();
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
        const QPoint globalPos(mapToGlobal(event->pos()));
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
                const AZ::Entity::ComponentArrayType& sourceComponents = sourceComponentEditor->GetComponents();
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
            componentEditor->GetPropertyEditor()->SetFilterString(m_filterString);
            componentEditor->GetHeader()->SetFilterString(m_filterString);
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

    void EntityPropertyEditor::OnContextReset()
    {
        if (IsLockedToSpecificEntities() && !m_isLevelEntityEditor)
        {
            CloseInspectorWindow();
        }
    }

    void EntityPropertyEditor::CloseInspectorWindow()
    {
        for (auto& entityId : m_overrideSelectedEntityIds)
        {
            DisconnectFromEntityBuses(entityId);
        }
        EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, ClosePinnedInspector, this);
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
        QueuePropertyRefresh();
        m_shouldScrollToNewComponents = true;
    }

    void EntityPropertyEditor::ConnectToEntityBuses(const AZ::EntityId& entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AzToolsFramework::EditorInspectorComponentNotificationBus::MultiHandler::BusConnect(entityId);
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(entityId);
    }

    void EntityPropertyEditor::DisconnectFromEntityBuses(const AZ::EntityId& entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AzToolsFramework::EditorInspectorComponentNotificationBus::MultiHandler::BusDisconnect(entityId);
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(entityId);
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

        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_addComponentButton, on);

        AzQtComponents::SetWidgetInteractEnabled(propertyEditorUi->m_entitySearchBox, on);
    }

    AZ::Entity* EntityPropertyEditor::GetSelectedEntityById(AZ::EntityId& entityId) const
    {
        if (m_isLevelEntityEditor)
        {
            AZ::Entity* entity = nullptr;
            SliceMetadataEntityContextRequestBus::BroadcastResult(entity, &SliceMetadataEntityContextRequestBus::Events::GetMetadataEntity, entityId);
            return entity;
        }

        return GetEntityById(entityId);
    }

    // add/remove (enable/disable) all component related actions
    // that can be performed on an entity from the inspector
    static void EnableDisableComponentActions(
        QWidget* widget, const QVector<QAction*>& actions, const bool enable)
    {
        using AddRemoveFunc = void (QWidget::*)(QAction*);

        const AddRemoveFunc addRemove = enable
            ? &QWidget::addAction
            : &QWidget::removeAction;

        for (QAction* action : actions)
        {
            (widget->*addRemove)(action);
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

    void EntityPropertyEditor::OnEditorModeActivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Component)
        {
            DisableComponentActions(this, m_entityComponentActions);
            SetPropertyEditorState(m_gui, false);
            const auto componentModeTypes = m_componentModeCollection->GetComponentTypes();
            m_disabled = true;
            
            if (!componentModeTypes.empty())
            {
                m_componentEditorLastSelectedIndex = GetComponentEditorIndexFromType(componentModeTypes.front());
            }

            for (auto componentEditor : m_componentEditors)
            {
                componentEditor->EnteredComponentMode(componentModeTypes);
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
            const auto componentModeTypes = m_componentModeCollection->GetComponentTypes();
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



#include "UI/PropertyEditor/moc_EntityPropertyEditor.cpp"

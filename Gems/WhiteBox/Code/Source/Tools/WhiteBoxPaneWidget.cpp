/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxPaneWidget.h"
#include "Util/WhiteBoxModelingOps.h"

#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxComponentModeBus.h"
#include "Tools/WhiteBoxLayerUtil.h"
#include "Viewport/WhiteBoxViewportConstants.h"
#include "Util/WhiteBoxModelConvert.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDropEvent>
#include <QEvent>
#include <QGraphicsEffect>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPainterPath>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QSpinBox>
#include <QTimer>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace WhiteBox
{
    namespace
    {
        //! False for entities outside the prefab being edited. The Editor doesn't let those be selected or
        //! edited, and editing one anyway patches the wrong template and reloads the level's entities.
        bool IsEditableInFocus(const AZ::EntityId entityId)
        {
            const auto* focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get();
            return focusModeInterface == nullptr || focusModeInterface->IsInFocusSubTree(entityId);
        }

        //! Every entity in the level that has an Editor White Box component and can be edited in the current focus.
        AZStd::vector<AZ::EntityId> CollectWhiteBoxEntities()
        {
            AZStd::vector<AZ::EntityId> entityIds;
            auto callback = [&entityIds](AZ::Entity* entity)
            {
                if (entity != nullptr && entity->FindComponent<EditorWhiteBoxComponent>() != nullptr &&
                    IsEditableInFocus(entity->GetId()))
                {
                    entityIds.push_back(entity->GetId());
                }
            };
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::EnumerateEntities, callback);
            return entityIds;
        }

        AZ::Entity* FindEntity(const AZ::EntityId entityId)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            return entity;
        }

        QString EntityDisplayName(const AZ::EntityId entityId)
        {
            if (AZ::Entity* entity = FindEntity(entityId))
            {
                return QString::fromUtf8(entity->GetName().c_str());
            }
            return QStringLiteral("<unknown>");
        }

        void SetColorButtonSwatch(QPushButton* button, const QColor& color)
        {
            button->setStyleSheet(QStringLiteral("background-color: %1;").arg(color.name()));
        }

        QColor ToQColor(const AZ::Vector3& tint)
        {
            return QColor::fromRgbF(tint.GetX(), tint.GetY(), tint.GetZ());
        }

        QDoubleSpinBox* MakeSpin(double min, double max, double step, int decimals = 3)
        {
            auto* spin = new QDoubleSpinBox();
            spin->setRange(min, max);
            spin->setSingleStep(step);
            spin->setDecimals(decimals);
            return spin;
        }

        QWidget* MakeVec3Row(QDoubleSpinBox* (&spins)[3], double min, double max, double step)
        {
            auto* row = new QWidget();
            auto* layout = new QHBoxLayout(row);
            layout->setContentsMargins(0, 0, 0, 0);
            for (int i = 0; i < 3; ++i)
            {
                spins[i] = MakeSpin(min, max, step);
                layout->addWidget(spins[i]);
            }
            return row;
        }

        //! What a layer row carries beyond its name and check state.
        constexpr int LayerTintRole = Qt::UserRole + 1;
        constexpr int LayerCombineRole = Qt::UserRole + 2;

        //! Row metrics, in pixels at the list's own font size.
        constexpr int RowPadding = 6;
        constexpr int EyeWidth = 14;
        constexpr int SwatchWidth = 10;
        constexpr int RowGap = 7;
        constexpr int MinimumRowHeight = 24;

        //! The badge text for a layer's combine mode - the Combine combo's own labels.
        QString CombineModeLabel(const LayerCombineMode mode)
        {
            switch (mode)
            {
            case LayerCombineMode::Union:
                return QObject::tr("Union");
            case LayerCombineMode::Subtract:
                return QObject::tr("Subtract");
            case LayerCombineMode::Intersect:
                return QObject::tr("Intersect");
            case LayerCombineMode::Separate:
            default:
                return QObject::tr("Separate");
            }
        }

        //! Two offset squares, for the Duplicate button. Painted rather than typed: no copy glyph
        //! has dependable font coverage, and a missing one shows up as a box.
        QPixmap DuplicatePixmap(const QColor& color)
        {
            QPixmap pixmap(16, 16);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(color, 1.2));
            painter.drawRoundedRect(QRectF(2.0, 2.0, 8.0, 8.0), 1.5, 1.5);
            // Punch the front square out of the back one so they read as stacked, not crossed.
            painter.setCompositionMode(QPainter::CompositionMode_Clear);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::black);
            painter.drawRoundedRect(QRectF(4.6, 4.6, 9.8, 9.8), 2.0, 2.0);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.setPen(QPen(color, 1.2));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(QRectF(5.6, 5.6, 8.0, 8.0), 1.5, 1.5);
            return pixmap;
        }

        QIcon MakeDuplicateIcon(const QColor& normal, const QColor& accent)
        {
            QIcon icon(DuplicatePixmap(normal));
            icon.addPixmap(DuplicatePixmap(accent), QIcon::Active);
            return icon;
        }

        //! A square with an arrow leaving its corner, for Export Layer To Entity.
        QPixmap ExportPixmap(const QColor& color)
        {
            QPixmap pixmap(16, 16);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(color, 1.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
            QPainterPath box;
            box.moveTo(7.0, 4.0);
            box.lineTo(3.5, 4.0);
            box.quadTo(2.5, 4.0, 2.5, 5.0);
            box.lineTo(2.5, 12.5);
            box.quadTo(2.5, 13.5, 3.5, 13.5);
            box.lineTo(11.0, 13.5);
            box.quadTo(12.0, 13.5, 12.0, 12.5);
            box.lineTo(12.0, 9.0);
            painter.drawPath(box);
            painter.drawLine(QPointF(7.0, 9.0), QPointF(13.5, 2.5));
            painter.drawLine(QPointF(9.0, 2.5), QPointF(13.5, 2.5));
            painter.drawLine(QPointF(13.5, 2.5), QPointF(13.5, 7.0));
            return pixmap;
        }

        QIcon MakeExportIcon(const QColor& normal, const QColor& accent)
        {
            QIcon icon(ExportPixmap(normal));
            icon.addPixmap(ExportPixmap(accent), QIcon::Active);
            return icon;
        }

        //! Layer buttons painted explicitly (the tool-button style can drop their borders), from the palette.
        class LayerToolButton : public QToolButton
        {
        public:
            explicit LayerToolButton(AZStd::function<QIcon(const QColor&, const QColor&)> iconPainter = {})
                : m_iconPainter(AZStd::move(iconPainter))
            {
                setObjectName(QStringLiteral("whiteBoxLayerToolButton"));
                setAutoRaise(true);
                setAttribute(Qt::WA_Hover);
                setFocusPolicy(Qt::NoFocus);
                setFixedSize(26, 26);
                setIconSize(QSize(16, 16));
            }

        protected:
            void paintEvent(QPaintEvent* /*event*/) override
            {
                const auto group = isEnabled() ? QPalette::Active : QPalette::Disabled;
                const QColor foreground = palette().color(group, QPalette::Text);
                const QColor accent = palette().color(group, QPalette::Highlight);
                const bool hovered = isEnabled() && underMouse();
                const bool pressed = isEnabled() && (isDown() || isChecked());

                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing);
                const QRectF frame = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
                QColor border = foreground;
                border.setAlpha(isEnabled() ? 110 : 55);
                painter.setPen(Qt::NoPen);
                painter.setBrush(palette().brush(group, QPalette::Button));
                painter.drawRoundedRect(frame, 3.0, 3.0);
                if (hovered || pressed)
                {
                    QColor wash = accent;
                    wash.setAlpha(pressed ? 90 : 45);
                    painter.setBrush(wash);
                    painter.drawRoundedRect(frame, 3.0, 3.0);
                    border = accent;
                }
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(border, 1.0));
                painter.drawRoundedRect(frame, 3.0, 3.0);

                const QColor content = hovered || pressed ? accent : foreground;
                if (m_iconPainter)
                {
                    // Generate artwork from the live palette, including its disabled text colour.
                    // Using Normal avoids Qt applying an additional automatic greyscale treatment.
                    const QIcon artwork = m_iconPainter(content, accent);
                    const QRect iconRect(
                        (width() - iconSize().width()) / 2, (height() - iconSize().height()) / 2,
                        iconSize().width(), iconSize().height());
                    artwork.paint(&painter, iconRect, Qt::AlignCenter, QIcon::Normal);
                }
                else
                {
                    painter.setPen(content);
                    painter.drawText(rect(), Qt::AlignCenter, text());
                }
            }

        private:
            AZStd::function<QIcon(const QColor&, const QColor&)> m_iconPainter;
        };

        QRect EyeRect(const QRect& row)
        {
            return QRect(row.left() + RowPadding, row.center().y() - EyeWidth / 2, EyeWidth, EyeWidth);
        }

        //! An eye outline with a pupil, struck through when the layer is hidden. Drawn rather than
        //! loaded so it takes its colour from the editor theme, like everything else on the row.
        void DrawEye(QPainter* painter, const QRect& rect, const QColor& color, const bool open)
        {
            QPen pen(color, 1.2);
            pen.setCapStyle(Qt::RoundCap);
            painter->setPen(pen);
            painter->setBrush(Qt::NoBrush);

            const QPointF center = rect.center();
            const qreal halfWidth = rect.width() * 0.5;
            const qreal lidDepth = rect.height() * 0.6;
            QPainterPath eye;
            eye.moveTo(center.x() - halfWidth, center.y());
            eye.quadTo(center.x(), center.y() - lidDepth, center.x() + halfWidth, center.y());
            eye.quadTo(center.x(), center.y() + lidDepth, center.x() - halfWidth, center.y());
            painter->drawPath(eye);
            if (open)
            {
                painter->setBrush(color);
                painter->drawEllipse(center, halfWidth * 0.26, halfWidth * 0.26);
                painter->setBrush(Qt::NoBrush);
            }
            else
            {
                painter->drawLine(
                    QPointF(center.x() - halfWidth, center.y() + halfWidth),
                    QPointF(center.x() + halfWidth, center.y() - halfWidth));
            }
        }

        //! Paints a layer row: eye, tint swatch, name and combine-mode badge.
        class LayerRowDelegate : public QStyledItemDelegate
        {
        public:
            using QStyledItemDelegate::QStyledItemDelegate;

            QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
            {
                QSize size = QStyledItemDelegate::sizeHint(option, index);
                size.setHeight(AZStd::max(size.height(), MinimumRowHeight));
                return size;
            }

            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index)
                const override
            {
                QStyleOptionViewItem opt = option;
                initStyleOption(&opt, index);
                // The eye stands in for the check indicator, and the name is placed by hand so the
                // badge can share the row - leave the style the background and the selection only.
                opt.features &= ~QStyleOptionViewItem::HasCheckIndicator;
                opt.text.clear();
                opt.icon = QIcon();
                const QWidget* widget = opt.widget;
                QStyle* style = widget != nullptr ? widget->style() : QApplication::style();
                style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

                painter->save();
                painter->setRenderHint(QPainter::Antialiasing, true);

                const bool selected = (option.state & QStyle::State_Selected) != 0;
                const QColor textColor =
                    opt.palette.color(selected ? QPalette::HighlightedText : QPalette::Text);
                const bool visible = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;

                QColor eyeColor = textColor;
                eyeColor.setAlpha(visible ? 210 : 80);
                DrawEye(painter, EyeRect(option.rect), eyeColor, visible);

                int left = EyeRect(option.rect).right() + RowGap;
                const QColor tint = index.data(LayerTintRole).value<QColor>();
                if (tint.isValid())
                {
                    const QRect swatch(
                        left, option.rect.center().y() - SwatchWidth / 2, SwatchWidth, SwatchWidth);
                    painter->setPen(QPen(QColor(0, 0, 0, 90), 1.0));
                    painter->setBrush(tint);
                    painter->drawRoundedRect(QRectF(swatch).adjusted(0.5, 0.5, -0.5, -0.5), 2.0, 2.0);
                    left = swatch.right() + RowGap;
                }

                int right = option.rect.right() - RowPadding;
                const QString badge = index.data(LayerCombineRole).toString();
                if (!badge.isEmpty())
                {
                    const QFontMetrics metrics(opt.font);
                    const int badgeWidth = metrics.horizontalAdvance(badge) + 14;
                    const int badgeHeight = metrics.height() + 2;
                    const QRect badgeRect(
                        right - badgeWidth, option.rect.center().y() - badgeHeight / 2, badgeWidth,
                        badgeHeight);
                    QColor fill = textColor;
                    fill.setAlpha(selected ? 60 : 30);
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(fill);
                    painter->drawRoundedRect(badgeRect, 3.0, 3.0);
                    QColor badgeText = textColor;
                    badgeText.setAlpha(200);
                    painter->setPen(badgeText);
                    painter->setFont(opt.font);
                    painter->drawText(badgeRect, Qt::AlignCenter, badge);
                    right = badgeRect.left() - RowGap;
                }

                const QRect nameRect(
                    left, option.rect.top(), AZStd::max(right - left, 0), option.rect.height());
                QColor nameColor = textColor;
                if (!visible)
                {
                    nameColor.setAlpha(120); // a hidden layer reads as dimmed, like its eye
                }
                painter->setPen(nameColor);
                painter->setFont(opt.font);
                painter->drawText(
                    nameRect, Qt::AlignVCenter | Qt::AlignLeft,
                    QFontMetrics(opt.font).elidedText(
                        index.data(Qt::DisplayRole).toString(), Qt::ElideRight, nameRect.width()));
                painter->restore();
            }

            //! Clicking the eye toggles visibility, and never starts a rename.
            bool editorEvent(
                QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                const QModelIndex& index) override
            {
                if (event->type() == QEvent::MouseButtonRelease ||
                    event->type() == QEvent::MouseButtonPress ||
                    event->type() == QEvent::MouseButtonDblClick)
                {
                    auto* mouse = static_cast<QMouseEvent*>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                    const QPoint position = mouse->position().toPoint();
#else
                    const QPoint position = mouse->pos();
#endif
                    if (mouse->button() == Qt::LeftButton &&
                        EyeRect(option.rect).adjusted(-3, -3, 3, 3).contains(position))
                    {
                        if (event->type() == QEvent::MouseButtonRelease)
                        {
                            const bool visible = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
                            model->setData(
                                index, visible ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
                        }
                        return true;
                    }
                }
                return QStyledItemDelegate::editorEvent(event, model, option, index);
            }

            //! Rename in place over the name, not across the eye and the badge.
            void updateEditorGeometry(
                QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
            {
                QStyledItemDelegate::updateEditorGeometry(editor, option, index);
                const int left = EyeRect(option.rect).right() + RowGap + SwatchWidth + RowGap;
                editor->setGeometry(QRect(
                    left, option.rect.top(), AZStd::max(option.rect.right() - RowPadding - left, 40),
                    option.rect.height()));
            }
        };

        class LayerListWidget : public QListWidget
        {
        public:
            using QListWidget::QListWidget;

            //! Called with (fromRow, toRow) when the user drops a dragged layer.
            AZStd::function<void(int, int)> m_onRowMoved;

        protected:
            void dropEvent(QDropEvent* event) override
            {
                const int from = currentRow();

                // Where the drop indicator sits translates to an INSERTION index in the list.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QPoint dropPos = event->position().toPoint();
#else
                const QPoint dropPos = event->pos();
#endif
                int insertIndex = count();
                if (const QModelIndex index = indexAt(dropPos); index.isValid())
                {
                    switch (dropIndicatorPosition())
                    {
                    case QAbstractItemView::AboveItem:
                        insertIndex = index.row();
                        break;
                    case QAbstractItemView::BelowItem:
                    case QAbstractItemView::OnItem:
                        insertIndex = index.row() + 1;
                        break;
                    default:
                        break;
                    }
                }

                // Removing the dragged row first shifts everything after it down by one.
                const int to = (insertIndex > from) ? insertIndex - 1 : insertIndex;

                // Swallow the event so the base class never touches the model.
                event->setDropAction(Qt::IgnoreAction);
                event->accept();

                if (m_onRowMoved && from >= 0 && to >= 0 && to != from)
                {
                    m_onRowMoved(from, to);
                }
            }
        };
    } // namespace

    WhiteBoxPaneWidget::WhiteBoxPaneWidget(QWidget* parent)
        : QScrollArea(parent)
    {
        auto* container = new QWidget();
        auto* mainLayout = new QVBoxLayout(container);
        mainLayout->setContentsMargins(4, 4, 4, 4);
        mainLayout->setSpacing(6);

        m_layerGizmo = AZStd::make_unique<WhiteBoxLayerGizmo>();
        m_entityGizmo = AZStd::make_unique<WhiteBoxEntityGizmo>();

        // The pane holds the layer stack; other settings live on the component card and the viewport tools.
        mainLayout->addWidget(BuildEntitySection());
        mainLayout->addWidget(BuildEntityTransformSection());
        mainLayout->addWidget(BuildModeSection());
        mainLayout->addWidget(BuildLayersSection());
        mainLayout->addWidget(BuildViewportHandlesSection());
        mainLayout->addStretch();

        setWidget(container);
        setWidgetResizable(true);

        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusConnect();

        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            editorEntityContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);
        AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(editorEntityContextId);
        AzToolsFramework::FocusModeNotificationBus::Handler::BusConnect(editorEntityContextId);

        RefreshEntityList();
        RefreshFromComponent();
    }

    WhiteBoxPaneWidget::~WhiteBoxPaneWidget()
    {
        RemoveEntityGizmoCluster();
        m_entityGizmo.reset();
        m_layerGizmo.reset();
        EditorWhiteBoxComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::FocusModeNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    }

    // ---- UI construction ------------------------------------------------------------------

    QWidget* WhiteBoxPaneWidget::BuildEntitySection()
    {
        auto* group = new QGroupBox(tr("Entity"));
        auto* layout = new QVBoxLayout(group);

        auto* comboRow = new QHBoxLayout();
        m_entityCombo = new QComboBox();
        m_entityCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        comboRow->addWidget(m_entityCombo);
        auto* refreshButton = new QPushButton(tr("Refresh"));
        refreshButton->setToolTip(tr("Re-scan the level for entities with a White Box component."));
        comboRow->addWidget(refreshButton);
        layout->addLayout(comboRow);

        auto* buttonRow = new QHBoxLayout();
        m_newEntityButton = new QPushButton(tr("New Entity"));
        m_newEntityButton->setToolTip(tr("Create a new entity with a White Box component and start editing it."));
        m_newChildButton = new QPushButton(tr("New Layer (Child)"));
        m_newChildButton->setToolTip(
            tr("Create a child entity with its own White Box component under the current one and switch edit "
               "focus to it (same as the old component button)."));
        m_editButton = new QPushButton(tr("Edit"));
        m_editButton->setToolTip(tr("Select the entity and enter White Box edit mode."));
        m_doneButton = new QPushButton(tr("Done"));
        m_doneButton->setToolTip(tr("Leave White Box edit mode."));
        buttonRow->addWidget(m_newEntityButton);
        buttonRow->addWidget(m_newChildButton);
        buttonRow->addWidget(m_editButton);
        buttonRow->addWidget(m_doneButton);
        layout->addLayout(buttonRow);
        m_convertMeshButton = new QPushButton(tr("Convert Selected Mesh"));
        m_convertMeshButton->setToolTip(
            tr("Replace the selected entity's Mesh component with a White Box built from its model, keeping each slot's "
               "material and smoothing curved areas. One undo step."));
        layout->addWidget(m_convertMeshButton);

        connect(
            m_entityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &WhiteBoxPaneWidget::OnEntityComboChanged);
        connect(refreshButton, &QPushButton::clicked, this,
            [this]()
            {
                RefreshEntityList();
                RefreshFromComponent();
            });
        connect(m_newEntityButton, &QPushButton::clicked, this, [this]() { CreateWhiteBoxEntity(false); });
        connect(m_newChildButton, &QPushButton::clicked, this, [this]() { CreateWhiteBoxEntity(true); });
        connect(m_convertMeshButton, &QPushButton::clicked, this,
            [this]()
            {
                AzToolsFramework::EntityIdList selected;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                    selected, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
                if (selected.size() != 1)
                {
                    QMessageBox::information(this, tr("Convert Mesh"), tr("Select one entity with a Mesh component."));
                    return;
                }
                AZStd::string message;
                if (!ConvertMeshEntityToWhiteBox(selected.front(), message))
                {
                    QMessageBox::warning(this, tr("Convert Mesh"), QString::fromUtf8(message.c_str()));
                    return;
                }
                AZ_Printf("White Box", "%s", message.c_str());
                RefreshEntityList();
                SetCurrentEntity(selected.front());
                RefreshFromComponent();
            });
        connect(m_editButton, &QPushButton::clicked, this,
            [this]()
            {
                const AZ::EntityId entityId = m_currentEntityId;
                if (!entityId.IsValid())
                {
                    return;
                }
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
                    AzToolsFramework::EntityIdList{ entityId });
                // Enter component mode on the next tick so the selection has fully applied first.
                AZ::TickBus::QueueFunction(
                    [entityId]()
                    {
                        if (AZ::Entity* entity = FindEntity(entityId))
                        {
                            if (auto* whiteBox = entity->FindComponent<EditorWhiteBoxComponent>())
                            {
                                whiteBox->EnterComponentMode();
                            }
                        }
                    });
            });
        connect(m_doneButton, &QPushButton::clicked, this,
            []()
            {
                namespace Cmf = AzToolsFramework::ComponentModeFramework;
                Cmf::ComponentModeSystemRequestBus::Broadcast(&Cmf::ComponentModeSystemRequests::EndComponentMode);
            });

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildEntityTransformSection()
    {
        auto* group = new QGroupBox(tr("Entity Transform"));
        auto* layout = new QFormLayout(group);

        // Space the Position/Rotation values are shown and edited in. "Parent" is the entity's
        // local transform (what the Transform component shows); "World" is absolute.
        m_entitySpaceCombo = new QComboBox();
        m_entitySpaceCombo->addItem(tr("Parent (Local)"));
        m_entitySpaceCombo->addItem(tr("World"));
        layout->addRow(tr("Space"), m_entitySpaceCombo);

        layout->addRow(tr("Position"), MakeVec3Row(m_entityPos, -100000.0, 100000.0, 0.1));
        layout->addRow(tr("Rotation"), MakeVec3Row(m_entityRot, -3600.0, 3600.0, 1.0));
        m_entityScale = MakeSpin(0.001, 1000.0, 0.1);
        layout->addRow(tr("Uniform Scale"), m_entityScale);
        auto* nonUniformRow = MakeVec3Row(m_entityNonUniformScale, 0.001, 1000.0, 0.1);
        for (QDoubleSpinBox* spin : m_entityNonUniformScale)
        {
            spin->setToolTip(
                tr("Per-axis (non-uniform) scale for the whole entity. Setting a non-(1,1,1) value adds an "
                   "O3DE Non-Uniform Scale component if the entity doesn't already have one; it also drives the "
                   "transform Scale gizmo. Multiplies with Uniform Scale above."));
        }
        layout->addRow(tr("Non-Uniform Scale"), nonUniformRow);

        // Viewport gizmo driving the entity transform - available even during component mode
        // (when the editor's built-in entity gizmo is suppressed).
        auto* gizmoRow = new QWidget();
        auto* gizmoLayout = new QHBoxLayout(gizmoRow);
        gizmoLayout->setContentsMargins(0, 0, 0, 0);
        const auto makeEntityGizmoButton =
            [this, gizmoLayout](const QString& label, WhiteBoxEntityGizmo::Mode mode) -> QPushButton*
        {
            auto* button = new QPushButton(label);
            button->setCheckable(true);
            button->setAutoExclusive(true);
            gizmoLayout->addWidget(button);
            connect(button, &QPushButton::clicked, this,
                [this, mode]()
                {
                    m_entityGizmo->SetTarget(m_currentEntityId);
                    m_entityGizmo->SetMode(mode);
                });
            return button;
        };
        m_entityGizmoOff = makeEntityGizmoButton(tr("Off"), WhiteBoxEntityGizmo::Mode::None);
        m_entityGizmoMove = makeEntityGizmoButton(tr("Move"), WhiteBoxEntityGizmo::Mode::Translate);
        m_entityGizmoRotate = makeEntityGizmoButton(tr("Rotate"), WhiteBoxEntityGizmo::Mode::Rotate);
        m_entityGizmoScale = makeEntityGizmoButton(tr("Scale"), WhiteBoxEntityGizmo::Mode::Scale);
        m_entityGizmoOff->setChecked(true);
        layout->addRow(tr("Gizmo"), gizmoRow);
        m_entityGizmo->SetChangedCallback([this]() { RefreshFromComponent(); });

        connect(
            m_entitySpaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int)
            {
                if (!m_updating)
                {
                    UpdateEntityTransformUi(); // re-read the values in the newly chosen space
                }
            });

        const auto applyEntityTransform = [this]()
        {
            if (m_updating || !m_currentEntityId.IsValid())
            {
                return;
            }
            const bool world = m_entitySpaceCombo->currentIndex() == 1;
            const AZ::Vector3 position(
                aznumeric_cast<float>(m_entityPos[0]->value()), aznumeric_cast<float>(m_entityPos[1]->value()),
                aznumeric_cast<float>(m_entityPos[2]->value()));
            const AZ::Quaternion rotation = AZ::Quaternion::CreateFromEulerAnglesDegrees(AZ::Vector3(
                aznumeric_cast<float>(m_entityRot[0]->value()), aznumeric_cast<float>(m_entityRot[1]->value()),
                aznumeric_cast<float>(m_entityRot[2]->value())));
            const float scale = aznumeric_cast<float>(m_entityScale->value());

            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Entity Transform");
            if (world)
            {
                AZ::TransformBus::Event(
                    m_currentEntityId, &AZ::TransformBus::Events::SetWorldTranslation, position);
                AZ::TransformBus::Event(
                    m_currentEntityId, &AZ::TransformBus::Events::SetWorldRotationQuaternion, rotation);
            }
            else
            {
                AZ::TransformBus::Event(
                    m_currentEntityId, &AZ::TransformBus::Events::SetLocalTranslation, position);
                AZ::TransformBus::Event(
                    m_currentEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, rotation);
            }
            AZ::TransformBus::Event(m_currentEntityId, &AZ::TransformBus::Events::SetLocalUniformScale, scale);
            undoBatch.MarkEntityDirty(m_currentEntityId);
        };
        for (QDoubleSpinBox* spin :
             { m_entityPos[0], m_entityPos[1], m_entityPos[2], m_entityRot[0], m_entityRot[1], m_entityRot[2],
               m_entityScale })
        {
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, applyEntityTransform);
        }

        // Non-uniform scale goes through its own component, added the first time it is not (1,1,1).
        const auto applyNonUniformScale = [this]()
        {
            if (m_updating || !m_currentEntityId.IsValid())
            {
                return;
            }
            const AZ::Vector3 scale(
                aznumeric_cast<float>(m_entityNonUniformScale[0]->value()),
                aznumeric_cast<float>(m_entityNonUniformScale[1]->value()),
                aznumeric_cast<float>(m_entityNonUniformScale[2]->value()));

            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Non-Uniform Scale");
            if (!AZ::NonUniformScaleRequestBus::HasHandlers(m_currentEntityId) &&
                !scale.IsClose(AZ::Vector3::CreateOne()))
            {
                AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome outcome =
                    AZ::Failure(AZStd::string("uninitialized"));
                const AzToolsFramework::EntityIdList entities{ m_currentEntityId };
                const AZ::ComponentTypeList types{
                    azrtti_typeid<AzToolsFramework::Components::EditorNonUniformScaleComponent>()
                };
                AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
                    outcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entities, types);
            }
            AZ::NonUniformScaleRequestBus::Event(m_currentEntityId, &AZ::NonUniformScaleRequests::SetScale, scale);
            undoBatch.MarkEntityDirty(m_currentEntityId);
        };
        for (QDoubleSpinBox* spin : m_entityNonUniformScale)
        {
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, applyNonUniformScale);
        }

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildModeSection()
    {
        auto* group = new QGroupBox(tr("Tool Mode"));
        auto* layout = new QHBoxLayout(group);

        const auto makeModeButton = [this, layout](const QString& label, SubMode subMode) -> QPushButton*
        {
            auto* button = new QPushButton(label);
            button->setCheckable(true);
            button->setAutoExclusive(true);
            layout->addWidget(button);
            connect(button, &QPushButton::clicked, this,
                [this, subMode]()
                {
                    EditorWhiteBoxComponent* component = CurrentComponent();
                    if (component == nullptr)
                    {
                        return;
                    }

                    namespace Cmf = AzToolsFramework::ComponentModeFramework;
                    bool inComponentMode = false;
                    Cmf::ComponentModeSystemRequestBus::BroadcastResult(
                        inComponentMode, &Cmf::ComponentModeSystemRequests::InComponentMode);

                    if (inComponentMode)
                    {
                        EditorWhiteBoxComponentModeRequestBus::Event(
                            AZ::EntityComponentIdPair(m_currentEntityId, component->GetId()),
                            &EditorWhiteBoxComponentModeRequests::SetSubMode, subMode);
                        return;
                    }

                    // Not editing yet: select the entity, enter White Box edit mode, then apply
                    // the requested sub-mode (deferred a tick so the mode exists first).
                    const AZ::EntityId entityId = m_currentEntityId;
                    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
                        AzToolsFramework::EntityIdList{ entityId });
                    AZ::TickBus::QueueFunction(
                        [entityId, subMode]()
                        {
                            if (AZ::Entity* entity = FindEntity(entityId))
                            {
                                if (auto* whiteBox = entity->FindComponent<EditorWhiteBoxComponent>())
                                {
                                    whiteBox->EnterComponentMode();
                                    EditorWhiteBoxComponentModeRequestBus::Event(
                                        AZ::EntityComponentIdPair(entityId, whiteBox->GetId()),
                                        &EditorWhiteBoxComponentModeRequests::SetSubMode, subMode);
                                }
                            }
                        });
                });
            return button;
        };

        m_modeSketch = makeModeButton(tr("Sketch"), SubMode::Default);
        m_modeEdgeRestore = makeModeButton(tr("Edge Restore"), SubMode::EdgeRestore);
        m_modeTransform = makeModeButton(tr("Transform"), SubMode::Transform);
        m_modeDrawShape = makeModeButton(tr("Draw Shape"), SubMode::DrawShape);
        m_modePaint = makeModeButton(tr("Vertex Paint"), SubMode::VertexPaint);
        m_modeTransform->setToolTip(tr(
            "Click a vertex, edge, or polygon to select it. Ctrl-click adds or removes elements of the same type. "
            "Selected elements transform together around their shared center."));

        return group;
    }


    namespace
    {
        constexpr const char* ViewportHandlesObjectName = "WhiteBoxViewportHandlesSection";
    }

    QWidget* WhiteBoxPaneWidget::BuildViewportHandlesSection()
    {
        auto* group = new QGroupBox(tr("Viewport Handles"));
        group->setObjectName(ViewportHandlesObjectName);
        auto* layout = new QFormLayout(group);

        // One spin box per setting, in step with its console variable: a change applies at once and is saved for
        // the next session. The boxes are kept so Reset can put them back without writing the values twice.
        auto spinBoxes = AZStd::make_shared<AZStd::vector<QDoubleSpinBox*>>();
        for (const ViewportHandleSetting& setting : ViewportHandleSettings())
        {
            auto* spin = new QDoubleSpinBox();
            spin->setDecimals(3);
            spin->setRange(setting.m_min, setting.m_max);
            spin->setSingleStep(setting.m_step);
            spin->setValue(setting.m_get());
            spin->setToolTip(tr(setting.m_tooltip));
            layout->addRow(tr(setting.m_label), spin);
            spinBoxes->push_back(spin);

            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [settingPtr = &setting](const double value)
                {
                    settingPtr->m_set(aznumeric_cast<float>(value));
                    SaveViewportHandleSettings();
                });
        }

        auto* resetButton = new QPushButton(tr("Reset to Defaults"));
        layout->addRow(resetButton);
        connect(resetButton, &QPushButton::clicked, this,
            [spinBoxes]()
            {
                ResetViewportHandleSettings();
                const auto settings = ViewportHandleSettings();
                for (size_t i = 0; i < settings.size() && i < spinBoxes->size(); ++i)
                {
                    const QSignalBlocker blocker((*spinBoxes)[i]); // the values were just reset and cleared from the saved set
                    (*spinBoxes)[i]->setValue(settings[i].m_get());
                }
            });

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildLayersSection()
    {
        auto* group = new QGroupBox(tr("Layers"));
        auto* layout = new QVBoxLayout(group);

        auto* activeRow = new QFormLayout();
        m_activeLayerCombo = new QComboBox();
        m_activeLayerCombo->setToolTip(tr("Which layer edits (draw / stamp / carve) target."));
        activeRow->addRow(tr("Active Layer"), m_activeLayerCombo);
        layout->addLayout(activeRow);

        // Create Parametric Shape: one click adds a new parametric layer generating that shape
        // (made active + selected); its parameters are then edited live in "Selected Layer".
        auto* createGrid = new QGridLayout();
        const AZStd::pair<const char*, DrawShapeType> shapeDefs[] = {
            { "Box", DrawShapeType::Box },           { "Cylinder", DrawShapeType::Cylinder },
            { "Pyramid", DrawShapeType::Pyramid },   { "Cone", DrawShapeType::Cone },
            { "Sphere", DrawShapeType::Sphere },     { "Staircase", DrawShapeType::Staircase },
            { "Room", DrawShapeType::Room },
            { "Door", DrawShapeType::Door }, { "Circular Stairs", DrawShapeType::CircularStairs },
            { "Plane", DrawShapeType::Plane }, { "Torus", DrawShapeType::Torus }, { "Pipe", DrawShapeType::Pipe },
        };
        int shapeButtonIndex = 0;
        for (const auto& [label, shapeType] : shapeDefs)
        {
            auto* button = new QPushButton(tr(label));
            createGrid->addWidget(button, shapeButtonIndex / 3, shapeButtonIndex % 3);
            ++shapeButtonIndex;
            connect(button, &QPushButton::clicked, this,
                [this, shapeType]()
                {
                    ModifyComponent(
                        "Add Parametric Shape",
                        [shapeType](EditorWhiteBoxComponent* c) { c->AddParametricShapeLayer(shapeType); });
                    // Move the list selection to the freshly added (now active) layer.
                    if (EditorWhiteBoxComponent* component = CurrentComponent())
                    {
                        m_layerList->setCurrentRow(component->GetActiveLayerIndex());
                    }
                });
        }
        auto* createGroup = new QGroupBox(tr("Create Parametric Shape"));
        createGroup->setLayout(createGrid);
        layout->addWidget(createGroup);

        // One compact strip above the list rather than two rows of wide buttons: these actions are
        // all about the list right below them, and the count belongs with it too.
        const QString moveTip =
            tr("Reorder the selected layer. Layer order is the order the Combine modes accumulate "
               "in, so moving a layer restacks its boolean.");
        auto* buttonRow = new QHBoxLayout();
        buttonRow->setSpacing(2);
        const auto makeToolButton = [buttonRow](const QString& glyph, const QString& tip)
        {
            auto* button = new LayerToolButton();
            button->setText(glyph);
            button->setToolTip(tip);
            buttonRow->addWidget(button);
            return button;
        };
        auto* newLayerButton = makeToolButton(QStringLiteral("+"), tr("Add a new layer."));
        auto* deleteLayerButton = makeToolButton(QString(QChar(0x2212)), tr("Delete the selected layer."));
        auto* duplicateLayerButton = new LayerToolButton(&MakeDuplicateIcon);
        duplicateLayerButton->setToolTip(tr("Duplicate the selected layer, geometry and settings alike."));
        buttonRow->addWidget(duplicateLayerButton);
        auto* exportLayerButton = new LayerToolButton(&MakeExportIcon);
        exportLayerButton->setToolTip(
            tr("Move the selected layer to a new entity with the same transform and settings. "
               "Shift+click to copy it instead."));
        buttonRow->addWidget(exportLayerButton);
        auto* moveUpButton = makeToolButton(QString(QChar(0x2191)), moveTip);
        auto* moveDownButton = makeToolButton(QString(QChar(0x2193)), moveTip);
        buttonRow->addStretch();
        m_layerCountLabel = new QLabel();
        m_layerCountLabel->setEnabled(false); // the theme's secondary text, which is what a count is
        buttonRow->addWidget(m_layerCountLabel);
        layout->addLayout(buttonRow);

        auto* layerList = new LayerListWidget();
        m_layerList = layerList;
        m_layerList->setItemDelegate(new LayerRowDelegate(m_layerList));
        m_layerList->setUniformItemSizes(true);
        m_layerList->setToolTip(
            tr("All layers, in combine order. Double-click to rename; use the checkbox to "
               "show/hide; drag a layer (or use Move Up / Move Down) to reorder."));
        // The list grows to show EVERY layer (the pane itself scrolls); no inner scrollbar.
        m_layerList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        // Drag to reorder. The drop is intercepted by LayerListWidget and applied to the
        // component (which owns the order); the widget is then rebuilt from it.
        m_layerList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_layerList->setDragEnabled(true);
        m_layerList->viewport()->setAcceptDrops(true);
        m_layerList->setDropIndicatorShown(true);
        m_layerList->setDragDropMode(QAbstractItemView::InternalMove);
        m_layerList->setDefaultDropAction(Qt::MoveAction);
        // Rename on double-click / F2 only - a single click on an already-selected layer must
        // start a drag, not an inline rename.
        m_layerList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        layerList->m_onRowMoved = [this](int from, int to)
        {
            MoveLayerRow(from, to);
        };
        layout->addWidget(m_layerList);

        m_layerMetaGroup = new QGroupBox(tr("Selected Layer"));
        auto* metaLayout = new QFormLayout(m_layerMetaGroup);
        m_layerTintButton = new QPushButton();
        m_layerTintButton->setToolTip(tr("Render colour for this layer (used when 'Use Global Tint' is off)."));
        metaLayout->addRow(tr("Tint"), m_layerTintButton);
        m_layerCombineCombo = new QComboBox();
        m_layerCombineCombo->addItem(tr("Separate"), static_cast<int>(LayerCombineMode::Separate));
        m_layerCombineCombo->addItem(tr("Union"), static_cast<int>(LayerCombineMode::Union));
        m_layerCombineCombo->addItem(tr("Subtract"), static_cast<int>(LayerCombineMode::Subtract));
        m_layerCombineCombo->addItem(tr("Intersect"), static_cast<int>(LayerCombineMode::Intersect));
        metaLayout->addRow(tr("Combine"), m_layerCombineCombo);
        m_layerInvertNormals = new QCheckBox(tr("Invert Normals"));
        m_layerInvertNormals->setToolTip(
            tr("Flip this layer's winding so it renders, collides and selects inside-out "
               "(e.g. a room built from an inverted box collides from the inside)."));
        metaLayout->addRow(QString(), m_layerInvertNormals);
        m_layerEdgesOnly = new QCheckBox(tr("Edges Only"));
        m_layerEdgesOnly->setToolTip(
            tr("Hide this layer's solid faces and draw only its edges (visual only - collision "
               "and selection are unaffected)."));
        metaLayout->addRow(QString(), m_layerEdgesOnly);
        m_layerCollision = new QCheckBox(tr("Collision"));
        m_layerCollision->setToolTip(tr("Include this layer in the physics collision mesh."));
        metaLayout->addRow(QString(), m_layerCollision);
        metaLayout->addRow(tr("Position"), MakeVec3Row(m_layerPos, -100000.0, 100000.0, 0.1));
        metaLayout->addRow(tr("Rotation"), MakeVec3Row(m_layerRot, -3600.0, 3600.0, 1.0));
        metaLayout->addRow(tr("Scale"), MakeVec3Row(m_layerScale, 0.001, 1000.0, 0.1));

        // Modifiers: live copies of the layer, in layer space before the transform. Edit one side; the rest follows.
        auto* mirrorRow = new QWidget();
        auto* mirrorLayout = new QHBoxLayout(mirrorRow);
        mirrorLayout->setContentsMargins(0, 0, 0, 0);
        const char* mirrorAxes[3] = { "X", "Y", "Z" };
        for (int axis = 0; axis < 3; ++axis)
        {
            m_layerMirror[axis] = new QCheckBox(QString::fromLatin1(mirrorAxes[axis]));
            m_layerMirror[axis]->setToolTip(
                tr("Mirror the layer across its %1 = 0 plane, or across its low side when it straddles that plane (a centred "
                   "shape). Faces on the plane are dropped so the halves close into one shell; edit the original side and the "
                   "reflection follows.").arg(QString::fromLatin1(mirrorAxes[axis])));
            mirrorLayout->addWidget(m_layerMirror[axis]);
        }
        mirrorLayout->addStretch();
        metaLayout->addRow(tr("Mirror"), mirrorRow);
        m_layerArrayCount = new QSpinBox();
        m_layerArrayCount->setRange(1, 256);
        m_layerArrayCount->setToolTip(tr("Copies of the layer in a row (1 = off). Fences, pillars, steps, windows."));
        metaLayout->addRow(tr("Array Count"), m_layerArrayCount);
        metaLayout->addRow(tr("Array Offset"), MakeVec3Row(m_layerArrayOffset, -100000.0, 100000.0, 0.1));
        auto* applyModifiersButton = new QPushButton(tr("Apply Modifiers"));
        applyModifiersButton->setToolTip(
            tr("Bake the active layer's mirror and array into its geometry and switch them off, so the copies can be edited one by one."));
        metaLayout->addRow(QString(), applyModifiersButton);
        connect(applyModifiersButton, &QPushButton::clicked, this,
            [this]()
            {
                ModifyComponent(
                    "White Box Apply Layer Modifiers",
                    [](EditorWhiteBoxComponent* c) { c->ApplyActiveLayerModifiers(); });
            });

        auto* applyTransformButton = new QPushButton(tr("Apply Transform"));
        applyTransformButton->setToolTip(
            tr("Bake the active layer's Position/Rotation/Scale into its geometry and reset them to identity."));
        metaLayout->addRow(QString(), applyTransformButton);

        // Viewport gizmo for the selected layer's transform (same manipulators as the editor's
        // Transform component, but driving the layer's non-destructive Position/Rotation/Scale).
        auto* gizmoRow = new QWidget();
        auto* gizmoLayout = new QHBoxLayout(gizmoRow);
        gizmoLayout->setContentsMargins(0, 0, 0, 0);
        const auto makeGizmoButton =
            [this, gizmoLayout](const QString& label, WhiteBoxLayerGizmo::Mode mode) -> QPushButton*
        {
            auto* button = new QPushButton(label);
            button->setCheckable(true);
            button->setAutoExclusive(true);
            gizmoLayout->addWidget(button);
            connect(button, &QPushButton::clicked, this,
                [this, mode]()
                {
                    m_layerGizmo->SetTarget(m_currentEntityId, m_layerList->currentRow());
                    m_layerGizmo->SetMode(mode);
                });
            return button;
        };
        m_gizmoOff = makeGizmoButton(tr("Off"), WhiteBoxLayerGizmo::Mode::None);
        m_gizmoMove = makeGizmoButton(tr("Move"), WhiteBoxLayerGizmo::Mode::Translate);
        m_gizmoRotate = makeGizmoButton(tr("Rotate"), WhiteBoxLayerGizmo::Mode::Rotate);
        m_gizmoScale = makeGizmoButton(tr("Scale"), WhiteBoxLayerGizmo::Mode::Scale);
        m_gizmoOff->setChecked(true);
        metaLayout->addRow(tr("Gizmo"), gizmoRow);

        // Parametric shapes update live as their values change.
        m_shapeParamsGroup = new QGroupBox(tr("Shape Parameters"));
        auto* shapeLayout = new QFormLayout(m_shapeParamsGroup);
        m_shapeParamShape = new QComboBox();
        m_shapeParamShape->addItem(tr("Box"), static_cast<int>(DrawShapeType::Box));
        m_shapeParamShape->addItem(tr("Cylinder"), static_cast<int>(DrawShapeType::Cylinder));
        m_shapeParamShape->addItem(tr("Pyramid"), static_cast<int>(DrawShapeType::Pyramid));
        m_shapeParamShape->addItem(tr("Cone"), static_cast<int>(DrawShapeType::Cone));
        m_shapeParamShape->addItem(tr("Sphere"), static_cast<int>(DrawShapeType::Sphere));
        m_shapeParamShape->addItem(tr("Staircase"), static_cast<int>(DrawShapeType::Staircase));
        m_shapeParamShape->addItem(tr("Room"), static_cast<int>(DrawShapeType::Room));
        m_shapeParamShape->addItem(tr("Door"), static_cast<int>(DrawShapeType::Door));
        m_shapeParamShape->addItem(tr("Circular Stairs"), static_cast<int>(DrawShapeType::CircularStairs));
        m_shapeParamShape->addItem(tr("Plane"), static_cast<int>(DrawShapeType::Plane));
        m_shapeParamShape->addItem(tr("Torus"), static_cast<int>(DrawShapeType::Torus));
        m_shapeParamShape->addItem(tr("Pipe"), static_cast<int>(DrawShapeType::Pipe));
        shapeLayout->addRow(tr("Shape"), m_shapeParamShape);
        m_shapeParamWidth = MakeSpin(0.01, 10000.0, 0.1);
        shapeLayout->addRow(tr("Width"), m_shapeParamWidth);
        m_shapeParamDepth = MakeSpin(0.01, 10000.0, 0.1);
        m_shapeParamDepth->setToolTip(tr("Extent along local Y. Stretch a Sphere's Height (or Depth) to make a bullet."));
        shapeLayout->addRow(tr("Depth"), m_shapeParamDepth);
        m_shapeParamHeight = MakeSpin(0.01, 10000.0, 0.1);
        shapeLayout->addRow(tr("Height"), m_shapeParamHeight);
        m_shapeParamSides = new QSpinBox();
        m_shapeParamSides->setRange(3, 128);
        m_shapeParamSidesLabel = new QLabel(tr("Sides"));
        shapeLayout->addRow(m_shapeParamSidesLabel, m_shapeParamSides);
        m_shapeParamSteps = new QSpinBox();
        m_shapeParamSteps->setRange(1, 128);
        m_shapeParamStepsLabel = new QLabel(tr("Steps"));
        shapeLayout->addRow(m_shapeParamStepsLabel, m_shapeParamSteps);
        m_shapeParamStepsByHeight = new QCheckBox(tr("Divide By Step Height"));
        shapeLayout->addRow(QString(), m_shapeParamStepsByHeight);
        m_shapeParamStepHeight = MakeSpin(0.001, 10000.0, 0.05);
        m_shapeParamStepHeight->setToolTip(tr(
            "Derives the nearest whole step count from Height / Step Height (1?128 steps), "
            "then fits equal risers to the total height, like the staircase Draw tool."));
        m_shapeParamStepHeightLabel = new QLabel(tr("Step Height"));
        shapeLayout->addRow(m_shapeParamStepHeightLabel, m_shapeParamStepHeight);
        // Room-only controls. Width/Depth/Height above are the interior dimensions for a Room.
        m_shapeParamWallThickness = MakeSpin(0.001, 100.0, 0.05);
        m_shapeParamWallThickness->setToolTip(tr("Thickness of each wall leaf (inner and outer)."));
        m_shapeParamWallThicknessLabel = new QLabel(tr("Wall Thickness"));
        shapeLayout->addRow(m_shapeParamWallThicknessLabel, m_shapeParamWallThickness);
        m_shapeParamCavityGap = MakeSpin(0.0, 100.0, 0.05);
        m_shapeParamCavityGap->setToolTip(tr("Empty gap between the inner and outer wall leaves (0 = single solid wall)."));
        m_shapeParamCavityGapLabel = new QLabel(tr("Cavity Gap"));
        shapeLayout->addRow(m_shapeParamCavityGapLabel, m_shapeParamCavityGap);
        m_shapeParamFloor = new QCheckBox(tr("Floor"));
        m_shapeParamFloor->setToolTip(tr("Add a floor slab beneath the interior."));
        shapeLayout->addRow(QString(), m_shapeParamFloor);
        m_shapeParamCeiling = new QCheckBox(tr("Ceiling"));
        m_shapeParamCeiling->setToolTip(tr("Add a ceiling slab above the interior."));
        shapeLayout->addRow(QString(), m_shapeParamCeiling);
        m_shapeParamDoorFrame = new QCheckBox(tr("Doorway Frame (open)"));
        m_shapeParamDoorFrame->setToolTip(tr("Uncheck to create a solid door panel. Width and height describe the opening when checked."));
        shapeLayout->addRow(QString(), m_shapeParamDoorFrame);
        m_shapeParamArchHeight = MakeSpin(0.0, 10000.0, 0.05);
        m_shapeParamArchHeight->setToolTip(tr("Zero gives a rectangular door. Increase for an elliptical arch; half the width gives a semicircle."));
        shapeLayout->addRow(tr("Arch Height"), m_shapeParamArchHeight);
        m_shapeParamInnerRadius = MakeSpin(0.01, 10000.0, 0.1);
        shapeLayout->addRow(tr("Inner Radius"), m_shapeParamInnerRadius);
        m_shapeParamSweepAngle = MakeSpin(1.0, 360.0, 15.0);
        m_shapeParamSweepAngle->setToolTip(tr("Counterclockwise sweep around local Z. 360 degrees creates a full circular stairway."));
        shapeLayout->addRow(tr("Sweep Angle (degrees)"), m_shapeParamSweepAngle);
        m_shapeParamHoleRatio = MakeSpin(0.05, 0.95, 0.05);
        m_shapeParamHoleRatio->setToolTip(tr("Hole diameter as a fraction of the outer diameter. Higher values make a thinner wall or tube."));
        shapeLayout->addRow(tr("Hole Ratio"), m_shapeParamHoleRatio);
        m_shapeParamTubeSides = new QSpinBox();
        m_shapeParamTubeSides->setRange(MinTubeSides, MaxTubeSides);
        m_shapeParamTubeSides->setToolTip(
            tr("Segments around the torus tube's cross-section. A torus has Sides x Tube Sides quads, so this "
               "is the other half of its triangle count - raise Sides for a rounder ring, this for a rounder tube."));
        m_shapeParamTubeSidesLabel = new QLabel(tr("Tube Sides"));
        shapeLayout->addRow(m_shapeParamTubeSidesLabel, m_shapeParamTubeSides);
        m_bakeShapeButton = new QPushButton(tr("Bake To Mesh"));
        m_bakeShapeButton->setToolTip(
            tr("Freeze this parametric shape into an ordinary mesh layer so vertex-level edits are safe "
               "(the parameters stop driving it)."));
        shapeLayout->addRow(m_bakeShapeButton);
        metaLayout->addRow(m_shapeParamsGroup);

        // Reads the controls. Returns false when there is no layer to apply them to; `changed` reports
        // whether they actually differ from what the component already holds.
        const auto readShapeParams = [this](int& row, EditorWhiteBoxComponent::ShapeParams& params, bool& changed)
        {
            row = m_layerList->currentRow();
            if (row < 0)
            {
                return false;
            }
            params = EditorWhiteBoxComponent::ShapeParams{};
            params.m_shape = static_cast<DrawShapeType>(m_shapeParamShape->currentData().toInt());
            params.m_width = aznumeric_cast<float>(m_shapeParamWidth->value());
            params.m_depth = aznumeric_cast<float>(m_shapeParamDepth->value());
            params.m_height = aznumeric_cast<float>(m_shapeParamHeight->value());
            params.m_sides = m_shapeParamSides->value();
            params.m_steps = m_shapeParamSteps->value();
            params.m_stepsByHeight = m_shapeParamStepsByHeight->isChecked();
            params.m_stepHeight = aznumeric_cast<float>(m_shapeParamStepHeight->value());
            params.m_wallThickness = aznumeric_cast<float>(m_shapeParamWallThickness->value());
            params.m_cavityGap = aznumeric_cast<float>(m_shapeParamCavityGap->value());
            params.m_floor = m_shapeParamFloor->isChecked();
            params.m_ceiling = m_shapeParamCeiling->isChecked();
            params.m_doorFrame = m_shapeParamDoorFrame->isChecked();
            params.m_archHeight = aznumeric_cast<float>(m_shapeParamArchHeight->value());
            params.m_innerRadius = aznumeric_cast<float>(m_shapeParamInnerRadius->value());
            params.m_sweepAngle = aznumeric_cast<float>(m_shapeParamSweepAngle->value());
            params.m_holeRatio = aznumeric_cast<float>(m_shapeParamHoleRatio->value());
            params.m_tubeSides = m_shapeParamTubeSides->value();

            changed = true;
            if (auto* component = CurrentComponent())
            {
                const auto current = component->GetLayerShapeParams(row);
                changed = !(params.m_shape == current.m_shape && params.m_width == current.m_width &&
                    params.m_depth == current.m_depth && params.m_height == current.m_height &&
                    params.m_sides == current.m_sides && params.m_steps == current.m_steps &&
                    params.m_stepsByHeight == current.m_stepsByHeight && params.m_stepHeight == current.m_stepHeight &&
                    params.m_wallThickness == current.m_wallThickness && params.m_cavityGap == current.m_cavityGap &&
                    params.m_floor == current.m_floor && params.m_ceiling == current.m_ceiling &&
                    params.m_doorFrame == current.m_doorFrame && params.m_archHeight == current.m_archHeight &&
                    params.m_innerRadius == current.m_innerRadius && params.m_sweepAngle == current.m_sweepAngle &&
                    params.m_holeRatio == current.m_holeRatio && params.m_tubeSides == current.m_tubeSides);
            }
            return true;
        };

        // Intermediate values: regenerate geometry now; cook, bake and undo wait for the commit below.
        const auto previewShapeParams = [this, readShapeParams]()
        {
            if (m_updating)
            {
                return;
            }
            int row = -1;
            EditorWhiteBoxComponent::ShapeParams params;
            bool changed = false;
            if (!readShapeParams(row, params, changed) || !changed)
            {
                return;
            }
            if (auto* component = CurrentComponent())
            {
                component->SetLayerShapeParamsPreview(row, params);
                m_shapeParamsPendingCommit = true;
                m_shapeParamsPendingRow = row;
            }
            m_shapeParamCommitTimer->start();
        };

        // The real edit: full rebuild inside an undo batch. Runs once the value settles, or straight
        // away for the controls that cannot stream (the shape combo and the check boxes).
        const auto commitShapeParams = [this, readShapeParams]()
        {
            m_shapeParamCommitTimer->stop();
            if (m_updating)
            {
                return;
            }
            int row = -1;
            EditorWhiteBoxComponent::ShapeParams params;
            bool changed = false;
            if (!readShapeParams(row, params, changed))
            {
                return;
            }
            // A pending preview already wrote these values into the component, so `changed` is false by
            // then - the undo step and the deferred work still have to happen.
            if (!changed && !m_shapeParamsPendingCommit)
            {
                return;
            }
            if (m_shapeParamsPendingCommit && m_shapeParamsPendingRow >= 0)
            {
                row = m_shapeParamsPendingRow; // commit where the preview landed, not where the selection is now
            }
            m_shapeParamsPendingCommit = false;
            m_shapeParamsPendingRow = -1;
            ModifyComponent(
                "Edit Parametric Shape",
                [row, params](EditorWhiteBoxComponent* c) { c->SetLayerShapeParams(row, params); });
        };

        m_shapeParamCommitTimer = new QTimer(this);
        m_shapeParamCommitTimer->setSingleShot(true);
        m_shapeParamCommitTimer->setInterval(300);
        connect(m_shapeParamCommitTimer, &QTimer::timeout, this, commitShapeParams);

        connect(m_shapeParamShape, QOverload<int>::of(&QComboBox::currentIndexChanged), this, commitShapeParams);
        for (QDoubleSpinBox* spin :
             { m_shapeParamWidth, m_shapeParamDepth, m_shapeParamHeight, m_shapeParamWallThickness,
               m_shapeParamCavityGap, m_shapeParamArchHeight, m_shapeParamInnerRadius, m_shapeParamSweepAngle,
               m_shapeParamStepHeight, m_shapeParamHoleRatio })
        {
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, previewShapeParams);
            connect(spin, &QAbstractSpinBox::editingFinished, this, commitShapeParams);
        }
        for (QSpinBox* spin : { m_shapeParamSides, m_shapeParamSteps, m_shapeParamTubeSides })
        {
            connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, previewShapeParams);
            connect(spin, &QAbstractSpinBox::editingFinished, this, commitShapeParams);
        }
        connect(m_shapeParamStepsByHeight, &QCheckBox::toggled, this, commitShapeParams);
        connect(m_shapeParamFloor, &QCheckBox::toggled, this, commitShapeParams);
        connect(m_shapeParamCeiling, &QCheckBox::toggled, this, commitShapeParams);
        connect(m_shapeParamDoorFrame, &QCheckBox::toggled, this, commitShapeParams);
        connect(m_bakeShapeButton, &QPushButton::clicked, this,
            [this]()
            {
                const int row = m_layerList->currentRow();
                if (row >= 0)
                {
                    ModifyComponent(
                        "Bake Parametric Shape",
                        [row](EditorWhiteBoxComponent* c) { c->BakeParametricLayer(row); });
                }
            });

        // When a gizmo drag finishes, reload the pane so the spin boxes show the new values.
        m_layerGizmo->SetChangedCallback([this]() { RefreshFromComponent(); });

        layout->addWidget(m_layerMetaGroup);

        connect(newLayerButton, &QAbstractButton::clicked, this,
            [this]() {
                ModifyComponent("White Box New Layer", [](EditorWhiteBoxComponent* c) { c->AddLayer(); });
            });
        connect(deleteLayerButton, &QAbstractButton::clicked, this,
            [this]() {
                ModifyComponent("White Box Delete Layer", [](EditorWhiteBoxComponent* c) { c->DeleteActiveLayer(); });
            });
        connect(applyTransformButton, &QPushButton::clicked, this,
            [this]()
            {
                ModifyComponent(
                    "White Box Apply Layer Transform",
                    [](EditorWhiteBoxComponent* c) { c->ApplyActiveLayerTransform(); });
            });
        connect(duplicateLayerButton, &QAbstractButton::clicked, this,
            [this]() {
                ModifyComponent(
                    "White Box Duplicate Layer",
                    [](EditorWhiteBoxComponent* c) { c->DuplicateActiveLayer(); });
            });
        connect(exportLayerButton, &QAbstractButton::clicked, this,
            [this]()
            {
                const int row = m_layerList->currentRow();
                if (row < 0 || CurrentComponent() == nullptr)
                {
                    return;
                }
                const bool keepInSource = QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
                AZStd::string error;
                const AZ::EntityId newEntityId = ExportWhiteBoxLayerToEntity(m_currentEntityId, row, keepInSource, error);
                if (!newEntityId.IsValid())
                {
                    QMessageBox::warning(this, tr("Export Layer To Entity"), QString::fromUtf8(error.c_str()));
                    RefreshFromComponent();
                    return;
                }
                SetCurrentEntity(newEntityId);
            });
        connect(moveUpButton, &QAbstractButton::clicked, this,
            [this]()
            {
                const int row = m_layerList->currentRow();
                MoveLayerRow(row, row - 1);
            });
        connect(moveDownButton, &QAbstractButton::clicked, this,
            [this]()
            {
                const int row = m_layerList->currentRow();
                MoveLayerRow(row, row + 1);
            });
        connect(
            m_activeLayerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                if (m_updating || index < 0)
                {
                    return;
                }
                const int layerIndex = m_activeLayerCombo->itemData(index).toInt();
                ModifyComponent(
                    "White Box Active Layer",
                    [layerIndex](EditorWhiteBoxComponent* c) { c->SetActiveLayer(layerIndex); });
            });
        connect(m_layerList, &QListWidget::itemChanged, this,
            [this](QListWidgetItem* item)
            {
                if (m_updating || item == nullptr)
                {
                    return;
                }
                const int row = m_layerList->row(item);
                const AZStd::string name = item->text().toUtf8().constData();
                const bool visible = item->checkState() == Qt::Checked;
                ModifyComponent(
                    "White Box Layer Edit",
                    [row, name, visible](EditorWhiteBoxComponent* c)
                    {
                        EditorWhiteBoxComponent::LayerMeta meta = c->GetLayerMeta(row);
                        meta.m_name = name;
                        meta.m_visible = visible;
                        c->SetLayerMeta(row, meta);
                    });
            });
        connect(m_layerList, &QListWidget::currentRowChanged, this,
            [this](int row)
            {
                if (m_updating || row < 0)
                {
                    return;
                }
                // Selecting a layer in the list also makes it the EDIT TARGET (active layer),
                // so viewport tools immediately edit the layer whose data the pane shows.
                ModifyComponent(
                    "Switch White Box Layer",
                    [row](EditorWhiteBoxComponent* c) { c->SetActiveLayer(row); });
            });
        connect(m_layerTintButton, &QPushButton::clicked, this,
            [this]()
            {
                EditorWhiteBoxComponent* component = CurrentComponent();
                const int row = m_layerList->currentRow();
                if (component == nullptr || row < 0)
                {
                    return;
                }
                const QColor initial = ToQColor(component->GetLayerMeta(row).m_tint);
                const QColor picked = QColorDialog::getColor(initial, this, tr("Layer Tint"));
                if (!picked.isValid())
                {
                    return;
                }
                ModifyComponent(
                    "White Box Layer Tint",
                    [row, picked](EditorWhiteBoxComponent* c)
                    {
                        EditorWhiteBoxComponent::LayerMeta meta = c->GetLayerMeta(row);
                        meta.m_tint = AZ::Vector3(
                            aznumeric_cast<float>(picked.redF()), aznumeric_cast<float>(picked.greenF()),
                            aznumeric_cast<float>(picked.blueF()));
                        c->SetLayerMeta(row, meta);
                    });
            });

        // One handler applies every remaining meta control to the selected layer.
        const auto applyMeta = [this]()
        {
            if (m_updating)
            {
                return;
            }
            const int row = m_layerList->currentRow();
            if (row < 0)
            {
                return;
            }
            const auto combine = static_cast<LayerCombineMode>(m_layerCombineCombo->currentData().toInt());
            const bool invert = m_layerInvertNormals->isChecked();
            const bool edgesOnly = m_layerEdgesOnly->isChecked();
            const bool collision = m_layerCollision->isChecked();
            const AZ::Vector3 pos(
                aznumeric_cast<float>(m_layerPos[0]->value()), aznumeric_cast<float>(m_layerPos[1]->value()),
                aznumeric_cast<float>(m_layerPos[2]->value()));
            const AZ::Vector3 rot(
                aznumeric_cast<float>(m_layerRot[0]->value()), aznumeric_cast<float>(m_layerRot[1]->value()),
                aznumeric_cast<float>(m_layerRot[2]->value()));
            const AZ::Vector3 scale(
                aznumeric_cast<float>(m_layerScale[0]->value()), aznumeric_cast<float>(m_layerScale[1]->value()),
                aznumeric_cast<float>(m_layerScale[2]->value()));
            const bool mirror[3] = { m_layerMirror[0]->isChecked(), m_layerMirror[1]->isChecked(), m_layerMirror[2]->isChecked() };
            const int arrayCount = m_layerArrayCount->value();
            const AZ::Vector3 arrayOffset(
                aznumeric_cast<float>(m_layerArrayOffset[0]->value()), aznumeric_cast<float>(m_layerArrayOffset[1]->value()),
                aznumeric_cast<float>(m_layerArrayOffset[2]->value()));
            const bool mirrorX = mirror[0];
            const bool mirrorY = mirror[1];
            const bool mirrorZ = mirror[2];
            ModifyComponent(
                "White Box Layer Edit",
                [row, combine, invert, edgesOnly, collision, pos, rot, scale, mirrorX, mirrorY, mirrorZ, arrayCount,
                 arrayOffset](EditorWhiteBoxComponent* c)
                {
                    EditorWhiteBoxComponent::LayerMeta meta = c->GetLayerMeta(row);
                    meta.m_combineMode = combine;
                    meta.m_invertNormals = invert;
                    meta.m_edgesOnly = edgesOnly;
                    meta.m_collision = collision;
                    meta.m_position = pos;
                    meta.m_rotation = rot;
                    meta.m_scale = scale;
                    meta.m_mirrorX = mirrorX;
                    meta.m_mirrorY = mirrorY;
                    meta.m_mirrorZ = mirrorZ;
                    meta.m_arrayCount = arrayCount;
                    meta.m_arrayOffset = arrayOffset;
                    c->SetLayerMeta(row, meta);
                });
        };
        connect(m_layerCombineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, applyMeta);
        connect(m_layerInvertNormals, &QCheckBox::toggled, this, applyMeta);
        connect(m_layerEdgesOnly, &QCheckBox::toggled, this, applyMeta);
        connect(m_layerCollision, &QCheckBox::toggled, this, applyMeta);
        for (QDoubleSpinBox* spin :
             { m_layerPos[0], m_layerPos[1], m_layerPos[2], m_layerRot[0], m_layerRot[1], m_layerRot[2],
               m_layerScale[0], m_layerScale[1], m_layerScale[2], m_layerArrayOffset[0], m_layerArrayOffset[1], m_layerArrayOffset[2] })
        {
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, applyMeta);
        }
        for (QCheckBox* mirror : m_layerMirror)
        {
            connect(mirror, &QCheckBox::toggled, this, applyMeta);
        }
        connect(m_layerArrayCount, QOverload<int>::of(&QSpinBox::valueChanged), this, applyMeta);

        return group;
    }















    // ---- data plumbing --------------------------------------------------------------------

    EditorWhiteBoxComponent* WhiteBoxPaneWidget::CurrentComponent() const
    {
        // The focus can change before the dropdown is refreshed; never edit an entity outside it.
        if (!IsEditableInFocus(m_currentEntityId))
        {
            return nullptr;
        }
        if (AZ::Entity* entity = FindEntity(m_currentEntityId))
        {
            return entity->FindComponent<EditorWhiteBoxComponent>();
        }
        return nullptr;
    }

    void WhiteBoxPaneWidget::ModifyComponent(
        const char* undoLabel, const AZStd::function<void(EditorWhiteBoxComponent*)>& fn)
    {
        EditorWhiteBoxComponent* component = CurrentComponent();
        if (component == nullptr)
        {
            return;
        }
        {
            AzToolsFramework::ScopedUndoBatch undoBatch(undoLabel);
            fn(component);
            undoBatch.MarkEntityDirty(m_currentEntityId);
        }
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::MoveLayerRow(const int from, const int to)
    {
        EditorWhiteBoxComponent* component = CurrentComponent();
        if (component == nullptr)
        {
            return;
        }
        const int layerCount = component->GetLayerCount();
        if (from < 0 || from >= layerCount || to < 0 || to >= layerCount || from == to)
        {
            return; // nothing selected, or already at the top / bottom
        }

        ModifyComponent(
            "White Box Reorder Layer",
            [from, to](EditorWhiteBoxComponent* c)
            {
                c->MoveLayer(from, to);
                // Selecting a layer in this list is what makes it the edit target, so keep the
                // two in step: the moved layer stays selected below, so make it active too.
                c->SetActiveLayer(to);
            });

        // Select the moved layer now so the highlight does not flash on the row it left.
        const bool wasUpdating = m_updating;
        m_updating = true;
        m_layerList->setCurrentRow(to);
        m_updating = wasUpdating;
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::RefreshEntityList()
    {
        const bool wasUpdating = m_updating;
        m_updating = true;

        const AZStd::vector<AZ::EntityId> entityIds = CollectWhiteBoxEntities();

        m_entityCombo->clear();
        int currentIndex = -1;
        for (const AZ::EntityId& entityId : entityIds)
        {
            m_entityCombo->addItem(EntityDisplayName(entityId), QVariant(static_cast<qulonglong>(static_cast<AZ::u64>(entityId))));
            if (entityId == m_currentEntityId)
            {
                currentIndex = m_entityCombo->count() - 1;
            }
        }

        if (currentIndex >= 0)
        {
            m_entityCombo->setCurrentIndex(currentIndex);
        }
        else if (m_entityCombo->count() > 0)
        {
            m_entityCombo->setCurrentIndex(0);
            m_currentEntityId = AZ::EntityId(m_entityCombo->itemData(0).toULongLong());
        }
        else
        {
            m_currentEntityId = AZ::EntityId();
        }

        m_updating = wasUpdating;
    }

    void WhiteBoxPaneWidget::RefreshLayerControls(EditorWhiteBoxComponent* component)
    {
        // Active layer combo.
        m_activeLayerCombo->clear();
        const auto layerNames = component->GetLayerNames();
        for (const auto& [index, name] : layerNames)
        {
            m_activeLayerCombo->addItem(QString::fromUtf8(name.c_str()), index);
        }
        const int activeIndex = component->GetActiveLayerIndex();
        for (int i = 0; i < m_activeLayerCombo->count(); ++i)
        {
            if (m_activeLayerCombo->itemData(i).toInt() == activeIndex)
            {
                m_activeLayerCombo->setCurrentIndex(i);
                break;
            }
        }

        // Layer list. The highlighted row is the active layer, so rebuild it around that: a change made anywhere
        // else (the combo, the viewport tools, a script) then shows up here instead of leaving the old row lit.
        m_layerList->clear();
        const int layerCount = component->GetLayerCount();
        for (int i = 0; i < layerCount; ++i)
        {
            const EditorWhiteBoxComponent::LayerMeta meta = component->GetLayerMeta(i);
            auto* item = new QListWidgetItem(QString::fromUtf8(meta.m_name.c_str()));
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
            item->setCheckState(meta.m_visible ? Qt::Checked : Qt::Unchecked);
            item->setData(
                LayerTintRole,
                QColor::fromRgbF(meta.m_tint.GetX(), meta.m_tint.GetY(), meta.m_tint.GetZ()));
            item->setData(LayerCombineRole, CombineModeLabel(meta.m_combineMode));
            m_layerList->addItem(item);
        }
        if (m_layerCountLabel != nullptr)
        {
            m_layerCountLabel->setText(layerCount == 1 ? tr("1 layer") : tr("%1 layers").arg(layerCount));
        }
        const int row = layerCount > 0 ? AZStd::clamp(activeIndex, 0, layerCount - 1) : -1;
        m_layerList->setCurrentRow(row);

        // Size the list to its contents so every layer is visible without an inner scrollbar.
        const int rowHeight = AZStd::max(m_layerList->sizeHintForRow(0), 18);
        m_layerList->setFixedHeight(
            AZStd::max(layerCount, 1) * rowHeight + 2 * m_layerList->frameWidth() + 4);

        // Meta editors for the selected layer.
        const bool hasLayer = row >= 0;
        m_layerMetaGroup->setEnabled(hasLayer);

        // Shape Parameters only exist for parametric layers.
        const bool parametric = hasLayer && component->IsLayerParametric(row);
        m_shapeParamsGroup->setVisible(parametric);
        if (parametric)
        {
            const EditorWhiteBoxComponent::ShapeParams params = component->GetLayerShapeParams(row);
            m_shapeParamShape->setCurrentIndex(m_shapeParamShape->findData(static_cast<int>(params.m_shape)));
            m_shapeParamWidth->setValue(params.m_width);
            m_shapeParamDepth->setValue(params.m_depth);
            m_shapeParamHeight->setValue(params.m_height);
            m_shapeParamSides->setValue(params.m_sides);
            m_shapeParamSteps->setValue(params.m_steps);
            m_shapeParamStepsByHeight->setChecked(params.m_stepsByHeight);
            m_shapeParamStepHeight->setValue(params.m_stepHeight);
            m_shapeParamWallThickness->setValue(params.m_wallThickness);
            m_shapeParamCavityGap->setValue(params.m_cavityGap);
            m_shapeParamFloor->setChecked(params.m_floor);
            m_shapeParamCeiling->setChecked(params.m_ceiling);
            m_shapeParamDoorFrame->setChecked(params.m_doorFrame);
            m_shapeParamArchHeight->setMaximum(AZStd::max(0.0f, params.m_height - 0.001f));
            m_shapeParamArchHeight->setValue(params.m_archHeight);
            m_shapeParamInnerRadius->setValue(params.m_innerRadius);
            m_shapeParamSweepAngle->setValue(params.m_sweepAngle);
            m_shapeParamHoleRatio->setValue(params.m_holeRatio);
            m_shapeParamTubeSides->setValue(params.m_tubeSides);
            // Room has no Sides/Steps; every other non-staircase shape honours Sides (round shapes
            // inscribe the N-gon in the ellipse, angular shapes fill the rectangle with it).
            const bool circular = params.m_shape == DrawShapeType::CircularStairs;
            const bool door = params.m_shape == DrawShapeType::Door;
            const bool stair = params.m_shape == DrawShapeType::Staircase || circular;
            const bool room = params.m_shape == DrawShapeType::Room;
            const bool plane = params.m_shape == DrawShapeType::Plane;
            const bool torus = params.m_shape == DrawShapeType::Torus;
            const bool ringShape = params.m_shape == DrawShapeType::Pipe || torus;
            const bool hasSides = !plane && !stair && !room && (!door || params.m_archHeight > 0.0f);
            m_shapeParamSides->setVisible(hasSides);
            m_shapeParamSidesLabel->setVisible(hasSides);
            m_shapeParamSidesLabel->setText(
                door ? tr("Arch Segments") : (params.m_shape == DrawShapeType::Sphere ? tr("Subdivision") : tr("Sides")));
            m_shapeParamTubeSides->setVisible(torus);
            m_shapeParamTubeSidesLabel->setVisible(torus);
            m_shapeParamSteps->setVisible(stair && !params.m_stepsByHeight);
            m_shapeParamStepsLabel->setVisible(stair && !params.m_stepsByHeight);
            m_shapeParamStepsByHeight->setVisible(stair);
            m_shapeParamStepHeight->setVisible(stair && params.m_stepsByHeight);
            m_shapeParamStepHeightLabel->setVisible(stair && params.m_stepsByHeight);
            // Room-only controls (Width/Depth/Height read as the interior dimensions for a Room).
            m_shapeParamWallThickness->setVisible(room || (door && params.m_doorFrame));
            m_shapeParamWallThicknessLabel->setVisible(room || (door && params.m_doorFrame));
            m_shapeParamWallThicknessLabel->setText(door ? tr("Frame Thickness") : tr("Wall Thickness"));
            m_shapeParamWallThickness->setToolTip(door ? tr("Thickness around the door opening.") : tr("Thickness of each wall leaf."));
            m_shapeParamCavityGap->setVisible(room);
            m_shapeParamCavityGapLabel->setVisible(room);
            m_shapeParamFloor->setVisible(room);
            m_shapeParamCeiling->setVisible(room);
            m_shapeParamDoorFrame->setVisible(door);
            auto* shapeLayout = qobject_cast<QFormLayout*>(m_shapeParamsGroup->layout());
            const auto showField = [shapeLayout](QWidget* field, bool visible)
            {
                field->setVisible(visible);
                if (auto* label = shapeLayout->labelForField(field))
                {
                    label->setVisible(visible);
                }
            };
            showField(m_shapeParamArchHeight, door);
            showField(m_shapeParamInnerRadius, circular);
            showField(m_shapeParamSweepAngle, circular);
            showField(m_shapeParamDepth, !circular);
            showField(m_shapeParamHeight, !plane);
            showField(m_shapeParamHoleRatio, ringShape);
            const auto setLabel = [shapeLayout](QWidget* field, const QString& text)
            {
                if (auto* label = qobject_cast<QLabel*>(shapeLayout->labelForField(field)))
                {
                    label->setText(text);
                }
            };
            setLabel(m_shapeParamWidth, circular ? tr("Tread Width") :
                (door && params.m_doorFrame ? tr("Opening Width") : tr("Width")));
            setLabel(m_shapeParamHeight, door && params.m_doorFrame ? tr("Opening Height") : tr("Height"));
            setLabel(m_shapeParamDepth, door ? tr("Thickness / Depth") : tr("Depth"));
        }

        if (hasLayer)
        {
            const EditorWhiteBoxComponent::LayerMeta meta = component->GetLayerMeta(row);
            SetColorButtonSwatch(m_layerTintButton, ToQColor(meta.m_tint));
            m_layerCombineCombo->setCurrentIndex(
                m_layerCombineCombo->findData(static_cast<int>(meta.m_combineMode)));
            m_layerInvertNormals->setChecked(meta.m_invertNormals);
            m_layerEdgesOnly->setChecked(meta.m_edgesOnly);
            m_layerCollision->setChecked(meta.m_collision);
            m_layerMirror[0]->setChecked(meta.m_mirrorX);
            m_layerMirror[1]->setChecked(meta.m_mirrorY);
            m_layerMirror[2]->setChecked(meta.m_mirrorZ);
            m_layerArrayCount->setValue(meta.m_arrayCount);
            m_layerArrayOffset[0]->setValue(meta.m_arrayOffset.GetX());
            m_layerArrayOffset[1]->setValue(meta.m_arrayOffset.GetY());
            m_layerArrayOffset[2]->setValue(meta.m_arrayOffset.GetZ());
            const AZ::Vector3 vecs[3] = { meta.m_position, meta.m_rotation, meta.m_scale };
            QDoubleSpinBox* (*rows[3])[3] = { &m_layerPos, &m_layerRot, &m_layerScale };
            for (int v = 0; v < 3; ++v)
            {
                (*rows[v])[0]->setValue(vecs[v].GetX());
                (*rows[v])[1]->setValue(vecs[v].GetY());
                (*rows[v])[2]->setValue(vecs[v].GetZ());
            }
        }
    }

    void WhiteBoxPaneWidget::UpdateEntityTransformUi()
    {
        if (!m_currentEntityId.IsValid())
        {
            return;
        }
        const bool wasUpdating = m_updating;
        m_updating = true;

        const bool world = m_entitySpaceCombo->currentIndex() == 1;
        AZ::Vector3 translation = AZ::Vector3::CreateZero();
        AZ::Quaternion rotation = AZ::Quaternion::CreateIdentity();
        if (world)
        {
            AZ::TransformBus::EventResult(
                translation, m_currentEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
            AZ::TransformBus::EventResult(
                rotation, m_currentEntityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
        }
        else
        {
            AZ::TransformBus::EventResult(
                translation, m_currentEntityId, &AZ::TransformBus::Events::GetLocalTranslation);
            AZ::TransformBus::EventResult(
                rotation, m_currentEntityId, &AZ::TransformBus::Events::GetLocalRotationQuaternion);
        }
        float scale = 1.0f;
        AZ::TransformBus::EventResult(scale, m_currentEntityId, &AZ::TransformBus::Events::GetLocalUniformScale);

        const AZ::Vector3 rotationDegrees = rotation.GetEulerDegrees();
        for (int i = 0; i < 3; ++i)
        {
            m_entityPos[i]->setValue(translation.GetElement(i));
            m_entityRot[i]->setValue(rotationDegrees.GetElement(i));
        }
        m_entityScale->setValue(scale);

        // Non-uniform scale from the entity's Non-Uniform Scale component (identity when absent).
        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, m_currentEntityId, &AZ::NonUniformScaleRequests::GetScale);
        for (int i = 0; i < 3; ++i)
        {
            m_entityNonUniformScale[i]->setValue(nonUniformScale.GetElement(i));
        }

        m_updating = wasUpdating;
    }

    void WhiteBoxPaneWidget::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        UpdateEntityTransformUi();
    }

    void WhiteBoxPaneWidget::RefreshFromComponent()
    {
        m_updating = true;

        EditorWhiteBoxComponent* component = CurrentComponent();
        const bool hasComponent = component != nullptr;
        // Follow the current entity's transform (for the Entity Transform section).
        if (m_transformBusEntityId != m_currentEntityId)
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            if (m_currentEntityId.IsValid())
            {
                AZ::TransformNotificationBus::Handler::BusConnect(m_currentEntityId);
            }
            m_transformBusEntityId = m_currentEntityId;
        }
        UpdateEntityTransformUi();

        // Follow the current component's white box notifications (viewport edits - drawing,
        // stamping - change component state the pane must mirror, e.g. the auto-created first layer).
        const AZ::EntityComponentIdPair meshBusPair =
            hasComponent ? AZ::EntityComponentIdPair(m_currentEntityId, component->GetId()) : AZ::EntityComponentIdPair();
        if (!(meshBusPair == m_meshBusPair))
        {
            EditorWhiteBoxComponentNotificationBus::Handler::BusDisconnect();
            if (hasComponent)
            {
                EditorWhiteBoxComponentNotificationBus::Handler::BusConnect(meshBusPair);
            }
            m_meshBusPair = meshBusPair;
        }

        // Everything below the entity picker only makes sense with a component.
        if (QWidget* container = widget())
        {
            const QList<QGroupBox*> groups = container->findChildren<QGroupBox*>();
            for (QGroupBox* group : groups)
            {
                // The viewport handle sizes are global, so that section stays usable with nothing selected.
                if (group->title() != tr("Entity") && group->objectName() != ViewportHandlesObjectName)
                {
                    group->setEnabled(hasComponent);
                }
            }
        }
        m_newChildButton->setEnabled(hasComponent);
        m_editButton->setEnabled(hasComponent);

        if (hasComponent)
        {
            RefreshLayerControls(component);

            // Tool sub-mode (only meaningful while in component mode).
            SubMode subMode = SubMode::Default;
            EditorWhiteBoxComponentModeRequestBus::EventResult(
                subMode, AZ::EntityComponentIdPair(m_currentEntityId, component->GetId()),
                &EditorWhiteBoxComponentModeRequests::GetCurrentSubMode);
            m_modeSketch->setChecked(subMode == SubMode::Default);
            m_modeEdgeRestore->setChecked(subMode == SubMode::EdgeRestore);
            m_modeTransform->setChecked(subMode == SubMode::Transform);
            m_modeDrawShape->setChecked(subMode == SubMode::DrawShape);
            m_modePaint->setChecked(subMode == SubMode::VertexPaint);

            // Layer gizmo: keep it targeting the selected layer.
            m_layerGizmo->SetTarget(m_currentEntityId, m_layerList->currentRow());
            m_layerGizmo->Refresh();

            // Entity gizmo: keep it targeting the current entity.
            m_entityGizmo->SetTarget(m_currentEntityId);
            m_entityGizmo->Refresh();

            // Keep the pane's gizmo buttons (and the viewport cluster highlight) in sync with
            // the actual gizmo modes, wherever they were changed from.
            const auto entityMode = m_entityGizmo->GetMode();
            m_entityGizmoOff->setChecked(entityMode == WhiteBoxEntityGizmo::Mode::None);
            m_entityGizmoMove->setChecked(entityMode == WhiteBoxEntityGizmo::Mode::Translate);
            m_entityGizmoRotate->setChecked(entityMode == WhiteBoxEntityGizmo::Mode::Rotate);
            m_entityGizmoScale->setChecked(entityMode == WhiteBoxEntityGizmo::Mode::Scale);
            UpdateEntityGizmoClusterHighlight();

            const auto layerMode = m_layerGizmo->GetMode();
            m_gizmoOff->setChecked(layerMode == WhiteBoxLayerGizmo::Mode::None);
            m_gizmoMove->setChecked(layerMode == WhiteBoxLayerGizmo::Mode::Translate);
            m_gizmoRotate->setChecked(layerMode == WhiteBoxLayerGizmo::Mode::Rotate);
            m_gizmoScale->setChecked(layerMode == WhiteBoxLayerGizmo::Mode::Scale);
        }
        else
        {
            m_layerGizmo->SetMode(WhiteBoxLayerGizmo::Mode::None);
            m_gizmoOff->setChecked(true);
            m_entityGizmo->SetMode(WhiteBoxEntityGizmo::Mode::None);
            m_entityGizmoOff->setChecked(true);
        }

        m_updating = false;
    }

    void WhiteBoxPaneWidget::SetCurrentEntity(const AZ::EntityId entityId)
    {
        if (entityId == m_currentEntityId)
        {
            return;
        }
        m_currentEntityId = entityId;
        RefreshEntityList(); // re-sync the dropdown (and the boolean source list) to the new entity
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::OnEntityComboChanged(const int index)
    {
        if (m_updating || index < 0)
        {
            return;
        }
        const AZ::EntityId entityId(m_entityCombo->itemData(index).toULongLong());
        if (entityId == m_currentEntityId)
        {
            return;
        }
        m_currentEntityId = entityId;

        // Component mode stays latched to the old entity, so end it and re-enter it on the new one.
        namespace Cmf = AzToolsFramework::ComponentModeFramework;
        bool wasInComponentMode = false;
        Cmf::ComponentModeSystemRequestBus::BroadcastResult(
            wasInComponentMode, &Cmf::ComponentModeSystemRequests::InComponentMode);
        if (wasInComponentMode)
        {
            Cmf::ComponentModeSystemRequestBus::Broadcast(&Cmf::ComponentModeSystemRequests::EndComponentMode);
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
            AzToolsFramework::EntityIdList{ entityId });

        if (wasInComponentMode)
        {
            // Deferred a tick so the selection has fully applied before the mode begins.
            AZ::TickBus::QueueFunction(
                [entityId]()
                {
                    if (AZ::Entity* entity = FindEntity(entityId))
                    {
                        if (auto* whiteBox = entity->FindComponent<EditorWhiteBoxComponent>())
                        {
                            whiteBox->EnterComponentMode();
                        }
                    }
                });
        }

        RefreshEntityList();
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::CreateWhiteBoxEntity(const bool asChildLayer)
    {
        const AZ::EntityId parentId = asChildLayer ? m_currentEntityId : AZ::EntityId();
        const AZ::EntityId newEntityId = CreateChildWhiteBoxLayer(parentId);
        if (!newEntityId.IsValid())
        {
            return;
        }
        if (!asChildLayer)
        {
            // The layer utility names children "White Box Layer"; a standalone entity is just "White Box".
            if (AZ::Entity* entity = FindEntity(newEntityId))
            {
                entity->SetName("White Box");
            }
        }
        SetCurrentEntity(newEntityId);
    }

    // ---- notifications ----------------------------------------------------------------------

    void WhiteBoxPaneWidget::QueueRefresh()
    {
        if (m_refreshQueued)
        {
            return;
        }
        m_refreshQueued = true;
        QTimer::singleShot(
            0, this,
            [this]()
            {
                m_refreshQueued = false;
                RefreshEntityList(); // falls back to another White Box if the current one is really gone
                RefreshFromComponent();
            });
    }

    void WhiteBoxPaneWidget::OnEditorEntityCreated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        QueueRefresh();
    }

    void WhiteBoxPaneWidget::OnEditorEntityDeleted([[maybe_unused]] const AZ::EntityId& entityId)
    {
        QueueRefresh();
    }

    void WhiteBoxPaneWidget::AfterEntitySelectionChanged(
        const AzToolsFramework::EntityIdList& newlySelectedEntities,
        [[maybe_unused]] const AzToolsFramework::EntityIdList& newlyDeselectedEntities)
    {
        // Follow the selection: if a White Box entity is selected, make it the pane's edit target.
        for (const AZ::EntityId& entityId : newlySelectedEntities)
        {
            if (AZ::Entity* entity = FindEntity(entityId))
            {
                if (entity->FindComponent<EditorWhiteBoxComponent>() != nullptr)
                {
                    SetCurrentEntity(entityId);
                    return;
                }
            }
        }
    }

    void WhiteBoxPaneWidget::OnEditorFocusChanged(
        [[maybe_unused]] AZ::EntityId previousFocusEntityId, [[maybe_unused]] AZ::EntityId newFocusEntityId)
    {
        // Drops White Boxes outside the new focus from the dropdown (switching off one that was current).
        RefreshEntityList();
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::AfterUndoRedo()
    {
        // Component state may have changed under the pane (layers added/removed, settings undone).
        RefreshEntityList();
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::OnEntityComponentAdded(
        [[maybe_unused]] const AZ::EntityId& entityId, [[maybe_unused]] const AZ::ComponentId& componentId)
    {
        QueueRefresh();
    }

    void WhiteBoxPaneWidget::OnEntityComponentRemoved(
        [[maybe_unused]] const AZ::EntityId& entityId, [[maybe_unused]] const AZ::ComponentId& componentId)
    {
        QueueRefresh();
    }

    void WhiteBoxPaneWidget::EnsureEnabledInComponentMode()
    {
        // Deferred so it runs AFTER whoever is disabling the dock panes has finished.
        QTimer::singleShot(
            0, this,
            [this]()
            {
                namespace Cmf = AzToolsFramework::ComponentModeFramework;
                bool inComponentMode = false;
                Cmf::ComponentModeSystemRequestBus::BroadcastResult(
                    inComponentMode, &Cmf::ComponentModeSystemRequests::InComponentMode);
                if (!inComponentMode)
                {
                    return; // respect the disable outside component mode (e.g. game/sim mode)
                }
                for (QWidget* w = this; w != nullptr; w = w->parentWidget())
                {
                    if (!w->isEnabled())
                    {
                        w->setEnabled(true);
                    }
                    // The editor also dims disabled panes with a translucent graphics effect
                    // (it doesn't block input, it just LOOKS gray) - remove it from our chain.
                    if (w->graphicsEffect() != nullptr)
                    {
                        w->setGraphicsEffect(nullptr);
                    }
                }
                // The dimming effect may sit on our child widgets as well - clear those too.
                if (QWidget* container = widget())
                {
                    const QList<QWidget*> children = container->findChildren<QWidget*>();
                    for (QWidget* child : children)
                    {
                        if (child->graphicsEffect() != nullptr)
                        {
                            child->setGraphicsEffect(nullptr);
                        }
                    }
                }
                RefreshFromComponent(); // pick up the active sub-mode for the Tool Mode buttons
            });
    }

    void WhiteBoxPaneWidget::changeEvent(QEvent* event)
    {
        QScrollArea::changeEvent(event);
        // The editor disables dock panes in component mode; this pane is the editing UI, so undo that.
        if (event->type() == QEvent::EnabledChange && !isEnabled())
        {
            EnsureEnabledInComponentMode();
        }
    }

    void WhiteBoxPaneWidget::OnWhiteBoxMeshModified()
    {
        // Defer a tick (the component may auto-create a layer on the same bus) and rebuild only if the
        // layer structure changed.
        QTimer::singleShot(
            0, this,
            [this]()
            {
                if (EditorWhiteBoxComponent* component = CurrentComponent())
                {
                    if (component->GetLayerCount() != m_layerList->count() ||
                        component->GetLayerCount() != m_activeLayerCombo->count())
                    {
                        RefreshFromComponent();
                    }
                }
            });
    }

    void WhiteBoxPaneWidget::OnDefaultShapeTypeChanged([[maybe_unused]] DefaultShapeType defaultShape)
    {
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::OnLayerStructureChanged()
    {
        // Deferred one event-loop tick: this arrives mid SyncLayerStructure, before the
        // component has finished settling its working state.
        QTimer::singleShot(0, this, [this]() { RefreshFromComponent(); });
    }

    void WhiteBoxPaneWidget::CreateEntityGizmoCluster()
    {
        namespace Vui = AzToolsFramework::ViewportUi;
        if (m_entityGizmoClusterId != Vui::ClusterId{})
        {
            return; // already shown
        }

        Vui::ViewportUiRequestBus::Event(
            Vui::DefaultViewportId,
            [this](Vui::ViewportUiRequests* requests)
            {
                const auto fetchIcon = [](const char* iconName)
                {
                    return AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName);
                };
                m_entityGizmoClusterId = requests->CreateCluster(Vui::Alignment::TopLeft);
                m_entityGizmoMoveButtonId = requests->CreateClusterButton(m_entityGizmoClusterId, fetchIcon("Move"));
                m_entityGizmoRotateButtonId = requests->CreateClusterButton(m_entityGizmoClusterId, fetchIcon("Rotate"));
                m_entityGizmoScaleButtonId = requests->CreateClusterButton(m_entityGizmoClusterId, fetchIcon("Scale"));
                requests->SetClusterButtonTooltip(
                    m_entityGizmoClusterId, m_entityGizmoMoveButtonId, "Move the ENTITY (White Box pane gizmo)");
                requests->SetClusterButtonTooltip(
                    m_entityGizmoClusterId, m_entityGizmoRotateButtonId, "Rotate the ENTITY (White Box pane gizmo)");
                requests->SetClusterButtonTooltip(
                    m_entityGizmoClusterId, m_entityGizmoScaleButtonId, "Scale the ENTITY (White Box pane gizmo)");
            });

        m_entityGizmoClusterHandler = AZ::Event<Vui::ButtonId>::Handler(
            [this](Vui::ButtonId buttonId)
            {
                WhiteBoxEntityGizmo::Mode mode = WhiteBoxEntityGizmo::Mode::None;
                if (buttonId == m_entityGizmoMoveButtonId)
                {
                    mode = WhiteBoxEntityGizmo::Mode::Translate;
                }
                else if (buttonId == m_entityGizmoRotateButtonId)
                {
                    mode = WhiteBoxEntityGizmo::Mode::Rotate;
                }
                else if (buttonId == m_entityGizmoScaleButtonId)
                {
                    mode = WhiteBoxEntityGizmo::Mode::Scale;
                }
                // Clicking the active mode's button again toggles the gizmo off.
                if (m_entityGizmo->GetMode() == mode)
                {
                    mode = WhiteBoxEntityGizmo::Mode::None;
                }
                m_entityGizmo->SetTarget(m_currentEntityId);
                m_entityGizmo->SetMode(mode);
                UpdateEntityGizmoClusterHighlight();
                RefreshFromComponent(); // sync the pane's gizmo buttons
            });
        Vui::ViewportUiRequestBus::Event(
            Vui::DefaultViewportId, &Vui::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_entityGizmoClusterId, m_entityGizmoClusterHandler);
    }

    void WhiteBoxPaneWidget::RemoveEntityGizmoCluster()
    {
        namespace Vui = AzToolsFramework::ViewportUi;
        if (m_entityGizmoClusterId != Vui::ClusterId{})
        {
            Vui::ViewportUiRequestBus::Event(
                Vui::DefaultViewportId, &Vui::ViewportUiRequestBus::Events::RemoveCluster, m_entityGizmoClusterId);
            m_entityGizmoClusterId = Vui::ClusterId{};
        }
    }

    void WhiteBoxPaneWidget::UpdateEntityGizmoClusterHighlight()
    {
        namespace Vui = AzToolsFramework::ViewportUi;
        if (m_entityGizmoClusterId == Vui::ClusterId{})
        {
            return;
        }
        Vui::ButtonId activeButton{};
        switch (m_entityGizmo->GetMode())
        {
        case WhiteBoxEntityGizmo::Mode::Translate:
            activeButton = m_entityGizmoMoveButtonId;
            break;
        case WhiteBoxEntityGizmo::Mode::Rotate:
            activeButton = m_entityGizmoRotateButtonId;
            break;
        case WhiteBoxEntityGizmo::Mode::Scale:
            activeButton = m_entityGizmoScaleButtonId;
            break;
        default:
            break;
        }
        if (activeButton != Vui::ButtonId{})
        {
            Vui::ViewportUiRequestBus::Event(
                Vui::DefaultViewportId, &Vui::ViewportUiRequestBus::Events::SetClusterActiveButton,
                m_entityGizmoClusterId, activeButton);
        }
        else
        {
            Vui::ViewportUiRequestBus::Event(
                Vui::DefaultViewportId, &Vui::ViewportUiRequestBus::Events::ClearClusterActiveButton,
                m_entityGizmoClusterId);
        }
    }

    void WhiteBoxPaneWidget::OnEditorModeActivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
        const AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Component)
        {
            EnsureEnabledInComponentMode();
            // The editor's own entity gizmo is suppressed during component mode; offer ours
            // via an in-viewport Move/Rotate/Scale cluster (like the tool-mode clusters).
            CreateEntityGizmoCluster();
        }
    }

    void WhiteBoxPaneWidget::OnEditorModeDeactivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
        const AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Component)
        {
            // Leaving edit mode: drop the cluster and the gizmo (the editor's own entity
            // gizmo takes over again outside component mode).
            RemoveEntityGizmoCluster();
            m_entityGizmo->SetMode(WhiteBoxEntityGizmo::Mode::None);
            RefreshFromComponent();
        }
    }
} // namespace WhiteBox

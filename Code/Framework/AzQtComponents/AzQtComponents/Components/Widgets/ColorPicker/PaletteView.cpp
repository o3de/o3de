/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteView.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Swatch.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Utilities/ColorUtilities.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzCore/Casting/numeric_cast.h>
#include <QMenu>
#include <QShortcut>
#include <QPainter>
#include <QToolButton>
#include <QKeyEvent>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QBuffer>
#include <QSettings>
#include <QTimer>
#include <QUndoCommand>

namespace AzQtComponents
{
    namespace
    {
        Palette Unmarshal(const QMimeData* mimeData)
        {
            Palette palette;
            palette.load(mimeData);
            return palette;
        }

        Palette UnmarshalFromClipboard()
        {
            QClipboard* clipboard = QGuiApplication::clipboard();
            return Unmarshal(clipboard->mimeData());
        }

        template<typename T>
        void MaybeSet(QSettings& settings, const QString& name, T& value)
        {
            if (settings.contains(name))
            {
                value = settings.value(name).value<T>();
            }
        }

        void ReadDropIndicator(QSettings& settings, const QString& name, PaletteView::DropIndicator& indicator)
        {
            settings.beginGroup(name);
            MaybeSet(settings, QStringLiteral("width"), indicator.width);
            MaybeSet(settings, QStringLiteral("color"), indicator.color);
            settings.endGroup();
        }
    }

class PaletteModel : public QAbstractListModel
{
public:
    enum {
        ColorRole = Qt::UserRole + 1
    };

    explicit PaletteModel(Palette* palette, QUndoStack* undoStack, QObject* parent = nullptr);
    ~PaletteModel() override;

    QVector<AZ::Color> colorsFromIndices(const QModelIndexList& indices) const;
    Palette paletteFromIndices(const QModelIndexList& indices = {}) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

    QStringList mimeTypes() const override;
    Qt::DropActions supportedDropActions() const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QMimeData* mimeData(const QModelIndexList& indices) const override;
    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    bool contains(const AZ::Color& color) const;
    bool lacksAnyOf(const QVector<AZ::Color>& colors) const;

    int indexOf(const AZ::Color& color) const;

    bool tryAppend(const AZ::Color& color);
    bool tryAppend(const QVector<AZ::Color>& colors);

    bool tryInsert(int row, QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last, bool isUndoRedo = false);
    void insertIgnoringDuplicates(int row, QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last, bool isUndoRedo = false);

    bool trySet(int row, const AZ::Color& color, bool isUndoRedo = false);

    bool tryRemove(int row, int count = 1, bool isUndoRedo = false);

private:
    Palette* m_palette;
    QUndoStack* m_undoStack;
};

    namespace UndoRedo
    {

        class PaletteModelInsertionCommand : public QUndoCommand
        {
        public:
            PaletteModelInsertionCommand(PaletteModel& target, int index, const QVector<AZ::Color>& colors, QUndoCommand* parent = nullptr);
            ~PaletteModelInsertionCommand() override;

            void undo() override;
            void redo() override;

        private:
            PaletteModel& m_target;
            const int m_index;
            const QVector<AZ::Color> m_colors;
        };

        PaletteModelInsertionCommand::PaletteModelInsertionCommand(PaletteModel& target, int index, const QVector<AZ::Color>& colors, QUndoCommand* parent)
            : QUndoCommand(parent)
            , m_target(target)
            , m_index(index)
            , m_colors(colors)
        {
        }

        PaletteModelInsertionCommand::~PaletteModelInsertionCommand()
        {
        }

        void PaletteModelInsertionCommand::undo()
        {
            m_target.tryRemove(m_index, m_colors.size(), true);
        }

        void PaletteModelInsertionCommand::redo()
        {
            m_target.tryInsert(m_index, m_colors.cbegin(), m_colors.cend(), true);
        }

        class PaletteModelRemovalCommand : public QUndoCommand
        {
        public:
            PaletteModelRemovalCommand(PaletteModel& target, int index, int count, QUndoCommand* parent = nullptr);
            ~PaletteModelRemovalCommand() override;

            void undo() override;
            void redo() override;

        private:
            static QModelIndexList indicesBetween(PaletteModel& model, int start, int count)
            {
                QModelIndexList indices;
                for (int i = start; i < start + count; ++i)
                {
                    indices.append(model.index(i));
                }

                return indices;
            }

            PaletteModel& m_target;
            const int m_index;
            const QVector<AZ::Color> m_colors;
        };

        PaletteModelRemovalCommand::PaletteModelRemovalCommand(PaletteModel& target, int index, int count, QUndoCommand* parent)
            : QUndoCommand(parent)
            , m_target(target)
            , m_index(index)
            , m_colors(target.colorsFromIndices(PaletteModelRemovalCommand::indicesBetween(target, index, count)))
        {
        }

        PaletteModelRemovalCommand::~PaletteModelRemovalCommand()
        {
        }

        void PaletteModelRemovalCommand::undo()
        {
            m_target.tryInsert(m_index, m_colors.cbegin(), m_colors.cend(), true);
        }

        void PaletteModelRemovalCommand::redo()
        {
            m_target.tryRemove(m_index, m_colors.count(), true);
        }

        class PaletteModelSetColorCommand : public QUndoCommand
        {
        public:
            PaletteModelSetColorCommand(PaletteModel& target, int index, const AZ::Color& color, QUndoCommand* parent = nullptr);
            ~PaletteModelSetColorCommand() override;

            void undo() override;
            void redo() override;

        private:
            PaletteModel & m_target;
            const int m_index;
            const AZ::Color m_previousColor;
            const AZ::Color m_color;
        };

        PaletteModelSetColorCommand::PaletteModelSetColorCommand(PaletteModel& target, int index, const AZ::Color& color, QUndoCommand* parent)
            : QUndoCommand(parent)
            , m_target(target)
            , m_index(index)
            , m_previousColor(target.index(index).data(PaletteModel::ColorRole).value<AZ::Color>())
            , m_color(color)
        {
        }

        PaletteModelSetColorCommand::~PaletteModelSetColorCommand()
        {
        }

        void PaletteModelSetColorCommand::undo()
        {
            m_target.trySet(m_index, m_previousColor, true);
        }

        void PaletteModelSetColorCommand::redo()
        {
            m_target.trySet(m_index, m_color, true);
        }

    }

class PaletteItemDelegate : public QAbstractItemDelegate
{
public:
    PaletteItemDelegate(PaletteView* parent = nullptr);
    ~PaletteItemDelegate() override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void polish(const PaletteView::Config& configuration);

private:
    PaletteView* m_view;
    PaletteView::Config m_configuration;
    Swatch* m_renderer;
};

PaletteModel::PaletteModel(Palette* palette, QUndoStack* undoStack, QObject* parent)
    : QAbstractListModel(parent)
    , m_palette(palette)
    , m_undoStack(undoStack)
{
}

PaletteModel::~PaletteModel()
{
}

bool PaletteModel::contains(const AZ::Color& color) const
{
    return m_palette->containsColor(color);
}

QVector<AZ::Color> PaletteModel::colorsFromIndices(const QModelIndexList& indices) const
{
    QVector<AZ::Color> colors;
    std::transform(indices.cbegin(), indices.cend(), std::back_inserter(colors), [](QModelIndex i) { return i.data(PaletteModel::ColorRole).value<AZ::Color>(); });
    return colors;
}

Palette PaletteModel::paletteFromIndices(const QModelIndexList& indices) const
{
    return Palette(colorsFromIndices(indices));
}

int PaletteModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_palette->colors().size();
}

QVariant PaletteModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return {};
    }

    const int row = index.row();
    if (row < 0 || row >= m_palette->colors().size())
    {
        return {};
    }

    switch (role)
    {
        case Qt::DisplayRole:
            return ToQColor(m_palette->colors()[row]).name(QColor::HexArgb);
        case Qt::DecorationRole:
            return ToQColor(m_palette->colors()[row]);
        case ColorRole:
            return QVariant::fromValue(m_palette->colors()[row]);

        default:
            return {};
    }
}

bool PaletteModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid() || row < 0 || row + count > rowCount())
    {
        return false;
    }

    return tryRemove(row, count);
}

bool PaletteModel::lacksAnyOf(const QVector<AZ::Color>& colors) const
{
    return std::any_of(colors.cbegin(), colors.cend(), [this](const AZ::Color& color) {
        return std::none_of(m_palette->colors().cbegin(), m_palette->colors().cend(), [color](const AZ::Color& existing) {
            return existing.IsClose(color);
        });
    });
}

int PaletteModel::indexOf(const AZ::Color& color) const
{
    auto i = std::find_if(m_palette->colors().cbegin(), m_palette->colors().cend(), [&color](const AZ::Color& candidate) { return candidate.IsClose(color); });
    return i == m_palette->colors().cend() ? -1 : aznumeric_cast<int>(std::distance(m_palette->colors().cbegin(), i));
}

bool PaletteModel::tryAppend(const AZ::Color& color)
{
    QVector<AZ::Color> colors{ color };
    return tryInsert(rowCount(), colors.cbegin(), colors.cend());
}

bool PaletteModel::tryAppend(const QVector<AZ::Color>& colors)
{
    return tryInsert(m_palette->colors().size(), colors.cbegin(), colors.cend());
}

bool PaletteModel::tryInsert(int row, QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last, bool isUndoRedo)
{
    insertIgnoringDuplicates(row, first, last, isUndoRedo);
    return true;
}

void PaletteModel::insertIgnoringDuplicates(int row, QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last, bool isUndoRedo)
{
    if (!isUndoRedo)
    {
        auto insertion = new UndoRedo::PaletteModelInsertionCommand(*this, row, QVector<AZ::Color>(first, last));
        m_undoStack->push(insertion);
    }
    else
    {
        beginInsertRows({}, row, row + aznumeric_cast<int>(std::distance(first, last)) - 1);
        m_palette->insertColorsIgnoringDuplicates(row, first, last);
        endInsertRows();
    }
}

QStringList PaletteModel::mimeTypes() const
{
    return { MIME_TYPE_PALETTE };
}

Qt::DropActions PaletteModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags PaletteModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

    if (index.isValid())
    {
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemNeverHasChildren | defaultFlags;
    }
    else
    {
        return Qt::ItemIsDropEnabled | defaultFlags;
    }
}

QMimeData* PaletteModel::mimeData(const QModelIndexList& indices) const
{
    QMimeData* mimeData = new QMimeData();
    paletteFromIndices(indices).save(mimeData);
    return mimeData;
}

bool PaletteModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(action);

    if (!data->hasFormat(MIME_TYPE_PALETTE))
    {
        return false;
    }

    if (column > 0)
    {
        return false;
    }

    Palette dropped = Unmarshal(data);
    if (dropped.colors().size() == 0)
    {
        return false;
    }

    if (m_palette->containsAnyColor(dropped.colors()))
    {
        return false;
    }

    if (parent.isValid() && dropped.colors().size() == 1)
    {
        // If we set the targetted item to the dragged color, do we have a duplicate in the palette?
        return !m_palette->colors()[parent.row()].IsClose(dropped.colors()[0]);
    }
    else if (!parent.isValid() && row != -1)
    {
        return true;
    }
    return false;
}

bool PaletteModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_UNUSED(column);

    if (action == Qt::IgnoreAction)
    {
        return true;
    }

    Palette dropped = Unmarshal(data);

    if (parent.isValid())
    {
        trySet(parent.row(), dropped.colors().at(0));
        return true;
    }

    insertIgnoringDuplicates(row, dropped.colors().cbegin(), dropped.colors().cend());
    return true;
}

bool PaletteModel::trySet(int row, const AZ::Color& color, bool isUndoRedo)
{
    if (m_palette->containsColor(color))
    {
        return false;
    }

    if (!isUndoRedo)
    {
        auto set = new UndoRedo::PaletteModelSetColorCommand(*this, row, color);
        m_undoStack->push(set);
    }
    else
    {
        m_palette->trySetColor(row, color);
        emit dataChanged(index(row), index(row), { Qt::DisplayRole, Qt::DecorationRole, ColorRole });
    }

    return true;
}

bool PaletteModel::tryRemove(int row, int count, bool isUndoRedo)
{
    if (row >= rowCount() || row + count > rowCount())
    {
        return false;
    }

    if (!isUndoRedo)
    {
        auto removal = new UndoRedo::PaletteModelRemovalCommand(*this, row, count);
        m_undoStack->push(removal);
    }
    else
    {
        beginRemoveRows({}, row, row + count - 1);
        m_palette->tryRemoveColors(row, count);
        endRemoveRows();

    }
    return true;
}

PaletteItemDelegate::PaletteItemDelegate(PaletteView* parent)
    : QAbstractItemDelegate(parent)
    , m_view(parent)
    , m_renderer(new Swatch(parent))
{
    m_renderer->hide();
}

PaletteItemDelegate::~PaletteItemDelegate()
{
}

void PaletteItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    AZ::Color color = index.data(PaletteModel::ColorRole).value<AZ::Color>();
    if (m_view->isGammaEnabled())
    {
        color = AdjustGamma(color, aznumeric_cast<float>(m_view->gamma()));
    }

    m_renderer->setColor(color);
    m_renderer->setFixedSize(option.rect.size());
    QStyleOptionFrame frameOption;
    frameOption.initFrom(m_renderer);
    frameOption.state = option.state;
    m_renderer->drawSwatch(&frameOption, painter, option.rect.topLeft());
}

QSize PaletteItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);
    return option.rect.size();
}

void PaletteItemDelegate::polish(const PaletteView::Config& configuration)
{
    m_configuration = configuration;
}

Internal::AddSwatch::AddSwatch(QWidget* parent)
    : QFrame(parent)
{
    // Avoid forced background painting
    setAttribute(Qt::WA_NoSystemBackground);

    // If we try to do this in the stylesheet, via 'icon-size' and 'icon' then the icon is set, but
    // icon-size is ignored and the icon is scaled to fill m_itemSize, even if we try to keep the
    // icon size restriction here
    m_icon.addPixmap(QPixmap(":/ColorPickerDialog/Palette/add-normal.png"), QIcon::Normal);
}

void Internal::AddSwatch::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    QStyleOptionFrame option;
    option.initFrom(this);

    style()->drawPrimitive(QStyle::PE_Frame, &option, &painter, this);

    if (!m_icon.isNull())
    {
        const QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
        const QIcon::State state = QIcon::On;
        QRect iconRect(QPoint(), m_iconSize);
        iconRect.moveCenter(rect().center());
        m_icon.paint(&painter, iconRect, Qt::AlignCenter, mode, state);
    }
}

void Internal::AddSwatch::mouseReleaseEvent(QMouseEvent* event)
{
    QFrame::mouseReleaseEvent(event);
    emit clicked();
}

void Internal::AddSwatch::keyReleaseEvent(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Select:
        case Qt::Key_Space:
            if (!event->isAutoRepeat())
            {
                emit clicked();
            }
            break;
        default:
            event->ignore();
            break;
    }
}

PaletteView::PaletteView(Palette* palette, Internal::ColorController* controller, QUndoStack* undoStack, QWidget* parent)
    : QAbstractItemView(parent)
    , m_paletteModel(new PaletteModel(palette, undoStack, this))
    , m_delegate(new PaletteItemDelegate(this))
    , m_controller(controller)
    , m_undoStack(undoStack)
    , m_itemRows(0)
    , m_itemColumns(0)
    , m_itemSize(QSize(16, 16))
    , m_itemHSpacing(4)
    , m_itemVSpacing(4)
    , m_addColorButton(new Internal::AddSwatch(viewport()))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setFocusPolicy(Qt::ClickFocus);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);

    viewport()->setContentsMargins(QMargins(0, 0, 0, 0));

    setModel(m_paletteModel);
    setItemDelegate(m_delegate);

    m_addColorButton->setObjectName(QLatin1String("addColorButton"));
    m_addColorButton->setFixedSize(m_itemSize);
    connect(m_controller, &Internal::ColorController::colorChanged, this, &PaletteView::onColorChanged);
    connect(m_addColorButton, &Internal::AddSwatch::clicked, this, [this]() {
        m_paletteModel->tryAppend(m_controller->color());

        // make sure the state of the add color button is updated after we add a new color
        // to our palette
        onColorChanged(m_controller->color());
    });
    onColorChanged(m_controller->color());

    m_context = new QMenu(this);
    m_cut = m_context->addAction(tr("Cut"), this, &PaletteView::cut, QKeySequence::Cut);
    m_copy = m_context->addAction(tr("Copy"), this, &PaletteView::copy, QKeySequence::Copy);
    m_paste = m_context->addAction(tr("Paste"), this, &PaletteView::paste, QKeySequence::Paste);
    m_remove = m_context->addAction(tr("Remove"), this, &PaletteView::removeSelection, QKeySequence::Delete);
    m_update = m_context->addAction(tr("Update to current"), this, [this]() {
        m_paletteModel->trySet(selectionModel()->currentIndex().row(), m_controller->color());
    });

    m_cut->setShortcutContext(Qt::WidgetShortcut);
    m_copy->setShortcutContext(Qt::WidgetShortcut);
    m_paste->setShortcutContext(Qt::WidgetShortcut);
    m_remove->setShortcutContext(Qt::WidgetShortcut);
    m_update->setShortcutContext(Qt::WidgetShortcut);

    addAction(m_cut);
    addAction(m_copy);
    addAction(m_paste);
    addAction(m_remove);
    addAction(m_update);

    QClipboard* clipboard = QGuiApplication::clipboard();
    auto pasteEnabler = [this]() {
        Palette available = UnmarshalFromClipboard();
        m_paste->setEnabled(m_paletteModel->lacksAnyOf(available.colors()));
    };
    connect(clipboard, &QClipboard::changed, this, pasteEnabler);

    auto updateEnabler = [this]() {
        m_update->setEnabled(selectionModel()->selectedIndexes().size() == 1 && !m_paletteModel->contains(m_controller->color()));
    };
    connect(m_controller, &Internal::ColorController::colorChanged, m_update, updateEnabler);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, m_update, updateEnabler);
    connect(m_paletteModel, &QAbstractItemModel::rowsInserted, this, updateEnabler);
    connect(m_paletteModel, &QAbstractItemModel::rowsRemoved, this, updateEnabler);
    connect(m_paletteModel, &QAbstractItemModel::dataChanged, this, updateEnabler);

    m_cut->setEnabled(false);
    m_copy->setEnabled(false);
    m_remove->setEnabled(false);
    m_update->setEnabled(false);
    pasteEnabler();
}

PaletteView::~PaletteView()
{
}

bool PaletteView::isGammaEnabled() const
{
    return m_gammaEnabled;
}

qreal PaletteView::gamma() const
{
    return m_gamma;
}

void PaletteView::addDefaultRGBColors()
{
    using namespace AZ;

    QVector<AZ::Color> colors({
        Color(u8(0xBF), u8(0x28), u8(0x28), u8(0xff)),
        Color(u8(0xE9), u8(0xA9), u8(0x44), u8(0xff)),
        Color(u8(0x84), u8(0x59), u8(0x32), u8(0xff)),
        Color(u8(0x92), u8(0xD1), u8(0x48), u8(0xff)),
        Color(u8(0xAD), u8(0x29), u8(0xD8), u8(0xff)),
        Color(u8(0x9B), u8(0x9B), u8(0x9B), u8(0xff)),
        Color(u8(0xB4), u8(0xCA), u8(0x9A), u8(0xff)),
        Color(u8(0x6E), u8(0x5D), u8(0x81), u8(0xff)),
        Color(u8(0x77), u8(0xCF), u8(0xA8), u8(0xff)),
        Color(u8(0x53), u8(0x55), u8(0x51), u8(0xff)),
        Color(u8(0x4D), u8(0x74), u8(0x21), u8(0xff))
    });

    m_paletteModel->insertIgnoringDuplicates(m_paletteModel->rowCount(), colors.cbegin(), colors.cend(), true);
}

void PaletteView::setModel(QAbstractItemModel* model)
{
    QAbstractItemView::setModel(model);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &PaletteView::handleSelectionChanged);
}

void PaletteView::setSelectionModel(QItemSelectionModel* selectionModel)
{
    QAbstractItemView::setSelectionModel(selectionModel);
    connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PaletteView::handleSelectionChanged);
}

void PaletteView::setGammaEnabled(bool enabled)
{
    if (enabled == m_gammaEnabled)
    {
        return;
    }

    m_gammaEnabled = enabled;
    update();
}

void PaletteView::setGamma(qreal gamma)
{
    if (qFuzzyCompare(gamma, m_gamma))
    {
        return;
    }

    m_gamma = gamma;
    update();
}

void PaletteView::tryAddColor(const AZ::Color& color)
{
    m_paletteModel->tryAppend(color);
}

void PaletteView::handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);

    const QModelIndexList selection = selectionModel()->selectedIndexes();

    m_cut->setEnabled(!selection.isEmpty());
    m_copy->setEnabled(!selection.isEmpty());
    m_remove->setEnabled(!selection.isEmpty());

    if (!m_aboutToShowContextMenu)
    {
        QVector<AZ::Color> selectedColors;
        std::transform(selection.cbegin(), selection.cend(), std::back_inserter(selectedColors), [](const QModelIndex i) -> AZ::Color { return i.data(PaletteModel::ColorRole).value<AZ::Color>(); });
        emit selectedColorsChanged(selectedColors);
    }
}

QModelIndex PaletteView::indexAt(const QPoint& point) const
{
    if (!model())
    {
        return {};
    }

    const int itemCount = model()->rowCount();
    for (int i = 0; i < itemCount; ++i)
    {
        if (itemGeometry(i).contains(point))
        {
            return model()->index(i, 0, rootIndex());
        }
    }

    return {};
}

void PaletteView::scrollTo(const QModelIndex& index, ScrollHint hint)
{
    Q_UNUSED(index);
    Q_UNUSED(hint);
}

QRect PaletteView::visualRect(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return {};
    }

    return itemGeometry(index.row());
}

QModelIndex PaletteView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(modifiers);

    if (!model())
    {
        return {};
    }

    int itemCount = model()->rowCount();
    if (!itemCount)
    {
        return {};
    }

    const auto moveArrow = [this, itemCount](int dr, int dc) -> QModelIndex {
        QModelIndex current = currentIndex();
        if (current.isValid())
        {
            int item = current.row();

            const int r = item/m_itemColumns + dr;
            const int c = item%m_itemColumns + dc;

            if (c >= 0 && c < m_itemColumns && r >= 0 && r < m_itemRows)
            {
                int nextItem = r*m_itemColumns + c;
                if (nextItem < itemCount)
                {
                    return model()->index(nextItem, 0, rootIndex());
                }
            }
        }
        return {};
    };

    switch (cursorAction)
    {
        case MoveHome:
            return model()->index(0, 0, rootIndex());

        case MoveEnd:
            return model()->index(itemCount - 1, 0, rootIndex());

        case MovePrevious:
            {
                QModelIndex current = currentIndex();
                if (current.isValid())
                {
                    int previousRow = current.row() - 1;
                    if (previousRow < 0)
                    {
                        previousRow = itemCount - 1;
                    }
                    return model()->index(previousRow, 0, rootIndex());
                }
            }
            break;

        case MoveNext:
            {
                QModelIndex current = currentIndex();
                if (current.isValid())
                {
                    int nextRow = current.row() + 1;
                    if (nextRow >= itemCount)
                    {
                        nextRow = 0;
                    }
                    return model()->index(nextRow, 0, rootIndex());
                }
            }
            break;

        case MoveUp:
            return moveArrow(-1, 0);

        case MoveDown:
            return moveArrow(1, 0);

        case MoveLeft:
            return moveArrow(0, -1);

        case MoveRight:
            return moveArrow(0, 1);

        case MovePageUp:
        case MovePageDown:
            // TODO
            break;
    }

    return {};
}

int PaletteView::horizontalOffset() const
{
    return 0;
}

int PaletteView::verticalOffset() const
{
    return 0;
}

bool PaletteView::isIndexHidden(const QModelIndex& index) const
{
    Q_UNUSED(index);

    // not implemented
    return false;
}

void PaletteView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags)
{
    if (!model())
    {
        return;
    }

    QItemSelection selection;
    const int itemCount = model()->rowCount();
    for (int i = 0; i < itemCount; ++i)
    {
        if (itemGeometry(i).intersects(rect))
        {
            QModelIndex index = model()->index(i, 0, rootIndex());
            selection.select(index, index);
        }
    }

    selectionModel()->select(selection, flags);
}

QRegion PaletteView::visualRegionForSelection(const QItemSelection& selection) const
{
    QRegion region;
    for (const auto& index : selection.indexes())
    {
        region += visualRect(index);
    }
    return region;
}

void PaletteView::updateGeometries()
{
    if (!model())
    {
        return;
    }

    updateItemLayout();

    const int itemCount = model()->rowCount();

    QSize buttonSize = m_addColorButton->size();
    QPoint offset{ (m_itemSize.width() - buttonSize.width()) / 2, (m_itemSize.height() - buttonSize.height()) / 2 };
    m_addColorButton->setGeometry({ itemPosition(itemCount) + offset, buttonSize });
}

QPoint PaletteView::itemPosition(int index) const
{
    const int r = index / m_itemColumns;
    const int c = index % m_itemColumns;

    const int x = viewport()->contentsRect().left() + (c * (m_itemSize.width() + m_itemHSpacing));
    const int y = viewport()->contentsRect().top() + (r * (m_itemSize.height() + m_itemVSpacing));

    return { x, y };
}

QRect PaletteView::itemGeometry(int index) const
{
    return { itemPosition(index), m_itemSize };
}

void PaletteView::paintEvent(QPaintEvent* event)
{
    QAbstractItemView::paintEvent(event);

    if (!model())
    {
        return;
    }

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int itemCount = model()->rowCount();
    for (int i = 0; i < itemCount; ++i)
    {
        const QModelIndex& index = model()->index(i, 0, rootIndex());

        QStyleOptionViewItem option;

        option.rect = itemGeometry(i);
        option.state = QStyle::State_None;

        if (selectionModel()->isSelected(index))
        {
            option.state |= QStyle::State_Selected;
        }

        if (currentIndex() == index)
        {
            option.state |= QStyle::State_HasFocus;
        }

        itemDelegate(index)->paint(&painter, option, index);
    }

    if (!m_dropIndicatorRect.isNull())
    {
        QStyleOption option;
        option.init(this);
        option.rect = m_dropIndicatorRect;
        style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &option, &painter, this);
    }
}

void PaletteView::rowsInserted(const QModelIndex& parent, int start, int end)
{
    QAbstractItemView::rowsInserted(parent, start, end);
    scheduleDelayedItemsLayout();
}

void PaletteView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
    scheduleDelayedItemsLayout();
}

void PaletteView::setItemSize(const QSize& size)
{
    if (size == m_itemSize)
    {
        return;
    }

    m_itemSize = size;
    m_addColorButton->setFixedSize(m_itemSize);

    updateItemLayout();
}

void PaletteView::setItemHorizontalSpacing(int spacing)
{
    if (m_itemHSpacing == spacing)
    {
        return;
    }

    m_itemHSpacing = spacing;

    updateItemLayout();
}

void PaletteView::setItemVerticalSpacing(int spacing)
{
    if (m_itemVSpacing == spacing)
    {
        return;
    }

    m_itemVSpacing = spacing;

    updateItemLayout();
}

bool PaletteView::contains(const AZ::Color& color) const
{
    return m_paletteModel->contains(color);
}

void PaletteView::setMarginSize(const QSize& size)
{
    const QMargins margins(size.width(), size.height(), size.width(), size.height());

    if (viewport()->contentsMargins() == margins)
    {
        return;
    }

    viewport()->setContentsMargins(margins);
    updateItemLayout();
}

QSize PaletteView::sizeHint() const
{
    return m_preferredSize;
}

QSize PaletteView::minimumSizeHint() const
{
    return { 8, 8 };
}

void PaletteView::resizeEvent(QResizeEvent* event)
{
    QAbstractItemView::resizeEvent(event);

    updateItemLayout();
}

// If signals are not blocked on a right-click, then when any index that was right-clicked on
// is selected by the QAbstractItemView implementation (we actually want this) we will emit
// PaletteView::selectionChanged and cause the ColorController to have its current color
// updated.
// Signals are re-enabled in the contextMenuEvent handler and for safety in a timer on the
// main event queue.
void PaletteView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        m_aboutToShowContextMenu = true;
    }
    QAbstractItemView::mousePressEvent(event);
}

void PaletteView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        // Put something at the end of the event queue, after the context menu has been processed, if it's
        // going to then this will guarantee that no matter what, signals will always be emitted even if the
        // context menu doesn't show for some reason
        QTimer::singleShot(0, this, [this]() {
            m_aboutToShowContextMenu = false;
        });
    }
    QAbstractItemView::mouseReleaseEvent(event);
}

void PaletteView::contextMenuEvent(QContextMenuEvent* event)
{
    if (indexAt(event->pos()).isValid())
    {
        m_context->exec(event->globalPos());
    }
    else
    {
        Q_EMIT unselectedContextMenuRequested(event->globalPos());
    }

    m_aboutToShowContextMenu = false;
}

void PaletteView::updateItemLayout()
{
    if (!model())
    {
        return;
    }

    const int itemCount = model()->rowCount() + 1; // model items plus add button
    const int usableWidth = viewport()->contentsRect().width();

    m_itemColumns = qMax((usableWidth + m_itemHSpacing) / (m_itemSize.width() + m_itemHSpacing), 1);
    m_itemRows = (itemCount + m_itemColumns - 1) / m_itemColumns;

    const int width = viewport()->contentsMargins().left()
            + m_itemColumns * (m_itemSize.width() + m_itemHSpacing)
            - (m_itemColumns == 0 ? 0 : m_itemHSpacing)
            + viewport()->contentsMargins().right();
    const int height = viewport()->contentsMargins().top()
            + m_itemRows * (m_itemSize.height() + m_itemVSpacing)
            - (m_itemRows == 0 ? 0 : m_itemVSpacing)
            + viewport()->contentsMargins().bottom();

    QSize size{ width + (2 * frameWidth()), height + (2 * frameWidth()) };
    if (size != m_preferredSize)
    {
        m_preferredSize = size;
        updateGeometry();
    }
}

void PaletteView::removeSelection()
{
    auto selection = selectionModel()->selection();
    std::sort(selection.begin(), selection.end(), [](const QItemSelectionRange& l, const QItemSelectionRange& r) {
        return l.top() > r.top();
    });
    for (QItemSelectionRange range : selection)
    {
        m_paletteModel->tryRemove(range.top(), range.height());
    }
}

void PaletteView::cut()
{
    m_undoStack->beginMacro(tr("Cut swatches"));
    copy();
    removeSelection();
    m_undoStack->endMacro();
}

void PaletteView::copy() const
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    const auto selection = selectionModel()->selectedIndexes();
    QMimeData* mimeData = new QMimeData();
    m_paletteModel->paletteFromIndices(selection).save(mimeData);
    clipboard->setMimeData(mimeData);
}

void PaletteView::paste()
{
    m_undoStack->beginMacro(tr("Paste swatches"));
    Palette palette = UnmarshalFromClipboard();
    QVector<AZ::Color> pastedColors = palette.colors();

    // Turn the selection into a set of colors
    const auto selection = selectionModel()->selectedIndexes();
    QVector<AZ::Color> selectedColors = m_paletteModel->colorsFromIndices(selection);

    // We aren't interested in pasting colors that are already in the palette, unless they're
    // selected and will be overwritten (because this could change their position). We will,
    // however, select any pre-existing colors after we finish.
    QVector<AZ::Color> colorsToSelect;
    auto pasted = pastedColors.begin();
    while (pasted != pastedColors.end())
    {
        if (m_paletteModel->contains(*pasted))
        {
            if (std::none_of(selectedColors.cbegin(), selectedColors.cend(), [pasted](AZ::Color c) { return c.IsClose(*pasted); }))
            {
                // The color being pasted isn't selected, so we won't overwrite it, but we will select it later
                colorsToSelect.append(*pasted);
                pasted = pastedColors.erase(pasted);
                continue;
            }
        }

        // The color is selected, so it'll be overwritten OR it's new
        ++pasted;
    }

    QItemSelection updatedSelection;

    // Overwrite any selected colors
    auto source = pastedColors.cbegin();
    auto target = selection.cbegin();
    while (source != pastedColors.cend() && target != selection.cend()) {
        m_paletteModel->trySet(target->row(), *source);
        updatedSelection.select(*target, *target);
        ++source;
        ++target;
    }

    if (source != pastedColors.cend())
    {
        // Append the rest of the colors to the end of the selection or palette (if there's no selection)
        int row = selection.isEmpty() ? model()->rowCount() : selection.last().row() + 1;
        m_paletteModel->tryInsert(row, source, pastedColors.cend());
        for (int r = row; r < m_paletteModel->rowCount(); ++r)
        {
            QModelIndex index = m_paletteModel->index(r);
            updatedSelection.select(index, index);
        }
    }
    else if (target != selection.cend())
    {
        // Delete any other things that were selected but not replaced
        while (target != selection.cend())
        {
            m_paletteModel->tryRemove(target->row());
            ++target;
        }
    }

    // Select those pre-existing colors we identified earlier
    for (const AZ::Color& existing : colorsToSelect)
    {
        QModelIndex index = m_paletteModel->index(m_paletteModel->indexOf(existing));
        updatedSelection.select(index, index);
    }

    selectionModel()->select(updatedSelection, QItemSelectionModel::Select);
    m_undoStack->endMacro();
}

PaletteView::Config PaletteView::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();
    ReadDropIndicator(settings, QStringLiteral("DropIndicator"), config.dropIndicator);
    ConfigHelpers::read<QSize>(settings, QStringLiteral("MarginSize"), config.marginSize);
    ConfigHelpers::read<int>(settings, QStringLiteral("HorizontalSpacing"), config.horizontalSpacing);
    ConfigHelpers::read<int>(settings, QStringLiteral("VerticalSpacing"), config.verticalSpacing);
    return config;
}

PaletteView::Config PaletteView::defaultConfig()
{
    return {
        DropIndicator{
            1,
            QColor{Qt::white} // Color
        },
        QSize(0, 0),
        4,
        4
    };
}

bool PaletteView::polish(Style* style, QWidget* widget, const PaletteView::Config& config)
{
    PaletteView* paletteView = qobject_cast<PaletteView*>(widget);
    if (paletteView == nullptr)
    {
        return false;
    }

    paletteView->m_delegate->polish(config);
    paletteView->setMarginSize(config.marginSize);
    paletteView->setItemHorizontalSpacing(config.horizontalSpacing);
    paletteView->setItemVerticalSpacing(config.verticalSpacing);
    style->repolishOnSettingsChange(paletteView);

    return true;
}

bool PaletteView::drawDropIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const PaletteView::Config& config)
{
    Q_UNUSED(style);

    const PaletteView* view = qobject_cast<const PaletteView*>(widget);
    if (!view)
    {
        return false;
    }

    QPen pen(config.dropIndicator.color);
    pen.setWidth(config.dropIndicator.width);
    painter->setPen(pen);

    if (view->m_droppingOn)
    {
        painter->setBrush(Qt::NoBrush);
        QRect adjusted = option->rect.adjusted(-config.dropIndicator.width, -config.dropIndicator.width, config.dropIndicator.width, config.dropIndicator.width);
        painter->drawRoundedRect(adjusted, 2.0, 2.0);
    }
    else
    {
        painter->setBrush(config.dropIndicator.color);

        QPoint top = { option->rect.center().x(), option->rect.top() };
        QPoint bottom = { option->rect.center().x(), option->rect.bottom() };
        painter->drawLine(top, bottom);
    }

    return true;
}

bool PaletteView::canDrop(QDropEvent* event)
{
    QModelIndex index;
    int row;
    return canDrop(event, index, row);
}

bool PaletteView::canDrop(QDropEvent* event, QModelIndex& index, int& row)
{
    index = indexAt(event->pos());
    row = getRootRow(event->pos(), index);

    m_droppingOn = false;
    m_dropIndicatorRect = {};
    viewport()->update();

    bool setIndicator = true;
    if (event->source() == this)
    {
        if (index.isValid()) {
            row = index.row();
            index = {};
        }

        int firstSelectedRow = selectedIndexes().first().row();
        if (row == -1 || firstSelectedRow == row || firstSelectedRow + 1 == row)
        {
            setIndicator = false;
        }
    }

    if (event->source() == this || model()->canDropMimeData(event->mimeData(), event->dropAction(), row, -1, index))
    {
        if (setIndicator)
        {
            m_droppingOn = row == -1;
            m_dropIndicatorRect = getDropIndicatorRect(index, row);
        }
        return true;
    }

    return false;
}

void PaletteView::startDrag(Qt::DropActions supportedActions)
{
    m_undoStack->beginMacro(tr("Drag swatches"));
    QAbstractItemView::startDrag(supportedActions);
    m_undoStack->endMacro();
}

void PaletteView::dragEnterEvent(QDragEnterEvent* event)
{
    if (canDrop(event))
    {
        event->accept();
        setState(DraggingState);
    }
    else
    {
        event->ignore();
    }
}

void PaletteView::dragMoveEvent(QDragMoveEvent* event)
{
    if (canDrop(event))
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

void PaletteView::dragLeaveEvent(QDragLeaveEvent* event)
{
    QAbstractItemView::dragLeaveEvent(event);

    m_droppingOn = false;
    m_dropIndicatorRect = {};
}

void PaletteView::dropEvent(QDropEvent* event)
{
    QModelIndex index;
    int row;
    bool ok = canDrop(event, index, row);

    if (ok && model()->dropMimeData(event->mimeData(), event->dropAction(), row, -1, index))
    {
        event->acceptProposedAction();
    }
    m_dropIndicatorRect = {};
    stopAutoScroll();
    setState(NoState);
    viewport()->update();
}

QRect PaletteView::getDropIndicatorRect(QModelIndex index, int row) const
{
    if (!index.isValid() && row == -1)
    {
        return {};
    }

    if (index.isValid())
    {
        return itemGeometry(index.row());
    }

    QRect geometry = itemGeometry(row);
    QPoint marginAdjust = QPoint{ m_itemHSpacing, 0 };
    return { geometry.topLeft() - marginAdjust, geometry.bottomLeft() };
}

int PaletteView::getRootRow(const QPoint point, const QModelIndex index) const
{
    if (!index.isValid())
    {
        const int itemCount = model()->rowCount();
        for (int i = 0; i < itemCount; ++i)
        {
            int w = m_itemHSpacing;
            int h = m_itemVSpacing;
            QRect margin = itemGeometry(i).adjusted(-w, -h, 0, h);
            if (margin.contains(point)) {
                return i;
            }
        }

        return itemCount;
    }

    return -1;
}

void PaletteView::onColorChanged(const AZ::Color& c)
{
    m_addColorButton->setEnabled(!m_paletteModel->contains(c));
}

} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_PaletteView.cpp"

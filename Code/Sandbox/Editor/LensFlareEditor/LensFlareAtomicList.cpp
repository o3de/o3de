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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorDefs.h"

#include "LensFlareAtomicList.h"

// Qt
#include <QScrollBar>
#include <QMimeData>

// Editor
#include "LensFlareUtil.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"


struct QLensFlareAtomicListModel::Item
{
    QString text;
    QSize size;
    QPixmap pixmap;
    EFlareType flareType;
};

CLensFlareAtomicList::CLensFlareAtomicList(QWidget* parent)
    : CImageListCtrl(parent)
    , m_model(new QLensFlareAtomicListModel(this))
{
    setDragEnabled(true);
    setDragDropMode(DragOnly);
    setModel(m_model.data());
}

CLensFlareAtomicList::~CLensFlareAtomicList()
{
}

QModelIndex QLensFlareAtomicListModel::InsertItem(const FlareInfo& flareInfo)
{
    Item* pPreviewItem = new Item;

    if (flareInfo.imagename)
    {
        CImageEx* pImage = new CImageEx();
        if (CImageUtil::LoadImage(flareInfo.imagename, *pImage))
        {
            pImage->SwapRedAndBlue();
            pPreviewItem->size = QSize(pImage->GetWidth(), pImage->GetHeight());
            QImage img(reinterpret_cast<const uchar*>(pImage->GetData()), pImage->GetWidth(), pImage->GetHeight(), QImage::Format_RGB32);
            pPreviewItem->pixmap = QPixmap::fromImage(img.copy());
        }

        delete pImage;
    }

    if (pPreviewItem->pixmap.isNull())
    {
        pPreviewItem->pixmap = QPixmap(":/water.png");
        pPreviewItem->size = QSize(64, 64);
    }

    pPreviewItem->text = flareInfo.name;
    pPreviewItem->flareType = flareInfo.type;

    const int row = m_items.count();
    beginInsertRows(QModelIndex(), row, row);
    m_items.append(pPreviewItem);
    endInsertRows();

    return index(row, 0);
}

void QLensFlareAtomicListModel::Populate()
{
    Clear();
    const FlareInfoArray::Props array = FlareInfoArray::Get();
    for (size_t i = 0; i < array.size; ++i)
    {
        const FlareInfo& flareInfo(array.p[i]);
        if (LensFlareUtil::IsElement(flareInfo.type))
        {
            InsertItem(flareInfo);
        }
    }
}

void CLensFlareAtomicList::FillAtomicItems()
{
    if (m_model)
    {
        m_model->Populate();
    }
}

void CLensFlareAtomicList::updateGeometries()
{
    ClearItemGeometries();

    if (!model())
    {
        return;
    }

    const int rowCount = model()->rowCount();

    const int nPageHorz = viewport()->width();
    const int nPageVert = viewport()->height();

    if (nPageHorz == 0 || nPageVert == 0 || rowCount <= 0)
    {
        return;
    }

    const QSize& borderSize = BorderSize();
    int x = borderSize.width();
    int y = borderSize.height();

    QSize itemSize = ItemSize();
    const int nTextHeight = fontMetrics().height();
    const int xMax = nPageHorz - borderSize.width();
    int itemHeightMax = 0;

    for (int row = 0; row < rowCount; ++row)
    {
        QModelIndex index = m_model->index(row, 0);
        itemSize = index.data(Qt::SizeHintRole).toSize();

        if ((x + itemSize.width()) > xMax)
        {
            y += itemHeightMax + borderSize.height() + nTextHeight;
            x = borderSize.width();
            itemHeightMax = 0;
        }

        if (itemSize.height() > itemHeightMax)
        {
            itemHeightMax = itemSize.height();
        }

        SetItemGeometry(index, QRect(QPoint(x, y), itemSize));

        x += itemSize.width() + borderSize.width();
    }

    verticalScrollBar()->setPageStep(viewport()->height());
    verticalScrollBar()->setRange(0, (y + itemHeightMax - viewport()->height()));
}

QLensFlareAtomicListModel::QLensFlareAtomicListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

QLensFlareAtomicListModel::~QLensFlareAtomicListModel()
{
}

void QLensFlareAtomicListModel::Clear()
{
    qDeleteAll(m_items);
    m_items.clear();
}

int QLensFlareAtomicListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return m_items.count();
}

QVariant QLensFlareAtomicListModel::data(const QModelIndex& index, int role) const
{
    Item* item;
    if (!index.isValid())
    {
        return QVariant();
    }
    else
    {
        item = ItemFromIndex(index);
        if (!item)
        {
            return QVariant();
        }
    }

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return item->text;

    case Qt::SizeHintRole:
        return item->size;

    case Qt::DecorationRole:
        return item->pixmap;

    case Qt::UserRole:
        return item->flareType;
    }

    return QVariant();
}

bool QLensFlareAtomicListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Item* item;
    if (index.isValid())
    {
        return false;
    }
    else
    {
        item = ItemFromIndex(index);
        if (!item)
        {
            return false;
        }
    }

    switch (role)
    {
    case Qt::EditRole:
        item->text = value.toString();
        break;

    case Qt::DecorationRole:
        item->pixmap = value.value<QPixmap>();
        break;

    case Qt::SizeHintRole:
        item->size = value.toSize();
        break;

    default:
        return false;
    }

    emit dataChanged(index, index, QVector<int>() << role);
    return false;
}

Qt::ItemFlags QLensFlareAtomicListModel::flags(const QModelIndex& index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled;
}

EFlareType QLensFlareAtomicListModel::FlareTypeFromIndex(QModelIndex index) const
{
    Item* item;
    if (!index.isValid())
    {
        return eFT_Max;
    }
    else
    {
        item = ItemFromIndex(index);
        if (!item)
        {
            return eFT_Max;
        }
    }

    return item->flareType;
}

QLensFlareAtomicListModel::Item* QLensFlareAtomicListModel::ItemFromIndex(QModelIndex index) const
{
    if (!index.isValid())
    {
        return nullptr;
    }

    return m_items.at(index.row());
}

QStringList QLensFlareAtomicListModel::mimeTypes() const
{
    return {
               QStringLiteral("application/x-o3de-flaretypes")
    };
}

QMimeData* QLensFlareAtomicListModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* data = new QMimeData();

    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);

    for (const QModelIndex& index : indexes)
    {
        stream << static_cast<int>(FlareTypeFromIndex(index));
    }

    data->setData(QStringLiteral("application/x-o3de-flaretypes"), encoded);

    return data;
}

bool QLensFlareAtomicListModel::dropMimeData([[maybe_unused]] const QMimeData* data, [[maybe_unused]] Qt::DropAction action, [[maybe_unused]] int row, [[maybe_unused]] int column, [[maybe_unused]] const QModelIndex& parent)
{
    return false;
}

Qt::DropActions QLensFlareAtomicListModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

#include <LensFlareEditor/moc_LensFlareAtomicList.cpp>

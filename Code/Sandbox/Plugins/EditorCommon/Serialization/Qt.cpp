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

#include "EditorCommon_precompiled.h"
#include "EditorCommonAPI.h"
#include "Serialization/Serializer.h"
#include "Serialization/Qt.h"
#include "Serialization/STL.h"
#include "Serialization/IArchive.h"
#include <QSplitter>
#include <QString>
#include <QTreeView>
#include <QHeaderView>
#include <QPalette>

class StringQt
    : public Serialization::IWString
{
public:
    StringQt(QString& str)
        : str_(str) {}

    void set(const wchar_t* value) { str_.setUnicode((const QChar*)value, (int)wcslen(value)); }
    const wchar_t* get() const { return (wchar_t*)str_.data(); }
    const void* handle() const { return &str_; }
    Serialization::TypeID type() const{ return Serialization::TypeID::get<QString>(); }
private:
    QString& str_;
};

bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QString& value, const char* name, const char* label)
{
    StringQt str(value);
    return ar(static_cast<Serialization::IWString&>(str), name, label);
}

bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QByteArray& byteArray, const char* name, const char* label)
{
    std::vector<unsigned char> temp(byteArray.begin(), byteArray.end());
    if (!ar(temp, name, label))
    {
        return false;
    }
    if (ar.IsInput())
    {
        byteArray = QByteArray(temp.empty() ? (char*)0 : (char*)&temp[0], (int)temp.size());
    }
    return true;
}

QString GetIndexPath(QAbstractItemModel* model, const QModelIndex& index)
{
    QString path;

    QModelIndex cur = index;
    while (cur.isValid() && cur != QModelIndex())
    {
        if (!path.isEmpty())
        {
            path = QString("|") + path;
        }
        path = model->data(cur).toString() + path;
        cur = model->parent(cur);
    }
    return path;
}

QModelIndex FindIndexChildByText(QAbstractItemModel* model, const QModelIndex& parent, const QString& text)
{
    int rowCount = model->rowCount(parent);
    for (int i = 0; i < rowCount; ++i)
    {
        QModelIndex child = model->index(i, 0, parent);
        QString childText = model->data(child).toString();
        if (childText == text)
        {
            return child;
        }
    }
    return QModelIndex();
}

QModelIndex GetIndexByPath(QAbstractItemModel* model, const QString& path)
{
    QStringList items = path.split('|');
    QModelIndex cur = QModelIndex();
    for (int i = 0; i < items.size(); ++i)
    {
        cur = FindIndexChildByText(model, cur, items[i]);
        if (!cur.isValid())
        {
            return QModelIndex();
        }
    }
    return cur;
}

std::vector<QString> GetIndexPaths(QAbstractItemModel* model, const QModelIndexList& indices)
{
    std::vector<QString> result;
    for (int i = 0; i < indices.size(); ++i)
    {
        QString path = GetIndexPath(model, indices[i]);
        if (!path.isEmpty())
        {
            result.push_back(path);
        }
    }
    return result;
}

QModelIndexList GetIndicesByPath(QAbstractItemModel* model, const std::vector<QString>& paths)
{
    QModelIndexList result;
    for (int i = 0; i < paths.size(); ++i)
    {
        QModelIndex index = GetIndexByPath(model, paths[i]);
        if (index.isValid())
        {
            result.push_back(index);
        }
    }
    return result;
}

struct QTreeViewStateSerializer
{
    QTreeView* treeView;
    QTreeViewStateSerializer(QTreeView* treeView)
        : treeView(treeView) {}

    void Serialize(Serialization::IArchive& ar)
    {
        QAbstractItemModel* model = treeView->model();

        std::vector<QString> expandedItems;
        if (ar.IsOutput())
        {
            std::vector<QModelIndex> stack;
            stack.push_back(QModelIndex());
            while (!stack.empty())
            {
                QModelIndex index = stack.back();
                stack.pop_back();

                int rowCount = model->rowCount(index);
                for (int i = 0; i < rowCount; ++i)
                {
                    QModelIndex child = model->index(i, 0, index);
                    if (treeView->isExpanded(child))
                    {
                        stack.push_back(child);
                        expandedItems.push_back(GetIndexPath(model, child));
                    }
                }
            }
        }
        ar(expandedItems, "expandedItems");
        if (ar.IsInput())
        {
            treeView->collapseAll();
            for (size_t i = 0; i < expandedItems.size(); ++i)
            {
                QModelIndex index = GetIndexByPath(model, expandedItems[i]);
                if (index.isValid())
                {
                    treeView->expand(index);
                }
            }
        }

        std::vector<QString> selectedItems;
        if (ar.IsOutput())
        {
            selectedItems = GetIndexPaths(model, treeView->selectionModel()->selectedIndexes());
        }
        ar(selectedItems, "selectedItems");
        if (ar.IsInput())
        {
            QModelIndexList indices = GetIndicesByPath(model, selectedItems);
            if (!indices.empty())
            {
                treeView->selectionModel()->select(QModelIndex(), QItemSelectionModel::ClearAndSelect);
                for (int i = 0; i < indices.size(); ++i)
                {
                    treeView->selectionModel()->select(indices[i], QItemSelectionModel::Select);
                }
            }
        }

        QString currentItem;
        if (ar.IsOutput())
        {
            currentItem = GetIndexPath(model, treeView->selectionModel()->currentIndex());
        }
        ar(currentItem, "currentItem");
        if (ar.IsInput())
        {
            QModelIndex currentIndex = GetIndexByPath(model, currentItem);
            treeView->scrollTo(currentIndex, QAbstractItemView::PositionAtCenter);
            treeView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::Current);
        }

        std::vector<int> sectionsHidden;
        std::vector<int> sectionsVisible;
        if (ar.IsOutput())
        {
            for (int i = 0; i < treeView->model()->columnCount(); ++i)
            {
                if (treeView->header()->isSectionHidden(i))
                {
                    sectionsHidden.push_back(i);
                }
                else
                {
                    sectionsVisible.push_back(i);
                }
            }
        }
        ar(sectionsHidden, "sectionsHidden");
        ar(sectionsVisible, "sectionsVisible");
        if (ar.IsInput())
        {
            int columnCount = treeView->model()->columnCount();
            for (int i = 0; i < sectionsHidden.size(); ++i)
            {
                int section = sectionsHidden[i];
                if (section >= 0 && section < columnCount)
                {
                    treeView->header()->hideSection(section);
                }
            }
            for (int i = 0; i < sectionsVisible.size(); ++i)
            {
                int section = sectionsVisible[i];
                if (section >= 0 && section < columnCount)
                {
                    treeView->header()->showSection(section);
                }
            }
        }
    }
};

bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QTreeView* treeView, const char* name, const char* label)
{
    return ar(QTreeViewStateSerializer(treeView), name, label);
}


static const char* g_paletteColorGroupNames[QPalette::NColorGroups] = {
    "Active", "Disabled", "Inactive"
};

static const char* g_paletteColorRoleNames[QPalette::NColorRoles] = {
    "WindowText", "Button", "Light", "Midlight", "Dark", "Mid",
    "Text", "BrightText", "ButtonText", "Base", "Window", "Shadow",
    "Highlight", "HighlightedText",
    "Link", "LinkVisited",
    "AlternateBase",
    "NoRole",
    "ToolTipBase", "ToolTipText",
#if !defined(AZ_PLATFORM_LINUX)
    "PlaceholderText",
#endif // !defined(AZ_PLATFORM_LINUX)
};

struct QPaletteSerializable
{
    QPalette& palette;
    QPaletteSerializable(QPalette& palette)
        : palette(palette)
    {
    }

    struct SRole
    {
        int role;
        QPalette& palette;

        SRole(QPalette& palette, int role)
            : palette(palette)
            , role(role)
        {
        }

        void Serialize(Serialization::IArchive& ar)
        {
            for (int group = 0; group < QPalette::NColorGroups; ++group)
            {
                QColor color = palette.color(QPalette::ColorGroup(group), QPalette::ColorRole(role));
                ar(color, g_paletteColorGroupNames[group], g_paletteColorGroupNames[group]);
                if (ar.IsInput())
                {
                    palette.setColor(QPalette::ColorGroup(group), QPalette::ColorRole(role), color);
                }
            }
        }
    };

    void Serialize(Serialization::IArchive& ar)
    {
        for (int roleIndex = 0; roleIndex < QPalette::NColorRoles; ++roleIndex)
        {
            SRole role(palette, roleIndex);
            ar(role, g_paletteColorRoleNames[roleIndex], g_paletteColorRoleNames[roleIndex]);
        }
    }
};

bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QPalette& palette, const char* name, const char* label)
{
    QPaletteSerializable serializer(palette);
    return ar(serializer, name, label);
}

struct QColorSerializable
{
    QColor& color;
    QColorSerializable(QColor& color)
        : color(color) {}

    void Serialize(Serialization::IArchive& ar)
    {
        // this is not comprehensive, as QColor can store color components
        // in diffrent models, depending on the way they were specified
        unsigned char r = color.red();
        unsigned char g = color.green();
        unsigned char b = color.blue();
        unsigned char a = color.alpha();
        ar(r, "r", "^R");
        ar(g, "g", "^G");
        ar(b, "b", "^B");
        ar(a, "a", "^A");
        if (ar.IsInput())
        {
            color.setRed(r);
            color.setGreen(g);
            color.setBlue(b);
            color.setAlpha(a);
        }
    }
};

bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QColor& color, const char* name, const char* label)
{
    QColorSerializable serializer(color);
    return ar(serializer, name, label);
}

struct QSplitterSerializer
{
    QSplitter& splitter;

    QSplitterSerializer(QSplitter& splitter)
        : splitter(splitter) {}

    void Serialize(Serialization::IArchive& ar)
    {
        QList<int> qsizes = splitter.sizes();
        std::vector<int> sizes(qsizes.begin(), qsizes.end());
        ar(sizes, "sizes", "Sizes");
        if (ar.IsInput())
        {
            qsizes.clear();
            for (int i = 0; i < sizes.size(); ++i)
            {
                qsizes.push_back(sizes[i]);
            }
            splitter.setSizes(qsizes);
        }
    }
};

bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QSplitter* splitter, const char* name, const char* label)
{
    if (!splitter)
    {
        return false;
    }
    QSplitterSerializer serializer(*splitter);
    return ar(serializer, name, label);
}

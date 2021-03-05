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
#ifndef QABSTRACTQVARIANTTREEDATAMODEL_H
#define QABSTRACTQVARIANTTREEDATAMODEL_H

#include <QAbstractItemModel>
#include <memory>
#include <vector>

#include "EditorCommonAPI.h"


class EDITOR_COMMON_API QAbstractQVariantTreeDataModel
    : public QAbstractItemModel
{
public:
    QAbstractQVariantTreeDataModel(QObject* parent)
        : QAbstractItemModel(parent) { }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

protected:
    struct Folder;

    struct Item
    {
        Item()
            : m_parent(0) { }

        QMap<int, QVariant> m_data;
        Folder* m_parent;

        virtual ~Item() = default;
        virtual const Folder* asFolder() const { return 0; }        // need this as we don't have RTTI
    };

    struct Folder
        : public Item
    {
        Folder(const QString& name)
        {
            m_data.insert(Qt::DisplayRole, name);
        }
        std::vector < std::shared_ptr<Item> >   m_children;
        const Folder* asFolder() const override { return this; }    // need this as we don't have RTTI

        int row(Item* item) const
        {
            for (int i = 0; i < m_children.size(); ++i)
            {
                if (m_children[i].get() == item)
                {
                    return i;
                }
            }
            return -1;
        }
        void addChild(std::shared_ptr<Item> item)
        {
            item->m_parent = this;
            m_children.push_back(item);
        }
    };

    Item* itemFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromItem(Item* item, int col = 0) const;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    std::shared_ptr<Folder> m_root;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // QABSTRACTQVARIANTTREEDATAMODEL_H

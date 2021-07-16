/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AssetBrowserFolderPage.h"
#include <AzQtComponents/Gallery/ui_AssetBrowserFolderPage.h>

#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserFolderWidget.h>
#include <AzCore/Casting/numeric_cast.h>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QIcon>

namespace
{
    class MockupAssetModel : public QAbstractItemModel
    {
    public:
        MockupAssetModel(QObject* parent = nullptr)
            : QAbstractItemModel(parent)
        {
            m_data.append({"01_Planes_Materials", QPixmap{":/MockupAssetModel/01_Planes_Materials.png"}, "15 MB", "Folder"});

            Item airshipItem{"01_Airship.fbx", QPixmap{":/MockupAssetModel/01_Redpatch.png"}, "106 KB", "FBX"};
            airshipItem.children.append({"Gray", QPixmap{":/MockupAssetModel/01_Cave.png"}, "4.3 GB"});
            airshipItem.children.append({"Concrete", QPixmap{":/MockupAssetModel/01_Redpatch.png"}, "150 MB"});
            airshipItem.children.append({"Steel Gray", QPixmap{":/MockupAssetModel/01_Concrete.png"}, "15 MB"});
            airshipItem.children.append({"Black", QPixmap{":/MockupAssetModel/01_Cave.png"}, "2 GB"});
            airshipItem.children.append({"Gray_2", QPixmap{":/MockupAssetModel/01_Redpatch.png"}, "156 MB"});
            airshipItem.children.append({"Concrete_2", QPixmap{":/MockupAssetModel/01_Concrete.png"}, "899 MB"});
            airshipItem.children.append({"Steel Gray_2", QPixmap{":/MockupAssetModel/01_Cave.png"}, "200 MB"});
            airshipItem.children.append({"Black_2", QPixmap{":/MockupAssetModel/01_Redpatch.png"}, "6.4 GB"});
            airshipItem.children.append({"Gray_3", QPixmap{":/MockupAssetModel/01_Concrete.png"}, "8 MB"});
            m_data.append(airshipItem);

            m_data.append({"01_helicopters.png", QPixmap{":/MockupAssetModel/01_Concrete.png"}, "95 MB", "PNG"});
            m_data.append({"01_Redpatch.mtl", QPixmap{":/MockupAssetModel/01_Redpatch.png"}, "12 MB", "PNG"});
            m_data.append({"01_Concrete.tga", QPixmap{":/MockupAssetModel/01_Concrete.png"}, "197 MB", "PNG"});
            m_data.append({"01_Cave.tga", QPixmap{":/MockupAssetModel/01_Cave.png"}, "877 MB", "PNG"});
        }

        QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override
        {
            if (!parent.isValid())
            {
                if (row >= 0 && row < m_data.count() && column >= 0 && column < 3)
                {
                    return createIndex(row, column);
                }
            }
            else
            {
                if (parent.row() >= 0 && parent.row() < m_data.count() && column >= 0 && column < 3)
                {
                    const auto& parentItem = m_data[parent.row()];
                    if (row >= 0 && row < parentItem.children.count())
                    {
                        return createIndex(row, column, const_cast<Item*>(&parentItem));
                    }
                }
            }

            return {};
        }

        QModelIndex parent(const QModelIndex& index) const override
        {
            if (index.isValid() && index.internalPointer() != nullptr)
            {
                const auto parentItem = reinterpret_cast<Item*>(index.internalPointer());
                auto it = std::find_if(m_data.begin(), m_data.end(), [parentItem](const Item& item) {
                    return parentItem == &item;
                });
                if (it != m_data.end())
                {
                    return createIndex(aznumeric_cast<int>(std::distance(m_data.begin(), it)), 0);
                }
            }
            return {};
        }

        int rowCount(const QModelIndex& parent = {}) const override
        {
            if (!parent.isValid())
            {
                return m_data.count();
            }
            else
            {
                if (parent.internalPointer() == nullptr)
                {
                    const auto& parentItem = m_data[parent.row()];
                    return parentItem.children.count();
                }
            }
            return 0;
        }

        int columnCount(const QModelIndex& parent = {}) const override
        {
            return !parent.isValid() || parent.column() == 0 ? 3 : 0;
        }

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
        {
            if (hasIndex(index.row(), index.column(), index.parent()))
            {
                const auto item = [this, &index]() -> const Item* {
                    const auto parentItem = reinterpret_cast<Item*>(index.internalPointer());
                    if (parentItem == nullptr)
                    {
                        if (index.row() >= 0 && index.row() < m_data.count())
                        {
                            return &m_data[index.row()];
                        }
                    }
                    else
                    {
                        if (index.row() >= 0 && index.row() < parentItem->children.count())
                        {
                            return &parentItem->children[index.row()];
                        }
                    }
                    return nullptr;
                }();
                if (item == nullptr)
                {
                    return {};
                }

                switch (role)
                {
                    case Qt::DisplayRole:
                    {
                        switch (index.column())
                        {
                        case 0: return item->name;
                        case 1: return item->size;
                        case 2: return item->type;
                        }
                        break;
                    }

                    case Qt::DecorationRole:
                    {
                        if (index.column() == 0)
                        {
                            return item->icon;
                        }
                        break;
                    }

                    case Qt::FontRole:
                    {
                        QFont font = QApplication::font("QTreeView");
                        font.setItalic(index.column() == 0 && index.parent().isValid());
                        return QVariant::fromValue(font);
                    }
                }
            }

            return {};
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
        {
            if (orientation == Qt::Horizontal)
            {
                if (role == Qt::DisplayRole)
                {
                    switch (section)
                    {
                        case 0: return tr("Name");
                        case 1: return tr("Size");
                        case 2: return tr("Type");
                    }
                }
            }

            return QAbstractItemModel::headerData(section, orientation, role);
        }

        void loadNewPath(const QString& newPath)
        {
            qDebug() << "Model load path" << newPath;
        }

    private:
        struct Item
        {
            QString name;
            QIcon icon;
            QString size;
            QString type;
            QVector<Item> children;
        };
        QVector<Item> m_data;
    };
}

AssetBrowserFolderPage::AssetBrowserFolderPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::AssetBrowserFolderPage)
{
    ui->setupUi(this);

    QString exampleText = R"(

AssetBrowserFolderWidget is a view for a model with the asset structure. The model should provide thumbnails for the assets in Qt::DecorationRole.

<pre>
#include &lt;AzToolsFramework/AssetBrowser/Views/AssetBrowserFolderWidget.h&gt;

AzQtComponents::AssetBrowserFolderWidget *assetBrowserFolder;
QAbstractItemModel *assetModel;

assetBrowserFolder->setModel(assetModel);
</pre>

)";

    auto model = new MockupAssetModel(this);
    auto proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setDynamicSortFilter(true);
    proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxy->setSortRole(Qt::DisplayRole);
    proxy->setSortLocaleAware(true);
    ui->folderWidget->setModel(proxy);
    ui->folderWidget->setCurrentPath(QDir::currentPath());
    connect(ui->folderWidget, &AzToolsFramework::AssetBrowser::AssetBrowserFolderWidget::pathChanged, this, [this, model](const QString& newPath) {
        model->loadNewPath(newPath);
        ui->folderWidget->setCurrentPath(newPath);
    });

    ui->exampleText->setHtml(exampleText);
}

AssetBrowserFolderPage::~AssetBrowserFolderPage() = default;

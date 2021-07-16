/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TreeViewPage.h"

#include <AzCore/Casting/numeric_cast.h>

#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzQtComponents/Components/Widgets/TreeView.h>

#include <AzQtComponents/Gallery/ui_TreeViewPage.h>

namespace
{
    class TestModel
        : public QAbstractItemModel
    {
    public:
        explicit TestModel(QObject* parent = nullptr)
            : QAbstractItemModel(parent)
        {
            static const auto folderIcon = QIcon(":/TreeView/folder-icon.svg");

            auto colors = std::make_unique<TreeNode>("Colors", folderIcon);
            colors->addChild("Blue");
            colors->addChild("Green");
            colors->addChild("Red");

            auto black = std::make_unique<TreeNode>("Black", folderIcon);
            black->addChild("Black");
            black->addChild("Black");
            black->addChild("Black");

            colors->addChild(std::move(black));

            m_topLevelItems.push_back(std::move(colors));

            auto apple = std::make_unique<TreeNode>("Apple");
            apple->addChild("Fuji");
            apple->addChild("Gala");
            apple->addChild("McIntosh");

            auto mango = std::make_unique<TreeNode>("Mango");
            mango->addChild("Kent");
            mango->addChild("Haden");

            auto orange = std::make_unique<TreeNode>("Orange");
            orange->addChild("Valencia");
            orange->addChild("Tangerin");
            orange->addChild("Mandarin");

            auto fruits = std::make_unique<TreeNode>("Fruits", folderIcon);
            fruits->addChild(std::move(apple));
            fruits->addChild("Kiwii");
            fruits->addChild(std::move(mango));
            fruits->addChild(std::move(orange));
            fruits->addChild("Strawberry");

            m_topLevelItems.push_back(std::move(fruits));
        }

        QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override
        {
            const TreeNode* parentNode = nullptr;
            if (parent.isValid())
            {
                parentNode = nodeForIndex(parent);
                Q_ASSERT(parentNode);
            }
            const auto& children = parentNode ? parentNode->children : m_topLevelItems;
            Q_ASSERT(row >= 0 && row < static_cast<int>(children.size()) && column == 0);
            return createIndex(row, column, const_cast<TreeNode*>(parentNode));
        }

        QModelIndex parent(const QModelIndex& child) const override
        {
            const auto* node = nodeForIndex(child);
            Q_ASSERT(node);
            const auto* parent = node->parent;
            if (!parent)
                return {};
            const auto* parentOfParent = parent->parent;
            const auto& children = parentOfParent ? parentOfParent->children : m_topLevelItems;
            auto it = std::find_if(children.begin(), children.end(), [parent](const auto& child) {
                return child.get() == parent;
            });
            Q_ASSERT(it != children.end());
            return createIndex(aznumeric_cast<int>(std::distance(children.begin(), it)), 0, const_cast<TreeNode*>(parentOfParent));
        }

        int rowCount(const QModelIndex& parent = {}) const override
        {
            if (!parent.isValid())
                return static_cast<int>(m_topLevelItems.size());
            const auto* node = nodeForIndex(parent);
            Q_ASSERT(node);
            return static_cast<int>(node->children.size());
        }

        int columnCount(const QModelIndex& parent = {}) const override
        {
            Q_UNUSED(parent);
            return 1;
        }

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
        {
            const auto* node = nodeForIndex(index);
            Q_ASSERT(node);
            switch (role)
            {
                case Qt::DisplayRole:
                case Qt::EditRole:
                    return node->m_name;
                case Qt::DecorationRole:
                {
                    static const auto defaultIcon = QIcon(":/TreeView/default-icon.svg");

                    if (!m_iconsEnabled)
                    {
                        return {};
                    }

                    return !node->icon.isNull() ? node->icon : defaultIcon;
                }
                case Qt::CheckStateRole:
                    return m_isCheckable ? (node->checked ? Qt::Checked : Qt::Unchecked) : QVariant();
                default:
                    break;
            }
            return {};
        }

        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
        {
            const auto* node = nodeForIndex(index);
            Q_ASSERT(node);
            switch (role)
            {
                case Qt::EditRole:
                {
                    const_cast<TreeNode*>(node)->m_name = value.toString();
                    emit dataChanged(index, index);
                    return true;
                }
                case Qt::CheckStateRole:
                {
                    const_cast<TreeNode*>(node)->checked = value.toBool();
                    emit dataChanged(index, index);
                    return true;
                }
                default:
                    break;
            }
            return false;
        }

        void setCheckable(bool checkable)
        {
            if (m_isCheckable != checkable)
            {
                m_isCheckable = checkable;
                emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1), {Qt::CheckStateRole});
            }
        }

        void setIconsEnabled(bool iconsEnabled)
        {
            if (m_iconsEnabled != iconsEnabled)
            {
                m_iconsEnabled = iconsEnabled;
                emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1), {Qt::DecorationRole});
            }
        }

        Qt::ItemFlags flags(const QModelIndex& index) const override
        {
            auto flags = QAbstractItemModel::flags(index);
            flags.setFlag(Qt::ItemIsUserCheckable, m_isCheckable);
            flags.setFlag(Qt::ItemIsEditable, true);

            return flags;
        }

    private:
        struct TreeNode
        {
            QString m_name;
            QIcon icon;
            const TreeNode* parent = nullptr;
            std::vector<std::unique_ptr<TreeNode>> children;
            bool checked = false;

            explicit TreeNode(const QString& name, const QIcon &icon = {}) : m_name(name), icon(icon) { }

            void addChild(std::unique_ptr<TreeNode> child)
            {
                child->parent = this;
                children.push_back(std::move(child));
            }

            void addChild(const QString& name)
            {
                addChild(std::make_unique<TreeNode>(name));
            }
        };

        const TreeNode* nodeForIndex(const QModelIndex& index) const
        {
            if (!index.isValid())
                return nullptr;
            const auto parent = static_cast<TreeNode*>(index.internalPointer());
            const auto& children = parent ? parent->children : m_topLevelItems;
            Q_ASSERT(index.row() >= 0 && index.row() < static_cast<int>(children.size()));
            return children[index.row()].get();
        }

        std::vector<std::unique_ptr<TreeNode>> m_topLevelItems;
        bool m_isCheckable = false;
        bool m_iconsEnabled = false;
    };
} // namespace

TreeViewPage::TreeViewPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::TreeViewPage)
{
    ui->setupUi(this);

    const auto titles = {ui->qTreeViewBranchTitle, ui->qTreeViewNoBranchTitle};
    for (auto title : titles)
    {
        AzQtComponents::Text::addTitleStyle(title);
    }

    connect(ui->showBranchLines, &QCheckBox::toggled, [this]()
    {
        using namespace AzQtComponents;
        TreeView::setBranchLinesEnabled(ui->qTreeView, !TreeView::isBranchLinesEnabled(ui->qTreeView));
        TreeView::setBranchLinesEnabled(ui->qTreeViewNoBranch, !TreeView::isBranchLinesEnabled(ui->qTreeViewNoBranch));
    });

    auto model = new TestModel(this);

    connect(ui->showCheckBoxes, &QCheckBox::toggled, [this, model](){
        model->setCheckable(ui->showCheckBoxes->isChecked());
    });

    connect(ui->showIcons, &QCheckBox::toggled, [this, model]() {
        model->setIconsEnabled(ui->showIcons->isChecked());
    });

    ui->qTreeView->setModel(model);
    ui->qTreeView->setHeaderHidden(true);
    ui->qTreeView->setSelectionMode(QTreeView::ExtendedSelection);
    ui->qTreeView->setAllColumnsShowFocus(true);
    ui->qTreeView->setItemDelegate(new AzQtComponents::BranchDelegate());

    ui->qTreeViewNoBranch->setModel(model);
    ui->qTreeViewNoBranch->setHeaderHidden(true);
    ui->qTreeViewNoBranch->setSelectionMode(QTreeView::ExtendedSelection);
    ui->qTreeViewNoBranch->setAllColumnsShowFocus(true);

    const QString exampleText = R"(
<p>QTreeView docs:
<a href="http://doc.qt.io/qt-5/qtreeview.html">http://doc.qt.io/qt-5/qtreeview.html</a>
</p>
<p>Example:</p>
<pre>
#include &lt;AzQtComponents/Components/Widgets/TreeView.h&gt;

auto treeview = new QTreeview();

// Create some model
auto model = new FileSystemModel(this);

treeView->setModel(model);

// Use branch delegate if you want to show branch lines
treeView->setItemDelegate(new AzQtComponents::BranchDelegate());
// Display connecting lines for branches
AzQtComponents::TreeView::setBranchLinesEnabled(treeView, true);
)";
    ui->exampleText->setHtml(exampleText);
}

#include <Gallery/moc_TreeViewPage.cpp>

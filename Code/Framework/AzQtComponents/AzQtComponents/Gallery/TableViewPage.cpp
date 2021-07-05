/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TableViewPage.h"

#include <AzToolsFramework/UI/Logging/LogTableModel.h>
#include <AzToolsFramework/UI/Logging/LogTableItemDelegate.h>

#include <AzToolsFramework/UI/Logging/LogLine.h>

#include <AzQtComponents/Components/Widgets/Text.h>

#include <AzQtComponents/Gallery/ui_TableViewPage.h>

#include <QDateTime>
#include <QFileSystemModel>
#include <QIdentityProxyModel>
#include <QStandardItemModel>
#include <QStringListModel>

class TextAlignmentProxyModel
    : public QIdentityProxyModel
{
public:
    using QIdentityProxyModel::QIdentityProxyModel;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        switch (role)
        {
            case Qt::TextAlignmentRole:
                // QFileSystemModel returns Qt::AlignLeft, which ruins the style preview.
                return QAbstractItemModel::headerData(section, orientation, role);
            default:
                break;
        }
        return QIdentityProxyModel::headerData(section, orientation, role);
    }
};

static void AppendMessage(AzToolsFramework::Logging::LogTableModel* logModel, const char* message)
{
    logModel->AppendLine(
        AzToolsFramework::Logging::LogLine(
            message,
            "Window",
            AzToolsFramework::Logging::LogLine::TYPE_MESSAGE,
            QDateTime::currentMSecsSinceEpoch()));
}

static void AppendWarning(AzToolsFramework::Logging::LogTableModel* logModel, const char* message)
{
    logModel->AppendLine(
        AzToolsFramework::Logging::LogLine(
            message,
            "Window",
            AzToolsFramework::Logging::LogLine::TYPE_WARNING,
            QDateTime::currentMSecsSinceEpoch()));
}

static void AppendError(AzToolsFramework::Logging::LogTableModel* logModel, const char* message)
{
    logModel->AppendLine(
        AzToolsFramework::Logging::LogLine(
            message,
            "Window",
            AzToolsFramework::Logging::LogLine::TYPE_ERROR,
            QDateTime::currentMSecsSinceEpoch()));
}

static void AppendContextToPreviousLine(AzToolsFramework::Logging::LogTableModel* logModel, const char* key, const char* value)
{
    // Context data format is C: [key] = value
    QString temp(QStringLiteral("C: [%0] = %1").arg(key, value));

    QByteArray tempUtf8 = temp.toUtf8();
    logModel->AppendLine(
        AzToolsFramework::Logging::LogLine(
            tempUtf8.data(),
            "Window",
            AzToolsFramework::Logging::LogLine::TYPE_CONTEXT,
            QDateTime::currentMSecsSinceEpoch()));
}

TableViewPage::TableViewPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::TableViewPage)
{
    ui->setupUi(this);

    const auto titles = {ui->azTableViewTitle, ui->qTableViewTitle, ui->qListViewTitle};
    for (auto title : titles)
    {
        AzQtComponents::Text::addTitleStyle(title);
    }

    auto model = new QFileSystemModel(this);
    model->setRootPath(QDir::currentPath());
    // This proxy model is only needed because QFileSystemModel applies unwanted alignment to the
    // header data
    auto proxy = new TextAlignmentProxyModel(this);
    proxy->setSourceModel(model);
    ui->tableView->setModel(proxy);
    ui->tableView->setRootIndex(proxy->mapFromSource(model->index(QDir::currentPath())));
    ui->tableView->setSortingEnabled(true);
    ui->tableView->setSelectionMode(QTreeView::ExtendedSelection);

    auto fruitModel = new QStringListModel(this);
    QStringList fruit;
    fruit << "Apple" << "Orange" << "Banana" << "Kiwi" << "Pear";
    fruitModel->setStringList(fruit);
    ui->tableView2->setModel(fruitModel);
    ui->tableView2->setHeaderHidden(true);

    auto logModel = new AzToolsFramework::Logging::LogTableModel(this);

    AppendMessage(logModel, "An informative message for debugging purposes.");

    AppendWarning(logModel, "A warning message for things that may not have gone as expected.");
    AppendContextToPreviousLine(logModel, "Who", "James");
    AppendContextToPreviousLine(logModel, "What", "Break his leg");

    AppendError(logModel, "Critical error message, something went wrong.");
    AppendContextToPreviousLine(logModel, "Who", "Sonia");
    AppendContextToPreviousLine(logModel, "What", "Bought a jewel");

    ui->logTableView->setModel(logModel);
    ui->logTableView->setExpandOnSelection();
    auto logItemDelegate = new AzToolsFramework::Logging::LogTableItemDelegate(ui->logTableView);
    ui->logTableView->setItemDelegate(logItemDelegate);

    ui->qTableView->setModel(logModel);
    ui->qTableView->setAlternatingRowColors(true);
    ui->qTableView->setShowGrid(false);
    ui->qTableView->horizontalHeader()->setStretchLastSection(true);
    ui->qTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->qTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->qListView->setModel(fruitModel);
    ui->qListView->setAlternatingRowColors(true);

    QString exampleText = R"(
<p>QTreeView docs:
<a href="http://doc.qt.io/qt-5/qtreeview.html">http://doc.qt.io/qt-5/qtreeview.html</a></p>
<p>AzQtComponents::TableView derives from QTreeView. It hides the branch controls and therefore
assumes the model is only one level deep.</p>
<p>Example:</p>
<pre>
#include &lt;AzQtComponents/Components/Widgets/TableView.h&gt;

auto tableView = new AzQtComponents::TableView();

// Create some model
auto model = new FileSystemModel(this);

tableView->setModel(model);
</pre>
)";

    ui->exampleText->setHtml(exampleText);
}

TableViewPage::~TableViewPage()
{
}

#include "Gallery/moc_TableViewPage.cpp"

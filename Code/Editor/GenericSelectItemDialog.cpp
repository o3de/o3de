/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "GenericSelectItemDialog.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_GenericSelectItemDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

// CGenericSelectItemDialog dialog

CGenericSelectItemDialog::CGenericSelectItemDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , ui(new Ui::CGenericSelectItemDialog)
    , m_initialized(false)
{
    m_mode = eMODE_LIST;
    m_bSet = false;
    m_bAllowNew = false;
    m_bShowDesc = true;

    setWindowTitle(tr("Please choose..."));

    ui->setupUi(this);

    connect(ui->m_listBox, &QListWidget::itemSelectionChanged, this, &CGenericSelectItemDialog::OnLbnSelchangeList);
    connect(ui->m_listBox, &QListWidget::itemDoubleClicked, this, &CGenericSelectItemDialog::OnLbnDoubleClick);
    connect(ui->m_tree, &QTreeWidget::itemSelectionChanged, this, &CGenericSelectItemDialog::OnTvnSelchangedTree);
    connect(ui->m_tree, &QTreeWidget::itemDoubleClicked, this, &CGenericSelectItemDialog::OnTvnDoubleClick);
    connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &CGenericSelectItemDialog::accept);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &CGenericSelectItemDialog::reject);
    connect(ui->m_newButton, &QAbstractButton::clicked, this, &CGenericSelectItemDialog::OnBnClickedNew);
}

CGenericSelectItemDialog::~CGenericSelectItemDialog()
{
}

// CGenericSelectItemDialog message handlers

//////////////////////////////////////////////////////////////////////////
void CGenericSelectItemDialog::OnInitDialog()
{
    if (m_mode == eMODE_LIST)
    {
        ui->m_tree->hide();
    }
    else
    {
        ui->m_listBox->hide();
    }

    ui->m_newButton->setVisible(m_bAllowNew);

    ui->m_desc->setVisible(m_bShowDesc);

    if (m_bSet == false)
    {
        GetItems(m_items);
    }

    ReloadItems();

    if (m_mode == eMODE_TREE)
    {
        ui->m_tree->setFocus();
    }
    else
    {
        ui->m_listBox->setFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CGenericSelectItemDialog::GetItems([[maybe_unused]] std::vector<SItem>& outItems)
{
}

//////////////////////////////////////////////////////////////////////////
void CGenericSelectItemDialog::ReloadTree()
{
    ui->m_tree->clear();

    QTreeWidgetItem* hSelected = nullptr;

    /*
    std::vector<CString>::const_iterator iter = m_items.begin();
    while (iter != m_items.end())
    {
        const CString& itemName = *iter;
        HTREEITEM hItem = m_tree.InsertItem(itemName, 0, 0, TVI_ROOT, TVI_SORT);
        if (!m_preselect.IsEmpty() && m_preselect.CompareNoCase(itemName) == 0)
        {
            hSelected = hItem;
        }
        ++iter;
    }
    */

    std::map<QString, QTreeWidgetItem*, less_qstring_icmp> items;

    QRegularExpression sep(QStringLiteral("[\\/.") + m_treeSeparator + QStringLiteral("]+"));

    for (SItem& item : m_items)
    {
        QString name = item.name;

        QTreeWidgetItem* hRoot = nullptr;

        QString itemName;

        for (const QString& token : name.split(sep))
        {
            itemName += token + QStringLiteral("/");

            QTreeWidgetItem* hParentItem = stl::find_in_map(items, itemName, nullptr);
            if (!hParentItem)
            {
                hRoot = hRoot == nullptr ? new QTreeWidgetItem(ui->m_tree) : new QTreeWidgetItem(hRoot);
                hRoot->setText(0, token);
                items[itemName] = hRoot;
            }
            else
            {
                hRoot = hParentItem;
            }
        }

        Q_ASSERT(hRoot);
        hRoot->setData(0, Qt::UserRole, QVariant::fromValue<void*>(&item));

        if (!m_preselect.isEmpty() && m_preselect.compare(name, Qt::CaseInsensitive) == 0)
        {
            hSelected = hRoot;
        }
    }

    ui->m_tree->expandAll();

    if (hSelected != nullptr)
    {
        ui->m_tree->scrollToItem(hSelected);
        ui->m_tree->setCurrentItem(hSelected, 0);
        OnTvnSelchangedTree();
    }
}

//////////////////////////////////////////////////////////////////////////
void CGenericSelectItemDialog::ReloadItems()
{
    m_selectedItem.clear();
    m_selectedDesc.clear();
    if (m_mode == eMODE_TREE)
    {
        ReloadTree();
    }
    else // eMODE_LIST
    {
        std::vector<SItem>::const_iterator iter = m_items.begin();
        while (iter != m_items.end())
        {
            const SItem* pItem = &*iter;
            QListWidgetItem* item = new QListWidgetItem(pItem->name, ui->m_listBox);
            item->setData(Qt::UserRole, pItem->desc);
            ui->m_listBox->addItem(item);
            ++iter;
        }
        if (!m_preselect.isEmpty())
        {
            QList<QListWidgetItem*> foundItems = ui->m_listBox->findItems(m_preselect, Qt::MatchExactly);
            if (!foundItems.isEmpty())
            {
                ui->m_listBox->setCurrentItem(foundItems[0]);
                OnLbnSelchangeList();
            }
        }
    }
    ItemSelected();
}


//////////////////////////////////////////////////////////////////////////
void CGenericSelectItemDialog::OnTvnSelchangedTree()
{
    QTreeWidgetItem* item = ui->m_tree->currentItem();
    if (item)
    {
        SItem* pItem = reinterpret_cast<SItem*>(item->data(0, Qt::UserRole).value<void*>());
        if (pItem)
        {
            m_selectedItem = pItem->name;
            m_selectedDesc = pItem->desc;
        }
        else
        {
            m_selectedItem.clear();
            m_selectedDesc.clear();
        }

        ItemSelected();
    }
}

//////////////////////////////////////////////////////////////////////////
void CGenericSelectItemDialog::OnTvnDoubleClick()
{
    if (m_selectedItem.isEmpty() == false)
    {
        accept();
    }
}

//////////////////////////////////////////////////////////////////////////
QString CGenericSelectItemDialog::GetSelectedItem()
{
    return m_selectedItem;
}

//////////////////////////////////////////////////////////////////////////
void CGenericSelectItemDialog::OnLbnDoubleClick()
{
    if (m_selectedItem.isEmpty() == false)
    {
        accept();
    }
}

//////////////////////////////////////////////////////////////////////////
void CGenericSelectItemDialog::OnLbnSelchangeList()
{
    QListWidgetItem* item = ui->m_listBox->currentItem();
    if (item == nullptr)
    {
        m_selectedItem.clear();
    }
    else
    {
        m_selectedItem = item->text();
        m_selectedDesc = item->data(Qt::UserRole).toString();
    }
    ItemSelected();
}

//////////////////////////////////////////////////////////////////////////
void CGenericSelectItemDialog::OnBnClickedNew()
{
    done(New);
}

//////////////////////////////////////////////////////////////////////////
void CGenericSelectItemDialog::ItemSelected()
{
    if (m_selectedItem.isEmpty())
    {
        ui->m_desc->setText(tr("<Nothing selected>"));
    }
    else
    {
        if (m_selectedDesc.isEmpty())
        {
            ui->m_desc->setText(m_selectedItem);
        }
        else
        {
            ui->m_desc->setText(m_selectedDesc);
        }
    }
}

void CGenericSelectItemDialog::showEvent(QShowEvent* event)
{
    if (!m_initialized)
    {
        OnInitDialog();
        m_initialized = true;
    }

    QDialog::showEvent(event);
}

#include <moc_GenericSelectItemDialog.cpp>

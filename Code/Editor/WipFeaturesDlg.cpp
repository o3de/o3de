/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "WipFeatureManager.h"

#include "WipFeaturesDlg.h"

// Qt
#include <QDialog>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "ui_WipFeaturesDlg.h"
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

#ifdef USE_WIP_FEATURES_MANAGER

// CWipFeaturesDlg dialog

class WipFeaturesModel
    : public QAbstractTableModel
{
public:
    WipFeaturesModel(QObject* parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : CWipFeatureManager::Instance()->GetFeatures().size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 5;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation != Qt::Horizontal || section >= columnCount())
        {
            return QVariant();
        }


        switch (role)
        {
        case Qt::TextAlignmentRole:
            return section == 0 ? Qt::AlignLeft : Qt::AlignCenter;
        case Qt::DisplayRole:
            switch (section)
            {
            case 0:
                return tr("Name");
            case 1:
                return tr("Id");
            case 2:
                return tr("Visible");
            case 3:
                return tr("Enabled");
            case 4:
                return tr("SafeMode");
            default:
                return QVariant();
            }
        default:
            return QVariant();
        }
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
    {
        if (!index.isValid() || index.column() >= columnCount(index.parent()) || index.row() >= rowCount(index.parent()))
        {
            return false;
        }

        if (role != Qt::EditRole || !value.canConvert<bool>())
        {
            return false;
        }

        auto it = CWipFeatureManager::Instance()->GetFeatures().begin();
        std::advance(it, index.row());

        auto id = it->first;

        switch (index.column())
        {
        case 2:
            CWipFeatureManager::Instance()->ShowFeature(id, value.toBool());
            break;
        case 3:
            CWipFeatureManager::Instance()->EnableFeature(id, value.toBool());
            break;
        case 4:
            CWipFeatureManager::Instance()->SetFeatureSafeMode(id, value.toBool());
            break;
        default:
            return false;
        }

        emit dataChanged(index, index);
        return true;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.column() >= columnCount(index.parent()) || index.row() >= rowCount(index.parent()))
        {
            return QVariant();
        }

        if (role == Qt::TextAlignmentRole)
        {
            return headerData(index.column(), Qt::Horizontal, role);
        }

        if (role != Qt::DisplayRole)
        {
            return QVariant();
        }

        auto it = CWipFeatureManager::Instance()->GetFeatures().begin();
        std::advance(it, index.row());

        auto feature = it->second;

        switch (index.column())
        {
        case 0:
            return QString(feature.m_displayName.c_str());
        case 1:
            return feature.m_id;
        case 2:
            return feature.m_bVisible ? tr("X") : QString();
        case 3:
            return feature.m_bEnabled ? tr("X") : QString();
        case 4:
            return feature.m_bSafeMode ? tr("X") : QString();
        default:
            return QVariant();
        }
    }
};

CWipFeaturesDlg::CWipFeaturesDlg(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_ui(new Ui::WipFeaturesDlg)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(size());

    OnInitDialog();

    connect(m_ui->buttonShow, &QPushButton::clicked, this, &CWipFeaturesDlg::OnBnClickedButtonShow);
    connect(m_ui->buttonHide, &QPushButton::clicked, this, &CWipFeaturesDlg::OnBnClickedButtonHide);
    connect(m_ui->buttonEnable, &QPushButton::clicked, this, &CWipFeaturesDlg::OnBnClickedButtonEnable);
    connect(m_ui->buttonDisable, &QPushButton::clicked, this, &CWipFeaturesDlg::OnBnClickedButtonDisable);
    connect(m_ui->buttonSafeMode, &QPushButton::clicked, this, &CWipFeaturesDlg::OnBnClickedButtonSafemode);
    connect(m_ui->buttonNormalMode, &QPushButton::clicked, this, &CWipFeaturesDlg::OnBnClickedButtonNormalmode);
}

CWipFeaturesDlg::~CWipFeaturesDlg()
{
}

// CWipFeaturesDlg message handlers

void CWipFeaturesDlg::OnInitDialog()
{
    m_ui->m_lstFeatures->setModel(new WipFeaturesModel(this));
    m_ui->m_lstFeatures->horizontalHeader()->resizeSection(0, 300);
    m_ui->m_lstFeatures->horizontalHeader()->resizeSection(1, 70);
    m_ui->m_lstFeatures->horizontalHeader()->resizeSection(2, 70);
    m_ui->m_lstFeatures->horizontalHeader()->resizeSection(3, 70);
    m_ui->m_lstFeatures->horizontalHeader()->resizeSection(4, 70);
}

void CWipFeaturesDlg::OnBnClickedButtonShow()
{
    for (auto index : m_ui->m_lstFeatures->selectionModel()->selectedRows())
    {
        m_ui->m_lstFeatures->model()->setData(index.sibling(index.row(), 2), true);
    }
}

void CWipFeaturesDlg::OnBnClickedButtonHide()
{
    for (auto index : m_ui->m_lstFeatures->selectionModel()->selectedRows())
    {
        m_ui->m_lstFeatures->model()->setData(index.sibling(index.row(), 2), false);
    }
}

void CWipFeaturesDlg::OnBnClickedButtonEnable()
{
    for (auto index : m_ui->m_lstFeatures->selectionModel()->selectedRows())
    {
        m_ui->m_lstFeatures->model()->setData(index.sibling(index.row(), 3), true);
    }
}

void CWipFeaturesDlg::OnBnClickedButtonDisable()
{
    for (auto index : m_ui->m_lstFeatures->selectionModel()->selectedRows())
    {
        m_ui->m_lstFeatures->model()->setData(index.sibling(index.row(), 3), false);
    }
}

void CWipFeaturesDlg::OnBnClickedButtonSafemode()
{
    for (auto index : m_ui->m_lstFeatures->selectionModel()->selectedRows())
    {
        m_ui->m_lstFeatures->model()->setData(index.sibling(index.row(), 4), true);
    }
}

void CWipFeaturesDlg::OnBnClickedButtonNormalmode()
{
    for (auto index : m_ui->m_lstFeatures->selectionModel()->selectedRows())
    {
        m_ui->m_lstFeatures->model()->setData(index.sibling(index.row(), 4), false);
    }
}

#include <moc_WipFeaturesDlg.cpp>

#endif // USE_WIP_FEATURES_MANAGER

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/std/string/string.h>

#include "TargetSelectorButton.hxx"

#include <QAction>
#include <QMenu>

namespace AzToolsFramework
{
    TargetSelectorButton::TargetSelectorButton(QWidget *pParent) 
        : QPushButton(pParent)
    {
        AzFramework::TargetManagerClient::Bus::Handler::BusConnect();
        UpdateStatus();
        this->setToolTip(tr("Click to change target"));
        connect(this, SIGNAL(clicked()), this, SLOT(DoPopup()));

        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sizePolicy().hasHeightForWidth());
        setSizePolicy(sizePolicy1);
        setMinimumSize(QSize(128, 24));
    }

    TargetSelectorButton::~TargetSelectorButton()
    {
        AzFramework::TargetManagerClient::Bus::Handler::BusDisconnect();
    }

    void TargetSelectorButton::DoPopup()
    {
        AzFramework::TargetContainer targets;

        EBUS_EVENT(AzFramework::TargetManager::Bus, EnumTargetInfos, targets);

        QMenu menu;

        QAction *pNoneAction = new QAction(QIcon(":/general/target_none"), "Disconnect", this);
        pNoneAction->setProperty("targetID", 0);

        menu.addAction(pNoneAction);

        for (AzFramework::TargetContainer::const_iterator it = targets.begin(); it != targets.end(); ++it)
        {
            const AzFramework::TargetInfo& info = it->second;
            bool isSelf = (info.GetStatusFlags() & AzFramework::TF_SELF) != 0;
            if (isSelf)
            {
                // Do not list the current application as a target
                continue;
            }

            bool isOnline = (info.GetStatusFlags() & AzFramework::TF_ONLINE) != 0;

            QString displayTargetString;
            ConstructDisplayTargetString(displayTargetString, info);
            
            QAction *targetAction = new QAction(isOnline ? QIcon(":/general/target_connected") : QIcon(":/general/target_disconnected"), displayTargetString, this);
            targetAction->setProperty("targetID", info.GetPersistentId());
            menu.addAction(targetAction);
        }
        
        QAction* resultAction = menu.exec(QCursor::pos());
        
        if (resultAction)
        {
            AZ::u32 networkId = resultAction->property("targetID").toUInt();
            EBUS_EVENT(AzFramework::TargetManager::Bus, SetDesiredTarget, networkId);
        }
    }

    
    void TargetSelectorButton::DesiredTargetConnected(bool connected)
    {
        if (!connected)
        {
            this->setIcon(QIcon(":/general/target_none"));
            this->setText("Target: None");
            return;
        }

        AzFramework::TargetInfo info;
        EBUS_EVENT_RESULT(info, AzFramework::TargetManager::Bus, GetDesiredTarget);
        if (!info.GetPersistentId())
        {
            this->setIcon(QIcon(":/general/target_none"));
            this->setText("Target: None");
            return;
        }

        if (!info.IsValid())
        {
            this->setIcon(QIcon(":/general/target_none"));
            this->setText("Target: None");
            return;
        }

        QString displayTargetString;
        ConstructDisplayTargetString(displayTargetString, info);

        this->setText(QString("Target: %1").arg(displayTargetString));
        if (info.GetStatusFlags() & AzFramework::TF_ONLINE)
        {
            this->setIcon(QIcon(":/general/target_connected"));
        }
        else
        {
            this->setIcon(QIcon(":/general/target_disconnected"));
        }
    }

    void TargetSelectorButton::UpdateStatus()
    {
        AzFramework::TargetInfo info;
        EBUS_EVENT_RESULT(info, AzFramework::TargetManager::Bus, GetDesiredTarget);
        if (!info.GetPersistentId())
        {
            this->setIcon(QIcon(":/general/target_none"));
            this->setText("Target: None");
            return;
        }

        if (!info.IsValid())
        {
            this->setIcon(QIcon(":/general/target_none"));
            this->setText("Target: None");
            return;
        }

        this->setText(QString("Target: %1").arg(info.GetDisplayName()));
        if (info.GetStatusFlags() & AzFramework::TF_ONLINE)
        {
            this->setIcon(QIcon(":/general/target_connected"));
        }
        else
        {
            this->setIcon(QIcon(":/general/target_disconnected"));
        }
        //updateGeometry();
    }

    void TargetSelectorButton::ConstructDisplayTargetString(QString& outputString, const AzFramework::TargetInfo& info)
    {
        outputString = QString("%1 (%2)").arg(info.GetDisplayName()).arg(QString::number(static_cast<unsigned int>(info.GetPersistentId()),16));
    }

    TargetSelectorButtonAction::TargetSelectorButtonAction(QObject *pParent) : QWidgetAction(pParent)
    {
    }    

    QWidget* TargetSelectorButtonAction::createWidget(QWidget* pParent)
    {
        return aznew TargetSelectorButton(pParent);
    }

}

#include "UI/UICore/moc_TargetSelectorButton.cpp"

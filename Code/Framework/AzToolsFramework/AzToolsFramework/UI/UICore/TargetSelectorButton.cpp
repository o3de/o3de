/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string.h>

#include "TargetSelectorButton.hxx"

#include <QAction>
#include <QMenu>

namespace AzToolsFramework
{
    TargetSelectorButton::TargetSelectorButton(AZ::Crc32 key, QWidget *pParent) 
        : QPushButton(pParent)
        , m_remoteToolsKey(key)
    {
        UpdateStatus();
        this->setToolTip(tr("Click to change target"));
        connect(this, SIGNAL(clicked()), this, SLOT(DoPopup()));

        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sizePolicy().hasHeightForWidth());
        setSizePolicy(sizePolicy1);
        setMinimumSize(QSize(128, 24));

        auto* remoteToolsInterface = AzFramework::RemoteToolsInterface::Get();
        if (remoteToolsInterface)
        {
            m_connectedEventHandler = AzFramework::RemoteToolsEndpointConnectedEvent::Handler(
                [this](bool value)
                {
                    this->DesiredTargetConnected(value);
                });
            remoteToolsInterface->RegisterRemoteToolsEndpointConnectedHandler(key, m_connectedEventHandler);
        }
    }

    void TargetSelectorButton::DoPopup()
    {
        AzFramework::RemoteToolsEndpointContainer targets;

        auto* remoteToolsInterface = AzFramework::RemoteToolsInterface::Get();
        if (remoteToolsInterface)
        {
            remoteToolsInterface->EnumTargetInfos(m_remoteToolsKey, targets);
        }

        QMenu menu;

        QAction *pNoneAction = new QAction(QIcon(":/general/target_none"), "Disconnect", this);
        pNoneAction->setProperty("targetID", 0);

        menu.addAction(pNoneAction);

        for (AzFramework::RemoteToolsEndpointContainer::const_iterator it = targets.begin(); it != targets.end(); ++it)
        {
            const AzFramework::RemoteToolsEndpointInfo& info = it->second;
            if (info.IsSelf())
            {
                // Do not list the current application as a target
                continue;
            }

            bool isOnline = remoteToolsInterface ?
                remoteToolsInterface->IsEndpointOnline(m_remoteToolsKey, info.GetPersistentId()) : false;

            QString displayTargetString;
            ConstructDisplayTargetString(displayTargetString, info);
            
            QAction *targetAction = new QAction(isOnline ? QIcon(":/general/target_connected") : QIcon(":/general/target_disconnected"), displayTargetString, this);
            targetAction->setProperty("targetID", info.GetPersistentId());
            menu.addAction(targetAction);
        }
        
        QAction* resultAction = menu.exec(QCursor::pos());
        
        if (resultAction && remoteToolsInterface)
        {
            AZ::u32 networkId = resultAction->property("targetID").toUInt();
            remoteToolsInterface->SetDesiredEndpoint(m_remoteToolsKey, networkId);
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

        AzFramework::RemoteToolsEndpointInfo info = AzFramework::RemoteToolsInterface::Get()->GetDesiredEndpoint(m_remoteToolsKey);
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
        if (AzFramework::RemoteToolsInterface::Get()->IsEndpointOnline(m_remoteToolsKey, info.GetNetworkId()))
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
        auto* remoteToolsInterface = AzFramework::RemoteToolsInterface::Get();
        if (remoteToolsInterface)
        {
            AzFramework::RemoteToolsEndpointInfo info = remoteToolsInterface->GetDesiredEndpoint(m_remoteToolsKey);
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

            if (remoteToolsInterface->IsEndpointOnline(m_remoteToolsKey, info.GetNetworkId()))
            {
                this->setIcon(QIcon(":/general/target_connected"));
                return;
            }
        }

        this->setIcon(QIcon(":/general/target_disconnected"));
    }

    void TargetSelectorButton::ConstructDisplayTargetString(QString& outputString, const AzFramework::RemoteToolsEndpointInfo& info)
    {
        outputString = QString("%1 (%2)").arg(info.GetDisplayName()).arg(QString::number(static_cast<unsigned int>(info.GetPersistentId()),16));
    }

    TargetSelectorButtonAction::TargetSelectorButtonAction(AZ::Crc32 key, QObject *pParent) : QWidgetAction(pParent)
    {
        m_remoteToolsKey = key;
    }    

    QWidget* TargetSelectorButtonAction::createWidget(QWidget* pParent)
    {
        return aznew TargetSelectorButton(m_remoteToolsKey, pParent);
    }

}

#include "UI/UICore/moc_TargetSelectorButton.cpp"

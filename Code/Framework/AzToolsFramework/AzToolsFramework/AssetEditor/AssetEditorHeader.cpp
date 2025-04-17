/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetEditorHeader.h"

#include <AzQtComponents/Components/Style.h>

namespace Ui
{
    namespace HeaderConstants
    {
        static const char* BackgroundId = "Background";
        static const char* NameId = "Name";
        static const char* IconId = "Icon";

        static const char* AssetEditorIconClassName = "AssetEditorHeaderIcon";

        static const int DefaultIconSize = 16;
    }

    AssetEditorHeader::AssetEditorHeader(QWidget* parent)
        : QFrame(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_backgroundFrame = new QFrame(this);
        m_backgroundFrame->setObjectName(HeaderConstants::BackgroundId);
        m_backgroundFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        m_backgroundFrame->setAutoFillBackground(true);

        m_iconLabel = new QLabel(m_backgroundFrame);
        m_iconLabel->setObjectName(HeaderConstants::IconId);
        m_iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_iconLabel->setFixedSize(16, 16);
        AzQtComponents::Style::addClass(m_iconLabel, HeaderConstants::AssetEditorIconClassName);
        m_iconLabel->hide();

        m_assetName = new AzQtComponents::ElidingLabel(m_backgroundFrame);
        m_assetName->setObjectName(HeaderConstants::NameId);
        m_assetName->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        m_backgroundLayout = new QHBoxLayout(this);
        m_backgroundLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_backgroundLayout->setSpacing(0);
        m_backgroundLayout->setContentsMargins(0, 0, 0, 0);

        m_backgroundLayout->addWidget(m_iconLabel);
        m_backgroundLayout->addWidget(m_assetName);
        m_backgroundFrame->setLayout(m_backgroundLayout);

        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_mainLayout->setSpacing(0);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->addWidget(m_backgroundFrame);

        m_backgroundFrame->setLayout(m_backgroundLayout);
    }

    void AssetEditorHeader::setName(const QString& name)
    {
        m_assetName->setText(name);
    }

    void AssetEditorHeader::setIcon(const QIcon& icon)
    {
        m_icon = icon;

        if (!icon.isNull())
        {
            m_iconLabel->setPixmap(icon.pixmap(HeaderConstants::DefaultIconSize, HeaderConstants::DefaultIconSize));
        }

        m_iconLabel->setVisible(!icon.isNull());
    }
} // namespace Ui

#include <AssetEditor/moc_AssetEditorHeader.cpp>

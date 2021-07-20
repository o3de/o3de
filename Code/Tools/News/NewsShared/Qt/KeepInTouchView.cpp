/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "KeepInTouchView.h"
#include "NewsShared/Qt/ui_KeepInTouchView.h"

#include <QMap>
#include <QUrl>
#include <QString>
#include <QDesktopServices>

namespace News
{
    KeepInTouchView::KeepInTouchView(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::KeepInTouchViewWidget())
    {
        m_ui->setupUi(this);

        m_ui->twich_container->setCursor(Qt::PointingHandCursor);
        m_ui->twitter_container->setCursor(Qt::PointingHandCursor);
        m_ui->youtube_container->setCursor(Qt::PointingHandCursor);
        m_ui->facebook_container->setCursor(Qt::PointingHandCursor);

        m_ui->twich_container->installEventFilter(this);
        m_ui->twitter_container->installEventFilter(this);
        m_ui->youtube_container->installEventFilter(this);
        m_ui->facebook_container->installEventFilter(this);
    }

    bool KeepInTouchView::eventFilter(QObject *watched, QEvent *event)
    {
        if (event->type() == QEvent::MouseButtonRelease)
        {
            if (watched == m_ui->twich_container)
            {
                return LaunchSocialMediaUrl(SocialMediaType::Twitch);
            }
            else if (watched == m_ui->twitter_container)
            {
                return LaunchSocialMediaUrl(SocialMediaType::Twitter);
            }
            else if (watched == m_ui->youtube_container)
            {
                return LaunchSocialMediaUrl(SocialMediaType::YouTube);
            }
            else if (watched == m_ui->facebook_container)
            {
                return LaunchSocialMediaUrl(SocialMediaType::Facebook);
            }
        }

        return QWidget::eventFilter(watched, event);
    }

    bool KeepInTouchView::LaunchSocialMediaUrl(SocialMediaType type)
    {
        static const QMap<SocialMediaType, const char*> socialMediaTypeToUrlMap =
        {
            { SocialMediaType::Twitch, m_twitchUrl },
            { SocialMediaType::Twitter, m_twitterUrl },
            { SocialMediaType::YouTube, m_youtubeUrl },
            { SocialMediaType::Facebook, m_facebookUrl }
        };

        if (socialMediaTypeToUrlMap.contains(type) == false)
        {
            return false;
        }

        QString link = socialMediaTypeToUrlMap[type];
        Q_EMIT linkActivatedSignal(link);

        return QDesktopServices::openUrl(QUrl(link));
    }
}

#include "NewsShared/Qt/moc_KeepInTouchView.cpp"

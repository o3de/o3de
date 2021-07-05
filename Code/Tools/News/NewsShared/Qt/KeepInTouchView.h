/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

namespace Ui 
{
    class KeepInTouchViewWidget;
}

namespace News
{
    class KeepInTouchView
        : public QWidget
    {
        Q_OBJECT

        enum class SocialMediaType
        {
            Twitch,
            Twitter,
            YouTube,
            Facebook
        };

    public:
        KeepInTouchView(QWidget* parent);
        ~KeepInTouchView() = default;

        bool eventFilter(QObject *watched, QEvent *event);

    Q_SIGNALS:
        void linkActivatedSignal(const QString& link);

    private:
        bool LaunchSocialMediaUrl(SocialMediaType type);

        Ui::KeepInTouchViewWidget* m_ui = nullptr;

        const char* m_twitchUrl = "https://docs.aws.amazon.com/console/lumberyard/twitch";
        const char* m_twitterUrl = "https://docs.aws.amazon.com/console/lumberyard/twitter";
        const char* m_youtubeUrl = "https://docs.aws.amazon.com/console/lumberyard/youtube";
        const char* m_facebookUrl = "https://docs.aws.amazon.com/console/lumberyard/facebook";
    };
}

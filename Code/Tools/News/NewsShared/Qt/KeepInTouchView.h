/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

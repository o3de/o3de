/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QObject>
#include <QSet>
#include <QTimer>

namespace WhiteBox
{
    //! Lives for the length of White Box component mode and keeps the Material and White Box Collider cards and the
    //! Material Instance Editor usable; the editor greys out every other card and pane in component mode.
    class WhiteBoxMaterialUiUnlock : public QObject
    {
    public:
        WhiteBoxMaterialUiUnlock();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        void Unlock();
        void QueueUnlock();

        QTimer m_rescan; //!< Catches cards and panes created after entering component mode.
        bool m_queued = false;
        QSet<QObject*> m_watched; //!< Widgets carrying our event filter.
    };
} // namespace WhiteBox

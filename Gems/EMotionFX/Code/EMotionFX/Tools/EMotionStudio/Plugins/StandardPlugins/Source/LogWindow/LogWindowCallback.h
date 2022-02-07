/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_LOGWINDOWCALLBACK_H
#define __EMSTUDIO_LOGWINDOWCALLBACK_H

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <MCore/Source/LogManager.h>
#include <QTableWidget>
#endif


namespace EMStudio
{
    class LogWindowCallback
        : public QTableWidget
        , public MCore::LogCallback
    {
        Q_OBJECT // AUTOMOC
                 MCORE_MEMORYOBJECTCATEGORY(LogWindowCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            TYPE_ID = 0x0034ffaa
        };

        LogWindowCallback(QWidget* parent);
        ~LogWindowCallback();

        void Log(const char* text, ELogLevel logLevel) override;
        uint32 GetType() const override;

        void SetFind(const QString& find);
        QString GetFind() const
        {
            return m_find;
        }

        void SetFilter(uint32 filter);
        uint32 GetFilter() const
        {
            return m_filter;
        }

    protected:
        void keyPressEvent(QKeyEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private slots:
        void Copy();
        void SelectAll();
        void UnselectAll();
        void Clear();

        void LogImpl(const QString text, MCore::LogCallback::ELogLevel logLevel);

    signals:
        void DoLog(const QString text, MCore::LogCallback::ELogLevel logLevel);

    private:
        void SetColumnWidthToTakeWholeSpace();

    private:
        QString m_find;
        uint32 m_filter;
        int m_maxSecondColumnWidth;
        bool m_scrollToBottom;
    };
} // namespace EMStudio

Q_DECLARE_METATYPE(MCore::LogCallback::ELogLevel)

#endif

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LogWindowPlugin.h"
#include "LogWindowCallback.h"
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace EMStudio
{
    // constructor
    LogWindowPlugin::LogWindowPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        m_logCallback = nullptr;
    }


    // destructor
    LogWindowPlugin::~LogWindowPlugin()
    {
        // remove the callback from the log manager (automatically deletes from memory as well)
        const size_t index = MCore::GetLogManager().FindLogCallback(m_logCallback);
        if (index != InvalidIndex)
        {
            MCore::GetLogManager().RemoveLogCallback(index);
        }
    }

    // get the name
    const char* LogWindowPlugin::GetName() const
    {
        return "Log Window";
    }

    // get the plugin type id
    uint32 LogWindowPlugin::GetClassID() const
    {
        return LogWindowPlugin::CLASS_ID;
    }

    // init after the parent dock window has been created
    bool LogWindowPlugin::Init()
    {
        // create the widget
        QWidget* windowWidget = new QWidget(m_dock);

        // create the layout
        QVBoxLayout* windowWidgetLayout = new QVBoxLayout();
        windowWidgetLayout->setSpacing(3);
        windowWidgetLayout->setMargin(3);

        // create the find widget
        m_searchWidget = new AzQtComponents::FilteredSearchWidget(windowWidget);
        AddFilter(tr("Fatal"), MCore::LogCallback::LOGLEVEL_FATAL, true);
        AddFilter(tr("Error"), MCore::LogCallback::LOGLEVEL_ERROR, true);
        AddFilter(tr("Warning"), MCore::LogCallback::LOGLEVEL_WARNING, true);
        AddFilter(tr("Info"), MCore::LogCallback::LOGLEVEL_INFO, true);
    #ifdef MCORE_DEBUG
        AddFilter(tr("Detailed Info"), MCore::LogCallback::LOGLEVEL_DETAILEDINFO, true);
        AddFilter(tr("Debug"), MCore::LogCallback::LOGLEVEL_DEBUG, true);
    #else
        AddFilter(tr("Detailed Info"), MCore::LogCallback::LOGLEVEL_DETAILEDINFO, false);
        AddFilter(tr("Debug"), MCore::LogCallback::LOGLEVEL_DEBUG, false);
    #endif
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &LogWindowPlugin::OnTextFilterChanged);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this, &LogWindowPlugin::OnTypeFilterChanged);

        // create the filter layout
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->addWidget(new QLabel("Filter:"));
        topLayout->addWidget(m_searchWidget);
        topLayout->addStretch();
        topLayout->setSpacing(6);

        // add the filter layout
        windowWidgetLayout->addLayout(topLayout);

        // create and add the table and callback
        m_logCallback = new LogWindowCallback(nullptr);
        windowWidgetLayout->addWidget(m_logCallback);

        // set the layout
        windowWidget->setLayout(windowWidgetLayout);

        // set the table as content
        m_dock->setWidget(windowWidget);

        // create the callback
        m_logCallback->SetLogLevels(MCore::LogCallback::LOGLEVEL_ALL);
        MCore::GetLogManager().AddLogCallback(m_logCallback);

        // return true because the plugin is correctly initialized
        return true;
    }


    // find changed
    void LogWindowPlugin::OnTextFilterChanged(const QString& text)
    {
        m_logCallback->SetFind(text);
    }


    // type filter changed
    void LogWindowPlugin::OnTypeFilterChanged(const QVector<AzQtComponents::SearchTypeFilter>& filters)
    {
        uint32 newFilter = 0;
        for (const auto& filter : filters)
        {
            newFilter |= filter.metadata.toInt();
        }
        m_logCallback->SetFilter(newFilter);
    }


    void LogWindowPlugin::AddFilter(const QString& name, MCore::LogCallback::ELogLevel level, bool enabled)
    {
        AzQtComponents::SearchTypeFilter filter(tr("Level"), name);
        filter.metadata = static_cast<int>(level);
        filter.enabled = enabled;
        m_searchWidget->AddTypeFilter(filter);
    }

} // namespace EMStudio

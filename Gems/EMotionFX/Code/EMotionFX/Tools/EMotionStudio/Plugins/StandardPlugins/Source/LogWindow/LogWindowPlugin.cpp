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
        mLogCallback = nullptr;
    }


    // destructor
    LogWindowPlugin::~LogWindowPlugin()
    {
        // remove the callback from the log manager (automatically deletes from memory as well)
        const uint32 index = MCore::GetLogManager().FindLogCallback(mLogCallback);
        if (index != MCORE_INVALIDINDEX32)
        {
            MCore::GetLogManager().RemoveLogCallback(index);
        }
    }


    // get the compile date
    const char* LogWindowPlugin::GetCompileDate() const
    {
        return MCORE_DATE;
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


    // get the creator name
    const char* LogWindowPlugin::GetCreatorName() const
    {
        return "Amazon";
    }


    // get the version
    float LogWindowPlugin::GetVersion() const
    {
        return 1.0f;
    }


    // clone the log window
    EMStudioPlugin* LogWindowPlugin::Clone()
    {
        LogWindowPlugin* newPlugin = new LogWindowPlugin();
        return newPlugin;
    }


    // init after the parent dock window has been created
    bool LogWindowPlugin::Init()
    {
        // create the widget
        QWidget* windowWidget = new QWidget(mDock);

        // create the layout
        QVBoxLayout* windowWidgetLayout = new QVBoxLayout();
        windowWidgetLayout->setSpacing(3);
        windowWidgetLayout->setMargin(3);

        // create the find widget
        mSearchWidget = new AzQtComponents::FilteredSearchWidget(windowWidget);
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
        connect(mSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &LogWindowPlugin::OnTextFilterChanged);
        connect(mSearchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this, &LogWindowPlugin::OnTypeFilterChanged);

        // create the filter layout
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->addWidget(new QLabel("Filter:"));
        topLayout->addWidget(mSearchWidget);
        topLayout->addStretch();
        topLayout->setSpacing(6);

        // add the filter layout
        windowWidgetLayout->addLayout(topLayout);

        // create and add the table and callback
        mLogCallback = new LogWindowCallback(nullptr);
        windowWidgetLayout->addWidget(mLogCallback);

        // set the layout
        windowWidget->setLayout(windowWidgetLayout);

        // set the table as content
        mDock->setWidget(windowWidget);

        // create the callback
        mLogCallback->SetLogLevels(MCore::LogCallback::LOGLEVEL_ALL);
        MCore::GetLogManager().AddLogCallback(mLogCallback);

        // return true because the plugin is correctly initialized
        return true;
    }


    // find changed
    void LogWindowPlugin::OnTextFilterChanged(const QString& text)
    {
        mLogCallback->SetFind(text);
    }


    // type filter changed
    void LogWindowPlugin::OnTypeFilterChanged(const QVector<AzQtComponents::SearchTypeFilter>& filters)
    {
        uint32 newFilter = 0;
        for (const auto& filter : filters)
        {
            newFilter |= filter.metadata.toInt();
        }
        mLogCallback->SetFilter(newFilter);
    }


    void LogWindowPlugin::AddFilter(const QString& name, MCore::LogCallback::ELogLevel level, bool enabled)
    {
        AzQtComponents::SearchTypeFilter filter(tr("Level"), name);
        filter.metadata = static_cast<int>(level);
        filter.enabled = enabled;
        mSearchWidget->AddTypeFilter(filter);
    }

} // namespace EMStudio

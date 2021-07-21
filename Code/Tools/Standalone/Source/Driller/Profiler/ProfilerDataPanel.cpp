/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <limits>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/containers/vector.h>

#include "ProfilerDataPanel.hxx"
#include <Source/Driller/Profiler/moc_ProfilerDataPanel.cpp>
#include "ProfilerDataAggregator.hxx"
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

#include "Source/Driller/StripChart.hxx"
#include "Source/Driller/DrillerAggregator.hxx"
#include "Source/Driller/Profiler/ProfilerOperationTelemetryEvent.h"

#include "ProfilerEvents.h"

#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QApplication>
#include <QAction>
#include <QToolTip>
#include <QPainter>

namespace Driller
{
    enum
    {
        PDM_FUNCTIONNAME = 0,
        PDM_COMMENT,
        PDM_EXCLUSIVE_TIME,
        PDM_INCLUSIVE_TIME,
        PDM_EXCLUSIVE_PCT,
        PDM_INCLUSIVE_PCT,
        PDM_CALLS,
        PDM_CHILDREN_TIME,
        PDM_ACCUMULATED_TIME,
        PDM_CHILDREN_CALLS,
        PDM_ACCUMULATED_CALLS,
        PDM_THREAD_ID,
        PDM_TIME_TOTAL
    };
    static const char* PDM_TIME_STRING[] = {
        "Function",
        "Comment",
        "Excl. Time (Micro)",
        "Incl. Time (Micro)",
        "Excl. Pct",
        "Incl. Pct",
        "Calls",
        "Child Time (Micro)",
        "Total Time (Micro)",
        "Child Calls",
        "Total Calls",
        "Thread ID"
    };
    enum
    {
        PDM_NUMERIC_DATA_ROLE = Qt::UserRole + 1
    };

    enum
    {
        PDM_VALUE_FUNCTIONNAME = 0,
        PDM_VALUE_COMMENT,
        PDM_VALUE_1,
        PDM_VALUE_2,
        PDM_VALUE_3,
        PDM_VALUE_4,
        PDM_VALUE_5,
        PDM_VALUE_THREAD_ID,
        PDM_VALUE_TOTAL
    };
    static const char* PDM_VALUE_STRING[] = {
        "Function",
        "Comment",
        "Value 1",
        "Value 2",
        "Value 3",
        "Value 4",
        "Value 5",
        "Thread ID"
    };

    int ProfilerDataModel::m_colorIndexTracker = 0;

    class ProfilerFilterModel
        : public QSortFilterProxyModel
    {
    public:
        AZ_CLASS_ALLOCATOR(ProfilerFilterModel, AZ::SystemAllocator, 0);
        ProfilerFilterModel(QObject* pParent)
            : QSortFilterProxyModel(pParent)
        {
            setFilterCaseSensitivity(Qt::CaseSensitive);
            setDynamicSortFilter(false);
        }
    protected:
        virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
        {
            switch (left.column())
            {
            case PDM_FUNCTIONNAME:
            case PDM_COMMENT:
                return QSortFilterProxyModel::lessThan(left, right);
                break;
            default:
                AZ::u64 leftNumber = 0;
                AZ::u64 rightNumber = 0;
                // inner switch sanity check that we only pull numbers from numeric fields and default the rest to 0<0 false
                switch (left.column())
                {
                case PDM_INCLUSIVE_TIME:
                case PDM_EXCLUSIVE_TIME:
                case PDM_INCLUSIVE_PCT:
                case PDM_EXCLUSIVE_PCT:
                case PDM_CHILDREN_TIME:
                case PDM_ACCUMULATED_TIME:
                case PDM_CALLS:
                case PDM_CHILDREN_CALLS:
                case PDM_ACCUMULATED_CALLS:
                case PDM_THREAD_ID:
                    QVariant retrievedColumnLeft = sourceModel()->data(left, PDM_NUMERIC_DATA_ROLE);
                    QVariant retrievedColumnRight = sourceModel()->data(right, PDM_NUMERIC_DATA_ROLE);

                    leftNumber = retrievedColumnLeft.toULongLong();
                    rightNumber = retrievedColumnRight.toULongLong();

                    break;
                }

                return leftNumber < rightNumber;
                break;
            }
        }
    };

    class ProfilerValueFilterModel
        : public QSortFilterProxyModel
    {
    public:
        AZ_CLASS_ALLOCATOR(ProfilerValueFilterModel, AZ::SystemAllocator, 0);
        ProfilerValueFilterModel(QObject* pParent)
            : QSortFilterProxyModel(pParent)
        {
            setFilterCaseSensitivity(Qt::CaseSensitive);
            setDynamicSortFilter(false);
        }
    protected:
        virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
        {
            switch (left.column())
            {
            case PDM_FUNCTIONNAME:
            case PDM_COMMENT:
                return QSortFilterProxyModel::lessThan(left, right);
                break;
            default:
                AZ::u64 leftNumber = 0;
                AZ::u64 rightNumber = 0;
                // inner switch sanity check that we only pull numbers from numeric fields and default the rest to 0<0 false
                switch (left.column())
                {
                case PDM_VALUE_1:
                case PDM_VALUE_2:
                case PDM_VALUE_3:
                case PDM_VALUE_4:
                case PDM_VALUE_5:
                case PDM_VALUE_THREAD_ID:
                    QVariant retrievedColumnLeft = sourceModel()->data(left, PDM_NUMERIC_DATA_ROLE);
                    QVariant retrievedColumnRight = sourceModel()->data(right, PDM_NUMERIC_DATA_ROLE);

                    leftNumber = retrievedColumnLeft.toULongLong();
                    rightNumber = retrievedColumnRight.toULongLong();

                    break;
                }

                return leftNumber < rightNumber;
                break;
            }
        }
    };

    ProfilerAxisFormatter::ProfilerAxisFormatter(QObject* pParent, int whichTypeOfRegister)
        : QAbstractAxisFormatter(pParent)
    {
        m_lastAxisValueForScaling = 1.0f;
        m_whatKindOfRegister = whichTypeOfRegister;
    }

    QString ProfilerAxisFormatter::formatMicroseconds(float value)
    {
        // data is in microseconds!
        // so how big is the division size?
        if (m_lastAxisValueForScaling > 100000.0f) // greater than a 0.1 second per division
        {
            if (m_lastAxisValueForScaling > 1000000.0f) // whole seconds
            {
                return QObject::tr("%1s").arg(QString::number(value / 1000000.0f, 'f', 0));
            }
            else
            {
                return QObject::tr("%1s").arg(QString::number(value / 1000000.0f, 'f', 1));
            }
        }
        else if (m_lastAxisValueForScaling > 100.0f) // greater than a 0.1 millisecond per division
        {
            if (m_lastAxisValueForScaling > 1000.0f) // whole milliseconds
            {
                return QObject::tr("%1%2").arg(QString::number(value / 1000.0f, 'f', 0)).arg("ms");
            }
            else
            {
                return QObject::tr("%1%2").arg(QString::number(value / 1000.0f, 'f', 1)).arg("ms");
            }
        }
        else if (m_lastAxisValueForScaling > 1.0f)
        {
            return QObject::tr("%1%2s").arg((int)value).arg(QChar(0x00b5));
        }
        else
        {
            return QObject::tr("%1%2s").arg(QString::number((double)value, 'f', 2)).arg(QChar(0x00b5));
        }
    }

    QString ProfilerAxisFormatter::convertAxisValueToText(Charts::AxisType axis, float value, float /*minDisplayedValue*/, float /*maxDisplayedValue*/, float divisionSize)
    {
        if (axis == Charts::AxisType::Vertical)
        {
            if (m_whatKindOfRegister == (int)Profiler::RegisterInfo::PRT_TIME)
            {
                m_lastAxisValueForScaling = divisionSize;
                return formatMicroseconds(value);
            }
            else
            {
                return QString::number(value);
            }
        }
        else
        {
            return QString::number((int)value);
        }
    };


    ///////////////////////
    // ProfilerDataWidget
    ///////////////////////

    ProfilerDataWidget::ProfilerDataWidget(QWidget* parent)
        : AzToolsFramework::QTreeViewWithStateSaving(parent)
        , m_dataModel(NULL)
    {
        m_cachedChart = NULL;
        m_cachedColumn = PDM_EXCLUSIVE_TIME;
        m_autoZoom = true;
        m_cachedFlatView = false;
        m_cachedDeltaData = true;
        m_manualZoomMin = 2000000000.0f;
        m_manualZoomMax = -2000000000.0f;
        m_iLastHighlightedChannel = -1;

        setFocusPolicy(Qt::StrongFocus); // required for key press handling

        setEnabled(true);
        setSortingEnabled(true);
        sortByColumn(0, Qt::AscendingOrder);
        header()->setSectionResizeMode(QHeaderView::Interactive);
        header()->setSectionsMovable(true);
        header()->setStretchLastSection(false);
        setUniformRowHeights(true);

        connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(OnDoubleClicked(const QModelIndex &)));
    }

    ProfilerDataWidget::~ProfilerDataWidget()
    {
        //m_stateSaver->Detach();

        if (m_dataModel)
        {
            delete m_dataModel;
        }
    }

    void ProfilerDataWidget::SetViewType(int viewType)
    {
        m_viewType = viewType;
        if (m_viewType == Profiler::RegisterInfo::PRT_TIME)
        {
            m_dataModel = new ProfilerDataModel();
            if (m_dataModel)
            {
                m_filterModel = aznew ProfilerFilterModel(this);
                m_filterModel->setSourceModel(m_dataModel);
                setModel(m_filterModel);
            }
        }
        if (m_viewType == Profiler::RegisterInfo::PRT_VALUE)
        {
            m_dataModel = new ProfilerCounterDataModel();
            if (m_dataModel)
            {
                m_filterModel = aznew ProfilerValueFilterModel(this);
                m_filterModel->setSourceModel(m_dataModel);
                setModel(m_filterModel);
            }
        }

        AZ_Assert(m_dataModel, "SetViewType has an invalid argument, profiler data model cannot be created!");
    }

    void ProfilerDataWidget::OnChartTypeMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            OnChartTypeMenu(qa->objectName());
        }
    }

    void ProfilerDataWidget::OnChartTypeMenu(QString typeStr)
    {
        ProfilerOperationTelemetryEvent chartTypeChanged;

        int newValue = 0;

        if (m_viewType == Profiler::RegisterInfo::PRT_TIME)
        {
            newValue = PDM_EXCLUSIVE_TIME;

            chartTypeChanged.SetAttribute("ChartTimeType", typeStr.toStdString().c_str());

            if (typeStr == "Incl.Time")
            {
                newValue = PDM_INCLUSIVE_TIME;
            }
            if (typeStr == "Excl.Time")
            {
                newValue = PDM_EXCLUSIVE_TIME;
            }
            if (typeStr == "Calls")
            {
                newValue = PDM_CALLS;
            }
            if (typeStr == "Acc.Time")
            {
                newValue = PDM_ACCUMULATED_TIME;
            }
            if (typeStr == "Acc.Calls")
            {
                newValue = PDM_ACCUMULATED_CALLS;
            }
        }
        else if (m_viewType == Profiler::RegisterInfo::PRT_VALUE)
        {
            newValue = PDM_VALUE_1;

            chartTypeChanged.SetAttribute("ChartValueType", typeStr.toStdString().c_str());

            if (typeStr == "Value 1")
            {
                newValue = PDM_VALUE_1;
            }
            if (typeStr == "Value 2")
            {
                newValue = PDM_VALUE_2;
            }
            if (typeStr == "Value 3")
            {
                newValue = PDM_VALUE_3;
            }
            if (typeStr == "Value 4")
            {
                newValue = PDM_VALUE_4;
            }
            if (typeStr == "Value 5")
            {
                newValue = PDM_VALUE_5;
            }
        }

        chartTypeChanged.Log();
        m_cachedColumn = newValue;
        RedrawChart();
    }

    void ProfilerDataWidget::BeginDataModelUpdate()
    {
        PauseTreeViewSaving();

        m_dataModel->BeginAddRegisters();
    }

    void ProfilerDataWidget::EndDataModelUpdate()
    {
        m_dataModel->EndAddRegisters();

        // this will capture any changes/zoom to the chart by the user
        if ((!m_autoZoom) && (m_cachedChart))
        {
            m_cachedChart->GetWindowRange(Charts::AxisType::Vertical, m_manualZoomMin, m_manualZoomMax);
        }

        UnpauseTreeViewSaving();
        ApplyTreeViewSnapshot();
    }

    void ProfilerDataWidget::OnExpandAll()
    {
        expandAll();
        CaptureTreeViewSnapshot(); // expand all doesn't signal, this captures the fully open tree

        // now that column organization is being stored in user settings
        // I'm disabling this so the user isn't surprised by his view suddenly changing
        //resizeColumnToContents( 0 );
    }

    void ProfilerDataWidget::OnHideSelected()
    {
        m_iLastHighlightedChannel = -1;
        QModelIndexList list = selectionModel()->selectedIndexes();
        foreach (QModelIndex index, list)
        {
            if (index.column() == 0)
            {
                QModelIndex sourceIndex = m_filterModel->mapToSource(index);
                const Driller::ProfilerDrillerUpdateRegisterEvent* tp = (Driller::ProfilerDrillerUpdateRegisterEvent*)(sourceIndex.internalPointer());
                if (tp)
                {
                    if (m_dataModel->m_enabledChartingMap.find(tp->GetRegister()->GetInfo().m_id) != m_dataModel->m_enabledChartingMap.end())
                    {
                        m_dataModel->m_enabledChartingMap[ tp->GetRegister()->GetInfo().m_id ] = 0;
                        QModelIndex column0(m_dataModel->index(index.row(), 0));
                        emit dataChanged(column0, column0);
                    }
                }
            }
        }
        update();
        RedrawChart();
    }

    void ProfilerDataWidget::OnShowSelected()
    {
        QModelIndexList list = selectionModel()->selectedIndexes();
        foreach (QModelIndex index, list)
        {
            if (index.column() == 0)
            {
                QModelIndex sourceIndex = m_filterModel->mapToSource(index);
                const Driller::ProfilerDrillerUpdateRegisterEvent* tp = (Driller::ProfilerDrillerUpdateRegisterEvent*)(sourceIndex.internalPointer());
                if (tp)
                {
                    if (m_dataModel->m_enabledChartingMap.find(tp->GetRegister()->GetInfo().m_id) != m_dataModel->m_enabledChartingMap.end())
                    {
                        m_dataModel->m_enabledChartingMap[ tp->GetRegister()->GetInfo().m_id ] = 1;
                        QModelIndex column0(m_dataModel->index(index.row(), 0));
                        emit dataChanged(column0, column0);
                    }
                }
            }
        }
        update();
        RedrawChart();
    }

    void ProfilerDataWidget::OnInvertHidden()
    {
        AZStd::map<AZ::u64, int>::iterator iter = m_dataModel->m_enabledChartingMap.begin();
        while (iter != m_dataModel->m_enabledChartingMap.end())
        {
            int current = iter->second;
            iter->second = !current;
            ++iter;
        }
        update();
        RedrawChart();
    }

    void ProfilerDataWidget::OnHideAll()
    {
        m_iLastHighlightedChannel = -1;
        AZStd::map<AZ::u64, int>::iterator iter = m_dataModel->m_enabledChartingMap.begin();
        while (iter != m_dataModel->m_enabledChartingMap.end())
        {
            iter->second = 0;
            ++iter;
        }
        update();
        RedrawChart();
    }

    void ProfilerDataWidget::OnShowAll()
    {
        AZStd::map<AZ::u64, int>::iterator iter = m_dataModel->m_enabledChartingMap.begin();
        while (iter != m_dataModel->m_enabledChartingMap.end())
        {
            iter->second = 1;
            ++iter;
        }
        update();
        RedrawChart();
    }

    void ProfilerDataWidget::OnAutoZoomChange(bool newValue)
    {
        if (!newValue)
        {
            m_autoZoom = false;
            m_cachedChart->GetWindowRange(Charts::AxisType::Vertical, m_manualZoomMin, m_manualZoomMax);
        }
        else
        {
            m_autoZoom = true;
            m_manualZoomMin = 2000000000.0f;
            m_manualZoomMax = -2000000000.0f;
        }
        update();
        RedrawChart();
    }

    void ProfilerDataWidget::OnFlatView(bool isOn)
    {
        m_cachedFlatView = isOn;
        m_dataModel->SetFlatView(m_cachedFlatView);

        update();
        RedrawChart();
    }

    void ProfilerDataWidget::OnDeltaData(bool isOn)
    {
        m_cachedDeltaData = isOn;
        m_dataModel->SetDeltaData(m_cachedDeltaData);

        update();
        RedrawChart();
    }

    void ProfilerDataWidget::OnDoubleClicked(const QModelIndex& index)
    {
        if (index.isValid())
        {
            QModelIndex sourceIndex = m_filterModel->mapToSource(index);
            const Driller::ProfilerDrillerUpdateRegisterEvent* tp = (Driller::ProfilerDrillerUpdateRegisterEvent*)(sourceIndex.internalPointer());
            if (tp)
            {
                if (m_dataModel->m_enabledChartingMap.find(tp->GetRegister()->GetInfo().m_id) != m_dataModel->m_enabledChartingMap.end())
                {
                    int current = m_dataModel->m_enabledChartingMap.find(tp->GetRegister()->GetInfo().m_id)->second;
                    m_dataModel->m_enabledChartingMap[ tp->GetRegister()->GetInfo().m_id ] = !current;
                    QModelIndex column0(m_dataModel->index(index.row(), 0));
                    emit dataChanged(column0, column0); // no matter where we clicked, update only column 0
                    RedrawChart();
                }
            }
        }
    }

    void ProfilerDataWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        QTreeView::selectionChanged(selected, deselected);
    }

    void ProfilerDataWidget::RedrawChart()
    {
        if (m_cachedChart)
        {
            m_iLastHighlightedChannel = -1;
            m_cachedChart->Reset();

            m_cachedChart->AddAxis("Frame", static_cast<float>(m_cachedStartFrame), static_cast<float>(m_cachedStartFrame + m_cachedDisplayRange), true, true);
            m_cachedChart->AddAxis("", m_manualZoomMin, m_manualZoomMax, false, false);

            m_cachedChart->SetDataDirty();
        }
    }

    void ProfilerDataWidget::ConfigureChart(StripChart::DataStrip* chart, FrameNumberType atFrame, int howFar, FrameNumberType frameCount)
    {
        (void)frameCount;
        // this can be NULL
        if (chart)
        {
            // stored against the need to update on local selection changes, and used internally
            if (m_cachedChart != chart)
            {
                m_cachedChart = chart;
                ProfilerAxisFormatter* prf = aznew ProfilerAxisFormatter(this, (int)m_viewType);
                m_cachedChart->SetAxisTextFormatter(prf);
                m_formatter = prf;

                m_cachedChart->AttachDataSourceWidget(this);
            }

            m_cachedDisplayRange = howFar;

            m_cachedCurrentFrame = atFrame;
            m_cachedEndFrame = atFrame;
            m_cachedStartFrame = AZStd::GetMax(atFrame - m_cachedDisplayRange, 0);

            RedrawChart();
        }
    }

    void ProfilerDataWidget::ProvideData(StripChart::DataStrip* chart)
    {
        PlotTimeHistory(chart);
    }

    void ProfilerDataWidget::onMouseOverNothing(float primaryAxisValue, float dependentAxisValue)
    {
        (void)primaryAxisValue;
        (void)dependentAxisValue;
        if (m_iLastHighlightedChannel != -1)
        {
            m_cachedChart->SetChannelHighlight(m_iLastHighlightedChannel, false);
            m_iLastHighlightedChannel = -1;
            m_dataModel->SetHighlightedRegisterID(0);
        }
    }

    void ProfilerDataWidget::onMouseOverDataPoint(int channelID, AZ::u64 sampleID, float primaryAxisValue, float dependentAxisValue)
    {
        if (!m_cachedChart)
        {
            return;
        }

        (void)primaryAxisValue;

        auto found = m_ChannelsToRegisters.find(channelID);
        if (found == m_ChannelsToRegisters.end())
        {
            return;
        }
        const Driller::ProfilerDrillerNewRegisterEvent* currentRegister = found->second;

        if (!currentRegister)
        {
            return;
        }

        if (m_iLastHighlightedChannel != -1)
        {
            m_cachedChart->SetChannelHighlight(m_iLastHighlightedChannel, false);
            m_iLastHighlightedChannel = -1;
            m_dataModel->SetHighlightedRegisterID(0);
        }

        m_iLastHighlightedChannel = channelID;
        m_cachedChart->SetChannelHighlight(m_iLastHighlightedChannel, true);
        auto foundChannel = m_ChannelsToRegisters.find(channelID);
        if (foundChannel != m_ChannelsToRegisters.end())
        {
            m_dataModel->SetHighlightedRegisterID(foundChannel->second->GetInfo().m_id);
        }

        const Driller::ProfilerDrillerUpdateRegisterEvent* previousRegister = currentRegister->GetLastSample();
        while ((previousRegister) && (previousRegister->GetGlobalEventId() != (unsigned int)sampleID))
        {
            previousRegister = previousRegister->GetPreviousSample();
        }

        if (previousRegister)
        {
            QString tooltip;
            QString identifier = tr("%1(%2) %3")
                    .arg(currentRegister->GetInfo().m_function ? currentRegister->GetInfo().m_function : "???")
                    .arg(currentRegister->GetInfo().m_line)
                    .arg(currentRegister->GetInfo().m_name ? QString("'%1'").arg(currentRegister->GetInfo().m_name) : "");

            if (previousRegister->GetRegister()->GetInfo().m_type == Profiler::RegisterInfo::PRT_TIME)
            {
                QString displayValue;

                switch (m_cachedColumn)
                {
                case PDM_INCLUSIVE_TIME:
                    displayValue = tr("Inclusive: %1").arg(m_formatter->formatMicroseconds(dependentAxisValue));
                    break;
                case PDM_EXCLUSIVE_TIME:
                    displayValue = tr("Exclusive: %1").arg(m_formatter->formatMicroseconds(dependentAxisValue));
                    break;
                case PDM_CALLS:
                    displayValue = tr("%1 calls").arg((int)dependentAxisValue);
                    break;
                case PDM_ACCUMULATED_TIME:
                    displayValue = tr("Accumulated: %1%").arg(m_formatter->formatMicroseconds(dependentAxisValue));
                    break;
                case PDM_ACCUMULATED_CALLS:
                    displayValue = tr("%1 accumulated calls").arg((int)dependentAxisValue);
                    break;
                }

                tooltip = QString("%1: %2").arg(identifier).arg(displayValue);

                if (QApplication::activeWindow() == this->parent())
                {
                    QToolTip::showText(m_cachedChart->mapToGlobal(QPoint(0, 0)), tooltip, m_cachedChart);
                }
            }
            else
            {
                // value register:
            }
        }
    }

    void ProfilerDataWidget::PlotTimeHistory(StripChart::DataStrip* chart)
    {
        m_ChannelsToRegisters.clear();

        if (chart)
        {
            float maxVerticalValue = 0;

            chart->SetMarkerColor(QColor(255, 0, 0));
            chart->SetMarkerPosition(static_cast<float>(m_cachedCurrentFrame));

            chart->StartBatchDataAdd();

            for (const Driller::ProfilerDrillerUpdateRegisterEvent* currentRegister : m_dataModel->m_profilerDrillerUpdateRegisterEvents)
            {
                const Driller::ProfilerDrillerUpdateRegisterEvent* previousRegister = currentRegister->GetPreviousSample();

                if (previousRegister)
                {
                    if (m_dataModel->m_enabledChartingMap.find(currentRegister->GetRegister()->GetInfo().m_id)->second)
                    {
                        FrameNumberType localAtFrame = m_cachedCurrentFrame;
                        FrameNumberType localHowFar = m_cachedDisplayRange;

                        int channelID  = -1;
                        const Driller::ProfilerDrillerNewRegisterEvent* reg = currentRegister->GetRegister();
                        if (reg)
                        {
                            if ((reg->GetInfo().m_name) && (strlen(reg->GetInfo().m_name) > 0))
                            {
                                channelID =  chart->AddChannel(reg->GetInfo().m_name);
                            }
                            else if ((reg->GetInfo().m_function) && (strlen(reg->GetInfo().m_function) > 0))
                            {
                                channelID = chart->AddChannel(tr("%1(%2)").arg(reg->GetInfo().m_function).arg(reg->GetInfo().m_line));
                            }
                            else
                            {
                                channelID = chart->AddChannel(tr("Unknown Register:%1").arg(reg->GetInfo().m_id));
                            }

                            chart->SetChannelStyle(channelID, StripChart::Channel::STYLE_CONNECTED_LINE);
                            chart->SetChannelColor(channelID, m_dataModel->m_colorMap.find(currentRegister->GetRegister()->GetInfo().m_id)->second);
                        }
                        else
                        {
                            channelID = chart->AddChannel("NULL");
                            chart->SetChannelStyle(channelID, StripChart::Channel::STYLE_CONNECTED_LINE);
                            chart->SetChannelColor(channelID, m_dataModel->m_colorMap.find(0)->second);
                        }

                        // If we don't have a valid channel ID skip over.
                        if (!chart->IsValidChannelId(channelID))
                        {
                            continue;
                        }

                        m_ChannelsToRegisters[channelID] = reg;

                        while (localAtFrame >= 0 && localHowFar >= 0)
                        {
                            float sample = 0.0f;

                            if (m_viewType == Profiler::RegisterInfo::PRT_TIME)
                            {
                                switch (m_cachedColumn)
                                {
                                case PDM_INCLUSIVE_TIME:
                                    sample = (float)(currentRegister->GetData().m_timeData.m_time - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_time));
                                    break;
                                case PDM_EXCLUSIVE_TIME:
                                    sample = (float)(((currentRegister->GetData().m_timeData.m_time - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_time)) - (currentRegister->GetData().m_timeData.m_childrenTime - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_childrenTime))));
                                    break;
                                case PDM_CALLS:
                                    sample = (float)(currentRegister->GetData().m_timeData.m_calls - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_calls));
                                    break;
                                case PDM_ACCUMULATED_TIME:
                                    sample = (float)(currentRegister->GetData().m_timeData.m_time);
                                    break;
                                case PDM_ACCUMULATED_CALLS:
                                    sample = (float)(currentRegister->GetData().m_timeData.m_calls);
                                    break;
                                }
                            }
                            else if (m_viewType == Profiler::RegisterInfo::PRT_VALUE)
                            {
                                AZ::u64 columnNumber = 0;
                                AZ::u64 columnDelta = 0;

                                switch (m_cachedColumn)
                                {
                                case PDM_VALUE_1:
                                    columnNumber = currentRegister->GetData().m_valueData.m_value1;
                                    break;
                                case PDM_VALUE_2:
                                    columnNumber = currentRegister->GetData().m_valueData.m_value2;
                                    break;
                                case PDM_VALUE_3:
                                    columnNumber = currentRegister->GetData().m_valueData.m_value3;
                                    break;
                                case PDM_VALUE_4:
                                    columnNumber = currentRegister->GetData().m_valueData.m_value4;
                                    break;
                                case PDM_VALUE_5:
                                    columnNumber = currentRegister->GetData().m_valueData.m_value5;
                                    break;
                                case PDM_VALUE_THREAD_ID:
                                    columnNumber = currentRegister->GetRegister()->GetInfo().m_threadId;
                                    break;
                                }
                                if (m_cachedDeltaData)
                                {
                                    switch (m_cachedColumn)
                                    {
                                    case PDM_VALUE_1:
                                        columnDelta = (previousRegister == NULL ? 0 : previousRegister->GetData().m_valueData.m_value1);
                                        break;
                                    case PDM_VALUE_2:
                                        columnDelta = (previousRegister == NULL ? 0 : previousRegister->GetData().m_valueData.m_value2);
                                        break;
                                    case PDM_VALUE_3:
                                        columnDelta = (previousRegister == NULL ? 0 : previousRegister->GetData().m_valueData.m_value3);
                                        break;
                                    case PDM_VALUE_4:
                                        columnDelta = (previousRegister == NULL ? 0 : previousRegister->GetData().m_valueData.m_value4);
                                        break;
                                    case PDM_VALUE_5:
                                        columnDelta = (previousRegister == NULL ? 0 : previousRegister->GetData().m_valueData.m_value5);
                                        break;
                                    }
                                }

                                sample = (float)(columnNumber - columnDelta);
                            }

                            maxVerticalValue = AZStd::GetMax(sample, maxVerticalValue);

                            chart->AddBatchedData(channelID, currentRegister->GetGlobalEventId(), (float)localAtFrame, sample);

                            currentRegister = previousRegister;
                            if (!currentRegister)
                            {
                                break;
                            }
                            previousRegister = currentRegister->GetPreviousSample();

                            --localAtFrame;
                            --localHowFar;
                        }
                    }
                }
            }

            // Always assume 0 as the minimum
            chart->SetWindowRange(Charts::AxisType::Vertical, 0, maxVerticalValue);

            // Adding one here so I can actually see the scrubber mark.
            float minValue = 0.0f;
            float maxValue = 0.0f;
            if (chart->GetAxisRange(Charts::AxisType::Horizontal, minValue, maxValue))
            {
                chart->SetWindowRange(Charts::AxisType::Horizontal, minValue, maxValue + 0.5f);
            }

            chart->EndBatchDataAdd();
        }

        if (m_autoZoom)
        {
            chart->ZoomExtents(Charts::AxisType::Vertical);
        }
        else
        {
            chart->ZoomManual(Charts::AxisType::Vertical, m_manualZoomMin, m_manualZoomMax);
        }

    }

    //------------------------------------------------------------------------

    ProfilerDataModel::ProfilerDataModel()
    {
        m_SourceAggregator = NULL;
        m_cachedFlatView = false;
        m_highlightedRegisterID = 0;
    }

    ProfilerDataModel::~ProfilerDataModel()
    {
        EmptyTheEventCache();
        m_colorMap.clear();
        m_iconMap.clear();
        m_enabledChartingMap.clear();
    }

    QVariant ProfilerDataModel::headerData (int section, Qt::Orientation orientation, int role) const
    {
        (void)orientation;

        if (role == Qt::DisplayRole)
        {
            return QVariant(PDM_TIME_STRING[section]);
        }

        return QVariant();
    }

    QVariant ProfilerDataModel::data (const QModelIndex& index, int role) const
    {
        if (index.isValid() && m_SourceAggregator && m_SourceAggregator->IsValid())
        {
            const Driller::ProfilerDrillerUpdateRegisterEvent* registerEvent = static_cast<Driller::ProfilerDrillerUpdateRegisterEvent*>(index.internalPointer());

            if (role == Qt::BackgroundRole)
            {
                if (m_highlightedRegisterID != 0)
                {
                    if (registerEvent->GetRegisterId() == m_highlightedRegisterID)
                    {
                        return QVariant::fromValue(QColor(94, 94, 178, 255));
                    }
                }
            }
            // a color swatch to match register to chart, or black if not drawn to the chart
            if (role == Qt::DecorationRole && index.column() == 0 /*COLOR SWATCH*/)
            {
                if (registerEvent->GetRegister())
                {
                    if (m_enabledChartingMap.find(registerEvent->GetRegister()->GetInfo().m_id) != m_enabledChartingMap.end())
                    {
                        if (m_enabledChartingMap.find(registerEvent->GetRegister()->GetInfo().m_id)->second)
                        {
                            return QVariant(m_iconMap.find(registerEvent->GetRegister()->GetInfo().m_id)->second);
                        }
                        else
                        {
                            return QVariant(m_iconMap.find(0)->second);
                        }
                    }
                    else
                    {
                        return QVariant(m_iconMap.find(0)->second);
                    }
                }
            }
            if (role == Qt::DisplayRole || role == PDM_NUMERIC_DATA_ROLE)
            {
                const Driller::ProfilerDrillerUpdateRegisterEvent* currentRegister = registerEvent;
                const Driller::ProfilerDrillerUpdateRegisterEvent* previousRegister = currentRegister->GetPreviousSample();

                if (role == Qt::DisplayRole)
                {
                    switch (index.column())
                    {
                    case PDM_FUNCTIONNAME:
                        if (currentRegister->GetRegister())
                        {
                            AZStd::string name = AZStd::string::format("%s(%d)"
                                    , currentRegister->GetRegister()->GetInfo().m_function ? currentRegister->GetRegister()->GetInfo().m_function : "N/A"
                                    , currentRegister->GetRegister()->GetInfo().m_line);
                            return QVariant(name.c_str());
                        }
                        else
                        {
                            return QVariant("N/A");
                        }
                        break;
                    case PDM_COMMENT:
                        return QVariant(QString(currentRegister->GetRegister() ? currentRegister->GetRegister()->GetInfo().m_name ? currentRegister->GetRegister()->GetInfo().m_name : "" : ""));
                        break;
                    }
                }

                AZ::u64 columnNumber = 0;

                switch (index.column())
                {
                case PDM_INCLUSIVE_TIME:
                    columnNumber = currentRegister->GetData().m_timeData.m_time - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_time);
                    break;
                case PDM_EXCLUSIVE_TIME:
                    columnNumber = (currentRegister->GetData().m_timeData.m_time - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_time)) - (currentRegister->GetData().m_timeData.m_childrenTime - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_childrenTime));
                    break;
                case PDM_INCLUSIVE_PCT:
                    columnNumber = currentRegister->GetData().m_timeData.m_time - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_time);
                    break;
                case PDM_EXCLUSIVE_PCT:
                    columnNumber = (currentRegister->GetData().m_timeData.m_time - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_time)) - (currentRegister->GetData().m_timeData.m_childrenTime - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_childrenTime));
                    break;
                case PDM_CHILDREN_TIME:
                    columnNumber = currentRegister->GetData().m_timeData.m_childrenTime - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_childrenTime);
                    break;
                case PDM_ACCUMULATED_TIME:
                    columnNumber = currentRegister->GetData().m_timeData.m_time;
                    break;
                case PDM_CALLS:
                    columnNumber = currentRegister->GetData().m_timeData.m_calls - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_calls);
                    break;
                case PDM_CHILDREN_CALLS:
                    columnNumber = currentRegister->GetData().m_timeData.m_childrenCalls - (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_childrenCalls);
                    break;
                case PDM_ACCUMULATED_CALLS:
                    columnNumber = currentRegister->GetData().m_timeData.m_calls;
                    break;
                case PDM_THREAD_ID:
                    columnNumber = currentRegister->GetRegister()->GetInfo().m_threadId;
                    break;
                }

                if (role == Qt::DisplayRole)
                {
                    if (index.column() == PDM_INCLUSIVE_PCT || index.column() == PDM_EXCLUSIVE_PCT)
                    {
                        float percent = (float)columnNumber / (float)m_totalTime;
                        return QVariant(QString::number(percent * 100.0f, 'f', 2));
                    }
                    if (index.column() == PDM_THREAD_ID)
                    {
                        return QVariant(QString("%1").arg(columnNumber));
                    }
                    else
                    {
                        return QVariant(QString("%L1").arg(columnNumber));
                    }
                }
                else if (role == PDM_NUMERIC_DATA_ROLE)
                {
                    return QVariant(columnNumber);
                }

                return QVariant();
            }
        }

        return QVariant();
    }

    Qt::ItemFlags ProfilerDataModel::flags (const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return Qt::ItemFlags();
        }

        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    QModelIndex ProfilerDataModel::index (int row, int column, const QModelIndex& parent) const
    {
        if (hasIndex(row, column, parent) && m_SourceAggregator && m_SourceAggregator->IsValid())
        {
            if (m_cachedFlatView)
            {
                // look up the rowTh data item
                return createIndex(row, column, (void*)(m_profilerDrillerUpdateRegisterEvents[row]));
            }

            if (!parent.isValid())
            {
                // look up the rowTh data item with no parent itself
                int foundCount = 0;
                DVVector::const_iterator iter = m_profilerDrillerUpdateRegisterEvents.begin();
                while (iter != m_profilerDrillerUpdateRegisterEvents.end())
                {
                    if ((*iter)->GetData().m_timeData.m_lastParentRegisterId == 0)
                    {
                        if (foundCount == row)
                        {
                            return createIndex(row, column, (void*)(*iter));
                        }

                        ++foundCount;
                    }
                    ++iter;
                }
            }
            else
            {
                Driller::ProfilerDrillerUpdateRegisterEvent* registerEvent = static_cast<Driller::ProfilerDrillerUpdateRegisterEvent*>(parent.internalPointer());
                // look up the rowTh data item with pt as a parent
                int foundCount = 0;
                DVVector::const_iterator iter = m_profilerDrillerUpdateRegisterEvents.begin();
                while (iter != m_profilerDrillerUpdateRegisterEvents.end())
                {
                    if ((*iter)->GetData().m_timeData.m_lastParentRegisterId == registerEvent->GetRegister()->GetInfo().m_id)
                    {
                        if (foundCount == row)
                        {
                            return createIndex(row, column, (void*)(*iter));
                        }
                        ++foundCount;
                    }
                    ++iter;
                }
            }
        }

        return QModelIndex();
    }

    QModelIndex ProfilerDataModel::parent (const QModelIndex& index) const
    {
        if (m_cachedFlatView)
        {
            return QModelIndex();
        }
        if (index.isValid() && m_SourceAggregator && m_SourceAggregator->IsValid())
        {
            Driller::ProfilerDrillerUpdateRegisterEvent* childItem = static_cast<Driller::ProfilerDrillerUpdateRegisterEvent*>(index.internalPointer());

            DVVector::const_iterator mainIter = m_profilerDrillerUpdateRegisterEvents.begin();
            while (mainIter != m_profilerDrillerUpdateRegisterEvents.end())
            {
                // 0th frame the updates haven't been linked to their registers yet, that happens on 1st
                // this is a guard against that
                if ((*mainIter)->GetRegister())
                {
                    if ((*mainIter)->GetRegister()->GetInfo().m_id == childItem->GetData().m_timeData.m_lastParentRegisterId)
                    {
                        // row() should be the parent's row in it's own parent
                        int parentRow = 0;
                        DVVector::const_iterator parentIter = m_profilerDrillerUpdateRegisterEvents.begin();
                        while (parentIter != m_profilerDrillerUpdateRegisterEvents.end())
                        {
                            if ((*parentIter)->GetRegister()->GetData().m_timeData.m_lastParentRegisterId == (*mainIter)->GetData().m_timeData.m_lastParentRegisterId)
                            {
                                if (*parentIter == *mainIter)
                                {
                                    return createIndex(parentRow, 0, (void*)(*mainIter));
                                }
                                ++parentRow;
                            }
                            ++parentIter;
                        }
                    }
                }

                ++mainIter;
            }
        }

        return QModelIndex();
    }

    int ProfilerDataModel::rowCount (const QModelIndex& parent) const
    {
        int foundCount = 0;

        if (m_SourceAggregator && m_SourceAggregator->IsValid())
        {
            if (m_cachedFlatView && !parent.isValid())
            {
                foundCount = (int)m_profilerDrillerUpdateRegisterEvents.size();
            }
            else if (m_cachedFlatView)
            {
                foundCount = 0;
            }
            else if (!parent.isValid())
            {
                // count data items with no parent id
                DVVector::const_iterator iter = m_profilerDrillerUpdateRegisterEvents.begin();
                while (iter != m_profilerDrillerUpdateRegisterEvents.end())
                {
                    if ((*iter)->GetData().m_timeData.m_lastParentRegisterId == 0)
                    {
                        ++foundCount;
                    }
                    ++iter;
                }
            }
            else
            {
                Driller::ProfilerDrillerUpdateRegisterEvent* registerEvent = static_cast<Driller::ProfilerDrillerUpdateRegisterEvent*>(parent.internalPointer());
                // count data items with pt as a parent
                DVVector::const_iterator iter = m_profilerDrillerUpdateRegisterEvents.begin();
                while (iter != m_profilerDrillerUpdateRegisterEvents.end())
                {
                    // 0th frame the updates haven't been linked to their registers yet, that happens on 1st
                    // this is a guard against that
                    if (registerEvent->GetRegister())
                    {
                        if ((*iter)->GetData().m_timeData.m_lastParentRegisterId == registerEvent->GetRegister()->GetInfo().m_id)
                        {
                            ++foundCount;
                        }
                    }
                    ++iter;
                }
            }
        }

        return foundCount;
    }

    int ProfilerDataModel::columnCount (const QModelIndex& /*parent*/) const
    {
        return PDM_TIME_TOTAL;
    }

    void ProfilerDataModel::EmptyTheEventCache()
    {
        m_profilerDrillerUpdateRegisterEvents.clear();
    }
    void ProfilerDataModel::BeginAddRegisters()
    {
        beginResetModel();
        EmptyTheEventCache();
        m_totalTime = 0;
    }
    void ProfilerDataModel::AddRegister(const Driller::ProfilerDrillerUpdateRegisterEvent* newData)
    {
        if (newData->GetRegister())
        {
            if (newData->GetRegister()->GetInfo().m_type == Profiler::RegisterInfo::PRT_TIME)
            {
                m_profilerDrillerUpdateRegisterEvents.push_back(newData);

                const Driller::ProfilerDrillerUpdateRegisterEvent* currentRegister = newData;
                const Driller::ProfilerDrillerUpdateRegisterEvent* previousRegister = currentRegister->GetPreviousSample();

                const AZ::u64 previousRegisterTime = (previousRegister == nullptr ? 0 : previousRegister->GetData().m_timeData.m_time);
                const AZ::u64 registerDeltaTime = currentRegister->GetData().m_timeData.m_time - previousRegisterTime;

                const AZ::u64 previousChildTime = (previousRegister == NULL ? 0 : previousRegister->GetData().m_timeData.m_childrenTime);
                const AZ::u64 childDeltaTime = currentRegister->GetData().m_timeData.m_childrenTime - previousChildTime;

                AZ::u64 t = registerDeltaTime - childDeltaTime;
                m_totalTime += t;
            }
        }
    }

    void ProfilerDataModel::EndAddRegisters()
    {
        Recolor();
        endResetModel();
    }
    void ProfilerDataModel::SetAggregator(Driller::ProfilerDataAggregator* aggregator)
    {
        m_SourceAggregator = aggregator;
    }

    QColor ProfilerDataModel::GetColorByIndex(int colorIdx, int maxNumColors)
    {
        QColor col;
        float sat = .9f;
        float val = .9f;
        col.setHsvF((float)(colorIdx % maxNumColors) / (float(maxNumColors)), sat, val);
        return col;
    }

    // lazy build of a mapping between Event ID# and QColor for chart display(s)
    void ProfilerDataModel::Recolor()
    {
        // these two numbers used to cycle broadly around the color wheel
        // so that proximal entries are never too similar in hue
        const int magicNumber = 32; // magic number
        const int magicIncrement = 5;

        // black for disabled on the chart
        if (m_colorMap.find(0) == m_colorMap.end())
        {
            QPixmap pixmap(16, 16);
            QPainter painter(&pixmap);
            painter.setBrush(Qt::black);
            painter.drawRect(0, 0, 16, 16);
            QIcon itemIcon(pixmap);
            m_iconMap[ 0 ] = itemIcon;
        }

        DVVector::const_iterator iter = m_profilerDrillerUpdateRegisterEvents.begin();
        while (iter != m_profilerDrillerUpdateRegisterEvents.end())
        {
            if ((*iter)->GetRegister())
            {
                if (m_colorMap.find((*iter)->GetRegister()->GetInfo().m_id) == m_colorMap.end())
                {
                    // charting map and color map always in lockstep
                    m_enabledChartingMap[ (*iter)->GetRegister()->GetInfo().m_id ] = 1;

                    QColor qc = GetColorByIndex(m_colorIndexTracker, magicNumber);
                    m_colorMap[ (*iter)->GetRegister()->GetInfo().m_id ] = qc;

                    QPixmap pixmap(16, 16);
                    QPainter painter(&pixmap);
                    painter.setBrush(qc);
                    painter.drawRect(0, 0, 16, 16);
                    QIcon itemIcon(pixmap);
                    m_iconMap[ (*iter)->GetRegister()->GetInfo().m_id ] = itemIcon;

                    m_colorIndexTracker += magicIncrement;
                }
            }

            ++iter;
        }
    }

    void ProfilerDataModel::SetFlatView(bool on)
    {
        emit layoutAboutToBeChanged();
        m_cachedFlatView = on;
        emit layoutChanged();
    }

    void ProfilerDataModel::SetDeltaData(bool on)
    {
        emit layoutAboutToBeChanged();
        m_cachedDeltaData = on;
        emit layoutChanged();
    }

    void ProfilerDataModel::SetHighlightedRegisterID(AZ::u64 regid)
    {
        if (m_highlightedRegisterID != regid)
        {
            m_highlightedRegisterID = regid;
            emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
        }
    }


    //------------------------------------------------------------------------

    ProfilerCounterDataModel::ProfilerCounterDataModel()
    {
    }

    ProfilerCounterDataModel::~ProfilerCounterDataModel()
    {
    }
    void ProfilerCounterDataModel::AddRegister(const Driller::ProfilerDrillerUpdateRegisterEvent* newData)
    {
        if (newData->GetRegister())
        {
            if (newData->GetRegister()->GetInfo().m_type == Profiler::RegisterInfo::PRT_VALUE)
            {
                m_profilerDrillerUpdateRegisterEvents.push_back(newData);
            }
        }
    }

    QVariant ProfilerCounterDataModel::headerData (int section, Qt::Orientation orientation, int role) const
    {
        (void)orientation;

        if (role == Qt::DisplayRole)
        {
            return QVariant(PDM_VALUE_STRING[section]);
        }

        return QVariant();
    }

    QVariant ProfilerCounterDataModel::data (const QModelIndex& index, int role) const
    {
        if (index.isValid() && m_SourceAggregator && m_SourceAggregator->IsValid())
        {
            const Driller::ProfilerDrillerUpdateRegisterEvent* registerEvent = static_cast<Driller::ProfilerDrillerUpdateRegisterEvent*>(index.internalPointer());

            if (role == Qt::BackgroundRole)
            {
                if (m_highlightedRegisterID != 0)
                {
                    if (registerEvent->GetRegisterId() == m_highlightedRegisterID)
                    {
                        return QVariant::fromValue(QColor(94, 94, 178, 255));
                    }
                }
            }
            // a color swatch to match register to chart, or black if not drawn to the chart
            if (role == Qt::DecorationRole && index.column() == 0 /*COLOR SWATCH*/)
            {
                if (registerEvent->GetRegister())
                {
                    if (m_enabledChartingMap.find(registerEvent->GetRegister()->GetInfo().m_id) != m_enabledChartingMap.end())
                    {
                        if (m_enabledChartingMap.find(registerEvent->GetRegister()->GetInfo().m_id)->second)
                        {
                            return QVariant(m_iconMap.find(registerEvent->GetRegister()->GetInfo().m_id)->second);
                        }
                        else
                        {
                            return QVariant(m_iconMap.find(0)->second);
                        }
                    }
                    else
                    {
                        return QVariant(m_iconMap.find(0)->second);
                    }
                }
            }
            if (role == Qt::DisplayRole || role == PDM_NUMERIC_DATA_ROLE)
            {
                const Driller::ProfilerDrillerUpdateRegisterEvent* currentRegister = registerEvent;
                const Driller::ProfilerDrillerUpdateRegisterEvent* previousRegister = currentRegister->GetPreviousSample();

                if (role == Qt::DisplayRole)
                {
                    switch (index.column())
                    {
                    case PDM_FUNCTIONNAME:
                        if (currentRegister->GetRegister())
                        {
                            AZStd::string name = AZStd::string::format("%s(%d)"
                                    , currentRegister->GetRegister()->GetInfo().m_function ? currentRegister->GetRegister()->GetInfo().m_function : "N/A"
                                    , currentRegister->GetRegister()->GetInfo().m_line);
                            return QVariant(name.c_str());
                        }
                        else
                        {
                            return QVariant("N/A");
                        }
                        break;
                    case PDM_COMMENT:
                        return QVariant(QString(currentRegister->GetRegister() ? currentRegister->GetRegister()->GetInfo().m_name ? currentRegister->GetRegister()->GetInfo().m_name : "" : ""));
                        break;
                    }
                }

                AZ::u64 columnNumber = 0;
                AZ::u64 columnDelta = 0;

                switch (index.column())
                {
                case PDM_VALUE_1:
                    columnNumber = currentRegister->GetData().m_valueData.m_value1;
                    break;
                case PDM_VALUE_2:
                    columnNumber = currentRegister->GetData().m_valueData.m_value2;
                    break;
                case PDM_VALUE_3:
                    columnNumber = currentRegister->GetData().m_valueData.m_value3;
                    break;
                case PDM_VALUE_4:
                    columnNumber = currentRegister->GetData().m_valueData.m_value4;
                    break;
                case PDM_VALUE_5:
                    columnNumber = currentRegister->GetData().m_valueData.m_value5;
                    break;
                case PDM_VALUE_THREAD_ID:
                    columnNumber = currentRegister->GetRegister()->GetInfo().m_threadId;
                    break;
                }
                if (m_cachedDeltaData)
                {
                    switch (index.column())
                    {
                    case PDM_VALUE_1:
                        columnDelta = (previousRegister == NULL ? 0 : previousRegister->GetData().m_valueData.m_value1);
                        break;
                    case PDM_VALUE_2:
                        columnDelta = (previousRegister == NULL ? 0 : previousRegister->GetData().m_valueData.m_value2);
                        break;
                    case PDM_VALUE_3:
                        columnDelta = (previousRegister == NULL ? 0 : previousRegister->GetData().m_valueData.m_value3);
                        break;
                    case PDM_VALUE_4:
                        columnDelta = (previousRegister == NULL ? 0 : previousRegister->GetData().m_valueData.m_value4);
                        break;
                    case PDM_VALUE_5:
                        columnDelta = (previousRegister == NULL ? 0 : previousRegister->GetData().m_valueData.m_value5);
                        break;
                    }
                }

                columnNumber -= columnDelta;

                if (role == Qt::DisplayRole)
                {
                    if (index.column() == PDM_VALUE_THREAD_ID)
                    {
                        return QVariant(QString("%1").arg(columnNumber));
                    }
                    else
                    {
                        return QVariant(QString("%L1").arg(columnNumber));
                    }
                }
                else if (role == PDM_NUMERIC_DATA_ROLE)
                {
                    return QVariant(columnNumber);
                }

                return QVariant();
            }
        }

        return QVariant();
    }

    int ProfilerCounterDataModel::columnCount (const QModelIndex& /*parent*/) const
    {
        return PDM_VALUE_TOTAL;
    }
}

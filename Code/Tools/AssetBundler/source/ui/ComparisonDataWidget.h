/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzToolsFramework/Asset/AssetBundler.h>

#include <QWidget>
#endif

namespace Ui
{
    class ComparisonDataWidget;
}

namespace AssetBundler
{
    //! Widget for displaying and editing a single Comparison Step inside a Comparison Rules file.
    class ComparisonDataWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit ComparisonDataWidget(
            AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> comparisonList,
            size_t comparisonDataIndex,
            const AZStd::string& defaultAssetListFileDirectory,
            QWidget* parent = nullptr);
        virtual ~ComparisonDataWidget() {}

        enum ComparisonTypeIndex : int
        {
            Default = 0,
            Delta,
            Union,
            Intersection,
            Complement,
            Wildcard,
            Regex,
            MAX
        };

        size_t GetComparisonDataIndex() { return m_comparisonDataIndex; }

        void UpdateListOfTokenNames();

    Q_SIGNALS:
        void comparisonDataChanged();

        void comparisonDataTokenNameChanged(size_t comparisonDataIndex);

    private:
        void SetAllDisplayValues(const AzToolsFramework::AssetFileInfoListComparison::ComparisonData& comparisonData);

        void OnNameLineEditChanged();

        void UpdateOnComparisonTypeChanged(bool isFilePatternOperation);

        void InitComparisonTypeComboBox(const AzToolsFramework::AssetFileInfoListComparison::ComparisonData& comparisonData);

        void OnComparisonTypeComboBoxChanged(int index);

        void OnFilePatternLineEditChanged();

        void OnFirstInputComboBoxChanged(int index);

        void SetFirstInputFileVisibility(bool isVisible);

        void OnFirstInputBrowseButtonPressed();

        void OnSecondInputComboBoxChanged(int index);

        void SetSecondInputFileVisibility(bool isVisible);

        void OnSecondInputBrowseButtonPressed();

        QString BrowseButtonPressed();

        bool IsComparisonDataIndexValid();

        QString RemoveTokenCharFromString(const AZStd::string& tokenName);

        QSharedPointer<Ui::ComparisonDataWidget> m_ui;

        AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> m_comparisonList;
        size_t m_comparisonDataIndex;

        AZStd::string m_defaultAssetListFileDirectory;

        QStringList m_inputTokenNameList;
        QString m_outputAssetListFileAbsolutePath;

        bool m_isFirstInputFileNameVisible = false;
        bool m_isSecondInputFileNameVisible = false;
    };

    class MouseWheelEventFilter
        : public QObject
    {
        Q_OBJECT

    public:
        explicit MouseWheelEventFilter(QObject* parent) : QObject(parent) {}

    protected:
        bool eventFilter(QObject* obj, QEvent* ev) override;
    };


    //! Wrapper widget that controls the expansion state and signals that trigger a context menu of a ComparisonDataWidget.
    class ComparisonDataCard
        : public AzQtComponents::Card
    {
        Q_OBJECT

    public:
        explicit ComparisonDataCard(
            AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> comparisonList,
            size_t comparisonDataIndex,
            const AZStd::string& defaultAssetListFileDirectory,
            QWidget* parent = nullptr);
        virtual ~ComparisonDataCard() {}

        ComparisonDataWidget* GetComparisonDataWidget() { return m_comparisonDataWidget; }

    Q_SIGNALS:
        void comparisonDataCardContextMenuRequested(size_t comparisonDataIndex, const QPoint& position);

    private:
        ComparisonDataWidget* m_comparisonDataWidget = nullptr;

    private slots:
        void OnContextMenuRequested(const QPoint& position);
    };


} // namespace AssetBundler

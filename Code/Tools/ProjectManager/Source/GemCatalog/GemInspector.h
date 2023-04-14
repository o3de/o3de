/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <GemCatalog/GemInfo.h>
#include <GemCatalog/GemModel.h>
#include <GemsSubWidget.h>
#include <LinkWidget.h>

#include <QItemSelection>
#include <QScrollArea>
#include <QThread>
#include <QDir>
#endif

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSpacerItem)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QThread)
QT_FORWARD_DECLARE_CLASS(QDir)

namespace O3DE::ProjectManager
{
    class GemInspectorWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit GemInspectorWorker();
        ~GemInspectorWorker() = default;

    signals:
        void Done(QString size);

    public slots:
        void SetDir(QString dir);

    private:
        void GetDirSize(QDir dir, quint64& sizeTotal);
    };

    class GemInspector
        : public QScrollArea
    {
        Q_OBJECT

    public:
        explicit GemInspector(GemModel* model, QWidget* parent, bool readOnly = false);
        ~GemInspector();

        void Update(const QPersistentModelIndex& modelIndex, const QString& version = "", const QString& path = "");
        static QLabel* CreateStyledLabel(QLayout* layout, int fontSize, const QString& colorCodeString);

        // Fonts
        inline constexpr static int s_baseFontSize = 12;

        // Colors
        inline constexpr static const char* s_headerColor = "#FFFFFF";
        inline constexpr static const char* s_textColor = "#DDDDDD";

    signals:
        void TagClicked(const Tag& tag);
        void UpdateGem(const QModelIndex& modelIndex, const QString& version = "", const QString& path = "");
        void UninstallGem(const QModelIndex& modelIndex, const QString& path = "");
        void EditGem(const QModelIndex& modelIndex, const QString& path = "");
        void DownloadGem(const QModelIndex& modelIndex, const QString& version = "", const QString& path = "");
        void ShowToastNotification(const QString& notification);

    private slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnVersionChanged(int index);
        void OnDirSizeSet(QString size);
        void OnCopyDownloadLinkClicked();

    private:
        void InitMainWidget();
        QString GetVersion() const;
        QString GetVersionPath() const;

        bool m_readOnly = false;

        GemModel* m_model = nullptr;
        QWidget* m_mainWidget = nullptr;
        QVBoxLayout* m_mainLayout = nullptr;
        QPersistentModelIndex m_curModelIndex;
        QThread m_workerThread;
        GemInspectorWorker m_worker;

        // General info (top) section
        QLabel* m_nameLabel = nullptr;
        QLabel* m_creatorLabel = nullptr;
        QLabel* m_summaryLabel = nullptr;
        QLabel* m_licenseLabel = nullptr;
        LinkLabel* m_licenseLinkLabel = nullptr;
        LinkLabel* m_directoryLinkLabel = nullptr;
        LinkLabel* m_documentationLinkLabel = nullptr;

        // Requirements
        QLabel* m_requirementsTextLabel = nullptr;

        // Compatibility
        QLabel* m_compatibilityTextLabel = nullptr;

        // Depending gems
        GemsSubWidget* m_dependingGems = nullptr;
        QSpacerItem* m_dependingGemsSpacer = nullptr;

        // Additional information
        QComboBox* m_versionComboBox = nullptr;
        QWidget* m_versionWidget = nullptr;
        QLabel* m_versionLabel = nullptr;
        QLabel* m_enginesTitleLabel = nullptr;
        QLabel* m_enginesLabel = nullptr;
        QLabel* m_lastUpdatedLabel = nullptr;
        QLabel* m_binarySizeLabel = nullptr;
        LinkLabel* m_copyDownloadLinkLabel = nullptr;

        QPushButton* m_updateVersionButton = nullptr;
        QPushButton* m_updateGemButton = nullptr;
        QPushButton* m_editGemButton = nullptr;
        QPushButton* m_uninstallGemButton = nullptr;
        QPushButton* m_downloadGemButton = nullptr;
    };
} // namespace O3DE::ProjectManager

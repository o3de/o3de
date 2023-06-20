/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemInspector.h>
#include <GemCatalog/GemItemDelegate.h>
#include <ProjectManagerDefs.h>
#include <ProjectUtils.h>

#include <QDir>
#include <QFrame>
#include <QLabel>
#include <QSpacerItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QComboBox>
#include <QClipboard>
#include <QGuiApplication>

namespace O3DE::ProjectManager
{
    GemInspectorWorker::GemInspectorWorker() : QObject()
    {
    }

    void GemInspectorWorker::GetDirSize(QDir dir, quint64& sizeTotal)
    {
        const QDir::Filters fileFilters = QDir::Files | QDir::System | QDir::Hidden;
        for (const QString& filePath : dir.entryList(fileFilters))
        {
            sizeTotal += QFileInfo(dir, filePath).size();
        }

        const QDir::Filters dirFilters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden;
        for (const QString& childDirPath : dir.entryList(dirFilters))
        {
            GetDirSize(dir.filePath(childDirPath), sizeTotal);
        }
    }

    void GemInspectorWorker::SetDir(QString dir)
    {
        quint64 size = 0;
        GetDirSize(QDir(dir), size);
        emit Done(QLocale().formattedDataSize(size, QLocale::DataSizeTraditionalFormat));
    }

    GemInspector::GemInspector(GemModel* model, QWidget* parent, bool readOnly)
        : QScrollArea(parent)
        , m_model(model)
        , m_readOnly(readOnly)
    {
        setObjectName("GemCatalogInspector");
        setWidgetResizable(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        m_mainWidget = new QWidget();
        if (parent)
        {
            m_mainWidget->setFixedWidth(parent->width());
        }
        setWidget(m_mainWidget);

        m_mainLayout = new QVBoxLayout();
        m_mainLayout->setMargin(15);
        m_mainLayout->setAlignment(Qt::AlignTop);
        m_mainWidget->setLayout(m_mainLayout);

        InitMainWidget();


        // worker for calculating folder sizes
        m_worker.moveToThread(&m_workerThread);
        connect(&m_worker, &GemInspectorWorker::Done, this, &GemInspector::OnDirSizeSet, Qt::QueuedConnection);
        m_workerThread.start();

        connect(m_model->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &GemInspector::OnSelectionChanged);
        Update({});
    }

    GemInspector::~GemInspector()
    {
        m_workerThread.quit();
        m_workerThread.wait();
    }

    void GemInspector::OnDirSizeSet(QString size)
    {
        m_binarySizeLabel->setText(tr("Binary Size:  %1").arg(size));
    }

    void GemInspector::OnSelectionChanged(const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
    {
        const QModelIndexList selectedIndices = selected.indexes();
        if (selectedIndices.empty())
        {
            Update({});
            return;
        }

        Update(selectedIndices[0]);
    }

    void SetLabelElidedText(QLabel* label, const QString& text, int labelWidth = 0)
    {
        QFontMetrics nameFontMetrics(label->font());
        if (!labelWidth)
        {
            labelWidth = label->width();
        }

        // Don't elide if the widgets are sized too small (sometimes occurs when loading gem catalog)
        if (labelWidth > 100)
        {
            label->setText(nameFontMetrics.elidedText(text, Qt::ElideRight, labelWidth));
        }
        else
        {
            label->setText(text);
        }
    }

    void GemInspector::Update(const QPersistentModelIndex& modelIndex, const QString& version, const QString& path)
    {
        m_curModelIndex = modelIndex;

        if (!modelIndex.isValid())
        {
            m_mainWidget->hide();
            return;
        }

        // use the provided version if available
        QString displayVersion = version;
        QString activeVersion = m_model->GetNewVersion(modelIndex);
        if (activeVersion.isEmpty())
        {
            // fallback to the current version
            activeVersion = m_model->GetVersion(modelIndex);
        }

        if (displayVersion.isEmpty())
        {
            displayVersion = activeVersion;
        }

        const GemInfo& gemInfo = m_model->GetGemInfo(modelIndex, displayVersion, path);
        const bool isMissing = gemInfo.m_path.isEmpty();

        SetLabelElidedText(m_nameLabel, gemInfo.m_displayName.isEmpty() ? gemInfo.m_name : gemInfo.m_displayName);
        SetLabelElidedText(m_creatorLabel, gemInfo.m_origin);

        m_summaryLabel->setText(gemInfo.m_summary);
        m_summaryLabel->adjustSize();

        m_licenseLinkLabel->SetText(gemInfo.m_licenseText);
        m_licenseLinkLabel->SetUrl(gemInfo.m_licenseLink);

        m_directoryLinkLabel->SetUrl(gemInfo.m_directoryLink);
        m_documentationLinkLabel->SetUrl(gemInfo.m_documentationLink);

        m_requirementsTextLabel->setVisible(!gemInfo.m_requirement.isEmpty());
        m_requirementsTextLabel->setText(gemInfo.m_requirement);

        // Depending gems
        const QVector<Tag>& dependingGemTags = m_model->GetDependingGemTags(modelIndex);
        if (!dependingGemTags.isEmpty())
        {
            m_dependingGemsSpacer->changeSize(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
            m_dependingGems->show();
            m_dependingGems->Update(tr("Depending Gems"), tr("The following Gems will be automatically enabled with this Gem."), dependingGemTags);
        }
        else
        {
            m_dependingGems->hide();
            m_dependingGemsSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }

        // Additional information
        m_lastUpdatedLabel->setText(tr("Last Updated: %1").arg(gemInfo.m_lastUpdatedDate));
        m_copyDownloadLinkLabel->setVisible(!gemInfo.m_downloadSourceUri.isEmpty());
        if (gemInfo.m_binarySizeInKB)
        {
            m_binarySizeLabel->setText(tr("Binary Size:  %1 KB").arg(gemInfo.m_binarySizeInKB));
        }
        else if (gemInfo.m_downloadStatus == GemInfo::Downloaded && !isMissing)
        {
            m_binarySizeLabel->setText(tr("Binary Size: ..."));
            QMetaObject::invokeMethod(&m_worker, "SetDir", Qt::QueuedConnection, Q_ARG(QString, gemInfo.m_path));
        }
        else
        {
            m_binarySizeLabel->setText(tr("Binary Size:  Unknown"));
        }

        // Versions
        disconnect(m_versionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GemInspector::OnVersionChanged);
        m_versionComboBox->clear();
        const auto& gemVersions = m_model->GetGemVersions(modelIndex);
        if (gemInfo.m_isEngineGem || gemVersions.count() < 2)
        {
            m_versionComboBox->setVisible(false);
            m_versionLabel->setText(gemInfo.m_version);
            m_versionLabel->setVisible(true);
            m_updateVersionButton->setVisible(false);
        }
        else
        {
            m_versionLabel->setVisible(false);
            m_versionComboBox->setVisible(true);
            for (const auto& gemVersion : gemVersions)
            {
                const GemInfo& gemVersionInfo = gemVersion.value<GemInfo>();
                m_versionComboBox->addItem(gemVersionInfo.m_version, gemVersionInfo.m_path);
            }

            if (m_versionComboBox->count() == 0)
            {
                m_versionComboBox->insertItem(0, "Unknown");
            }

            auto foundIndex = path.isEmpty()? m_versionComboBox->findText(displayVersion) : m_versionComboBox->findData(path);
            m_versionComboBox->setCurrentIndex(foundIndex > -1 ? foundIndex : 0);

            bool versionChanged = displayVersion != activeVersion && !m_readOnly && m_model->IsAdded(modelIndex);
            m_updateVersionButton->setVisible(versionChanged);
            if (versionChanged)
            {
                m_updateVersionButton->setText(tr("Use Version %1").arg(GetVersion()));
            }
            connect(m_versionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GemInspector::OnVersionChanged);
        }

        m_compatibilityTextLabel->setVisible(!gemInfo.IsCompatible());
        if(!gemInfo.IsCompatible())
        {
            if(!gemInfo.m_compatibleEngines.isEmpty())
            {
                if (m_readOnly)
                {
                    m_compatibilityTextLabel->setText(tr("This version is not known to be compatible with the current engine"));
                }
                else
                {
                    m_compatibilityTextLabel->setText(tr("This version is not known to be compatible with the engine this project uses"));
                }
            }
            else
            {
                m_compatibilityTextLabel->setText(tr("This version has missing or incompatible gem dependencies"));
            }
        }

        // Compatible engines
        m_enginesTitleLabel->setVisible(!gemInfo.m_isEngineGem);
        m_enginesLabel->setVisible(!gemInfo.m_isEngineGem);
        if (!gemInfo.m_isEngineGem)
        {
            if (gemInfo.m_compatibleEngines.isEmpty())
            {
                // reduce to one line for common case
                m_enginesTitleLabel->setText(tr("Compatible Engines: All engines"));
                m_enginesLabel->setVisible(false);
            }
            else
            {
                m_enginesTitleLabel->setText(tr("Compatible Engines:"));
                QString compatibleEngines;
                for (int i = 0; i < gemInfo.m_compatibleEngines.size(); ++i)
                {
                    if (i > 0)
                    {
                        compatibleEngines.append("\n");
                    }
                    compatibleEngines.append(ProjectUtils::GetDependencyString(gemInfo.m_compatibleEngines[i]));
                }
                m_enginesLabel->setText(compatibleEngines);
            }
        }

        const bool isRemote = gemInfo.m_gemOrigin == GemInfo::Remote;
        // for now we don't count engine or project gems as local so they cannot be removed
        // currently we don't have a way for users to add gems to the project or engine through
        // the project manager if they remove/unregister them by accident
        const bool isLocal = gemInfo.m_gemOrigin == GemInfo::Local && !gemInfo.m_isEngineGem && !gemInfo.m_isProjectGem;
        const bool isDownloaded = gemInfo.m_downloadStatus == GemInfo::Downloaded ||
                                  gemInfo.m_downloadStatus == GemInfo::DownloadSuccessful;

        m_updateGemButton->setVisible(isRemote && isDownloaded && !isMissing);
        m_uninstallGemButton->setText(isRemote ? tr("Uninstall Gem") : tr("Remove Gem"));
        m_uninstallGemButton->setVisible(!isMissing && ((isRemote && isDownloaded) || isLocal));
        m_editGemButton->setVisible(!isMissing && (!isRemote || (isRemote && isDownloaded)));
        m_downloadGemButton->setVisible(isRemote && !isDownloaded);

        m_mainWidget->adjustSize();
        m_mainWidget->show();
    }

    QLabel* GemInspector::CreateStyledLabel(QLayout* layout, int fontSize, const QString& colorCodeString)
    {
        QLabel* result = new QLabel();
        result->setStyleSheet(QString("font-size: %1px; color: %2;").arg(QString::number(fontSize), colorCodeString));
        layout->addWidget(result);
        return result;
    }

    void GemInspector::OnVersionChanged([[maybe_unused]] int index)
    {
        Update(m_curModelIndex, GetVersion(), GetVersionPath());

        // we don't update the version in the gem list when read only
        // because it can cause the row to disappear due to changing filters
        // but in a project-specific view it is necessary because
        // the checkbox to activate the gem is on the row
        if (!GemModel::IsAdded(m_curModelIndex) && !m_readOnly)
        {
            GemModel::UpdateWithVersion(*m_model, m_curModelIndex, GetVersion(), GetVersionPath());
        }
    }

    void GemInspector::OnCopyDownloadLinkClicked()
    {
        const GemInfo& gemInfo = m_model->GetGemInfo(m_curModelIndex, GetVersion(), GetVersionPath());
        if (!gemInfo.m_downloadSourceUri.isEmpty())
        {
            if(QClipboard* clipboard = QGuiApplication::clipboard(); clipboard != nullptr)
            {
                clipboard->setText(gemInfo.m_downloadSourceUri);

                QString displayname = gemInfo.m_displayName.isEmpty() ? gemInfo.m_name : gemInfo.m_displayName;
                if (gemInfo.m_version.isEmpty() || gemInfo.m_displayName.contains(gemInfo.m_version) || gemInfo.m_version.contains("Unknown", Qt::CaseInsensitive))
                {
                    emit ShowToastNotification(tr("%1 download URL copied to clipboard").arg(displayname));
                }
                else
                {
                    emit ShowToastNotification(tr("%1 %2 download URL copied to clipboard").arg(displayname, gemInfo.m_version));
                }
            }
            else
            {
                emit ShowToastNotification("Failed to copy download URL to clipboard");
            }
        }
    }

    QString GemInspector::GetVersion() const
    {
        return m_versionComboBox->count() > 0 ? m_versionComboBox->currentText() : m_model->GetVersion(m_curModelIndex);
    }

    QString GemInspector::GetVersionPath() const
    {
        return m_versionComboBox->count() > 0 ? m_versionComboBox->currentData().toString() : m_model->GetGemInfo(m_curModelIndex).m_path;
    }

    void GemInspector::InitMainWidget()
    {
        // Gem name, creator and summary
        m_nameLabel = CreateStyledLabel(m_mainLayout, 18, s_headerColor);
        m_creatorLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_headerColor);

        // Version
        {
            m_versionWidget = new QWidget();
            m_versionWidget->setObjectName("GemCatalogVersion");
            auto versionVLayout = new QVBoxLayout();
            versionVLayout->setMargin(0);
            versionVLayout->addSpacing(5);
            auto versionHLayout = new QHBoxLayout();
            versionHLayout->setMargin(0);
            versionVLayout->addLayout(versionHLayout);
            m_versionWidget->setLayout(versionVLayout);
            m_mainLayout->addWidget(m_versionWidget);

            auto versionLabelTitle = new QLabel(tr("Version: "));
            versionLabelTitle->setProperty("class", "label");
            versionHLayout->addWidget(versionLabelTitle);
            m_versionLabel = new QLabel();
            versionHLayout->addWidget(m_versionLabel);

            m_versionComboBox = new QComboBox();
            versionHLayout->addWidget(m_versionComboBox);

            m_updateVersionButton = new QPushButton(tr("Use Version"));
            m_updateVersionButton->setProperty("secondary", true);
            versionVLayout->addWidget(m_updateVersionButton);
            connect(m_updateVersionButton, &QPushButton::clicked, this , [this]{
                GemModel::SetIsAdded(*m_model, m_curModelIndex, true, GetVersion());
                GemModel::UpdateWithVersion(*m_model, m_curModelIndex, GetVersion(), GetVersionPath());

                const GemInfo& gemInfo = GemModel::GetGemInfo(m_curModelIndex);
                if (!gemInfo.m_repoUri.isEmpty())
                {
                    // this gem comes from a remote repository, see if we should download it
                    const auto downloadStatus = GemModel::GetDownloadStatus(m_curModelIndex);
                    if (downloadStatus == GemInfo::NotDownloaded || downloadStatus == GemInfo::DownloadFailed) 
                    {
                        emit DownloadGem(m_curModelIndex, GetVersion(), GetVersionPath());
                    }
                }

                m_updateVersionButton->setVisible(false);
            });

            // Compatibility 
            m_compatibilityTextLabel = new QLabel();
            m_compatibilityTextLabel->setObjectName("GemCatalogCompatibilityWarning");
            m_compatibilityTextLabel->setWordWrap(true);
            m_compatibilityTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            m_compatibilityTextLabel->setOpenExternalLinks(true);
            versionVLayout->addWidget(m_compatibilityTextLabel);

            m_enginesTitleLabel = new QLabel(tr("Compatible Engines: "));
            versionVLayout->addWidget(m_enginesTitleLabel);
            m_enginesLabel = new QLabel();
            versionVLayout->addWidget(m_enginesLabel);
        }

        m_mainLayout->addSpacing(5);

        // TODO: QLabel seems to have issues determining the right sizeHint() for our font with the given font size.
        // This results into squeezed elements in the layout in case the text is a little longer than a sentence.
        m_summaryLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_headerColor);
        m_mainLayout->addWidget(m_summaryLabel);
        m_summaryLabel->setWordWrap(true);
        m_summaryLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_summaryLabel->setOpenExternalLinks(true);
        m_mainLayout->addSpacing(5);

        // License
        {
            QHBoxLayout* licenseHLayout = new QHBoxLayout();
            licenseHLayout->setMargin(0);
            licenseHLayout->setAlignment(Qt::AlignLeft);
            m_mainLayout->addLayout(licenseHLayout);

            m_licenseLabel = CreateStyledLabel(licenseHLayout, s_baseFontSize, s_headerColor);
            m_licenseLabel->setText(tr("License: "));

            m_licenseLinkLabel = new LinkLabel("", QUrl(), s_baseFontSize);
            licenseHLayout->addWidget(m_licenseLinkLabel);

            m_mainLayout->addSpacing(5);
        }

        // Directory and documentation links
        {
            QHBoxLayout* linksHLayout = new QHBoxLayout();
            linksHLayout->setMargin(0);
            m_mainLayout->addLayout(linksHLayout);

            linksHLayout->addStretch();

            m_directoryLinkLabel = new LinkLabel(tr("View in Directory"));
            linksHLayout->addWidget(m_directoryLinkLabel);
            linksHLayout->addWidget(new QLabel("|"));
            m_documentationLinkLabel  = new LinkLabel(tr("Read Documentation"));
            linksHLayout->addWidget(m_documentationLinkLabel);

            linksHLayout->addStretch();

            m_mainLayout->addSpacing(8);
        }

        // Separating line
        QFrame* hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setObjectName("horizontalSeparatingLine");
        m_mainLayout->addWidget(hLine);

        m_mainLayout->addSpacing(10);

        // Requirements
        m_requirementsTextLabel = new QLabel();
        m_requirementsTextLabel->setObjectName("GemCatalogRequirements");
        m_requirementsTextLabel->setWordWrap(true);
        m_requirementsTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_requirementsTextLabel->setOpenExternalLinks(true);

        m_mainLayout->addWidget(m_requirementsTextLabel);

        // Additional information
        auto additionalInfoLabel = new QLabel(tr("Additional Information"));
        additionalInfoLabel->setProperty("class", "title");
        m_mainLayout->addWidget(additionalInfoLabel);

        m_lastUpdatedLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_textColor);
        m_binarySizeLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_textColor);
        m_copyDownloadLinkLabel = new LinkLabel(tr("Copy Download URL"));
        m_mainLayout->addWidget(m_copyDownloadLinkLabel);
        connect(m_copyDownloadLinkLabel, &LinkLabel::clicked, this, &GemInspector::OnCopyDownloadLinkClicked);

        m_mainLayout->addSpacing(20);

        // Depending gems
        m_dependingGems = new GemsSubWidget();
        connect(m_dependingGems, &GemsSubWidget::TagClicked, this, [this](const Tag& tag){ emit TagClicked(tag); });
        m_mainLayout->addWidget(m_dependingGems);
        m_dependingGemsSpacer = new QSpacerItem(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_mainLayout->addSpacerItem(m_dependingGemsSpacer);


        // Update and Uninstall buttons
        m_updateGemButton = new QPushButton(tr("Update Gem"));
        m_updateGemButton->setProperty("secondary", true);
        m_mainLayout->addWidget(m_updateGemButton);
        connect(m_updateGemButton, &QPushButton::clicked, this , [this]{ emit UpdateGem(m_curModelIndex, GetVersion(), GetVersionPath()); });

        m_mainLayout->addSpacing(10);

        m_editGemButton = new QPushButton(tr("Edit Gem"));
        m_editGemButton->setProperty("secondary", true);
        m_mainLayout->addWidget(m_editGemButton);
        connect(m_editGemButton, &QPushButton::clicked, this , [this]{ emit EditGem(m_curModelIndex, GetVersionPath()); });

        m_mainLayout->addSpacing(10);

        m_uninstallGemButton = new QPushButton(tr("Uninstall Gem"));
        m_uninstallGemButton->setProperty("danger", true);
        m_mainLayout->addWidget(m_uninstallGemButton);
        connect(m_uninstallGemButton, &QPushButton::clicked, this , [this]{ emit UninstallGem(m_curModelIndex, GetVersionPath()); });

        m_downloadGemButton = new QPushButton(tr("Download Gem"));
        m_downloadGemButton->setProperty("primary", true);
        m_mainLayout->addWidget(m_downloadGemButton);
        connect(m_downloadGemButton, &QPushButton::clicked, this , [this]{ emit DownloadGem(m_curModelIndex, GetVersion(), GetVersionPath()); });
    }
} // namespace O3DE::ProjectManager

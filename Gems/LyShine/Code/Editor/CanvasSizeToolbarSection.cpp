/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "CommandCanvasSize.h"

#include <QComboBox>
#include <QLineEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QLabel>
#include <QFile>
#include <QDir>

namespace
{
    const QString jsonPresetsEmbeddedPath(":/AppData/size_presets.json");

    //! Returns path to canvas size presets JSON file
    QString GetCanvasSizePresetJsonPath()
    {
        return FileHelpers::GetAppDataPath() + "/size_presets.json";
    }

    //! Attempts to retrieve the JSON presets document object from disk
    //
    //! If the JSON file doesn't exist on disk, we attempt to create it
    //! from our QRC embedded copy.
    QJsonDocument GetJsonFromDisk()
    {
        QString canvasPresetJsonPath(GetCanvasSizePresetJsonPath());
        if (!QFile::exists(canvasPresetJsonPath))
        {
            QDir appDataDir(FileHelpers::GetAppDataPath());
            if (!appDataDir.mkpath("."))
            {
                // Failed to create app data path
                return QJsonDocument();
            }

            // Copy the embedded/cached (QRC) copy of the JSON presets (that
            // we ship) to the expected path.
            QFile jsonPresetsEmbedded(jsonPresetsEmbeddedPath);
            if (!jsonPresetsEmbedded.copy(canvasPresetJsonPath))
            {
                return QJsonDocument();
            }
        }

        QFile canvasPresetJsonFile(canvasPresetJsonPath);
        if (!canvasPresetJsonFile.open(QFile::ReadOnly | QFile::Text))
        {
            return QJsonDocument();
        }

        QTextStream jsonStream(&canvasPresetJsonFile);
        QString jsonData(jsonStream.readAll());
        return QJsonDocument::fromJson(jsonData.toUtf8());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CanvasSizeToolbarSection class
////////////////////////////////////////////////////////////////////////////////////////////////////

CanvasSizeToolbarSection::CanvasSizeToolbarSection(QToolBar* parent)
    : m_comboIndex(-1)
    , m_isChangeUndoable(false)
    , m_canvasSizePresets()
    , m_combobox(new QComboBox(parent))
    , m_lineEditCanvasWidth(new QLineEdit(parent))
    , m_labelCustomSizeDelimiter(new QLabel(parent))
    , m_lineEditCanvasHeight(new QLineEdit(parent))
    , m_lineEditCanvasWidthConnection()
    , m_lineEditCanvasHeightConnection()
    , m_canvasWidthAction(nullptr)
    , m_canvasDelimiterAction(nullptr)
    , m_canvasHeightAction(nullptr)
{
}

CanvasSizeToolbarSection::~CanvasSizeToolbarSection()
{
    QObject::disconnect(m_lineEditCanvasWidthConnection);
    QObject::disconnect(m_lineEditCanvasHeightConnection);
}

void CanvasSizeToolbarSection::InitWidgets(QToolBar* parent, bool addSeparator)
{
    m_editorWindow = static_cast<EditorWindow*>(parent->parent());

    m_combobox->setMinimumContentsLength(20);

    // Canvas presets ComboBox
    {
        parent->addWidget(m_combobox);

        InitCanvasSizePresets();

        int i = 0;
        for (const auto& comboOption : m_canvasSizePresets)
        {
            m_combobox->addItem(comboOption.description, i);
            ++i;
        }

        m_lineEditCanvasWidth->setValidator(new QIntValidator(0, INT_MAX, parent));
        m_lineEditCanvasHeight->setValidator(new QIntValidator(0, INT_MAX, parent));

        QObject::connect(m_combobox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), m_combobox, // IMPORTANT: We HAVE to use static_cast<>() to specify which overload of QComboBox::currentIndexChanged() we want.
            [this](int index)
            {
                //! Called only when the canvas preset ComboBox selection changes
                //
                //! Can be triggered via user input or QComboBox::setCurrentIndex().

                if (m_comboIndex == index)
                {
                    // Nothing to do.
                    return;
                }

                OnComboBoxIndexChanged(index);

                m_comboIndex = index;
            });
    }

    // Width and height line edit boxes for inputting custom canvas sizes
    {
        m_canvasWidthAction = parent->addWidget(m_lineEditCanvasWidth);
        m_canvasDelimiterAction = parent->addWidget(m_labelCustomSizeDelimiter);
        m_canvasHeightAction = parent->addWidget(m_lineEditCanvasHeight);

        // Don't display the custom canvas size widgets in the toolbar by default until
        // the user loads or selects a custom resolution in the canvas size combo-box
        m_canvasWidthAction->setVisible(false);
        m_canvasDelimiterAction->setVisible(false);
        m_canvasHeightAction->setVisible(false);

        m_lineEditCanvasWidth->setMaximumWidth(35);
        m_lineEditCanvasWidth->setValidator(new QIntValidator(1, AZStd::numeric_limits<int>::max(), m_lineEditCanvasWidth));
        m_lineEditCanvasHeight->setMaximumWidth(35);
        m_lineEditCanvasHeight->setValidator(new QIntValidator(1, AZStd::numeric_limits<int>::max(), m_lineEditCanvasHeight));

        // Delimit between width x height
        m_labelCustomSizeDelimiter->setText("x");

        // Listen for changes to custom canvas size for width and height
        m_lineEditCanvasWidthConnection = QObject::connect(m_lineEditCanvasWidth,
                &QLineEdit::editingFinished, m_lineEditCanvasWidth,
                [this]()
                {
                    LineEditWidthEditingFinished();
                });

        m_lineEditCanvasHeightConnection = QObject::connect(m_lineEditCanvasHeight,
                &QLineEdit::editingFinished, m_lineEditCanvasHeight,
                [this]()
                {
                    LineEditHeightEditingFinished();
                });
    }

    if (addSeparator)
    {
        parent->addSeparator();
    }
}

const QString& CanvasSizeToolbarSection::IndexToString(int index)
{
    AZ_Assert(index < m_canvasSizePresets.size(), "Invalid index");
    if (index == GetCustomSizeIndex())
    {
        QString description(
            QString("%1 x %2 (%3)")
            .arg(QString::number(m_canvasSizePresets[index].width))
            .arg(QString::number(m_canvasSizePresets[index].height))
            .arg("other"));
        m_canvasSizePresets[index].description = description;
    }
    return m_canvasSizePresets[index].description;
}

void CanvasSizeToolbarSection::SetInitialResolution(const AZ::Vector2& canvasSize)
{
    int matchingIndex = GetPresetIndexFromSize(canvasSize);
    if (matchingIndex == GetCustomSizeIndex())
    {
        // We are selecting a resolution which doesn't match a preset so set the contents

        // Store the canvas size within the presets array
        m_canvasSizePresets[matchingIndex].width = static_cast<int>(canvasSize.GetX());
        m_canvasSizePresets[matchingIndex].height = static_cast<int>(canvasSize.GetY());
    }

    AZ_Assert(matchingIndex < m_combobox->count(), "Invalid index.");
    m_comboIndex = matchingIndex; // prevent callback of current index change
    m_combobox->setCurrentIndex(matchingIndex);

    // Always check if we need to enable or disable the custom canvas size boxes
    ToggleLineEditBoxes();
}

void CanvasSizeToolbarSection::SetIndex(int index)
{
    // Called by undo/redo commands

    AZ_Assert(index < m_combobox->count(), "Invalid index.");
    m_comboIndex = index; // prevent callback of current index change
    m_combobox->setCurrentIndex(index);

    HandleIndexChanged();
}

void CanvasSizeToolbarSection::SetCustomCanvasSize(AZ::Vector2 canvasSize, bool findPreset)
{
    // Called by undo/redo commands

    int customSizeIndex = GetCustomSizeIndex();

    int matchingIndex;
    if (findPreset)
    {
        matchingIndex = GetPresetIndexFromSize(canvasSize);
    }
    else
    {
        matchingIndex = customSizeIndex;
    }

    if (matchingIndex == customSizeIndex)
    {
        // Store the canvas size within the presets array
        m_canvasSizePresets[matchingIndex].width = static_cast<int>(canvasSize.GetX());
        m_canvasSizePresets[matchingIndex].height = static_cast<int>(canvasSize.GetY());
    }

    int prevIndex = m_comboIndex;

    AZ_Assert(matchingIndex < m_combobox->count(), "Invalid index.");
    m_comboIndex = matchingIndex; // prevent callback of current index change
    m_combobox->setCurrentIndex(matchingIndex);

    ToggleLineEditBoxes();

    if (m_comboIndex == customSizeIndex && prevIndex != m_comboIndex)
    {
        if (m_canvasWidthAction->isVisible())
        {
            // As a convenience, set focus on the width and select all the text so the user can
            // immediately enter their desired resolution.
            m_lineEditCanvasWidth->setFocus();
            m_lineEditCanvasWidth->selectAll();
        }
    }

    SetCanvasSizeByComboBoxIndex();
}

void CanvasSizeToolbarSection::LineEditWidthEditingFinished()
{
    const QString newText(m_lineEditCanvasWidth->text());

    bool success = false;
    int newWidth = newText.toInt(&success);
    AZ_Assert(success, "Only integers should be allowed to be entered");

    const int customSizeIndex = m_combobox->currentIndex();

    if (newWidth != m_canvasSizePresets[customSizeIndex].width)
    {
        if (m_isChangeUndoable)
        {
            // Check if the size is now set to one of the presets
            int presetIndex = GetPresetIndexFromSize(AZ::Vector2(aznumeric_cast<float>(newWidth), aznumeric_cast<float>(m_canvasSizePresets[customSizeIndex].height)));
            if (presetIndex != customSizeIndex)
            {
                // Change combo box index which will trigger an OnComboBoxIndexChanged event and add an undoable command
                m_combobox->setCurrentIndex(presetIndex);
            }
            else
            {
                // Add an undoable command to update the custom canvas size
                CommandCanvasSize::Push(m_editorWindow->GetActiveStack(),
                    m_editorWindow->GetCanvasSizeToolbarSection(),
                    AZ::Vector2(aznumeric_cast<float>(m_canvasSizePresets[customSizeIndex].width), aznumeric_cast<float>(m_canvasSizePresets[customSizeIndex].height)),
                    AZ::Vector2(aznumeric_cast<float>(newWidth), aznumeric_cast<float>(m_canvasSizePresets[customSizeIndex].height)), false);
            }
        }
        else
        {
            m_canvasSizePresets[customSizeIndex].width = newWidth;
            SetCanvasSizeByComboBoxIndex();
        }
    }

    // Get rid of multiple 0s or leading 0s
    m_lineEditCanvasWidth->setText(QString::number(m_canvasSizePresets[customSizeIndex].width));

    m_lineEditCanvasWidth->deselect();
    m_lineEditCanvasWidth->clearFocus();
}

void CanvasSizeToolbarSection::LineEditHeightEditingFinished()
{
    const QString newText(m_lineEditCanvasHeight->text());

    bool success = false;
    int newHeight = newText.toInt(&success);
    AZ_Assert(success, "Only integers should be allowed to be entered");

    const int customSizeIndex = m_combobox->currentIndex();

    if (newHeight != m_canvasSizePresets[customSizeIndex].height)
    {
        if (m_isChangeUndoable)
        {
            // Check if the size is now set to one of the presets
            int presetIndex = GetPresetIndexFromSize(AZ::Vector2(aznumeric_cast<float>(m_canvasSizePresets[customSizeIndex].width), aznumeric_cast<float>(newHeight)));
            if (presetIndex != customSizeIndex)
            {
                // Change combo box index which will trigger an OnComboBoxIndexChanged event and add an undoable command
                m_combobox->setCurrentIndex(presetIndex);
            }
            else
            {
                // Add an undoable command to update the custom canvas size
                CommandCanvasSize::Push(m_editorWindow->GetActiveStack(),
                    m_editorWindow->GetCanvasSizeToolbarSection(),
                    AZ::Vector2(aznumeric_cast<float>(m_canvasSizePresets[customSizeIndex].width), aznumeric_cast<float>(m_canvasSizePresets[customSizeIndex].height)),
                    AZ::Vector2(aznumeric_cast<float>(m_canvasSizePresets[customSizeIndex].width), aznumeric_cast<float>(newHeight)), false);
            }
        }
        else
        {
            m_canvasSizePresets[customSizeIndex].height = newHeight;
            SetCanvasSizeByComboBoxIndex();
        }
    }

    // Get rid of multiple 0s or leading 0s
    m_lineEditCanvasHeight->setText(QString::number(m_canvasSizePresets[customSizeIndex].height));

    m_lineEditCanvasHeight->deselect();
    m_lineEditCanvasHeight->clearFocus();
}

void CanvasSizeToolbarSection::ToggleLineEditBoxes()
{
    int currentIndex = m_combobox->currentIndex();

    const int customSizeIndex = GetCustomSizeIndex();
    const bool enableLineEditBoxes = currentIndex == customSizeIndex;
    m_canvasWidthAction->setVisible(enableLineEditBoxes);
    m_canvasDelimiterAction->setVisible(enableLineEditBoxes);
    m_canvasHeightAction->setVisible(enableLineEditBoxes);

    if (enableLineEditBoxes)
    {
        m_lineEditCanvasWidth->setText(QString::number(m_canvasSizePresets[customSizeIndex].width));
        m_lineEditCanvasHeight->setText(QString::number(m_canvasSizePresets[customSizeIndex].height));
    }
}

bool CanvasSizeToolbarSection::ParseCanvasSizePresetsJson(const QJsonDocument& jsonDoc)
{
    if (jsonDoc.isNull())
    {
        return false;
    }

    QJsonObject jsonObj(jsonDoc.object());
    QJsonValueRef rootElement = jsonObj["canvasSizeToolbar"];

    if (!rootElement.isArray())
    {
        return false;
    }

    QJsonArray canvasPresetArray(rootElement.toArray());
    for (QJsonArray::const_iterator citer = canvasPresetArray.begin();
         citer != canvasPresetArray.end();
         ++citer)
    {
        QJsonValue canvasPresetVal(*citer);

        if (!canvasPresetVal.isObject())
        {
            continue;
        }

        QJsonObject canvasPresetObj(canvasPresetVal.toObject());

        // Basic JSON "schema" validation
        const bool expectedObject =
            canvasPresetObj["width"].isDouble() &&
            canvasPresetObj["height"].isDouble() &&
            canvasPresetObj["title"].isString();

        if (!expectedObject)
        {
            continue;
        }

        int width = canvasPresetObj["width"].toInt();
        int height = canvasPresetObj["height"].toInt();
        QString title(canvasPresetObj["title"].toString());
        QString formattedTitle(
            QString("%1 x %2 (%3)")
                .arg(QString::number(width))
                .arg(QString::number(height))
                .arg(title));

        m_canvasSizePresets.push_back(CanvasSizePresets(formattedTitle, width, height));
    }

    return !m_canvasSizePresets.empty();
}

void CanvasSizeToolbarSection::InitCanvasSizePresets()
{
    // Assume that we haven't tried to add anything to the presets yet
    AZ_Assert(m_canvasSizePresets.empty(), "Canvas size presets already initialized");

    // Allow derived classes to add any special entries in the combo box
    AddSpecialPresets();

    QJsonDocument localJsonDoc(GetJsonFromDisk());
    if (localJsonDoc.isNull() || !ParseCanvasSizePresetsJson(localJsonDoc))
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,
            GetCanvasSizePresetJsonPath().toUtf8().constData(),
            "Couldn't load canvas size preset JSON.");

        QFile canvasPresetJsonFile(jsonPresetsEmbeddedPath);
        if (!canvasPresetJsonFile.open(QFile::ReadOnly | QFile::Text))
        {
            // We should always be able to open our embedded JSON
            AZ_Assert(false, "Failed to open embedded JSON");
        }

        QTextStream jsonStream(&canvasPresetJsonFile);
        QString jsonData(jsonStream.readAll());
        QJsonDocument jsonDoc(QJsonDocument::fromJson(jsonData.toUtf8()));
        if (!ParseCanvasSizePresetsJson(jsonDoc))
        {
            // We should always be able to parse our embedded JSON
            AZ_Assert(false, "Failed to parse embedded JSON");
        }
    }
    m_canvasSizePresets.push_back(CanvasSizePresets("Other...", m_canvasSizePresets[0].width, m_canvasSizePresets[0].height));
}

int CanvasSizeToolbarSection::GetPresetIndexFromSize(AZ::Vector2 size)
{
    ComboBoxOptions::iterator matchingResolution = AZStd::find_if(m_canvasSizePresets.begin(), m_canvasSizePresets.end(),
        [size](const CanvasSizePresets& option)
    {
        return option.width == size.GetX() && option.height == size.GetY();
    });

    int matchingIndex = GetCustomSizeIndex();
    if (matchingResolution != m_canvasSizePresets.end())
    {
        // Resolution matches one of our presets, so select it
        matchingIndex = aznumeric_cast<int>(matchingResolution - m_canvasSizePresets.begin());
    }

    return matchingIndex;
}

void CanvasSizeToolbarSection::HandleIndexChanged()
{
    ToggleLineEditBoxes();

    if (m_canvasWidthAction->isVisible())
    {
        // As a convenience, set focus on the width and select all the text so the user can
        // immediately enter their desired resolution.
        m_lineEditCanvasWidth->setFocus();
        m_lineEditCanvasWidth->selectAll();
    }

    SetCanvasSizeByComboBoxIndex();
}

int CanvasSizeToolbarSection::GetCustomSizeIndex()
{
    return static_cast<int>(m_canvasSizePresets.size() - 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ReferenceCanvasSizeToolbarSection class
////////////////////////////////////////////////////////////////////////////////////////////////////

ReferenceCanvasSizeToolbarSection::ReferenceCanvasSizeToolbarSection(QToolBar* parent, bool addSeparator)
    : CanvasSizeToolbarSection(parent)
{
    InitWidgets(parent, addSeparator);

    m_isChangeUndoable = true;

    m_combobox->setToolTip(QString("Canvas size is used to determine scaling on larger (or smaller) screens if 'scale to device' is used"));
}

void ReferenceCanvasSizeToolbarSection::SetCanvasSizeByComboBoxIndex()
{
    int comboIndex = m_combobox->currentIndex();

    // This is the low level function called by the undoable command (via SetIndex etc)
    AZ::Vector2 canvasSize(aznumeric_cast<float>(m_canvasSizePresets[comboIndex].width), aznumeric_cast<float>(m_canvasSizePresets[comboIndex].height));

    // set the canvas size on the canvas entity being edited
    UiCanvasBus::Event(m_editorWindow->GetCanvas(), &UiCanvasBus::Events::SetCanvasSize, canvasSize);
    m_editorWindow->GetViewport()->GetViewportInteraction()->CenterCanvasInViewport(&canvasSize);
    m_editorWindow->GetViewport()->Refresh();
}

void ReferenceCanvasSizeToolbarSection::OnComboBoxIndexChanged(int index)
{
    // use an undoable command to set the canvas size
    if (index == GetCustomSizeIndex())
    {
        int prevIndex = m_comboIndex;

        CommandCanvasSize::Push(m_editorWindow->GetActiveStack(),
            m_editorWindow->GetCanvasSizeToolbarSection(),
            AZ::Vector2(aznumeric_cast<float>(m_canvasSizePresets[prevIndex].width), aznumeric_cast<float>(m_canvasSizePresets[prevIndex].height)),
            AZ::Vector2(aznumeric_cast<float>(m_canvasSizePresets[index].width), aznumeric_cast<float>(m_canvasSizePresets[index].height)), true);
    }
    else
    {
        CommandCanvasSizeToolbarIndex::Push(m_editorWindow->GetActiveStack(),
            m_editorWindow->GetCanvasSizeToolbarSection(),
            m_comboIndex,
            index);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PreviewCanvasSizeToolbarSection class
////////////////////////////////////////////////////////////////////////////////////////////////////

PreviewCanvasSizeToolbarSection::PreviewCanvasSizeToolbarSection(QToolBar* parent, bool addSeparator)
    : CanvasSizeToolbarSection(parent)
{
    InitWidgets(parent, addSeparator);

    m_combobox->setToolTip(QString("Preview what the canvas would look like on a screen/window/texture of this size."));
}

void PreviewCanvasSizeToolbarSection::SetCanvasSizeByComboBoxIndex()
{
    int comboIndex = m_combobox->currentIndex();

    AZ::Vector2 canvasSize(aznumeric_cast<float>(m_canvasSizePresets[comboIndex].width), aznumeric_cast<float>(m_canvasSizePresets[comboIndex].height));

    // tell the EditorWindow what size we want to preview the canvas at
    m_editorWindow->SetPreviewCanvasSize(canvasSize);
}

void PreviewCanvasSizeToolbarSection::OnComboBoxIndexChanged([[maybe_unused]] int index)
{
    // no need to support undo when changing canvas size in preview mode, just update based on index
    HandleIndexChanged();
}

void PreviewCanvasSizeToolbarSection::AddSpecialPresets()
{
    // add a first entry for using whatever the viewport size is
    m_canvasSizePresets.push_back(CanvasSizePresets("Use viewport size", 0, 0));
}

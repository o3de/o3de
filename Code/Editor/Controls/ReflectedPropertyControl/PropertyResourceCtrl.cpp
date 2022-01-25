/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "PropertyResourceCtrl.h"

// Qt
#include <QHBoxLayout>
#include <QLineEdit>

// AzToolsFramework
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrl.h>

// Editor
#include "Controls/QToolTipWidget.h"
#include "Controls/BitmapToolTip.h"


BrowseButton::BrowseButton(PropertyType type, QWidget* parent /*= nullptr*/)
    : QToolButton(parent)
    , m_propertyType(type)
{
    setAutoRaise(true);
    setIcon(QIcon(QStringLiteral(":/stylesheet/img/UI20/browse-edit.svg")));
    connect(this, &QAbstractButton::clicked, this, &BrowseButton::OnClicked);
}

void BrowseButton::SetPathAndEmit(const QString& path)
{
    //only emit if path changes. Old property control
    if (path != m_path)
    {
        m_path = path;
        emit PathChanged(m_path);
    }
}

class FileBrowseButton
    : public BrowseButton
{
public:
    AZ_CLASS_ALLOCATOR(FileBrowseButton, AZ::SystemAllocator, 0);
    FileBrowseButton(PropertyType type, QWidget* pParent = nullptr)
        : BrowseButton(type, pParent)
    {
        setToolTip("Browse...");
    }

private:
    void OnClicked() override
    {
        QString tempValue("");
        if (!m_path.isEmpty() && !Path::GetExt(m_path).isEmpty())
        {
            tempValue = m_path;
        }

        AssetSelectionModel selection;

        if (m_propertyType == ePropertyTexture)
        {
            // Filters for texture.
            selection = AssetSelectionModel::AssetGroupSelection("Texture");
        }
        else
        {
            return;
        }

        AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
        if (selection.IsValid())
        {
            QString newPath = Path::FullPathToGamePath(selection.GetResult()->GetFullPath().c_str()).c_str();

            switch (m_propertyType)
            {
            case ePropertyTexture:
                newPath.replace("\\\\", "/");
                if (newPath.size() > MAX_PATH)
                {
                    newPath.resize(MAX_PATH);
                }
            }

            SetPathAndEmit(newPath);
        }
    }
};

class AudioControlSelectorButton
    : public BrowseButton
{
public:
    AZ_CLASS_ALLOCATOR(AudioControlSelectorButton, AZ::SystemAllocator, 0);

    AudioControlSelectorButton(PropertyType type, QWidget* pParent = nullptr)
        : BrowseButton(type, pParent)
    {
        setToolTip(tr("Select Audio Control"));
    }

private:
    void OnClicked() override
    {
        AZStd::string resourceResult;
        auto ConvertLegacyAudioPropertyType = [](const PropertyType type) -> AzToolsFramework::AudioPropertyType
        {
            switch (type)
            {
            case ePropertyAudioTrigger:
                return AzToolsFramework::AudioPropertyType::Trigger;
            case ePropertyAudioRTPC:
                return AzToolsFramework::AudioPropertyType::Rtpc;
            case ePropertyAudioSwitch:
                return AzToolsFramework::AudioPropertyType::Switch;
            case ePropertyAudioSwitchState:
                return AzToolsFramework::AudioPropertyType::SwitchState;
            case ePropertyAudioEnvironment:
                return AzToolsFramework::AudioPropertyType::Environment;
            case ePropertyAudioPreloadRequest:
                return AzToolsFramework::AudioPropertyType::Preload;
            default:
                return AzToolsFramework::AudioPropertyType::NumTypes;
            }
        };

        auto propType = ConvertLegacyAudioPropertyType(m_propertyType);
        if (propType != AzToolsFramework::AudioPropertyType::NumTypes)
        {
            AzToolsFramework::AudioControlSelectorRequestBus::EventResult(
                resourceResult, propType, &AzToolsFramework::AudioControlSelectorRequestBus::Events::SelectResource,
                AZStd::string_view{ m_path.toUtf8().constData() });
            SetPathAndEmit(QString{ resourceResult.c_str() });
        }
    }
};

class TextureEditButton
    : public BrowseButton
{
public:
    AZ_CLASS_ALLOCATOR(TextureEditButton, AZ::SystemAllocator, 0);
    TextureEditButton(QWidget* pParent = nullptr)
        : BrowseButton(ePropertyTexture, pParent)
    {
        setIcon(QIcon(QStringLiteral(":/stylesheet/img/UI20/open-in-internal-app.svg")));
        setToolTip(tr("Launch default editor"));
    }

private:
    void OnClicked() override
    {
        CFileUtil::EditTextureFile(m_path.toUtf8().data(), true);
    }
};

FileResourceSelectorWidget::FileResourceSelectorWidget(QWidget* pParent /*= nullptr*/)
    : QWidget(pParent)
    , m_propertyType(ePropertyInvalid)
    , m_tooltip(nullptr)
{
    m_pathEdit = new QLineEdit;
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->addWidget(m_pathEdit, 1);

    m_mainLayout->setContentsMargins(0, 0, 0, 0);

// KDAB just ported the MFC texture preview tooltip, but looks like Amazon added their own. Not sure which to use.
// To switch to Amazon QToolTipWidget, remove FileResourceSelectorWidget::event and m_previewTooltip 
#ifdef USE_QTOOLTIPWIDGET
    m_tooltip = new QToolTipWidget(this);

    installEventFilter(this);
#endif
    connect(m_pathEdit, &QLineEdit::editingFinished, this, [this]() { OnPathChanged(m_pathEdit->text()); });
}

bool FileResourceSelectorWidget::eventFilter([[maybe_unused]] QObject* obj, QEvent* event)
{
    if (m_propertyType == ePropertyTexture)
    {
        if (event->type() == QEvent::ToolTip)
        {
            QHelpEvent* e = (QHelpEvent*)event;

            m_tooltip->AddSpecialContent("TEXTURE", m_path);
            m_tooltip->TryDisplay(e->globalPos(), m_pathEdit, QToolTipWidget::ArrowDirection::ARROW_RIGHT);

            return true;
        }

        if (event->type() == QEvent::Leave)
        {
            m_tooltip->hide();
        }
    }

    return false;
}

void FileResourceSelectorWidget::SetPropertyType(PropertyType type)
{
    if (m_propertyType == type)
    {
        return;
    }

    //if the property type changed for some reason, delete all the existing widgets
    if (!m_buttons.isEmpty())
    {
        qDeleteAll(m_buttons.begin(), m_buttons.end());
        m_buttons.clear();
    }

    m_previewToolTip.reset();
    m_propertyType = type;

    switch (type)
    {
    case ePropertyTexture:
        AddButton(new FileBrowseButton(type));
        AddButton(new TextureEditButton);
        m_previewToolTip.reset(new CBitmapToolTip);
        break;
    case ePropertyAudioTrigger:
    case ePropertyAudioSwitch:
    case ePropertyAudioSwitchState:
    case ePropertyAudioRTPC:
    case ePropertyAudioEnvironment:
    case ePropertyAudioPreloadRequest:
        AddButton(new AudioControlSelectorButton(type));
        break;
    default:
        break;
    }

    m_mainLayout->invalidate();
}

void FileResourceSelectorWidget::AddButton(BrowseButton* button)
{
    m_mainLayout->addWidget(button);
    m_buttons.push_back(button);
    connect(button, &BrowseButton::PathChanged, this, &FileResourceSelectorWidget::OnPathChanged);
}

void FileResourceSelectorWidget::OnPathChanged(const QString& path)
{
    bool changed = SetPath(path);
    if (changed)
    {
        emit PathChanged(m_path);
    }
}

bool FileResourceSelectorWidget::SetPath(const QString& path)
{
    bool changed = false;

    const QString newPath = path.toLower();
    if (m_path != newPath)
    {
        m_path = newPath;
        UpdateWidgets();

        changed = true;
    }

    return changed;
}


void FileResourceSelectorWidget::UpdateWidgets()
{
    m_pathEdit->setText(m_path);

    foreach(BrowseButton * button, m_buttons)
    {
        button->SetPath(m_path);
    }

    if (m_previewToolTip)
    {
        m_previewToolTip->SetTool(this, rect());
    }
}

QString FileResourceSelectorWidget::GetPath() const
{
    return m_path;
}



QWidget* FileResourceSelectorWidget::GetLastInTabOrder()
{
    return m_buttons.empty() ? nullptr : m_buttons.last();
}

QWidget* FileResourceSelectorWidget::GetFirstInTabOrder()
{
    return m_buttons.empty() ? nullptr : m_buttons.first();
}

void FileResourceSelectorWidget::UpdateTabOrder()
{
    if (m_buttons.count() >= 2)
    {
        for (int i = 0; i < m_buttons.count() - 1; ++i)
        {
            setTabOrder(m_buttons[i], m_buttons[i + 1]);
        }
    }
}

bool FileResourceSelectorWidget::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip && m_previewToolTip && !m_previewToolTip->isVisible())
    {
        if (!m_path.isEmpty())
        {
            m_previewToolTip->LoadImage(m_path);
            m_previewToolTip->setVisible(true);
        }
        event->accept();
        return true;
    }

    if (event->type() == QEvent::Resize && m_previewToolTip)
    {
        m_previewToolTip->SetTool(this, rect());
    }

    return QWidget::event(event);
}

QWidget* FileResourceSelectorWidgetHandler::CreateGUI(QWidget* pParent)
{
    FileResourceSelectorWidget* newCtrl = aznew FileResourceSelectorWidget(pParent);
    connect(newCtrl, &FileResourceSelectorWidget::PathChanged, newCtrl, [newCtrl]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
        });
    return newCtrl;
}

void FileResourceSelectorWidgetHandler::ConsumeAttribute(FileResourceSelectorWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    Q_UNUSED(GUI);
    Q_UNUSED(attrib);
    Q_UNUSED(attrValue);
    Q_UNUSED(debugName);
}

void FileResourceSelectorWidgetHandler::WriteGUIValuesIntoProperty(size_t index, FileResourceSelectorWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    CReflectedVarResource val = instance;
    val.m_propertyType = GUI->GetPropertyType();
    val.m_path = GUI->GetPath().toUtf8().data();
    instance = static_cast<property_t>(val);
}

bool FileResourceSelectorWidgetHandler::ReadValuesIntoGUI(size_t index, FileResourceSelectorWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    CReflectedVarResource val = instance;
    GUI->SetPropertyType(val.m_propertyType);
    GUI->SetPath(val.m_path.c_str());
    return false;
}

#include <Controls/ReflectedPropertyControl/moc_PropertyResourceCtrl.cpp>

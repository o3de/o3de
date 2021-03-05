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
#include "UiCanvasEditor_precompiled.h"
#include "EditorCommon.h"

#include "PropertyHandlerSprite.h"

#include <QtGui/QIcon>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>

#include "SpriteBorderEditorCommon.h"

#include <IResourceCompilerHelper.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>

#include <QApplication>
#include <QMessageBox>

PropertySpriteCtrl::PropertySpriteCtrl(QWidget* parent)
    : QWidget(parent)
    , m_propertyAssetCtrl(aznew AzToolsFramework::PropertyAssetCtrl(this, (QString(LmbrCentral::TextureAsset::GetFileFilter()) + "; *.sprite")))
{
    QObject::connect(m_propertyAssetCtrl,
        &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged,
        this,
        [ this ]([[maybe_unused]] AZ::Data::AssetId newAssetID)
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, this);
        });

    setAcceptDrops(true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(m_propertyAssetCtrl);

    // Add Slicer button.
    {
        QPushButton* slicerButton = new QPushButton(this);

        slicerButton->setFlat(true);

        QSize fixedSize = QSize(AzToolsFramework::PropertyQTConstant_DefaultHeight, AzToolsFramework::PropertyQTConstant_DefaultHeight);
        slicerButton->setFixedSize(fixedSize);

        slicerButton->setFocusPolicy(Qt::StrongFocus);

        slicerButton->setIcon(QIcon("Editor/Icons/PropertyEditor/open_in.png"));

        // The icon size needs to be smaller than the fixed size to make sure it visually aligns properly.
        QSize iconSize = QSize(fixedSize.width() - 2, fixedSize.height() - 2);
        slicerButton->setIconSize(iconSize);

        QObject::connect(slicerButton,
            &QPushButton::clicked,
            this,
            [ this ]([[maybe_unused]] bool checked)
            {
                if (!m_propertyAssetCtrl->GetCurrentAssetID().IsValid())
                {
                    // Nothing to do.
                    return;
                }

                AZStd::string assetPath;
                EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, m_propertyAssetCtrl->GetCurrentAssetID());

                SpriteBorderEditor sbe(assetPath.c_str(), this->window());
                if (sbe.GetHasBeenInitializedProperly())
                {
                    sbe.exec();
                    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
                    return;
                }
                else
                {
                    QMessageBox(QMessageBox::Critical,
                        "Error",
                        QString("Failed to load texture. See log for details"),
                        QMessageBox::Ok, QApplication::activeWindow()).exec();
                }

                return;
            });

        layout->addWidget(slicerButton);
    }
}

void PropertySpriteCtrl::dragEnterEvent(QDragEnterEvent* ev)
{
    m_propertyAssetCtrl->dragEnterEvent(ev);
}

void PropertySpriteCtrl::dragLeaveEvent(QDragLeaveEvent* ev)
{
    m_propertyAssetCtrl->dragLeaveEvent(ev);
}

void PropertySpriteCtrl::dropEvent(QDropEvent* ev)
{
    m_propertyAssetCtrl->dropEvent(ev);
}

AzToolsFramework::PropertyAssetCtrl* PropertySpriteCtrl::GetPropertyAssetCtrl()
{
    return m_propertyAssetCtrl;
}

//-------------------------------------------------------------------------------

QWidget* PropertyHandlerSprite::CreateGUI(QWidget* pParent)
{
    return aznew PropertySpriteCtrl(pParent);
}

void PropertyHandlerSprite::ConsumeAttribute(PropertySpriteCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    (void)GUI;
    (void)attrib;
    (void)attrValue;
    (void)debugName;
}

void PropertyHandlerSprite::WriteGUIValuesIntoProperty(size_t index, PropertySpriteCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    (void)index;
    (void)node;

    AZStd::string assetPath;
    EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, GUI->GetPropertyAssetCtrl()->GetCurrentAssetID());

    instance.SetAssetPath(assetPath.c_str());
}

bool PropertyHandlerSprite::ReadValuesIntoGUI(size_t index, PropertySpriteCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    (void)index;
    (void)node;

    AzToolsFramework::PropertyAssetCtrl* ctrl = GUI->GetPropertyAssetCtrl();

    ctrl->blockSignals(true);
    {
        ctrl->SetCurrentAssetType(instance.GetAssetType());

        AZ::Data::AssetId assetId;
        if (!instance.GetAssetPath().empty())
        {
            EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, instance.GetAssetPath().c_str(), instance.GetAssetType(), false);
        }
        ctrl->SetSelectedAssetID(assetId);
    }
    ctrl->blockSignals(false);

    return false;
}

void PropertyHandlerSprite::Register()
{
    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew PropertyHandlerSprite());
}

#include <moc_PropertyHandlerSprite.cpp>

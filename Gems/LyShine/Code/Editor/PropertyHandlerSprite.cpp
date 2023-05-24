/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "Sprite.h"

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

#include <LmbrCentral/Rendering/TextureAsset.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>

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
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, m_propertyAssetCtrl);
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

        slicerButton->setIcon(QIcon(":/stylesheet/img/UI20/open-in-internal-app.svg"));

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
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, m_propertyAssetCtrl->GetCurrentAssetID());

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
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
        assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, GUI->GetPropertyAssetCtrl()->GetCurrentAssetID());

    // Convert streaming image's product path to relative source path to assign to the SimpleAssetReference<Texture>
    AZStd::string sourcePath = CSprite::GetImageSourcePathFromProductPath(assetPath);
    instance.SetAssetPath(sourcePath.c_str());
}

bool PropertyHandlerSprite::ReadValuesIntoGUI(size_t index, PropertySpriteCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    (void)index;
    (void)node;

    AzToolsFramework::PropertyAssetCtrl* ctrl = GUI->GetPropertyAssetCtrl();

    ctrl->blockSignals(true);
    {
        // Set the asset type for the PropertyAssetCtrl.
        // Use the hardcoded streaming image asset type instead of the passed in instance's asset type
        // since the passed in type is the legacy SimpleAssetReference<Texture>, and the asset picker
        // does not associate this type with streaming images
        AZ::Data::AssetType assetType = AZ::AzTypeInfo<AZ::RPI::StreamingImageAsset>::Uuid();
        ctrl->SetCurrentAssetType(assetType);

        AZ::Data::AssetId assetId;
        if (!instance.GetAssetPath().empty())
        {
            // Get the image path from the SimpleAssetReference<Texture> and fix it up since CSprite still
            // allows user specified paths that have the .sprite extension or the deprecated .dds extension
            AZStd::string sourcePath = CSprite::GetImageSourcePathFromProductPath(instance.GetAssetPath());
            AZStd::string fixedUpSourcePath;
            CSprite::FixUpSourceImagePathFromUserDefinedPath(sourcePath, fixedUpSourcePath);

            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &AZ::Data::AssetCatalogRequestBus::Events::GenerateAssetIdTEMP,
                fixedUpSourcePath.c_str());
            assetId.m_subId = AZ::RPI::StreamingImageAsset::GetImageAssetSubId();
        }
        ctrl->SetSelectedAssetID(assetId);
    }
    ctrl->blockSignals(false);

    return false;
}

void PropertyHandlerSprite::Register()
{
    AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
        &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew PropertyHandlerSprite());
}

#include <moc_PropertyHandlerSprite.cpp>

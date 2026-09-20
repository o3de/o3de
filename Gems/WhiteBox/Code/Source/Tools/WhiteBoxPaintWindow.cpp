/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Tools/WhiteBoxPaintWindow.h"

#include "EditorWhiteBoxComponent.h"
#include "Util/WhiteBoxEditorUtil.h"

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>

#include <QCheckBox>
#include <QColorDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace WhiteBox
{
    namespace
    {
        QString OperationName(const FacePaintOperation operation)
        {
            switch (operation)
            {
            case FacePaintOperation::Material: return QObject::tr("Paint Material");
            case FacePaintOperation::Color: return QObject::tr("Paint Color");
            case FacePaintOperation::ResetMaterial: return QObject::tr("Reset Material");
            case FacePaintOperation::ResetColor: return QObject::tr("Reset Color");
            default: return QObject::tr("Paint");
            }
        }

        QString OperationHint(const FacePaintOperation operation)
        {
            switch (operation)
            {
            case FacePaintOperation::Material:
                return QObject::tr("Drag over faces to assign this material.");
            case FacePaintOperation::Color:
                return QObject::tr("Drag over faces to assign this colour.");
            case FacePaintOperation::ResetMaterial:
                return QObject::tr("Drag over faces to drop their material override.");
            case FacePaintOperation::ResetColor:
                return QObject::tr("Drag over faces to drop their painted colour.");
            default: return QString();
            }
        }

        QColor ToQColor(const AZ::u32 packed)
        {
            // FacePaintSettings packs opaque RGBA with R in the low byte.
            return QColor(packed & 255, (packed >> 8) & 255, (packed >> 16) & 255);
        }

        AZ::u32 ToPacked(const QColor& color)
        {
            return static_cast<AZ::u32>(color.red()) | (static_cast<AZ::u32>(color.green()) << 8) |
                (static_cast<AZ::u32>(color.blue()) << 16) | 0xFF000000u;
        }
    } // namespace

    WhiteBoxPaintWindow::WhiteBoxPaintWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent)
        : WhiteBoxModelingWindow(tr("Paint"), parent)
        , m_pair(pair)
    {
        setObjectName("WhiteBoxPaintWindow");

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(1, 1, 1, 10);
        layout->setSpacing(10);

        auto* header = new QWidget(this);
        header->setObjectName("PaintHeader");
        SetDragHandle(header);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(12, 9, 12, 9);
        m_title = new QLabel(tr("Paint"), header);
        m_title->setObjectName("PaintTitle");
        headerLayout->addWidget(m_title);
        headerLayout->addStretch();
        layout->addWidget(header);

        auto* controls = new QGridLayout();
        controls->setContentsMargins(12, 0, 12, 0);
        controls->setHorizontalSpacing(9);
        controls->setVerticalSpacing(7);

        m_materialLabel = new QLabel(tr("Material"), this);
        m_material = new AzToolsFramework::PropertyAssetCtrl(this);
        m_material->SetCurrentAssetType(azrtti_typeid<AZ::RPI::MaterialAsset>());
        controls->addWidget(m_materialLabel, 0, 0);
        controls->addWidget(m_material, 0, 1);

        m_colorLabel = new QLabel(tr("Colour"), this);
        m_color = new QPushButton(this);
        m_color->setMinimumHeight(22);
        m_color->setToolTip(
            tr("Paint an opaque face colour. Custom materials need a base colour property; their textures stay on."));
        controls->addWidget(m_colorLabel, 1, 0);
        controls->addWidget(m_color, 1, 1);

        layout->addLayout(controls);

        // Applies to all four verbs, so it sits outside the per-operation rows.
        m_wholePolygon = new QCheckBox(tr("Whole polygon"), this);
        m_wholePolygon->setToolTip(
            tr("Paint every triangle of the face under the cursor. Off paints the single triangle, which "
               "leaves a drawn quad half-coloured unless you cover both halves."));
        m_wholePolygon->setContentsMargins(12, 0, 12, 0);
        layout->addWidget(m_wholePolygon);

        m_hint = new QLabel(this);
        m_hint->setWordWrap(true);
        m_hint->setObjectName("PaintHint");
        m_hint->setContentsMargins(12, 0, 12, 0);
        layout->addWidget(m_hint);

        connect(
            m_material, &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged, this,
            [this](const AZ::Data::AssetId& material)
            {
                if (m_updating)
                {
                    return;
                }
                auto* component = CurrentComponent();
                if (component == nullptr)
                {
                    Dismiss();
                    return;
                }
                auto settings = component->GetFacePaintSettings();
                settings.m_material = material;
                component->SetFacePaintSettings(settings);
            });
        connect(m_color, &QPushButton::clicked, this, &WhiteBoxPaintWindow::ChooseColor);
        connect(
            m_wholePolygon, &QCheckBox::toggled, this,
            [this](const bool wholePolygon)
            {
                if (m_updating)
                {
                    return;
                }
                if (auto* component = CurrentComponent())
                {
                    auto settings = component->GetFacePaintSettings();
                    settings.m_wholePolygon = wholePolygon;
                    component->SetFacePaintSettings(settings);
                }
            });

        RefreshValues();
    }

    EditorWhiteBoxComponent* WhiteBoxPaintWindow::CurrentComponent() const
    {
        return FindWhiteBoxComponent(m_pair);
    }

    void WhiteBoxPaintWindow::ApplySwatch(const AZ::u32 color)
    {
        const QColor swatch = ToQColor(color);
        // Label the swatch as well as filling it: colour alone is not a readable control, and the hex
        // is what someone matching two faces actually needs.
        m_color->setText(swatch.name().toUpper());
        const bool dark = swatch.lightness() < 128;
        m_color->setStyleSheet(QString("background-color: %1; color: %2;")
                                   .arg(swatch.name(), dark ? QStringLiteral("#FFFFFF") : QStringLiteral("#1F2226")));
    }

    void WhiteBoxPaintWindow::ChooseColor()
    {
        auto* component = CurrentComponent();
        if (component == nullptr)
        {
            Dismiss();
            return;
        }
        auto settings = component->GetFacePaintSettings();
        const QColor chosen = QColorDialog::getColor(ToQColor(settings.m_color), this, tr("Face Paint Colour"));
        if (!chosen.isValid())
        {
            return; // cancelled
        }
        settings.m_color = ToPacked(chosen);
        component->SetFacePaintSettings(settings);
        ApplySwatch(settings.m_color);
    }

    void WhiteBoxPaintWindow::ApplyVisibility(const FacePaintOperation operation)
    {
        const bool material = operation == FacePaintOperation::Material;
        const bool color = operation == FacePaintOperation::Color;

        m_materialLabel->setVisible(material);
        m_material->setVisible(material);
        m_colorLabel->setVisible(color);
        m_color->setVisible(color);

        // The two reset verbs carry nothing, so the window is title plus hint only.
        adjustSize();
    }

    void WhiteBoxPaintWindow::RefreshValues()
    {
        auto* component = CurrentComponent();
        if (component == nullptr)
        {
            Dismiss();
            return;
        }

        const FacePaintSettings settings = component->GetFacePaintSettings();

        m_updating = true;
        m_material->SetSelectedAssetID(settings.m_material);
        m_wholePolygon->setChecked(settings.m_wholePolygon);
        m_updating = false;

        ApplySwatch(settings.m_color);
        m_title->setText(OperationName(settings.m_operation));
        m_hint->setText(OperationHint(settings.m_operation));
        ApplyVisibility(settings.m_operation);
    }
} // namespace WhiteBox

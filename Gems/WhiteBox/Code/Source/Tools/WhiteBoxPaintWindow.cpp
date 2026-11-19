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
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QTimer>
#include <QStringList>
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
            case FacePaintOperation::BlendLayer2: return QObject::tr("Blend Layer 2");
            case FacePaintOperation::BlendLayer3: return QObject::tr("Blend Layer 3");
            case FacePaintOperation::BlendBase: return QObject::tr("Blend Base Layer");
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
            case FacePaintOperation::BlendLayer2:
            case FacePaintOperation::BlendLayer3:
            case FacePaintOperation::BlendBase:
                return QObject::tr(
                    "Brush vertices towards this layer. Shows on faces with a vertex-blend material (StandardMultilayerPBR, "
                    "Blend Source: Vertex Colors), such as materials/WhiteBoxVertexBlend; paint it on with Paint Material. "
                    "Blends run between vertices, so subdivide for finer detail.");
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
        m_material = new QComboBox(this);
        m_material->setToolTip(tr("The material the brush paints, from the White Box card's material list."));
        m_material->setPlaceholderText(tr("Add a material below"));
        controls->addWidget(m_materialLabel, 0, 0);
        controls->addWidget(m_material, 0, 1);
        // Last grid row, so it sits under the combo whenever the colour and brush rows are hidden.
        m_addMaterialLabel = new QLabel(tr("Add to List"), this);
        m_addMaterial = new AzToolsFramework::PropertyAssetCtrl(this);
        m_addMaterial->SetCurrentAssetType(azrtti_typeid<AZ::RPI::MaterialAsset>());
        m_addMaterial->setToolTip(tr("Pick a material to add it to the card's list and paint with it."));
        controls->addWidget(m_addMaterialLabel, 5, 0);
        controls->addWidget(m_addMaterial, 5, 1);

        m_colorLabel = new QLabel(tr("Colour"), this);
        m_color = new QPushButton(this);
        m_color->setMinimumHeight(22);
        m_color->setToolTip(
            tr("Paint an opaque face colour. Custom materials need a base colour property; their textures stay on."));
        controls->addWidget(m_colorLabel, 1, 0);
        controls->addWidget(m_color, 1, 1);

        // Blend brush rows; shown only for the three blend verbs.
        const auto makeSpin = [this](const double minimum, const double maximum, const double step, const QString& suffix)
        {
            auto* spin = new QDoubleSpinBox(this);
            spin->setRange(minimum, maximum);
            spin->setSingleStep(step);
            spin->setDecimals(2);
            spin->setSuffix(suffix);
            spin->setKeyboardTracking(false);
            return spin;
        };
        m_radiusLabel = new QLabel(tr("Radius"), this);
        m_radius = makeSpin(0.01, 100.0, 0.1, tr(" m"));
        m_radius->setToolTip(tr("How far the brush reaches, in metres."));
        m_strengthLabel = new QLabel(tr("Strength"), this);
        m_strength = makeSpin(0.01, 1.0, 0.05, QString());
        m_strength->setToolTip(tr("How far each dab moves the vertices towards the layer; 1 paints it fully at once."));
        m_hardnessLabel = new QLabel(tr("Hardness"), this);
        m_hardness = makeSpin(0.0, 0.99, 0.05, QString());
        m_hardness->setToolTip(tr("0 fades from the centre; higher keeps full strength further out before the edge falls off."));
        controls->addWidget(m_radiusLabel, 2, 0);
        controls->addWidget(m_radius, 2, 1);
        controls->addWidget(m_strengthLabel, 3, 0);
        controls->addWidget(m_strength, 3, 1);
        controls->addWidget(m_hardnessLabel, 4, 0);
        controls->addWidget(m_hardness, 4, 1);

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
            m_material, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](const int row)
            {
                if (!m_updating)
                {
                    ChooseMaterial(row);
                }
            });
        connect(
            m_addMaterial, &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged, this,
            [this](const AZ::Data::AssetId& material)
            {
                if (!m_updating && material.IsValid())
                {
                    // Deferred: clearing the picker from inside its own change signal is asking for trouble.
                    QTimer::singleShot(0, this, [this, material]() { AddMaterial(material); });
                }
            });
        // The card's list can change while the window is open.
        auto* listPoll = new QTimer(this);
        connect(listPoll, &QTimer::timeout, this, &WhiteBoxPaintWindow::RefreshMaterialList);
        listPoll->start(500);
        connect(m_color, &QPushButton::clicked, this, &WhiteBoxPaintWindow::ChooseColor);
        const auto brushSetting = [this](float FacePaintSettings::*member)
        {
            return [this, member](const double value)
            {
                if (m_updating)
                {
                    return;
                }
                if (auto* component = CurrentComponent())
                {
                    auto settings = component->GetFacePaintSettings();
                    settings.*member = static_cast<float>(value);
                    component->SetFacePaintSettings(settings);
                }
            };
        };
        connect(m_radius, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, brushSetting(&FacePaintSettings::m_brushRadius));
        connect(m_strength, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, brushSetting(&FacePaintSettings::m_brushStrength));
        connect(m_hardness, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, brushSetting(&FacePaintSettings::m_brushHardness));
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

    void WhiteBoxPaintWindow::RefreshMaterialList()
    {
        auto* component = CurrentComponent();
        if (component == nullptr)
        {
            return; // RefreshValues dismisses; the poll just waits
        }
        FacePaintSettings settings = component->GetFacePaintSettings();
        // A brush with nothing chosen follows the card's choice, so Assign and Paint start on the same material.
        if (!settings.m_material.IsValid())
        {
            const AZ::Data::AssetId chosen = component->PaletteChoiceAsset();
            if (chosen.IsValid())
            {
                settings.m_material = chosen;
                component->SetFacePaintSettings(settings);
            }
        }
        QStringList labels;
        AZStd::vector<AZ::Data::AssetId> ids;
        for (const auto& [label, id] : component->GetPaletteEntries())
        {
            labels.append(QString::fromUtf8(label.c_str()));
            ids.push_back(id);
        }
        int current = -1;
        for (int i = 0; i < static_cast<int>(ids.size()); ++i)
        {
            if (ids[i] == settings.m_material)
            {
                current = i;
                break;
            }
        }
        if (current < 0 && settings.m_material.IsValid())
        {
            // A material picked before the list existed stays paintable until another is chosen.
            labels.append(tr("%1 (not in list)").arg(QString::fromUtf8(EditorWhiteBoxComponent::MaterialDisplayName(settings.m_material).c_str())));
            ids.push_back(settings.m_material);
            current = static_cast<int>(ids.size()) - 1;
        }
        QStringList shown;
        for (int i = 0; i < m_material->count(); ++i)
        {
            shown.append(m_material->itemText(i));
        }
        m_updating = true;
        if (labels != shown || ids != m_materialIds)
        {
            m_material->clear();
            m_material->addItems(labels);
            m_materialIds = AZStd::move(ids);
        }
        if (m_material->currentIndex() != current)
        {
            m_material->setCurrentIndex(current);
        }
        m_updating = false;
    }

    void WhiteBoxPaintWindow::ChooseMaterial(const int row)
    {
        auto* component = CurrentComponent();
        if (component == nullptr)
        {
            Dismiss();
            return;
        }
        if (row < 0 || row >= static_cast<int>(m_materialIds.size()))
        {
            return;
        }
        auto settings = component->GetFacePaintSettings();
        settings.m_material = m_materialIds[row];
        component->SetFacePaintSettings(settings);
        component->SetPaletteChoice(settings.m_material); // the card's Assign follows the brush
    }

    void WhiteBoxPaintWindow::AddMaterial(const AZ::Data::AssetId& material)
    {
        auto* component = CurrentComponent();
        if (component == nullptr)
        {
            Dismiss();
            return;
        }
        component->AddToMaterialPalette(material);
        auto settings = component->GetFacePaintSettings();
        settings.m_material = material;
        component->SetFacePaintSettings(settings);
        m_updating = true;
        m_addMaterial->SetSelectedAssetID(AZ::Data::AssetId()); // ready for the next one
        m_updating = false;
        RefreshMaterialList();
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
        m_addMaterialLabel->setVisible(material);
        m_addMaterial->setVisible(material);
        m_colorLabel->setVisible(color);
        m_color->setVisible(color);
        const bool blend = IsBlendOperation(operation);
        for (QWidget* widget : std::initializer_list<QWidget*>{ m_radiusLabel, m_radius, m_strengthLabel, m_strength, m_hardnessLabel, m_hardness })
        {
            widget->setVisible(blend);
        }
        m_wholePolygon->setVisible(!blend); // the brush works per vertex, not per face

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
        m_wholePolygon->setChecked(settings.m_wholePolygon);
        m_radius->setValue(settings.m_brushRadius);
        m_strength->setValue(settings.m_brushStrength);
        m_hardness->setValue(settings.m_brushHardness);
        m_updating = false;

        ApplySwatch(settings.m_color);
        m_title->setText(OperationName(settings.m_operation));
        m_hint->setText(OperationHint(settings.m_operation));
        ApplyVisibility(settings.m_operation);
        RefreshMaterialList();
    }
} // namespace WhiteBox

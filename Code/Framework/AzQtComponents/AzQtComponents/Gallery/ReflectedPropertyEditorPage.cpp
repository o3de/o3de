/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectedPropertyEditorPage.h"
#include <AzQtComponents/Gallery/ui_ReflectedPropertyEditorPage.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>

#include <QDebug>
#include <QtMath>
#include <QVBoxLayout>

namespace
{
    static constexpr int g_propertyLabelWidth = 160;
}

void LevelThree::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
    if (serialize)
    {
        serialize->Class<LevelThree, AZ::Component>()
            ->Version(1)
            ->Field("Bool", &LevelThree::m_bool)
            ;

        AZ::EditContext* edit = serialize->GetEditContext();
        if (edit)
        {
            edit->Class<LevelThree>("Level 3", "A deeply nested component")
                ->DataElement(AZ::Edit::UIHandlers::CheckBox, &LevelThree::m_bool, "Horizontal limit", "A deeply nested CheckBox")
                ;
        }
    }
}

void LevelTwo::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
    if (serialize)
    {
        serialize->Class<LevelTwo, AZ::Component>()
            ->Version(1)
            ->Field("Quaternion", &LevelTwo::m_quaternion)
            ->Field("LevelThree", &LevelTwo::m_levelThree)
            ;

        AZ::EditContext* edit = serialize->GetEditContext();
        if (edit)
        {
            edit->Class<LevelTwo>("Level 2", "A deeply nested component")
                ->DataElement(AZ::Edit::UIHandlers::Default, &LevelTwo::m_quaternion, "Quaternion", "An AZ::Edit::UIHandlers::Default example with suffix")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " Deg")
                ->DataElement(AZ::Edit::UIHandlers::Default, &LevelTwo::m_levelThree, "Level 3 - Too cool for school", "A LevelThree component nested in LevelTwo")
                ;
        }
    }
}

void LevelOne::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
    if (serialize)
    {
        serialize->Class<LevelOne, AZ::Component>()
            ->Version(1)
            ->Field("Float", &LevelOne::m_float)
            ->Field("LevelTwo", &LevelOne::m_levelTwo)
            ;

        AZ::EditContext* edit = serialize->GetEditContext();
        if (edit)
        {
            edit->Class<LevelOne>("Level 1", "A deeply nested component")
                ->DataElement(AZ::Edit::UIHandlers::SpinBox, &LevelOne::m_float, "Friction", "A deeply nested SpinBox")
                ->DataElement(AZ::Edit::UIHandlers::Default, &LevelOne::m_levelTwo, "Level 2 - Great", "A LevelTwo component nested in LevelOne")
                ;
        }
    }
}

void GalleryComponent::onIntMultiplierForSliderChanged() const
{
    qDebug() << m_intMultiplierForSlider;
}

void GalleryComponent::onIntMultiplierForSpinBoxChanged() const
{
    qDebug() << m_intMultiplierForSpinBox;
}

void GalleryComponent::onFloatMultiplierForSliderChanged() const
{
    qDebug() << m_floatMultiplierForSlider;
}

void GalleryComponent::onFloatMultiplierForSpinBoxChanged() const
{
    qDebug() << m_floatMultiplierForSpinBox;
}

void GalleryComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
    if (serialize)
    {
        serialize->Class<GalleryComponent, AZ::Component>()
            ->Version(1)
            ->Field("BoolForButton", &GalleryComponent::m_boolForButton)
            ->Field("BoolForCheckBox", &GalleryComponent::m_boolForCheckBox)
            ->Field("Color", &GalleryComponent::m_color)
            ->Field("Mode", &GalleryComponent::m_mode)
            ->Field("BoolForRadioButton", &GalleryComponent::m_boolForRadioButton)
            ->Field("EntityId", &GalleryComponent::m_entityId)
            ->Field("AssetData", &GalleryComponent::m_assetData)
            ->Field("AssetDataWithEditButton", &GalleryComponent::m_assetDataWithEditButton)
            ->Field("FileTagAsset", &GalleryComponent::m_fileTagAsset)
            ->Field("FileTagAssetWithEditButton", &GalleryComponent::m_fileTagAssetWithEditButton)
            ->Field("CustomBrowseIconAsset", &GalleryComponent::m_customBrowseIconAsset)
            ->Field("StringForLineEdit", &GalleryComponent::m_stringForLineEdit)
            ->Field("StringForMultiLineEdit", &GalleryComponent::m_stringForMultiLineEdit)
            ->Field("Quaternion", &GalleryComponent::m_quaternion)
            ->Field("IntForSlider", &GalleryComponent::m_intForSlider)
            ->Field("IntMultiplierForSlider", &GalleryComponent::m_intMultiplierForSlider)
            ->Field("IntMultiplierForSpinBox", &GalleryComponent::m_intMultiplierForSpinBox)
            ->Field("FloatForSlider", &GalleryComponent::m_floatForSlider)
            ->Field("FloatForSliderCurve", &GalleryComponent::m_floatForSliderCurve)
            ->Field("FloatMultiplierForSlider", &GalleryComponent::m_floatMultiplierForSlider)
            ->Field("FloatForSpinBox", &GalleryComponent::m_floatForSpinBox)
            ->Field("FloatMultiplierForSpinBox", &GalleryComponent::m_floatMultiplierForSpinBox)
            ->Field("Crc", &GalleryComponent::m_crc)
            ->Field("Vector2", &GalleryComponent::m_vector2)
            ->Field("Vector3", &GalleryComponent::m_vector3)
            ->Field("Vector4", &GalleryComponent::m_vector4)
            ->Field("Vector4Suffix", &GalleryComponent::m_vector4Suffix)
            ->Field("LevelOne", &GalleryComponent::m_levelOne)
            ->Field("BoolForLongLabel", &GalleryComponent::m_boolForLongLabel)
            ->Field("Strings", &GalleryComponent::m_strings)
            ;
 
        AZ::EditContext* edit = serialize->GetEditContext();
        if (edit)
        {
            edit->Class<GalleryComponent>("Gallery Component", "A component used to demo the ReflectedPropertyEditor.")
                ->DataElement(AZ::Edit::UIHandlers::Button, &GalleryComponent::m_boolForButton, "Button", "An AZ::Edit::UIHandlers::Button example")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Button")
                ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GalleryComponent::m_boolForCheckBox, "CheckBox", "An AZ::Edit::UIHandlers::CheckBox example")
                ->DataElement(AZ::Edit::UIHandlers::Color, &GalleryComponent::m_color, "Color", "An AZ::Edit::UIHandlers::Color example")
                ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GalleryComponent::m_mode, "ComboBox", "An AZ::Edit::UIHandlers::ComboBox example")
                    ->EnumAttribute(GalleryComponent::Mode::Average, "Average")
                    ->EnumAttribute(GalleryComponent::Mode::Minimum, "Minimum")
                    ->EnumAttribute(GalleryComponent::Mode::Maximum, "Maximum")
                    ->EnumAttribute(GalleryComponent::Mode::Multiply, "Multiply")
                ->DataElement(AZ::Edit::UIHandlers::RadioButton, &GalleryComponent::m_boolForRadioButton, "RadioButton", "An AZ::Edit::UIHandlers::RadioButton example")
                ->DataElement(AZ::Edit::UIHandlers::EntityId, &GalleryComponent::m_entityId, "EntityId", "An AZ::Edit::UIHandlers::EntityId example")
                ->DataElement(AZ::Edit::UIHandlers::Default, &GalleryComponent::m_assetData, "AssetData", "An AZ::Edit::UIHandlers::Default example for AZ::Data::Asset<AZ::Data::AssetData>")
                ->DataElement(AZ::Edit::UIHandlers::Default, &GalleryComponent::m_assetDataWithEditButton, "AssetData (edit button)", "An AZ::Edit::UIHandlers::Default example for AZ::Data::Asset<AZ::Data::AssetData> with edit default button attributes")
                    ->Attribute("EditButton", "")
                    ->Attribute("EditDescription", "Edit button description")
                ->DataElement(AZ::Edit::UIHandlers::Default, &GalleryComponent::m_fileTagAsset, "AssetData (registered type)", "An AZ::Edit::UIHandlers::Default example for a registered AZ::Data::Asset<AZ::Data::AssetData> type")
                ->DataElement(AZ::Edit::UIHandlers::Default, &GalleryComponent::m_fileTagAssetWithEditButton, "AssetData (edit button)(registered type)", "An AZ::Edit::UIHandlers::Default example for a registered AZ::Data::Asset<AZ::Data::AssetData> type with custom edit button attributes")
                    ->Attribute("EditButton", ":/Gallery/Rotate.svg")
                    ->Attribute("EditDescription", "Edit button description")
                ->DataElement(AZ::Edit::UIHandlers::Default, &GalleryComponent::m_customBrowseIconAsset, "AssetData (browse icon)(registered type)", "An AZ::Edit::UIHandlers::Default example for a registered AZ::Data::Asset<AZ::Data::AssetData> type with custom browse icon")
                    ->Attribute("BrowseIcon", ":/Gallery/Settings.svg")
                // No AZ::Edit::UIHandlers::LayoutPadding example
                ->DataElement(AZ::Edit::UIHandlers::LineEdit, &GalleryComponent::m_stringForLineEdit, "LineEdit", "An AZ::Edit::UIHandlers::LineEdit example")
                ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &GalleryComponent::m_stringForMultiLineEdit, "MultiLineEdit", "An AZ::Edit::UIHandlers::MultiLineEdit example")
                ->DataElement(AZ::Edit::UIHandlers::Quaternion, &GalleryComponent::m_quaternion, "Quaternion", "An AZ::Edit::UIHandlers::Quaternion example")
                ->DataElement(AZ::Edit::UIHandlers::Slider, &GalleryComponent::m_intForSlider, "Slider (int)", "An AZ::Edit::UIHandlers::Slider int example")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 100)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 0)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 10)
                ->DataElement(AZ::Edit::UIHandlers::Slider, &GalleryComponent::m_intMultiplierForSlider, "Slider with Multiplier (int)", "An AZ::Edit::UIHandlers::Slider int with multiplier example")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 120)
                    ->Attribute("Multiplier", 2)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GalleryComponent::onIntMultiplierForSliderChanged)
                ->DataElement(AZ::Edit::UIHandlers::SpinBox, &GalleryComponent::m_intMultiplierForSpinBox, "SpinBox with Multiplier (int)", "An AZ::Edit::UIHandlers::Slider int with multiplier example")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 120)
                    ->Attribute("Multiplier", 2)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GalleryComponent::onIntMultiplierForSpinBoxChanged)
                ->DataElement(AZ::Edit::UIHandlers::Slider, &GalleryComponent::m_floatForSlider, "Slider (float)", "An AZ::Edit::UIHandlers::Slider float example")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                ->DataElement(AZ::Edit::UIHandlers::Slider, &GalleryComponent::m_floatForSliderCurve, "Slider with curveMidpoint (0.25)", "An AZ::Edit::UIHandlers::Slider float with SliderCurveMidpoint example")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::SliderCurveMidpoint, 0.25)
                ->DataElement(AZ::Edit::UIHandlers::Slider, &GalleryComponent::m_floatMultiplierForSlider, "Slider with Multiplier (float)", "An AZ::Edit::UIHandlers::Slider flot with multiplier example")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 120.0f)
                    ->Attribute("Multiplier", qRadiansToDegrees(1.0f))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GalleryComponent::onFloatMultiplierForSliderChanged)
                ->DataElement(AZ::Edit::UIHandlers::SpinBox, &GalleryComponent::m_floatForSpinBox, "SpinBox", "An AZ::Edit::UIHandlers::SpinBox example")
                ->DataElement(AZ::Edit::UIHandlers::SpinBox, &GalleryComponent::m_floatMultiplierForSpinBox, "SpinBox with Multiplier (float)", "An AZ::Edit::UIHandlers::SpinBox float with multiplier example")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 120.0f)
                    ->Attribute("Multiplier", qRadiansToDegrees(1.0f))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GalleryComponent::onFloatMultiplierForSpinBoxChanged)
                ->DataElement(AZ::Edit::UIHandlers::Crc, &GalleryComponent::m_crc, "Crc", "An AZ::Edit::UIHandlers::Crc example")
                ->DataElement(AZ::Edit::UIHandlers::Vector2, &GalleryComponent::m_vector2, "Vector2", "An AZ::Edit::UIHandlers::Vector2 example")
                ->DataElement(AZ::Edit::UIHandlers::Vector3, &GalleryComponent::m_vector3, "Vector3", "An AZ::Edit::UIHandlers::Vector3 example")
                ->DataElement(AZ::Edit::UIHandlers::Vector4, &GalleryComponent::m_vector4, "Vector4", "An AZ::Edit::UIHandlers::Vector4 example")
                ->DataElement(AZ::Edit::UIHandlers::Vector4, &GalleryComponent::m_vector4Suffix, "Vector4 (suffix)", "An AZ::Edit::UIHandlers::Vector4 with suffix example")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                ->DataElement(AZ::Edit::UIHandlers::Default, &GalleryComponent::m_levelOne, "Level 1", "The first level in deeply nested tree")
                ->ClassElement(AZ::Edit::ClassElements::Group, "Collision")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GalleryComponent::m_boolForLongLabel, "Continuous Collision Detection", "An example of a control with a very long label")
                ->DataElement(AZ::Edit::UIHandlers::Default, &GalleryComponent::m_strings, "Container", "An AZ::Edit::UIHandlers::Default example for a vector of strings")
            ;
        }
    }
}


ReflectedPropertyEditorPage::ReflectedPropertyEditorPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ReflectedPropertyEditorPage)
    , m_component(new GalleryComponent)
    , m_cardComponent(new GalleryComponent)
    , m_disabledCardComponent(new GalleryComponent)
{
    ui->setupUi(this);

    QVBoxLayout* layout = new QVBoxLayout(ui->contentsContainer);

    AZ::SerializeContext* serializeContext = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    if (!serializeContext)
    {
        AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
    }

    // Reflect our Component so that it can be used in the ReflectedPropertyEditor
    LevelThree::Reflect(serializeContext);
    LevelTwo::Reflect(serializeContext);
    LevelOne::Reflect(serializeContext);
    GalleryComponent::Reflect(serializeContext);

    // ReflectedPropertyEditor by itself
    auto propertyEditor = aznew AzToolsFramework::ReflectedPropertyEditor(this);
    propertyEditor->Setup(serializeContext, nullptr/*IPropertyEditorNotify*/, false/*enableScrollbars*/, g_propertyLabelWidth);

    layout->addWidget(propertyEditor);

    propertyEditor->AddInstance(m_component.get(), azrtti_typeid(m_component.get()));
    propertyEditor->InvalidateAll();
    propertyEditor->ExpandAll();

    // ReflectedPropertyEditor by in a Card
    AzQtComponents::Card* card = new AzQtComponents::Card(this);
    card->setTitle(QStringLiteral("Card"));
    card->header()->setIcon(QIcon(QStringLiteral(":/Gallery/Grid-small.svg")));

    auto cardPropertyEditor = aznew AzToolsFramework::ReflectedPropertyEditor(card);
    cardPropertyEditor->Setup(serializeContext, nullptr/*IPropertyEditorNotify*/, false/*enableScrollbars*/, g_propertyLabelWidth);
    cardPropertyEditor->SetHideRootProperties(true);

    cardPropertyEditor->AddInstance(m_cardComponent.get(), azrtti_typeid(m_component.get()));
    cardPropertyEditor->InvalidateAll();
    cardPropertyEditor->ExpandAll();

    card->setContentWidget(cardPropertyEditor);

    layout->addWidget(card);

    // ReflectedPropertyEditor by in a disabled Card
    AzQtComponents::Card* disabledCard = new AzQtComponents::Card(this);
    disabledCard->setTitle(QStringLiteral("Disabled card"));
    disabledCard->setEnabled(false);

    auto disabledCardPropertyEditor = aznew AzToolsFramework::ReflectedPropertyEditor(disabledCard);
    disabledCardPropertyEditor->Setup(serializeContext, nullptr/*IPropertyEditorNotify*/, false/*enableScrollbars*/, g_propertyLabelWidth);
    disabledCardPropertyEditor->SetHideRootProperties(true);

    disabledCardPropertyEditor->AddInstance(m_disabledCardComponent.get(), azrtti_typeid(m_component.get()));
    disabledCardPropertyEditor->InvalidateAll();
    disabledCardPropertyEditor->ExpandAll();

    disabledCard->setContentWidget(disabledCardPropertyEditor);

    layout->addWidget(disabledCard);
    layout->addStretch();

    QString exampleText = R"(
The Reflected Property Editor automatically lays out editable controls for properties reflected to the edit context.
See the documentation linked above for more details.

<pre>
    // Create a new Reflected Property Editor
    auto propertyEditor = aznew AzToolsFramework::ReflectedPropertyEditor(this);

    // RPEs are most commonly used as content widgets for Cards
    AzQtComponents::Card* card = new AzQtComponents::Card(this);
    auto cardPropertyEditor = aznew AzToolsFramework::ReflectedPropertyEditor(card);
    card->setContentWidget(cardPropertyEditor);
</pre>
)";

    ui->exampleText->setHtml(exampleText);
    ui->hyperlinkLabel->setText(QStringLiteral(R"(<a href="https://o3de.org/docs/user-guide/components/development/reflection/">Reflected Property Editor docs</a>)"));
}

ReflectedPropertyEditorPage::~ReflectedPropertyEditorPage()
{
}

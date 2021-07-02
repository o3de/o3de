/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <AzFramework/Asset/FileTagAsset.h>

#include <QWidget>
#include <QScopedPointer>
#endif

namespace AZ
{
    class SerializeContext;
}

namespace Ui
{
    class ReflectedPropertyEditorPage;
}

class LevelThree
    : public AZ::Component
{
public:
    AZ_COMPONENT(LevelThree, "{b912dd49-bfc7-4793-bd29-2e5bba8a98dd}", AZ::Component);

    // AZ::Component interface implementation.
    void Init() override {}
    void Activate() override {}
    void Deactivate() override {}

    // Required Reflect function
    static void Reflect(AZ::ReflectContext* context);

    // Members to reflect
    bool m_bool = false;
};

class LevelTwo
    : public AZ::Component
{
public:
    AZ_COMPONENT(LevelTwo, "{194ad97e-2e9a-4a47-9bba-643b41283d99}", AZ::Component);

    // AZ::Component interface implementation.
    void Init() override {}
    void Activate() override {}
    void Deactivate() override {}

    // Required Reflect function
    static void Reflect(AZ::ReflectContext* context);

    // Members to reflect
    AZ::Quaternion m_quaternion;
    LevelThree m_levelThree;
};

class LevelOne
    : public AZ::Component
{
public:
    AZ_COMPONENT(LevelOne, "{0648f811-77e9-45a6-99c9-caaaf326b2a7}", AZ::Component);

    // AZ::Component interface implementation.
    void Init() override {}
    void Activate() override {}
    void Deactivate() override {}

    // Required Reflect function
    static void Reflect(AZ::ReflectContext* context);

    // Members to reflect
    float m_float = 0;
    LevelTwo m_levelTwo;
};

class GalleryComponent
    : public AZ::Component
{
public:
    AZ_COMPONENT(GalleryComponent, "{84656cb6-1148-482b-b28b-7779a259c792}", AZ::Component);

    enum class Mode : AZ::u8
        {
            Average,
            Minimum,
            Maximum,
            Multiply
        };

    // AZ::Component interface implementation.
    void Init() override {}
    void Activate() override {}
    void Deactivate() override {}

    // Required Reflect function
    static void Reflect(AZ::ReflectContext* context);

    // Callback functions
    void onIntMultiplierForSliderChanged() const;
    void onIntMultiplierForSpinBoxChanged() const;
    void onFloatMultiplierForSliderChanged() const;
    void onFloatMultiplierForSpinBoxChanged() const;

    // Members to reflect
    bool m_boolForButton = false;
    bool m_boolForCheckBox = false;
    AZ::Color m_color = AZ::Color(0.0f, 0.467f, 0.784f, 1.0f);
    Mode m_mode = Mode::Average;
    bool m_boolForRadioButton = false;
    AZ::EntityId m_entityId;
    AZ::Data::Asset<AZ::Data::AssetData> m_assetData;
    AZ::Data::Asset<AZ::Data::AssetData> m_assetDataWithEditButton;
    AZ::Data::Asset<AzFramework::FileTag::FileTagAsset> m_fileTagAsset;
    AZ::Data::Asset<AzFramework::FileTag::FileTagAsset> m_fileTagAssetWithEditButton;
    AZ::Data::Asset<AzFramework::FileTag::FileTagAsset> m_customBrowseIconAsset;
    AZStd::string m_stringForLineEdit;
    AZStd::string m_stringForMultiLineEdit;
    AZ::Quaternion m_quaternion;
    int m_intForSlider = 0;
    int m_intMultiplierForSlider = 0;
    int m_intMultiplierForSpinBox = 0;
    float m_floatForSlider = 0;
    float m_floatForSliderCurve = 0;
    float m_floatMultiplierForSlider = 0;
    float m_floatForSpinBox = 0;
    float m_floatMultiplierForSpinBox = 0;
    AZ::u32 m_crc;
    AZ::Vector2 m_vector2;
    AZ::Vector3 m_vector3;
    AZ::Vector4 m_vector4;
    AZ::Vector4 m_vector4Suffix;
    bool m_boolForLongLabel = true;
    LevelOne m_levelOne;
    AZStd::vector<AZStd::string> m_strings = {"String 1", "String 2"};
};

class ReflectedPropertyEditorPage : public QWidget
{
    Q_OBJECT // AUTOMOC
public:
    explicit ReflectedPropertyEditorPage(QWidget* parent = nullptr);
    ~ReflectedPropertyEditorPage() override;

private:
    QScopedPointer<Ui::ReflectedPropertyEditorPage> ui;

    QScopedPointer<GalleryComponent> m_component;
    QScopedPointer<GalleryComponent> m_cardComponent;
    QScopedPointer<GalleryComponent> m_disabledCardComponent;
};



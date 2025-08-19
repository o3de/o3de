/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <QtWidgets/QComboBox>
#include <OpenParticleSystem/Serializer/ParticleSourceData.h>
#include <ParticleCommonData.h>
#include <Window/AssetWidget.h>
#include <QVBoxLayout>
#include <QCoreApplication>
#endif

namespace OpenParticleSystemEditor
{
    constexpr AZStd::string_view MATERIAL_EXTENTION = "material";
    constexpr AZStd::string_view MODEL_EXTENTION = "azmodel";

    const QString MATERIAL_DESCRIPTION = QCoreApplication::translate("ComboBoxWidget", "Material");
    const QString MESH_DESCRIPTION = QCoreApplication::translate("ComboBoxWidget", "Mesh");
    const QString SHAPE_LABEL = QCoreApplication::translate("ComboBoxWidget", "Shape");
    const QString RENDERER_LABEL = QCoreApplication::translate("ComboBoxWidget", "Renderer");

    using AssetChangeCB = AZStd::function<void(const char*)>;
    class ComboBoxWidget
        : public QWidget
    {
        Q_OBJECT;

    public:
        ComboBoxWidget(
            const AZStd::string& className,
            OpenParticle::ParticleSourceData::DetailInfo* m_detail,
            AZ::SerializeContext* serializeContext,
            AzToolsFramework::IPropertyEditorNotify* pnotify,
            QWidget* parent = nullptr);
        ~ComboBoxWidget() override {};

        void Clear();
        void Refresh();
        void Refresh(AZStd::any* instance);
        void SetAssetWidget(AZStd::string& materialAsset, AZStd::string& modelAsset, AZStd::string& skeletonModelAsset);

    private slots:
        void OnIndexChanged(const QString& curString);

    private:
        void SetAssetWidgetVisible();
        void InitComboBox();
        void SetUI();

        AZStd::string m_className;
        AZStd::string m_moduleName;
        AZStd::string m_lastModuleName;
        AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
        QLabel* m_nameLabel;
        QComboBox* m_comboBox;
        QVBoxLayout* m_layout;
        QWidget* m_parent;
        AssetWidget* m_materialAssetWidget = nullptr;
        AssetWidget* m_modelAssetWidget = nullptr;
        AssetWidget* m_skeletonModelAssetWidget = nullptr;
        AZStd::vector<AZStd::pair<AZStd::string, QString>> m_locationClasses;
        AZStd::vector<AZStd::pair<AZStd::string, QString>> m_rendererClasses;
    };
} // namespace OpenParticleSystemEditor

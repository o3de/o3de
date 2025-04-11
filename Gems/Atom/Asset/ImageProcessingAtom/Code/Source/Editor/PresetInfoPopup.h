/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/StyledDialog.h>
#include <Source/BuilderSettings/BuilderSettingManager.h>
#endif

namespace ImageProcessingAtom
{
    class PresetSettings;
}
namespace Ui
{
    class PresetInfoPopup;
}

namespace ImageProcessingAtomEditor
{
    class PresetInfoPopup
        : public AzQtComponents::StyledDialog
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PresetInfoPopup, AZ::SystemAllocator);
        explicit PresetInfoPopup(const ImageProcessingAtom::PresetSettings* preset, QWidget* parent = nullptr);
        ~PresetInfoPopup();
        void RefreshPresetInfoLabel(const ImageProcessingAtom::PresetSettings* presetSettings);
    private:
        QScopedPointer<Ui::PresetInfoPopup> m_ui;
    };
} //namespace ImageProcessingAtomEditor


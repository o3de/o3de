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

#pragma once


#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/StyledDialog.h>
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
        AZ_CLASS_ALLOCATOR(PresetInfoPopup, AZ::SystemAllocator, 0);
        explicit PresetInfoPopup(const ImageProcessingAtom::PresetSettings* preset, QWidget* parent = nullptr);
        ~PresetInfoPopup();
        void RefreshPresetInfoLabel(const ImageProcessingAtom::PresetSettings* presetSettings);
    private:
        QScopedPointer<Ui::PresetInfoPopup> m_ui;
    };
} //namespace ImageProcessingAtomEditor


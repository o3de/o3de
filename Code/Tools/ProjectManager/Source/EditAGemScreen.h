/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <CreateAGemScreen.h>

QT_FORWARD_DECLARE_CLASS(QDir)
#endif

namespace O3DE::ProjectManager
{
    class EditGem : public CreateGem
    {
        Q_OBJECT
    public:
        explicit EditGem(QWidget* parent = nullptr);
        ~EditGem() = default;

        void ResetWorkflow(const GemInfo& oldGemInfo);

        ProjectManagerScreen GetScreenEnum() override
        {
            return ProjectManagerScreen::EditGem;
        }

        void Init() override;

    signals:
        void GemEdited(const GemInfo& newGemInfo);
        
    private: 
        void GemAction() override;

        bool ValidateGemLocation(const QDir& chosenGemLocation) const override;

        QString m_oldGemName;

    };


} // namespace O3DE::ProjectManager

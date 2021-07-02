/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProjectSettingsTool_precompiled.h"

#include "ValidationHandler.h"

#include "PropertyFuncValLineEdit.h"
#include "PropertyFuncValBrowseEdit.h"

namespace ProjectSettingsTool
{
    void ValidationHandler::AddValidatorCtrl(PropertyFuncValLineEditCtrl* ctrl)
    {
        m_validators.push_back(ctrl);
    }

    void ValidationHandler::AddValidatorCtrl(PropertyFuncValBrowseEditCtrl* ctrl)
    {
        m_browseEditValidators.push_back(ctrl);
    }

    bool ValidationHandler::AllValid()
    {
        for (PropertyFuncValLineEditCtrl* ctrl : m_validators)
        {
            if (!ctrl->ValidateAndShowErrors())
            {
                return false;
            }
        }

        for (PropertyFuncValBrowseEditCtrl* ctrl : m_browseEditValidators)
        {
            if (!ctrl->ValidateAndShowErrors())
            {
                return false;
            }
        }

        return true;
    }
} // namespace ProjectSettingsTool

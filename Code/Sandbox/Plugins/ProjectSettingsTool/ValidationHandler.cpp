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

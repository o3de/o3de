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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "UiAnimViewKeyPropertiesDlg.h"

class CUiAnimViewTrackEventKeyUIControls
    : public CUiAnimViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariableEnum<QString> mv_event;
    CSmartVariable<QString> mv_value;

    virtual void OnCreateVars();
    bool SupportTrackType(const CUiAnimParamType& paramType, EUiAnimCurveType trackType, EUiAnimValue valueType) const override;

    bool OnKeySelectionChange(CUiAnimViewKeyBundle& selectedKeys) override;
    void OnUIChange(IVariable* pVar, CUiAnimViewKeyBundle& keys) override;

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        // {BBF52016-4935-4465-AEA6-62518D0EA499}
        static const GUID guid =
        {
            0xbbf52016, 0x4935, 0x4465, { 0xae, 0xa6, 0x62, 0x51, 0x8d, 0x0e, 0xa4, 0x99 }
        };
        return guid;
    }

private:
    void OnEventEdit();
    void BuildEventDropDown(QString& curEvent, const QString& addedEvent = "");

    QString m_lastEvent;

    static const char* GetAddEventString()
    {
        static const char* addEventString = "Add a new event...";

        return addEventString;
    }
};

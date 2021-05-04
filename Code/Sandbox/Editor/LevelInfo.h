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

#ifndef CRYINCLUDE_EDITOR_LEVELINFO_H
#define CRYINCLUDE_EDITOR_LEVELINFO_H
#pragma once

/*! CLevelInfo provides methods for getting information about current level.
 */
class CLevelInfo
{
public:
    CLevelInfo();
    void Validate();

    void SaveLevelResources(const QString& toPath);

private:
    void ValidateObjects();

    IErrorReport* m_pReport;
};

#endif // CRYINCLUDE_EDITOR_LEVELINFO_H

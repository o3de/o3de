/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

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

#ifndef CRYINCLUDE_EDITOR_INCLUDE_IKEYTIMESET_H
#define CRYINCLUDE_EDITOR_INCLUDE_IKEYTIMESET_H
#pragma once


class IKeyTimeSet
{
public:
    virtual int GetKeyTimeCount() const = 0;
    virtual float GetKeyTime(int index) const = 0;
    virtual void MoveKeyTimes(int numChanges, int* indices, float scale, float offset, bool copyKeys) = 0;
    virtual bool GetKeyTimeSelected(int index) const = 0;
    virtual void SetKeyTimeSelected(int index, bool selected) = 0;
    virtual int GetKeyCount(int index) const = 0;
    virtual int GetKeyCountBound() const = 0;
    virtual void BeginEdittingKeyTimes() = 0;
    virtual void EndEdittingKeyTimes() = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IKEYTIMESET_H

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

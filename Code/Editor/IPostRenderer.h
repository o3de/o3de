/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_IPOSTRENDERER_H
#define CRYINCLUDE_EDITOR_IPOSTRENDERER_H
#pragma once


class IPostRenderer
{
public:
    IPostRenderer()
        : m_refCount(0){}

    virtual void OnPostRender() const = 0;

    void AddRef() { m_refCount++; }
    void Release()
    {
        if (--m_refCount <= 0)
        {
            delete this;
        }
    }

protected:
    virtual ~IPostRenderer(){}

    int m_refCount;
};

#endif // CRYINCLUDE_EDITOR_IPOSTRENDERER_H

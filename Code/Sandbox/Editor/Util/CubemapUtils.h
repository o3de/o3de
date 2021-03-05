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

#ifndef CRYINCLUDE_EDITOR_UTIL_CUBEMAPUTILS_H
#define CRYINCLUDE_EDITOR_UTIL_CUBEMAPUTILS_H
#pragma once


namespace CubemapUtils
{
    //! Generate a cubemap
    //! \param filename
    //! \param pObject     The cubemap will be generated at this object's location
    //! \param size        Texel dimension of the cubemap
    //! \param hideObject  If true, pObject will be hidden when rendering the cubemap. For example, set this to true if pObject is a model that shouldn't
    //!                    show up in the cubemap, or set to false if pObject is a light or probe that should contribute to the cubemap.
    SANDBOX_API bool GenCubemapWithObjectPathAndSize(QString& filename, CBaseObject* pObject, const int size, const bool hideObject);

    SANDBOX_API bool GenHDRCubemapTiff(const QString& fileName, int size, Vec3& pos);
    SANDBOX_API void RegenerateAllEnvironmentProbeCubemaps();
}

#endif // CRYINCLUDE_EDITOR_UTIL_CUBEMAPUTILS_H

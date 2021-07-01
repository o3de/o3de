/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_AFFINEPARTS_H
#define CRYINCLUDE_EDITOR_UTIL_AFFINEPARTS_H
#pragma once


struct AffineParts
{
    Vec3 pos;               //!< Translation components
    Quat rot;           //!< Essential rotation.
    Quat rotScale;  //!< Stretch rotation.
    Vec3 scale;         //!< Stretch factors.
    float fDet;         //!< Sign of determinant.

    /** Decompose matrix to its affnie parts.
    */
    void Decompose(const Matrix34& mat);

    /** Decompose matrix to its affnie parts.
            Assume there`s no stretch rotation.
    */
    void SpectralDecompose(const Matrix34& mat);
};

#endif // CRYINCLUDE_EDITOR_UTIL_AFFINEPARTS_H

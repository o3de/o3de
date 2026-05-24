/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    AZ::Quaternion rot;      //!< Essential rotation.
    AZ::Quaternion rotScale; //!< Stretch rotation.
    Vec3 scale;         //!< Stretch factors.
    float fDet;         //!< Sign of determinant.

    /** Decompose matrix to its affine parts.
    */
    void Decompose(const AZ::Matrix3x4& mat);

    /** Decompose matrix to its affine parts.
            Assume there`s no stretch rotation.
    */
    void SpectralDecompose(const AZ::Matrix3x4& mat);
};

#endif // CRYINCLUDE_EDITOR_UTIL_AFFINEPARTS_H

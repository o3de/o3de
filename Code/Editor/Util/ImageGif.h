/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_IMAGEGIF_H
#define CRYINCLUDE_EDITOR_UTIL_IMAGEGIF_H
#pragma once

class CImageEx;

class CImageGif
{
public:
    bool Load(const QString& fileName, CImageEx& outImage);
};


#endif // CRYINCLUDE_EDITOR_UTIL_IMAGEGIF_H

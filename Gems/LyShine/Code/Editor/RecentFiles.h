/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

QStringList ReadRecentFiles();
void WriteRecentFiles(const QStringList& filenames);
void AddRecentFile(const QString& filename);
void ClearRecentFile();

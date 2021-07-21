/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

class SlicerView
    : public QGraphicsView
{
public:

    SlicerView(QGraphicsScene* scene, QWidget* parent = nullptr);

protected:

    // This is intentionally empty.
    void scrollContentsBy([[maybe_unused]] int dx, [[maybe_unused]] int dy) override {};
};

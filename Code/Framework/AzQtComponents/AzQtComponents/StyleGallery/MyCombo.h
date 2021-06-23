/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef MY_COMBO_H
#define MY_COMBO_H

#if !defined(Q_MOC_RUN)
#include <QComboBox>
#endif

class MyComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit MyComboBox(QWidget *parent = nullptr);
};


#endif

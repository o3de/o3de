/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "aztestrunner.h"

#include <iostream>

int main(int argc, char** argv)
{
    int result = AzTestRunner::wrapped_main(argc, argv);
    std::cout << "Returning code: " << result << std::endl;
    return result;
}

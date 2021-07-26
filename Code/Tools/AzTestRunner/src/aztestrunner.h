/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


namespace AzTestRunner
{

    // Set the console output stream to stream to null in order to suppress output from stdout
    void set_quiet_mode();

    // Get the current working directory
    const char* get_current_working_directory();

    // Pause the execution upon completion where supported
    void pause_on_completion();

    // Execute the AZ test runner logic with arguments collected from the command line on platforms
    // that support it. If the current platform does not support command line arguments, set argc
    // to 0.
    int wrapped_main(int argc, char** argv);
}

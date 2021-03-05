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

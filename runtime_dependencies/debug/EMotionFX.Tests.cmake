#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

cmake_policy(SET CMP0012 NEW) # new policy for the if that evaluates a boolean out of "if(NOT ${same_location})"

function(ly_copy source_file target_directory)
    get_filename_component(target_filename "${source_file}" NAME)
    cmake_path(COMPARE "${source_file}" EQUAL "${target_directory}/${target_filename}" same_location)
    if(NOT ${same_location})
        file(LOCK ${target_directory}/${target_filename}.lock GUARD FUNCTION TIMEOUT 300)
        if("${source_file}" IS_NEWER_THAN "${target_directory}/${target_filename}")
            message(STATUS "Copying \"${source_file}\" to \"${target_directory}\"...")
            file(COPY "${source_file}" DESTINATION "${target_directory}" FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE FOLLOW_SYMLINK_CHAIN)
            file(TOUCH_NOCREATE ${target_directory}/${target_filename})
        endif()
    endif()    
endfunction()

ly_copy("C:/Users/lesaelr/o3de/bin/debug/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/debug")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/AnimGraphExample.animgraph" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/AnimGraphExampleNoDependency.animgraph" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/EmptyAnimGraphExample.animgraph" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/EmptyMotionSetExample.motionset" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/MotionSetExample.motionset" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/MotionSetExampleNoDependency.motionset" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.actor" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.animgraph" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.emfxrecording" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.motionset" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_come_to_stop.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_forward_dive_roll.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_jump.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_readyattack_idle.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_run.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_run_turn_left.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_run_turn_right.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_shuffle_turn_left.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_shuffle_turn_right.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_stand_kick_punch_01.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_stand_kick_punch_02.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_stand_kick_punch_03.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_stand_kick_punch_04.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_stand_kick_punch_05.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_stand_kick_punch_06.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_turn_180.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_turn_180_clockwise.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_walk.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_walk_kick_01.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_walk_kick_02.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_walk_kick_03.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_walk_kick_04.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_walk_turn_left.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_walk_turn_right.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum/pendulum.actor" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum/pendulum.animgraph" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum/pendulum.emfxrecording" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum/pendulum.motion" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum")
ly_copy("C:/Users/lesaelr/o3de/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum/pendulum.motionset" "C:/Users/lesaelr/o3de/bin/debug/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Pendulum")


file(TOUCH C:/Users/lesaelr/o3de/runtime_dependencies/debug/EMotionFX.Tests_debug.stamp)

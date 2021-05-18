if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::SmokeTest
        TEST_SUITE smoke
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            AZ::AssetProcessor
            AZ::PythonBindingsExample
            Legacy::Editor
            AutomatedTesting.GameLauncher
            AutomatedTesting.Assets
        COMPONENT
            Smoke
    )
endif()
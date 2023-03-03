if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::MetadataRelocation_Main
        TEST_SUITE main
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main.py
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            MetadataRelocation
    )
endif()
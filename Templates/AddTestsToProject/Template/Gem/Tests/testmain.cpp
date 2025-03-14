 #include <AzTest/AzTest.h>

 TEST(MyTestSuiteName, ExampleTest)
 {
     ASSERT_TRUE(true);
 }

 // If you need to have your own environment preconditions
 // you can replace the test env with your own.
 AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
 
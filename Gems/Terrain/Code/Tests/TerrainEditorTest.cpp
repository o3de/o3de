
#include <AzTest/AzTest.h>

class TerrainEditorTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

TEST_F(TerrainEditorTest, SanityTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

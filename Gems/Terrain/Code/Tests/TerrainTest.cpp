
#include <AzTest/AzTest.h>

class TerrainTest
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

TEST_F(TerrainTest, SanityTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

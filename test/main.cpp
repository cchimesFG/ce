#define CE_USER_CHECKED
#include "ce/ce.h"

#include "ce/atomic.h"
#include "ce/cdt.h"
#include "ce/dictionary.h"
#include "ce/io.h"
#include "ce/lziii.h"
#include "ce/math.h"
#include "ce/mutex.h"
#include "ce/pool.h"
#include "ce/sort.h"
#include "ce/zorder.h"

#include "gtest/gtest.h"

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
}

enum class colors
{
    red,
    green,
    blue
};

GTEST_TEST(ce, main)
{
    int a = 4;

    CE_LOG(0);
    CE_LOG(info, "Hello World", a, a * a, a / a, a + a, a - a, a + 1, a + 2, a + 3, a + 4, a + 5);
    CE_LOG(snowball, "Hello Snowball");

    CE_LOG(to_text, char('A'), bool(true));
    CE_LOG(to_text, int8_t(-1), int16_t(-1), int32_t(-1), int64_t(-1));
    CE_LOG(to_text, uint8_t(-1), uint16_t(-1), uint32_t(-1), uint64_t(-1));
    CE_LOG(to_text, (signed char)-1, (short)-1, (int)-1, (long)-1, (long long)-1);
    CE_LOG(to_text, (unsigned char)-1, (unsigned short)-1, (unsigned int)-1, (unsigned long)-1, (unsigned long long)-1);
    CE_LOG(to_text, float(10.5), double(100.5));
    CE_LOG(to_text, colors::red, colors::green, colors::blue);

    int* ip = nullptr;// &a;

    ce::vec2<int> v2{ 0, 1 };
    ce::vec3<int> v3{ 0, 1, 2 };

    CE_LOG(text, v2);

    struct mine { };

    CE_LOG(to_text, nullptr, ip, v2, v3, mine{ });

    CE_LOG_MSG(messages, "Hello", ", ", "Earth ", a);


    CE_LOG_EX(7, "1", 1, 2);

    CE_LOG_MSG(info, "hello = ", 42);
}

GTEST_TEST(ce, bulk_simple)
{
    ce::bulk<3, int> b;

    GTEST_EXPECT_TRUE(b.size == 0);

    GTEST_EXPECT_TRUE(b.append(1));
    GTEST_EXPECT_TRUE(b.append(2));
    GTEST_EXPECT_TRUE(b.size == 2);

    GTEST_EXPECT_FALSE(b.append_n(10));
    GTEST_EXPECT_TRUE(b.size == 2);

    GTEST_EXPECT_TRUE(b.append(3));
    GTEST_EXPECT_TRUE(b.size == 3);

    GTEST_EXPECT_FALSE(b.append(4));
    GTEST_EXPECT_TRUE(b.size == 3);
}

namespace test
{
    struct stats_t
    {
        int destroy;
        int construct;

        int copy_construct;
        int move_construct;
        int copy_assign;
        int move_assign;

        void clear()
        {
            destroy = 0;
            construct = 0;

            copy_construct = 0;
            move_construct = 0;
            copy_assign = 0;
            move_assign = 0;
        }

        bool expect(int d, int c, int cc, int mc, int ca, int ma)
        {
            EXPECT_EQ(destroy, d);
            EXPECT_EQ(construct, c);

            EXPECT_EQ(copy_construct, cc);
            EXPECT_EQ(move_construct, mc);
            EXPECT_EQ(copy_assign, ca);
            EXPECT_EQ(move_assign, ma);

            return destroy == d
                && construct == c
                && copy_construct == cc
                && move_construct == mc
                && copy_assign == ca
                && move_assign == ma;
        }
    };

    struct big
    {
        static stats_t stats;

        ~big() { ++stats.destroy; }
        big() { ++stats.construct; }
        big(big const&) { ++stats.copy_construct; }
        big(big&&) { ++stats.move_construct; }
        big& operator=(big const&) { ++stats.copy_assign; return *this; }
        big& operator=(big&&) noexcept { ++stats.move_assign; return *this; }
    };

    stats_t big::stats{ };
}

GTEST_TEST(ce, bulk_complex)
{
    test::big::stats.clear();

    {
        ce::bulk<3, test::big> b;

        GTEST_EXPECT_TRUE(b.size == 0);
        GTEST_EXPECT_TRUE(test::big::stats.expect(0, 0, 0, 0, 0, 0));

        GTEST_EXPECT_TRUE(b.append());
        GTEST_EXPECT_TRUE(test::big::stats.expect(0, 1, 0, 0, 0, 0));

        GTEST_EXPECT_TRUE(b.append());
        GTEST_EXPECT_TRUE(b.size == 2);
        GTEST_EXPECT_TRUE(test::big::stats.expect(0, 2, 0, 0, 0, 0));

        GTEST_EXPECT_FALSE(b.append_n(10));
        GTEST_EXPECT_TRUE(b.size == 2);
        GTEST_EXPECT_TRUE(test::big::stats.expect(0, 2, 0, 0, 0, 0));

        GTEST_EXPECT_TRUE(b.append());
        GTEST_EXPECT_TRUE(b.size == 3);
        GTEST_EXPECT_TRUE(test::big::stats.expect(0, 3, 0, 0, 0, 0));

        GTEST_EXPECT_FALSE(b.append());
        GTEST_EXPECT_TRUE(b.size == 3);
        GTEST_EXPECT_TRUE(test::big::stats.expect(0, 3, 0, 0, 0, 0));

        GTEST_EXPECT_TRUE(b.remove_at(0));
        GTEST_EXPECT_TRUE(b.size == 2);
        GTEST_EXPECT_TRUE(test::big::stats.expect(0, 3, 0, 0, 0, 1));

        b.clear();
        GTEST_EXPECT_TRUE(b.size == 0);
        GTEST_EXPECT_TRUE(test::big::stats.expect(2, 3, 0, 0, 0, 1));

        b.append_n(3);
        GTEST_EXPECT_TRUE(b.size == 3);
        GTEST_EXPECT_TRUE(test::big::stats.expect(2, 6, 0, 0, 0, 1));
    }
    GTEST_EXPECT_TRUE(test::big::stats.expect(5, 6, 0, 0, 0, 1));
}
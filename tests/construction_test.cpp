#include <gtest/gtest.h>
#include <scheduler_types.h>
#include <scheduler.h>

TEST(constructionTest , workerThreadGenerationCount){
    cadence::WorkerCount count = 20;
    cadence::scheduler sch(count);

    sch.start();

    EXPECT_EQ(count,sch.get_worker_count());
    EXPECT_EQ(count,sch.get_thread_count());

}

TEST(constructionTest , multipleStartCalls){
    cadence::WorkerCount count = 20;
    cadence::scheduler sch(count);

    sch.start();
    sch.start();
    EXPECT_TRUE(sch.is_running());
    sch.start();
    EXPECT_TRUE(sch.is_running());

    EXPECT_EQ(count,sch.get_worker_count());
    EXPECT_EQ(count,sch.get_thread_count());
}


TEST(constructionTest , startStop){
    cadence::WorkerCount count = 20;
    cadence::scheduler sch(count);

    sch.start();
    EXPECT_EQ(count,sch.get_worker_count());
    EXPECT_EQ(count,sch.get_thread_count());

    sch.stop();
    EXPECT_EQ(0,sch.get_worker_count());
    EXPECT_EQ(0,sch.get_thread_count());
}
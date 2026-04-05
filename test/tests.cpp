// Copyright 2021 GHA Test Team
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include "TimedDoor.h"

class MockTimerClient : public TimerClient {
 public:
    MOCK_METHOD(void, Timeout, (), (override));
};

class MockDoor : public Door {
 public:
    MOCK_METHOD(void, lock, (), (override));
    MOCK_METHOD(void, unlock, (), (override));
    MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class TimedDoorTest : public ::testing::Test {
 protected:
    void SetUp() override {
        door = new TimedDoor(2);
    }

    void TearDown() override {
        delete door;
    }

    TimedDoor* door;
};

class DoorTimerAdapterTest : public ::testing::Test {
 protected:
    void SetUp() override {
        timedDoor = new TimedDoor(2);
        adapter = new DoorTimerAdapter(*timedDoor);
    }

    void TearDown() override {
        delete adapter;
        delete timedDoor;
    }

    TimedDoor* timedDoor;
    DoorTimerAdapter* adapter;
};

TEST_F(TimedDoorTest, ConstructorInitializesCorrectly) {
    EXPECT_FALSE(door->isDoorOpened());
    EXPECT_EQ(door->getTimeOut(), 2);
}

TEST_F(TimedDoorTest, LockAndUnlockWorkCorrectly) {
    door->unlock();
    EXPECT_TRUE(door->isDoorOpened());

    door->lock();
    EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, GetTimeOutReturnsCorrectValue) {
    EXPECT_EQ(door->getTimeOut(), 2);
}

TEST_F(TimedDoorTest, ThrowStateThrowsExceptionWhenDoorOpened) {
    door->unlock();
    EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST_F(TimedDoorTest, ThrowStateAlwaysThrowsException) {
    door->lock();
    EXPECT_THROW(door->throwState(), std::runtime_error);

    door->unlock();
    EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST_F(DoorTimerAdapterTest, TimeoutThrowsExceptionWhenDoorOpened) {
    timedDoor->unlock();
    EXPECT_THROW(adapter->Timeout(), std::runtime_error);
}

TEST_F(DoorTimerAdapterTest, TimeoutDoesNotThrowWhenDoorClosed) {
    timedDoor->lock();
    EXPECT_NO_THROW(adapter->Timeout());
}

TEST(TimerTest, TregisterCallsTimeoutAfterDelay) {
    MockTimerClient mockClient;
    Timer timer;

    EXPECT_CALL(mockClient, Timeout())
        .Times(1);

    std::thread timerThread([&timer, &mockClient]() {
        timer.tregister(1, &mockClient);
    });

    timerThread.join();
}

TEST(IntegrationTest, DoorOpenedAndTimerCausesException) {
    TimedDoor door(1);
    DoorTimerAdapter adapter(door);
    Timer timer;

    door.unlock();
    EXPECT_TRUE(door.isDoorOpened());

    EXPECT_THROW(timer.tregister(1, &adapter), std::runtime_error);
}

TEST(IntegrationTest, DoorClosedBeforeTimeoutNoException) {
    TimedDoor door(1);
    DoorTimerAdapter adapter(door);
    Timer timer;

    door.unlock();
    EXPECT_TRUE(door.isDoorOpened());

    std::thread closeDoor([&door]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        door.lock();
    });

    EXPECT_NO_THROW(timer.tregister(1, &adapter));
    closeDoor.join();
}

TEST_F(TimedDoorTest, MultipleLockUnlockCyclesWork) {
    for (int i = 0; i < 5; i++) {
        door->unlock();
        EXPECT_TRUE(door->isDoorOpened());
        door->lock();
        EXPECT_FALSE(door->isDoorOpened());
    }
}

TEST(TimedDoorTimeoutTest, DifferentTimeoutValues) {
    TimedDoor door1(5);
    TimedDoor door2(10);
    TimedDoor door3(0);

    EXPECT_EQ(door1.getTimeOut(), 5);
    EXPECT_EQ(door2.getTimeOut(), 10);
    EXPECT_EQ(door3.getTimeOut(), 0);
}

TEST(TimedDoorNegativeTimeoutTest, NegativeTimeoutValue) {
    TimedDoor door(-1);
    EXPECT_EQ(door.getTimeOut(), -1);

    door.unlock();
    EXPECT_TRUE(door.isDoorOpened());
    door.lock();
    EXPECT_FALSE(door.isDoorOpened());
}

TEST(IntegrationTest, OpenTimerCloseTimerSequence) {
    TimedDoor door(1);
    DoorTimerAdapter adapter(door);
    Timer timer;

    door.unlock();
    EXPECT_THROW(timer.tregister(1, &adapter), std::runtime_error);

    door.lock();

    door.unlock();
    EXPECT_THROW(timer.tregister(1, &adapter), std::runtime_error);
}

#include "circular_buffer.hpp"
#include "test_unit.hpp"
#include <iostream>
namespace {

using base::CircularBuffer;

TEST(Simple, ReadWrite1) {
  CircularBuffer b(1);
  b.write(0);
  EXPECT_EQ(0, b.read());
}

TEST(Simple, ReadWrite2) {
  CircularBuffer b(2);
  b.write(0);
  b.write(1);
  b.read();
  EXPECT_EQ(1, b.read());
}

TEST(Simple, Clear) {
  CircularBuffer b(2);
  b.write(0);
  b.write(1);
  b.clear();
  EXPECT_EQ(-1, b.read());
}

TEST(Complex, ReadWrite1) {
  CircularBuffer b(3);
  int i;
  for (i=0; i<9; i++) {
    b.write(i);
  }
  b.write(i);
  EXPECT_EQ(7, b.read());
  EXPECT_EQ(8, b.read());
}

TEST(Complex, ReadWrite2) {
  CircularBuffer b(2);
  b.write(0);
  b.write(1);
  b.read();
  EXPECT_EQ(1, b.read());
  EXPECT_EQ(-1, b.read());
}


TEST(Complex, ReadWrite3) {
  CircularBuffer b(20);
  for (int i=0; i<30; i++) {
    b.write(i);
    EXPECT_EQ(i, b.read());
  }
  for (int i=0; i<50; i++) {
    b.write(i);
  }
  for (int i=30; i<40; i++) {
    EXPECT_EQ(i, b.read());
  }
}

TEST(Complex, ReadWriteClear1) {
  CircularBuffer b(3);
  int i;
  for (i=0; i<3; i++) {
    b.write(i);
  }
  b.clear();
  b.write(i);
  EXPECT_EQ(3, b.read());
  EXPECT_EQ(-1, b.read());
}

TEST(Complex, ReadWriteClear2) {
  CircularBuffer b(3);
  b.write(0);
  b.write(1);
  b.clear();
  b.write(0);
  b.write(1);
  EXPECT_EQ(0, b.read());
  b.clear();
  b.write(2);
  b.write(3);
  EXPECT_EQ(2, b.read());
}


TEST(Exception, constructor1) {
  CircularBuffer b(0);
  b.write(0);
  EXPECT_EQ(0, b.read());
}

TEST(Exception, constructor2) {
  CircularBuffer b(-1);
  int i;
  for (i=0; i<10; i++) {
    b.write(i);
  }
  b.write(i);
  EXPECT_EQ(1, b.read());
}


} // unnamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}

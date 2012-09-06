#include "list_set.hpp"
#include "test_unit.hpp"

namespace {

using base::ListBasedSet;

struct TestData {
  ListBasedSet* listset;
  int* operands;
  int length;
};

TEST(Simple, Insertion) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(99));
  EXPECT_TRUE(s.insert(53));
}

TEST(Simple, Remove) {
  ListBasedSet s;
  s.insert(1);
  s.insert(5);
  s.insert(77);
  EXPECT_FALSE(s.remove(10));
  EXPECT_FALSE(s.remove(30));
  EXPECT_TRUE(s.remove(1));
  EXPECT_TRUE(s.remove(77));
}

TEST(Simple, Lookup) {
  ListBasedSet s;
  s.insert(99);
  s.insert(17);
  s.remove(17);
  s.insert(35);
  EXPECT_FALSE(s.lookup(17));
  EXPECT_TRUE(s.lookup(99));
  EXPECT_TRUE(s.lookup(35));
  EXPECT_FALSE(s.lookup(90));
}

TEST(Simple, checkIntegrity) {
  ListBasedSet s;
  s.insert(1);
  s.insert(13);
  s.insert(2);
  s.insert(76);
  s.insert(61);
  EXPECT_TRUE(s.checkIntegrity());
}

TEST(Simple, Clear) {
  ListBasedSet s;
  s.insert(1);
  s.insert(20);
  s.insert(21);
  s.clear();
  EXPECT_FALSE(s.lookup(20));
  EXPECT_FALSE(s.lookup(1));
  EXPECT_FALSE(s.lookup(21));
}

TEST(Complex, Insert) {
  ListBasedSet s;
  for (int i=0; i<50; i++) {
    s.insert(i*2);
  }
  for (int i=0; i<100; i++) {
    if (i%2 == 0) {
      EXPECT_TRUE(s.lookup(i));
    }
    else {
      EXPECT_FALSE(s.lookup(i));
    }
  }
}

TEST(Complex, Remove) {
  ListBasedSet s;
  for (int i=0; i<150; i++) {
    s.insert(i);
  }
  for (int i=49; i>=0; i--) {
    EXPECT_TRUE(s.remove(i*3));
  }
  for (int i=0; i<150; i++) {
    if (i%3 == 0) {
      EXPECT_FALSE(s.lookup(i));
    }
    else {
      EXPECT_TRUE(s.lookup(i));
    }
  }
}
	

void* TestFunctionInsert(void* data) {
  struct TestData* td;
  td = (struct TestData*)data;
  for (int i=0; i<td->length; i++) {
    td->listset->insert(*(td->operands+i));
  }
  pthread_exit(NULL);
}


TEST(Concurrent, insert1) {
  const int NumThreads = 10, NumOperands = 100;
  pthread_t threads[NumThreads];
  ListBasedSet* s = new ListBasedSet();
  struct TestData testdataarray[10];
  for (int i=0; i<NumThreads; i++) {
    testdataarray[i].operands = new int[NumOperands];
    for (int j=0; j<NumOperands; j++) {
      testdataarray[i].operands[j] = i*10;
    }
    testdataarray[i].listset = s;
    testdataarray[i].length = NumOperands;
    pthread_create(&threads[i], NULL, TestFunctionInsert, (void*)(testdataarray+i));
  }
  for (int i=0; i<NumThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  EXPECT_FALSE(s->lookup(25));
  EXPECT_FALSE(s->lookup(55));
  EXPECT_TRUE(s->lookup(20));
  EXPECT_TRUE(s->lookup(50));
  EXPECT_TRUE(s->checkIntegrity());
}

TEST(Concurrent, insert2) {
  const int NumThreads = 10, NumOperands = 100;
  pthread_t threads[NumThreads];
  ListBasedSet* s = new ListBasedSet();
  struct TestData testdataarray[10];
  for (int i=0; i<NumThreads; i++) {
    testdataarray[i].operands = new int[NumOperands];
    for (int j=0; j<NumOperands; j++) {
      testdataarray[i].operands[j] = j*5;
    }
    testdataarray[i].listset = s;
    testdataarray[i].length = NumOperands;
    pthread_create(&threads[i], NULL, TestFunctionInsert, (void*)(testdataarray+i));
  }
  for (int i=0; i<NumThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  EXPECT_FALSE(s->lookup(127));
  EXPECT_FALSE(s->lookup(550));
  EXPECT_TRUE(s->lookup(200));
  EXPECT_TRUE(s->lookup(400));
  EXPECT_TRUE(s->checkIntegrity());
}

void* TestFunctionRemove(void* data) {
  struct TestData* td;
  td = (struct TestData*)data;
  for (int i=0; i<td->length; i++) {
    td->listset->remove(*(td->operands+i));
  }
  pthread_exit(NULL);
}


TEST(Concurrent, remove1) {
  const int NumThreads = 10, NumOperands = 100;
  pthread_t threads[NumThreads];
  ListBasedSet* s = new ListBasedSet();
  struct TestData testdataarray[10];
  for (int n=0; n<20; n++) {
    s->insert(n*5);
  }
  for (int i=0; i<NumThreads; i++) {
    testdataarray[i].operands = new int[NumOperands];
    for (int j=0; j<NumOperands; j++) {
      testdataarray[i].operands[j] = i*10;
    }
    testdataarray[i].listset = s;
    testdataarray[i].length = NumOperands;
    pthread_create(&threads[i], NULL, TestFunctionRemove, (void*)(testdataarray+i));
  }
  for (int i=0; i<NumThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  EXPECT_FALSE(s->lookup(20));
  EXPECT_FALSE(s->lookup(50));
  EXPECT_FALSE(s->lookup(60));
  EXPECT_FALSE(s->lookup(90));
  EXPECT_TRUE(s->lookup(25));
  EXPECT_TRUE(s->lookup(55));
  EXPECT_TRUE(s->remove(35));
  EXPECT_TRUE(s->remove(75));
  EXPECT_TRUE(s->checkIntegrity());
}


TEST(Concurrent, remove2) {
  const int NumThreads = 10, NumOperands = 100;
  pthread_t threads[NumThreads];
  ListBasedSet* s = new ListBasedSet();
  struct TestData testdataarray[10];
  for (int n=0; n<20; n++) {
    s->insert(n*5);
  }
  for (int i=0; i<NumThreads; i++) {
    testdataarray[i].operands = new int[NumOperands];
    for (int j=0; j<NumOperands; j++) {
      testdataarray[i].operands[j] = j*10;
    }
    testdataarray[i].listset = s;
    testdataarray[i].length = NumOperands;
    pthread_create(&threads[i], NULL, TestFunctionRemove, (void*)(testdataarray+i));
  }
  for (int i=0; i<NumThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  EXPECT_FALSE(s->lookup(100));
  EXPECT_FALSE(s->lookup(60));
  EXPECT_FALSE(s->lookup(50));
  EXPECT_FALSE(s->lookup(70));
  EXPECT_FALSE(s->lookup(80));
  EXPECT_TRUE(s->lookup(25));
  EXPECT_TRUE(s->lookup(45));
  EXPECT_TRUE(s->remove(95));
  EXPECT_TRUE(s->remove(15));
  EXPECT_TRUE(s->checkIntegrity());
}



TEST(Concurrent, SingleInsertRemove) {
  pthread_t thread1, thread2;
  int NumOperands = 200;
  ListBasedSet* s = new ListBasedSet();
  struct TestData testdata1, testdata2;
  testdata1.operands = new int[NumOperands];
  testdata2.operands = new int[NumOperands];
  for (int i=0; i<NumOperands; i++) {
    testdata1.operands[i] = i*5;
    testdata2.operands[i] = i*10;
  }
  testdata1.length = NumOperands;
  testdata2.length = NumOperands;  
  testdata1.listset = s;
  testdata2.listset = s;
  pthread_create(&thread1, NULL, TestFunctionInsert, (void*)&testdata1);
  pthread_create(&thread2, NULL, TestFunctionRemove, (void*)&testdata2);
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  EXPECT_TRUE(s->checkIntegrity());
  EXPECT_TRUE(s->lookup(405));
  EXPECT_TRUE(s->lookup(525));
}

TEST(Concurrent, MultipleInsertRemove) {
  const int NumOperands = 100, NumThreads = 20;
  pthread_t threads[NumThreads];
  ListBasedSet* s = new ListBasedSet();
  struct TestData testdataarray[NumThreads];
  for (int i=0; i<NumThreads; i++) {
    testdataarray[i].operands = new int[NumOperands];
    for (int j=0; j<NumOperands; j++) {
      testdataarray[i].operands[j] = i*j;
    }
    testdataarray[i].listset = s;
    testdataarray[i].length = NumOperands;
    if (i%2 == 0) {
      pthread_create(&threads[i], NULL, TestFunctionInsert, (void*)(testdataarray+i));
    }
    else {
      pthread_create(&threads[i], NULL, TestFunctionRemove, (void*)(testdataarray+i));
    }
  }
  for (int i=0; i<NumThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  EXPECT_TRUE(s->lookup(16));
  EXPECT_TRUE(s->remove(64));
  EXPECT_FALSE(s->lookup(393));
  EXPECT_FALSE(s->insert(1024));
  EXPECT_TRUE(s->checkIntegrity());
}
 
TEST(Concurrent, SeparateInsertRemove) {  
  const int NumOperands = 100, NumThreads = 20;
  pthread_t threads[NumThreads];
  ListBasedSet* s = new ListBasedSet();
  struct TestData testdataarray[NumThreads];
  for (int i=0; i<NumThreads/2; i++) {
    testdataarray[i].operands = new int[NumOperands];
    for (int j=0; j<NumOperands; j++) {
      testdataarray[i].operands[j] = i*j;
    }
    testdataarray[i].listset = s;
    testdataarray[i].length = NumOperands;
    pthread_create(&threads[i], NULL, TestFunctionInsert, (void*)(testdataarray+i));   
  }
  for (int i=0; i<NumThreads/2; i++) {
    pthread_join(threads[i], NULL);
  }
  EXPECT_TRUE(s->lookup(16));
  EXPECT_TRUE(s->remove(64));
  EXPECT_TRUE(s->lookup(95));
  EXPECT_FALSE(s->lookup(393));
  EXPECT_FALSE(s->remove(1024));
  EXPECT_FALSE(s->lookup(700));
  EXPECT_TRUE(s->checkIntegrity());

  
  for (int i=NumThreads/2; i<NumThreads; i++) {
    testdataarray[i].operands = new int[NumOperands];
    for (int j=0; j<NumOperands; j++) {
      testdataarray[i].operands[j] = i*j;
    }
    testdataarray[i].listset = s;
    testdataarray[i].length = NumOperands;
    pthread_create(&threads[i], NULL, TestFunctionRemove, (void*)(testdataarray+i));   
  }
  for (int i=NumThreads/2; i<NumThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  EXPECT_FALSE(s->lookup(16));
  EXPECT_FALSE(s->lookup(256));
  EXPECT_FALSE(s->lookup(95));
  EXPECT_FALSE(s->lookup(630));
  EXPECT_TRUE(s->lookup(93));
  EXPECT_TRUE(s->remove(7));
  EXPECT_TRUE(s->checkIntegrity());
}

TEST(Concurrent, Lookup1) {
  const int NumThreads = 10, NumOperands = 100;
  pthread_t threads[NumThreads];
  ListBasedSet* s = new ListBasedSet();
  struct TestData testdataarray[NumThreads];
  for (int i=0; i<50; i++) {
    s->insert(i);
  } 
  for (int i=0; i<NumThreads; i++) {
    for (int j=0; j<NumOperands; j++) {
      testdataarray[i].operands = new int[NumOperands];
      testdataarray[i].operands[j] = i*j;
    }
    testdataarray[i].listset = s;
    testdataarray[i].length = NumOperands;
    pthread_create(&threads[i], NULL, TestFunctionInsert, (void*)(testdataarray+i));
  }
  EXPECT_TRUE(s->lookup(0));
  EXPECT_TRUE(s->lookup(5));
  EXPECT_TRUE(s->lookup(27));
  EXPECT_FALSE(s->lookup(53));
  EXPECT_TRUE(s->checkIntegrity());
  for (int i=0; i<NumThreads; i++) {
    pthread_join(threads[i], NULL);
  }
}


TEST(Concurrent, Lookup2) {
  const int NumThreads = 10, NumOperands = 100;
  pthread_t threads[NumThreads];
  ListBasedSet* s = new ListBasedSet();
  struct TestData testdataarray[NumThreads];
  for (int i=0; i<50; i++) {
    s->insert(i);
  } 
  for (int i=0; i<NumThreads; i++) {
    for (int j=0; j<NumOperands; j++) {
      testdataarray[i].operands = new int[NumOperands];
      testdataarray[i].operands[j] = i*j;
    }
    testdataarray[i].listset = s;
    testdataarray[i].length = NumOperands;
    pthread_create(&threads[i], NULL, TestFunctionRemove, (void*)(testdataarray+i));
  }
  EXPECT_TRUE(s->lookup(5));
  EXPECT_TRUE(s->lookup(13));
  EXPECT_TRUE(s->lookup(17));
  EXPECT_TRUE(s->lookup(19));
  EXPECT_TRUE(s->lookup(43));
  EXPECT_TRUE(s->lookup(23));
  EXPECT_TRUE(s->lookup(47));
  EXPECT_TRUE(s->checkIntegrity());
  for (int i=0; i<NumThreads; i++) {
    pthread_join(threads[i], NULL);
  }
}
  
} // unnamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}

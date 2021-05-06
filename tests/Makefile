GOOGLE_TEST_LIB = gtest
GOOGLE_TEST_INCLUDE = /usr/src/gtest/include
PROJECT_INCLUDES = ..

CXXFLAGS=-Wall -ggdb3 -pedantic -std=c++17 -I $(GOOGLE_TEST_INCLUDE) -I $(PROJECT_INCLUDES)
LINKERFLAGS=-lpthread -ldl -L gtest -l $(GOOGLE_TEST_LIB)

CSOURCES =  ../sqlite3.c
CPPSOURCES = test.cpp ../sqlite.cpp
OBJ = $(CSOURCES:.c=.o) $(CPPSOURCES:.cpp=.o)

TARGET = sqlite_test


$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LINKERFLAGS)

%.o: %.c
	cc -o $@ -c $<

%.o: %.cpp
	g++ $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean

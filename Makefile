# sqlite requires pthreads and dl to support dynamic loading
# https://sqlite.org/howtocompile.html
CXXFLAGS=-Wall -pedantic
LINKERFLAGS=-lpthread -ldl

CSOURCES =  sqlite3.c
CPPSOURCES = main.cpp sqlite.cpp
OBJ = $(CSOURCES:.c=.o) $(CPPSOURCES:.cpp=.o)

sqlite_test: $(OBJ)
	$(CXX) -o $@ $^ $(LINKERFLAGS)

%.o: %.cpp
	g++ $(CXXFLAGS) -o $@ -c $<

%.o: %.c
	cc -o $@ -c $<


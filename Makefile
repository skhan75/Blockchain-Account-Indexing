CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS = -lstdc++fs

SRC_FILES = BlockchainAccountIndexing.cpp
OBJ_FILES = $(SRC_FILES:.cpp=.o)
EXECUTABLE = blockchain_account_manager

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILES) $(EXECUTABLE)
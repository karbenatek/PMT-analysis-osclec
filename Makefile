include conandeps.mk # so far only tomlplusplus is included
SRC_DIR=src
LIB_DIR=lib
BUILD_DIR=build
BIN_DIR=bin

# compiler setup
CXX = g++
ROOTFLAGS :=`root-config --cflags` -I$(CONAN_INCLUDE_DIRS)
ROOTLIBS :=`root-config --libs`

# objects definition
LIB_OBJS = $(LIB_DIR)/routines.o $(LIB_DIR)/libosclec.o $(LIB_DIR)/libfit.o $(LIB_DIR)/libanalis.o $(LIB_DIR)/libinfn.o
SRC_INCS = $(SRC_DIR)/routines.cpp
SRC = $(SRC_DIR)/pmta.cpp
TARGET = $(BIN_DIR)/pmta

# create parrent path of the rule
dir_guard=@mkdir -p $(@D)

install: $(TARGET)

$(TARGET): $(SRC) $(LIB_OBJS) $(SRC_INCS)
	$(dir_guard)
	$(CXX) $(ROOTFLAGS) -o $@ $(SRC) $(LIB_OBJS) $(ROOTLIBS)

# add debug flag for compiler
debug: ROOTFLAGS += -DDEBUG -g
debug: $(TARGET)

# some run command for testing or debugind
test: $(TARGET)
	# pmta mess/3peDR_analysis.root -c mess/cfg.toml
	# pmta -c mess/cfgtest.toml -r
	# pmta -c mess/test_dr.toml
	pmta -c mess/tw.toml -r

# make all lib objects defined under $(LIB_OBJS) 	
build_libs: $(LIB_OBJS)

# compile lib object
$(LIB_DIR)/%.o: $(SRC_DIR)/%.cpp $(SRC_DIR)/routines.h
	$(dir_guard)
	$(CXX) $(ROOTFLAGS) -c $< -o $@ $(ROOTLIBS)

conandeps.mk:
	conan install . 
	rm *conanbuild* *conanrun*

clean:
	rm -f -r $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR) conandeps.mk

clean_libs:
	rm -f -r $(LIB_DIR)
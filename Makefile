include conandeps.mk
SRC_DIR=src
LIB_DIR=lib
BUILD_DIR=build
BIN_DIR=bin

CXX = g++
ROOTFLAGS :=`root-config --cflags` -I$(CONAN_INCLUDE_DIRS)
ROOTLIBS :=`root-config --libs`


# CXXFLAGS=-L/usr/lib/root -lGui -lCore -lImt -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lROOTVecOps -lTree -lTreePlayer -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -lROOTDataFrame -pthread -lm -ldl -rdynamic -pthread -std=c++17 -m64 --std=c++17 -O0 -fPIC
# ROOTLIBS :=-L/snap/root-framework/925/usr/local/lib -lCore -lImt -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lROOTVecOps -lTree -lTreePlayer -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -lROOTDataFrame -Wl,-rpath,/snap/root-framework/925/usr/local/lib -pthread -lm -ldl -rdynamic
# ROOTFLAGS :=-pthread -std=c++17 -m64 -I/snap/root-framework/925/usr/local/include
LIB_OBJS = $(LIB_DIR)/libosclec.o $(LIB_DIR)/libfit.o $(LIB_DIR)/libanalis.o
SRC = $(SRC_DIR)/pma.cpp
TARGET = $(BIN_DIR)/pma

dir_guard=@mkdir -p $(@D)

install: $(TARGET)

$(TARGET): $(SRC) $(LIB_OBJS) $(SRC_DIR)/routines.cpp
	$(dir_guard)
	$(CXX) $(ROOTFLAGS) -o $@ $(SRC) $(LIB_OBJS) $(ROOTLIBS)

debug: ROOTFLAGS += -DDEBUG -g
debug: $(TARGET)

test: $(TARGET)
	# pma mess/3peDR_analysis.root -c mess/cfg.toml
	pma test.root -c mess/cfgtest.toml -r

# $(BIN_DIR)/dr_3pe: $(SRC_DIR)/libanalis.cpp
# 	$(CXX) $(ROOTFLAGS) $< -o $@ $(ROOTLIBS)
# 	./$@



# install: $(BIN_DIR)/osc2root $(BIN_DIR)/root2hists $(BIN_DIR)/osc2fit

# $(BIN_DIR)/osc2fit: $(SRC_DIR)/libfit.cpp
# 	$(dir_guard)
# 	$(CXX) $(ROOTFLAGS) $< -o $@ $(ROOTLIBS)

# $(BIN_DIR)/osc2root: $(SRC_DIR)/osc2root.cpp $(LIB_DIR)/libascii2root.o
# 	$(dir_guard)
# 	$(CXX) $(ROOTFLAGS) $^ -o $@ $(ROOTLIBS)

# $(BIN_DIR)/root2hists: $(SRC_DIR)/root2hists.cpp $(LIB_DIR)/libanalis.o
# 	$(dir_guard)
# 	$(CXX) $(ROOTFLAGS) $^ -o $@ $(ROOTLIBS)

build_libs: $(LIB_OBJS)


$(LIB_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(dir_guard)
	$(CXX) $(ROOTFLAGS) -c $< -o $@ $(ROOTLIBS)

conandeps.mk:
	@conan install . 
	@rm *conanbuild* *conanrun*

clean:
	rm -f -r $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR) conandeps.mk

clean_libs:
	rm -f -r $(LIB_DIR)


# cling interactive
# build_libs: lib/libascii2root.so lib/libanalis.so lib/libfit.so

# lib/%.so: lib/%.cpp
# 	root -q -l -n -e '.L $<+'


# clean_libs:
# 	rm -r -f lib/*.so
# 	rm -r -f lib/*.d
# 	rm -r -f lib/*.pcm
# 	rm -r -f lib/*.o



# run_dev:
# 	echo $(SRC_DIR)/run.cpp | entr root -q -l -e '.X $(SRC_DIR)/run.cpp'


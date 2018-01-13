OPCV = -L/usr/local/lib -lopencv_core -lopencv_highgui -lopencv_features2d -lopencv_flann -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio
NCS = -lmvnc
SRC_DIR = src/
BUILD_DIR = build/
CFLAGS = -std=c++11

.PHONY: all
all: compile

.PHONY: compile
compile: objects
	@echo "[START] Building programs"
	g++ $(CFLAGS) $(BUILD_DIR)fp16.o $(BUILD_DIR)tracking.o $(SRC_DIR)test.cpp -o $(BUILD_DIR)test $(NCS) $(OPCV)
	@echo "[OK] Building programs"

.PHONY: objects
objects: build_dir
	@echo "[START] Compiling objects"
	g++ $(CFLAGS) $(SRC_DIR)tracking.cpp $(SRC_DIR)fp16.c -c $(NCS) $(OPCV)
	@mv *.o $(BUILD_DIR)
	@echo "[OK] Compiling objects"

.PHONY: run
run: compile
	@echo "\nStarting application"
	./build/test

build_dir:
	@if test ! -d build; \
	then mkdir build; \
	fi

clean:
	@echo "Cleaning programs";
	rm -rf build

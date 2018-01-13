OPCV = -L/usr/local/lib -lopencv_core -lopencv_highgui -lopencv_features2d -lopencv_flann -lopencv_imgproc -lopencv_imgcodecs
NCS = -lmvnc
SRC_DIR = src/
BUILD_DIR = build/

.PHONY: all
all: compile


.PHONY: compile
compile: build_dir
	@echo "Compiling test.cpp"
	g++ $(SRC_DIR)tracking.cpp $(SRC_DIR)fp16.c -c $(NCS) $(OPCV)
	@mv *.o $(BUILD_DIR)
	@echo "Finished building test"

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

FRAMEWORKS     = -framework Cocoa -framework Carbon -framework CoreServices
BUILD_PATH     = ./bin
BUILD_FLAGS    = -std=c99 -Wall -g -O0 -DOBJC_OLD_DISPATCH_PROTOTYPES=1
SKHD_SRC       = ./src/skhd.c
BINS           = $(BUILD_PATH)/skhd

.PHONY: all clean install

all: clean $(BINS)

install: BUILD_FLAGS=-std=c99 -O2 -DOBJC_OLD_DISPATCH_PROTOTYPES=1
install: clean $(BINS)

clean:
	rm -rf $(BUILD_PATH)

$(BUILD_PATH)/skhd: $(SKHD_SRC)
	mkdir -p $(BUILD_PATH)
	clang $^ $(BUILD_FLAGS) $(FRAMEWORKS) -o $@

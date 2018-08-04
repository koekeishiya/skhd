FRAMEWORKS     = -framework Carbon
BUILD_PATH     = ./bin
BUILD_FLAGS    = -std=c99 -Wall -g -O0
SKHD_SRC       = ./src/skhd.c
BINS           = $(BUILD_PATH)/skhd

.PHONY: all clean install profile fast_profile

all: clean $(BINS)

install: BUILD_FLAGS=-std=c99 -O3
install: clean $(BINS)

profile: BUILD_FLAGS=-std=c99 -Wall -g -O0 -DSKHD_PROFILE
profile: clean $(BINS)

fast_profile: BUILD_FLAGS=-std=c99 -O3 -DSKHD_PROFILE
fast_profile: clean $(BINS)

clean:
	rm -rf $(BUILD_PATH)

$(BUILD_PATH)/skhd: $(SKHD_SRC)
	mkdir -p $(BUILD_PATH)
	clang $^ $(BUILD_FLAGS) $(FRAMEWORKS) -o $@

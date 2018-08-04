FRAMEWORKS     = -framework Carbon
BUILD_PATH     = ./bin
BUILD_FLAGS    = -std=c99 -Wall -g -O0 -DSKHD_PROFILE
SKHD_SRC       = ./src/skhd.c
BINS           = $(BUILD_PATH)/skhd

.PHONY: all clean install

all: clean $(BINS)

install: BUILD_FLAGS=-std=c99 -O3
install: clean $(BINS)

segfault: BUILD_FLAGS=-O0 -g -Wall -std=c99 -D_FORTIFY_SOURCE=2 -fstack-protector-strong --param ssp-buffer-size=4 -fPIC -fno-strict-overflow -Wformat -Wformat-security -Werror=format-security
segfault: clean $(BINS)

clean:
	rm -rf $(BUILD_PATH)

$(BUILD_PATH)/skhd: $(SKHD_SRC)
	mkdir -p $(BUILD_PATH)
	clang $^ $(BUILD_FLAGS) $(FRAMEWORKS) -o $@

# Tests de la logica del firmware en el PC (sin placa).
#   make        -> compila y ejecuta
#   make FILTER=evolve  -> solo los tests cuyo nombre contenga "evolve"
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter \
            -fno-omit-frame-pointer
LDFLAGS  ?=
SRC_DIR  := ..
BUILD    := build
BIN      := $(BUILD)/tamapoke_tests

SOURCES := main.cpp shim/shim.cpp test_pet.cpp test_dex.cpp test_i18n.cpp test_blit.cpp test_batcurve.cpp test_power.cpp \
           test_chess.cpp test_shake.cpp test_steps.cpp test_wifipick.cpp test_weather.cpp test_holiday.cpp test_stackchan.cpp test_walltime.cpp $(SRC_DIR)/pet.cpp $(SRC_DIR)/i18n.cpp $(SRC_DIR)/chess.cpp \
           $(SRC_DIR)/weather.cpp $(SRC_DIR)/holiday.cpp $(SRC_DIR)/stackchan.cpp $(SRC_DIR)/walltime.cpp

# ArduinoJson (solo cabeceras): la libreria que instala arduino-cli
ARDUINOJSON ?= $(firstword $(wildcard $(HOME)/Documents/Arduino/libraries/ArduinoJson/src \
                                      $(HOME)/Arduino/libraries/ArduinoJson/src))

.PHONY: all run clean asan
all: run

# las cabeceras del firmware tambien: dex.h es generado y sin ellas el binario
# de tests se quedaba viejo al regenerarlo
HEADERS := $(wildcard $(SRC_DIR)/*.h)

$(BIN): $(SOURCES) $(HEADERS) framework.h shim/Arduino.h shim/Preferences.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I shim -I $(ARDUINOJSON) -o $@ $(SOURCES) $(LDFLAGS)

run: $(BIN)
	@./$(BIN) $(FILTER)

# misma suite bajo AddressSanitizer + UBSan: caza desbordes e indices malos
asan: CXXFLAGS += -fsanitize=address,undefined -fno-sanitize-recover=all
asan: BIN := $(BUILD)/tamapoke_tests_asan
asan:
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I shim -I $(ARDUINOJSON) -o $(BUILD)/tamapoke_tests_asan $(SOURCES) $(LDFLAGS)
	@./$(BUILD)/tamapoke_tests_asan $(FILTER)

clean:
	rm -rf $(BUILD)

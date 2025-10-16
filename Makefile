# Configuración del compilador
CXX = g++
TARGET = simulador_medico
SRC_DIR = .
BUILD_DIR = build
SRC = $(SRC_DIR)/main.cpp
OBJ = $(BUILD_DIR)/main.o

# Flags de compilación
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -lboost_system -lleveldb -lpthread

# Detectar sistema operativo
UNAME_S := $(shell uname -s)

# Ajustes específicos por sistema operativo
ifeq ($(UNAME_S),Linux)
    # Linux específico
    CXXFLAGS += -D LINUX
endif
ifeq ($(UNAME_S),Darwin)
    # macOS específico
    CXXFLAGS += -D MACOS
    LDFLAGS += -L/usr/local/lib
endif

# Crear directorio de build si no existe
$(shell mkdir -p $(BUILD_DIR))

# Regla principal
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS)
	@echo "Compilación completada: $(TARGET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Utilidades
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Limpieza completada"

distclean: clean
	rm -f *~ *.bak

# Instalación de dependencias
install-deps-ubuntu:
	sudo apt-get update
	sudo apt-get install -y libboost-all-dev libleveldb-dev build-essential

install-deps-fedora:
	sudo dnf install -y boost-devel leveldb-devel gcc-c++

install-deps-macos:
	brew update
	brew install boost leveldb

install-deps: install-deps-ubuntu

# Modos de compilación
debug: CXXFLAGS += -g -DDEBUG -O0
debug: $(TARGET)
	@echo "Compilado en modo DEBUG"

release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)
	@echo "Compilado en modo RELEASE"

# Ejecución
run: $(TARGET)
	./$(TARGET)

run-debug: debug
	./$(TARGET)

# Información del sistema
info:
	@echo "Sistema: $(UNAME_S)"
	@echo "Compilador: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Librerías: $(LDFLAGS)"

.PHONY: all clean distclean install-deps install-deps-ubuntu install-deps-fedora install-deps-macos debug release run run-debug info
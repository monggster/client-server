COMPILER = gcc
FLAGS = -g -Wall -fsanitize=address
SERVER_DEPS = server.o message.o associations.o nodes.o
CLIENT_DEPS = client.o message.o
BUILD_DIR = build

COMPILER += $(FLAGS)

all: server client

build:
	mkdir -p build

server: $(SERVER_DEPS)
	$(COMPILER) $(patsubst %,$(BUILD_DIR)/%, $(SERVER_DEPS)) -o $(BUILD_DIR)/server

client: $(CLIENT_DEPS)
	$(COMPILER) $(patsubst %,$(BUILD_DIR)/%, $(CLIENT_DEPS)) -o $(BUILD_DIR)/client

server.o: build server.c
	$(COMPILER) server.c -c -o $(BUILD_DIR)/server.o

client.o: build client.c
	$(COMPILER) client.c -c -o $(BUILD_DIR)/client.o

message.o: build message.c
	$(COMPILER) message.c -c -o $(BUILD_DIR)/message.o

associations.o: build associations.c
	$(COMPILER) associations.c -c -o $(BUILD_DIR)/associations.o

nodes.o: build nodes.c
	$(COMPILER) nodes.c -c -o $(BUILD_DIR)/nodes.o



.PHONY: clean
clean:
	rm -rf build

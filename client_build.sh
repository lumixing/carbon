#!/bin/bash
gcc -Wall \
util.c \
client/main.c \
client/chunk.c \
client/camera.c \
client/gl.c \
client/world.c \
client/glad.c \
-lGL \
-lglfw \
-lm \
-o client.bin

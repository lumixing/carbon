#!/bin/bash
gcc -Wall client/main.c client/chunk.c client/util.c client/glad.c -lGL -lglfw -lm

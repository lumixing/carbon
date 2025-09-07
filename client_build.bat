gcc -Wall ^
util.c ^
client/main.c ^
client/chunk.c ^
client/camera.c ^
client/gl.c ^
client/world.c ^
glad.o ^
-lglfw3 -lopengl32 -lgdi32 ^
-o client
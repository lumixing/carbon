gcc -Wall ^
client/main.c ^
client/chunk.c ^
client/util.c ^
glad.o ^
-lglfw3 -lopengl32 -lgdi32 ^
-o client
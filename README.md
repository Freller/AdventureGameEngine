A game-engine written in C++ with Autofs.

Features a robust AI system, a scripting system, dialog system, save system, baked lighting and other stuff.

![image](https://github.com/user-attachments/assets/5cba6e6a-fcd0-4749-aa88-e3979d7da631)


64b Win 11, powershell build with:

g++ objects.cpp -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
g++ map_editor.cpp -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
g++ lightcookies.cpp -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
g++ globals.cpp -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
g++ main.cpp -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
g++ main.o objects.o map_editor.o lightcookies.o globals.o -std=c++17 -o out.exe -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
gdb ./out.exe


To port to linux:

-change WinMain() to main()

-change getCurrentDir() in globals.cpp

-change .qoi to .png

-add lines to convert .qoi to .png on-the-fly for level tiles.

-change line endings for saves/configs

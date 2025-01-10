physfsobjects = physfs.o physfs_archiver_7z.o physfs_archiver_dir.o physfs_archiver_grp.o physfs_archiver_hog.o physfs_archiver_iso9660.o physfs_archiver_mvl.o physfs_archiver_qpak.o physfs_archiver_slb.o physfs_archiver_unpacked.o physfs_archiver_vdf.o physfs_archiver_wad.o physfs_archiver_zip.o physfs_byteorder.o physfs_platform_android.o physfs_platform_os2.o physfs_platform_posix.o physfs_platform_qnx.o physfs_platform_unix.o physfs_platform_windows.o physfs_unicode.o

all: globals.o lightcookies.o main.o map_editor.o objects.o specialobjects.o utils.o combat.o title.o loss.o $(physfsobjects)
	g++ main.o objects.o map_editor.o specialobjects.o lightcookies.o globals.o utils.o combat.o title.o loss.o $(physfsobjects) -std=c++17 -ggdb -o out.exe -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -fmax-errors=1 -g

globals.o: globals.cpp
	g++ globals.cpp -flto -std=c++17  -ggdb -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

lightcookies.o: lightcookies.cpp
	g++ lightcookies.cpp -flto -std=c++17  -ggdb -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O3 -march=native  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

main.o: main.cpp
	g++ main.cpp -std=c++17  -ggdb -flto -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

map_editor.o: map_editor.cpp
	g++ map_editor.cpp -flto -std=c++17  -ggdb -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

objects.o: objects.cpp
	g++ objects.cpp -flto -std=c++17  -ggdb -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

specialobjects.o: specialobjects.cpp
	g++ specialobjects.cpp -flto -std=c++17  -ggdb -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

utils.o: utils.cpp
	g++ utils.cpp -flto -std=c++17  -ggdb -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

combat.o: combat.cpp
	g++ combat.cpp -flto -std=c++17  -ggdb -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

title.o: title.cpp
	g++ title.cpp -flto -std=c++17  -ggdb -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

loss.o: loss.cpp
	g++ loss.cpp -flto -std=c++17  -ggdb -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

clean:
	del globals.o lightcookies.o main.o map_editor.o objects.o specialobjects.o utils.o combat.o title.o loss.o

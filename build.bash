gcc `pkg-config --cflags gtk+-3.0` main.c -o grid_app `pkg-config --libs gtk+-3.0` -lX11 -lXtst -lm
./grid_app
SET PATH=C:\msys64\mingw64\bin;%PATH%
SET CC=g++.exe
SET CXXFLAGS=-std=c++17 -O3 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
SET LDFLAGS=-O3 -static -s

%CC% -c %CXXFLAGS% -I. main.cpp -o main.o

%CC% -o playfair.exe main.o %LDFLAGS%

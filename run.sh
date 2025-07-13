g++ -std=c++17 -DENABLE_LOGS -USHOW_TREE -USHOW_BYTES -O2 -o compress compress.cpp
.\compress.exe .\input.txt

g++ -std=c++17 -UENABLE_LOGS -DSHOW_TREE -DSHOW_BYTES -O2 -o decompress decompress.cpp
.\decompress.exe .\input.txt.bin

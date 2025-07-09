g++ -std=c++17 -UENABLE_LOGS -DSHOW_TREE -O2 -o compress compress.cpp
.\compress.exe .\input.txt

g++ -std=c++17 -UENABLE_LOGS -DSHOW_TREE -O2 -o decompress decompress.cpp
.\decompress.exe .\input.txt.bin
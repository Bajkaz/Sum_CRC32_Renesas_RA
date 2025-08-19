#include <cstdint>
#include "obliczenia.h"

//// Tablica CRC32 (polinom 0xEDB88320)
//uint32_t crc32_table[256];
//
//// Funkcja do inicjalizacji tabeli CRC32
//void init_crc32()
//{
//    uint32_t polynomial = 0xEDB88320;
//    for (uint32_t i = 0; i < 256; i++)
//    {
//        uint32_t c = i;
//        for (int j = 0; j < 8; j++)
//        {
//            if (c & 1)
//                c = polynomial ^ (c >> 1);
//            else
//                c = c >> 1;
//        }
//        crc32_table[i] = c;
//    }
//}
//
//// Funkcja obliczaj¹ca CRC32 dla danych
//uint32_t calculate_crc32(const uint8_t* data, size_t length)
//{
//    uint32_t crc = 0xFFFFFFFF;
//    for (size_t i = 0; i < length; i++)
//    {
//        uint8_t byte = data[i];
//        crc = crc32_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
//    }
//    return crc ^ 0xFFFFFFFF;
//}

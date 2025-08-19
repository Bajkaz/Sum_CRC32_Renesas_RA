#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <sstream>

#define FLASH_SIZE      0x200
#define CHECKSUM_OFFSET 0x10

HINSTANCE hInst;
HWND hEditOutput;
WCHAR szTitle[] = L"CRC32 dla pliku HEX";
WCHAR szWindowClass[] = L"HexCRC32";

// symulacja pamięci Flash i tabela CRC
uint8_t flash_sim[FLASH_SIZE];
uint8_t g_flash_sum_table[2];

// CRC32 update (odwrócony polinom)
uint32_t crc32_update(uint32_t crc, uint8_t data)
{
    crc ^= data;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
        else crc >>= 1;
    }
    return crc;
}

void calculate_flash_crc()
{
    uint32_t crc = 0xFFFFFFFF;

    for (uint32_t i = 0; i < FLASH_SIZE; i++)
    {
        uint8_t byte;

        if (i >= CHECKSUM_OFFSET && i < CHECKSUM_OFFSET + 4)
            byte = 0xFF;
        else
            byte = flash_sim[i];

        crc = crc32_update(crc, byte);
    }

    crc ^= 0xFFFFFFFF;

    g_flash_sum_table[0] = (uint8_t)(crc & 0xFF);
    g_flash_sum_table[1] = (uint8_t)((crc >> 8) & 0xFF);
}

// funkcja wczytująca plik HEX
bool LoadHexFile(const std::wstring& filename)
{
    std::wifstream file(filename);
    if (!file.is_open()) return false;

    std::wstring line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] != L':') continue;

        int byteCount = std::stoi(line.substr(1, 2), nullptr, 16);
        int offset = std::stoi(line.substr(3, 4), nullptr, 16);
        int recordType = std::stoi(line.substr(7, 2), nullptr, 16);

        if (recordType != 0x00) continue;

        for (int i = 0; i < byteCount; i++)
        {
            int byteVal = std::stoi(line.substr(9 + i * 2, 2), nullptr, 16);
            if (offset + i < FLASH_SIZE)
                flash_sim[offset + i] = static_cast<uint8_t>(byteVal);
        }
    }
    return true;
}

// funkcja zapisująca plik HEX z dopisanym CRC w nazwie
bool SaveHexWithCRC(const std::wstring& originalFile)
{
    // utwórz nazwę pliku z sumą CRC
    wchar_t crcStr[5];
    swprintf(crcStr, 5, L"%02X%02X", g_flash_sum_table[1], g_flash_sum_table[0]); // MSB:LSB
    std::wstring newFile = originalFile;
    size_t dotPos = newFile.rfind(L".hex");
    if (dotPos != std::wstring::npos)
        newFile.insert(dotPos, L"_" + std::wstring(crcStr));
    else
        newFile += L"_" + std::wstring(crcStr) + L".hex";

    std::wifstream srcFile(originalFile);
    if (!srcFile.is_open()) return false;

    std::wofstream dstFile(newFile);
    if (!dstFile.is_open()) return false;

    std::wstring line;
    while (std::getline(srcFile, line))
        dstFile << line << L"\n";

    // dopisz CRC jako linia danych (rekord typu 00)
    dstFile << L":02000000"
        << std::hex << std::uppercase
        << std::setw(2) << std::setfill(L'0') << (int)g_flash_sum_table[0]
        << std::setw(2) << std::setfill(L'0') << (int)g_flash_sum_table[1]
        << L"00\n";

    dstFile << L":00000001FF\n"; // EOF

    return true;
}

//-------------------------------------------------------------------------------------
// Win32
//-------------------------------------------------------------------------------------
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    WNDCLASSEXW wcex{ sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc,
                      0,0, hInstance, nullptr, LoadCursor(nullptr, IDC_ARROW),
                      (HBRUSH)(COLOR_WINDOW + 1), nullptr, szWindowClass, nullptr };

    RegisterClassExW(&wcex);
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 500, 250, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;

    // Pole wynikowe
    hEditOutput = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_READONLY,
        20, 50, 440, 25, hWnd, nullptr, hInstance, nullptr);

    // Przycisk wczytaj HEX
    CreateWindowW(L"BUTTON", L"Wczytaj HEX", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 100, 150, 25, hWnd, (HMENU)1, hInstance, nullptr);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // wypełnij flash 0xFF
    std::fill(std::begin(flash_sim), std::end(flash_sim), 0xFF);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) // Wczytaj HEX
        {
            wchar_t filename[MAX_PATH] = { 0 };
            OPENFILENAMEW ofn = { sizeof(ofn) };
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = filename;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"HEX Files\0*.hex\0All Files\0*.*\0";
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

            if (GetOpenFileNameW(&ofn))
            {
                if (LoadHexFile(filename))
                {
                    calculate_flash_crc();

                    std::wstringstream ss;
                    ss << L"CRC32 (2 bajty LSB): "
                        << std::hex << std::setw(2) << std::setfill(L'0') << (int)g_flash_sum_table[0] << L" "
                        << std::hex << std::setw(2) << std::setfill(L'0') << (int)g_flash_sum_table[1];

                    SetWindowTextW(hEditOutput, ss.str().c_str());

                    if (SaveHexWithCRC(filename))
                    {
                        ss.str(L"");
                        ss << L"Suma CRC (2 bajty LSB): "
                            << std::hex << std::uppercase
                            << std::setw(2) << std::setfill(L'0') << (int)g_flash_sum_table[0] << " "
                            << std::setw(2) << std::setfill(L'0') << (int)g_flash_sum_table[1] <<". Plik zostal zapisany.";
                        SetWindowTextW(hEditOutput, ss.str().c_str());
                    }
                    else
                        SetWindowTextW(hEditOutput, L"Błąd zapisu pliku z CRC");
                }
                else SetWindowTextW(hEditOutput, L"Błąd odczytu pliku");
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

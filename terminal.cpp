#include <Windows.h>
#include <cstddef>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <list>
#include <mmsystem.h>
#include <queue>
#include <cmath>
#include <memory>

using namespace std;

const int DIGIT_HEIGHT = 5;
const int DIGIT_WIDTH = 4;
const char OPEN_SPACE = '.';
const char WALL = '#';
enum Direction { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3 };


bool debug = false;

int previousMouseX = 0;
int previousMouseY = 0;

float fPlayerX = 18.2f, fPlayerY = 15.5f, fPlayerAngle = 4.71f, fPlayerPitch = 0.0f;
float fEnemyX = 17.5f, fEnemyY = 15.5f;
bool bMoving = false, bFocused = false, bRecovery = false, bLocked = false, bStart = true, bStartTop = true, bEnd = false, bBossMusic = false;
int pauseSelection = 0;
const int nScreenWidth = 300, nScreenHeight = 90;
int nMapHeight = 32, nMapWidth = 32, width, height;
float fConstFOV = 1.0f, fDepth = 16.0f, fConstSpeed = 2.0f, fConstStamina = 10.f, fConstHealth = 10.0f, fSensitivityX = 0.005f, fSensitivityY = 0.5f;
float fFOV = fConstFOV, fSpeed = fConstSpeed, fEnemySpeed = 1.5f, fStamina = fConstStamina, fHealth = fConstHealth, fStaminaDrain = 2.0f, fHealthDrain = 5.0f, fStaminaGain = 1.0f;

vector<int> directions, lastDirections;
int dir;

wchar_t *screen = new wchar_t[nScreenWidth * nScreenHeight];
HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
DWORD dwBytesWritten = 0;
int renderBuffer[nScreenWidth * nScreenHeight];

HWND hwndFocus;
HWND hwndConsole;
HWND hwndTerminal;
RECT rect;

struct Orb
{
    float x, y;
};

struct Node
{
    int x, y;
    float fCost, gCost, hCost;
    shared_ptr<Node> parent;

    bool operator<(const Node& other) const
    {
        return fCost > other.fCost;
    }
};

list<Orb> orbs;

wstring map = 
        L"################################"
        L"#........#..........#..........#"
        L"#.######.#.######.#.####.#######"
        L"#.#......#......#.#......#.....#"
        L"#.#.######.####.#.#########.##.#"
        L"#.#.#......#....#..............#"
        L"#.#.#.####.#.#.#.#####.#######.#"
        L"#.#.#.#....#.#.#.#.............#"
        L"#.#.#.#.####.#.#.##.#.####.###.#"
        L"#.#.#.#.#..#.#.#...............#"
        L"#.#.#.#.#.#.#.#.#####.#####.##.#"
        L"#.#...#.#.#.#.#.....#.....#.#..#"
        L"#.#####.#.#.#.#####.#.###.#.#.##"
        L"#.......#.#.#.......#.#.#.#.#.##"
        L"#######.#.#.#########.#...#.#.##"
        L"#.......#.#...........#.#...#.##"
        L"#.#######.#.#############.###.##"
        L"#.#.......#.............#.....##"
        L"#.#.#######.####.######.########"
        L"#.........#.#.......#.#........#"
        L"#.#######.#.#.#####.#.#######.##"
        L"#######.#.#.#.#.#.#.#######.#.##"
        L"#.......#...#.#.#.#.......#....#"
        L"#.#############.#.#########.####"
        L"#.#...........#.#.#.........#.##"
        L"#.#.#########.#.#.#.#######.#.##"
        L"#.#.#.......#.#.#.#.#.....#.#.##"
        L"#.#.#.#####.#.#.#.#.#.###.#.#.##"
        L"#.#.#.....#.#...#...#.#...#.#.##"
        L"#.#.#####.#.######.##.#.###.#.##"
        L"#.......................#......#"
        L"################################";


void clearScreen()
{
    FillConsoleOutputAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED, nScreenHeight*nScreenWidth, {0, 0}, &dwBytesWritten);
    for (int i = 0; i < nScreenHeight*nScreenWidth; i++)
        {
            screen[i] = 0x2588;
        }
}


void fillRectangle(int startX, int startY, int width, int height, wchar_t fillChar = ' ') 
{
    for (int y = startY; y < startY + height; ++y) {
        for (int x = startX; x < startX + width; ++x) {
            screen[y * nScreenWidth + x] = fillChar;
        }
    }
}


void drawPointer(int startX, int startY, int width, int height, wchar_t fillChar = ' ') 
{

    for (int x = 0; x < width; ++x) {
        int colStartY = startY + (height / 2) - ((width-x) * height / width) / 2;
        int colEndY = startY + (height / 2) + ((width-x) * height / width) / 2;

        for (int y = colStartY; y <= colEndY; ++y) {
            screen[y * nScreenWidth + (startX + x)] = fillChar;
        }
    }
}


void drawInteger(int startX, int startY, int integer, int size = 1, wchar_t fillChar = ' ') 
{

    const int digitPatterns[10][DIGIT_HEIGHT][DIGIT_WIDTH] = {
        {{1, 1, 1, 1}, {1, 0, 0, 1}, {1, 0, 0, 1}, {1, 0, 0, 1}, {1, 1, 1, 1}}, //0
        {{0, 0, 1, 0}, {0, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 1, 1}}, //1
        {{1, 1, 1, 1}, {0, 0, 0, 1}, {1, 1, 1, 1}, {1, 0, 0, 0}, {1, 1, 1, 1}}, //2
        {{1, 1, 1, 1}, {0, 0, 0, 1}, {0, 1, 1, 1}, {0, 0, 0, 1}, {1, 1, 1, 1}}, //3
        {{1, 0, 0, 1}, {1, 0, 0, 1}, {1, 1, 1, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}}, //4
        {{1, 1, 1, 1}, {1, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 1}, {1, 1, 1, 1}}, //5
        {{1, 1, 1, 1}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 0, 0, 1}, {1, 1, 1, 1}}, //6
        {{1, 1, 1, 1}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}, //7
        {{1, 1, 1, 1}, {1, 0, 0, 1}, {1, 1, 1, 1}, {1, 0, 0, 1}, {1, 1, 1, 1}}, //8
        {{1, 1, 1, 1}, {1, 0, 0, 1}, {1, 1, 1, 1}, {0, 0, 0, 1}, {1, 1, 1, 1}}, //9
    };

    if (integer < 0 || integer > 9) return;

    for (int y = 0; y < DIGIT_HEIGHT; y++) {
        for (int x = 0; x < DIGIT_WIDTH; x++) {
            if (digitPatterns[integer][y][x] == 1) {
                fillRectangle(startX + (x*size), startY + (y*size), size, size, fillChar);
            }
        }
    }
}


void drawNumber(int startX, int startY, int number, int scale = 1, wchar_t fillChar = ' ')
{
    wstring numStr = to_wstring(number);
    int offSet = 0;

    for (wchar_t digit : numStr)
    {
        drawInteger(startX + ((DIGIT_WIDTH + 1) * offSet * scale), startY, digit - '0', scale, fillChar);
        offSet++;
    }
}


void drawCharacter(int startX, int startY, char character, int scale = 1, wchar_t fillChar = ' ') 
{
    
    const int charPatterns[26][5][5] = {
        {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}}, //A
        {{1, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 0}}, //B
        {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 0}, {1, 0, 0, 0, 1}, {0, 1, 1, 1, 0}}, //C
        {{1, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 0}}, //D
        {{1, 1, 1, 1, 1}, {1, 0, 0, 0, 0}, {1, 1, 1, 1, 0}, {1, 0, 0, 0, 0}, {1, 1, 1, 1, 1}}, //E
        {{1, 1, 1, 1, 1}, {1, 0, 0, 0, 0}, {1, 1, 1, 1, 0}, {1, 0, 0, 0, 0}, {1, 0, 0, 0, 0}}, //F
        {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 0}, {1, 0, 1, 1, 1}, {1, 0, 0, 0, 1}, {0, 1, 1, 1, 1}}, //G
        {{1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}}, //H
        {{0, 1, 1, 1, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 1, 1, 1, 0}}, //I
        {{0, 0, 0, 0, 1}, {0, 0, 0, 0, 1}, {0, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {0, 1, 1, 1, 0}}, //J
        {{1, 0, 0, 0, 1}, {1, 0, 0, 1, 0}, {1, 1, 1, 0, 0}, {1, 0, 0, 1, 0}, {1, 0, 0, 0, 1}}, //K
        {{1, 0, 0, 0, 0}, {1, 0, 0, 0, 0}, {1, 0, 0, 0, 0}, {1, 0, 0, 0, 0}, {1, 1, 1, 1, 1}}, //L
        {{1, 0, 0, 0, 1}, {1, 1, 0, 1, 1}, {1, 0, 1, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}}, //M
        {{1, 0, 0, 0, 1}, {1, 1, 0, 0, 1}, {1, 0, 1, 0, 1}, {1, 0, 0, 1, 1}, {1, 0, 0, 0, 1}}, //N
        {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {0, 1, 1, 1, 0}}, //O
        {{1, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 0}, {1, 0, 0, 0, 0}, {1, 0, 0, 0, 0}}, //P
        {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 1, 0, 1}, {0, 1, 1, 1, 1}}, //Q
        {{1, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 0}, {1, 0, 0, 1, 0}, {1, 0, 0, 0, 1}}, //R
        {{0, 1, 1, 1, 1}, {1, 0, 0, 0, 0}, {0, 1, 1, 1, 0}, {0, 0, 0, 0, 1}, {1, 1, 1, 1, 0}}, //S
        {{1, 1, 1, 1, 1}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}}, //T
        {{1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {0, 1, 1, 1, 0}}, //U
        {{1, 0, 0, 0, 1}, {1, 1, 0, 1, 1}, {0, 1, 0, 1, 0}, {0, 1, 1, 1, 0}, {0, 0, 1, 0, 0}}, //V
        {{1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 1, 1}, {1, 0, 0, 0, 1}}, //W
        {{1, 0, 0, 0, 1}, {0, 1, 0, 1, 0}, {0, 0, 1, 0, 0}, {0, 1, 0, 1, 0}, {1, 0, 0, 0, 1}}, //X
        {{1, 0, 0, 0, 1}, {1, 1, 0, 1, 1}, {0, 1, 1, 1, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}}, //Y
        {{1, 1, 1, 1, 1}, {0, 0, 0, 0, 1}, {0, 0, 1, 1, 0}, {0, 1, 0, 0, 0}, {1, 1, 1, 1, 1}}  //Z
    };

    if (character < 'A' || character > 'Z') return;

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            if (charPatterns[character-65][y][x] == 1) {
                fillRectangle(startX + (x*scale), startY + (y*scale), scale, scale, fillChar);
            }
        }
    }
}


void drawString(int startX, int startY, string word, int scale = 1, wchar_t fillChar = ' ')
{
    for (int i = 0; i < word.length(); i++)
    {
        drawCharacter(startX + (i * scale * 6), startY, word[i], scale);
    }
}


void drawPauseMenuOptions()
{
    drawString(112, 21, "PAUSED", 2);
    drawString(115, 38, "SENSITIVITY");
    drawString(140, 55, "FOV");
    drawString(136, 72, "EXIT");

    //Value
    int oneSens = (int)fSensitivityY;
    int tenthSens = (int)(fSensitivityY*10) % 10;
    int hundredthSens = (int)(fSensitivityY*100) % 10;

    fillRectangle(139, 45, 20, 5, 0x2588);
    drawInteger(139, 45, oneSens, 1);
    fillRectangle(144, 49, 1, 1);
    drawInteger(146, 45, tenthSens, 1);
    drawInteger(152, 45, hundredthSens, 1);

    //Value
    int hundredFOV = (int)fConstFOV;
    int tenFOV = (int)(fConstFOV*10) % 10;
    int oneFOV = (int)(fConstFOV*100) % 10;

    fillRectangle(139, 62, 20, 5, 0x2588);
    drawInteger(139, 62, hundredFOV, 1);
    drawInteger(144, 62, tenFOV, 1);
    drawInteger(150, 62, oneFOV, 1);
}


void drawMenuOptions()
{
    drawString(130, 50, "START");
    drawString(133, 60, "EXIT");
}


void render()
{
    screen[nScreenWidth * nScreenHeight - 1] = '\0';
    WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, {0, 0}, &dwBytesWritten);
}


void setupStats()
{
    WORD stamina = FOREGROUND_GREEN;
    WORD health = FOREGROUND_RED;
    FillConsoleOutputAttribute(hConsole, health, 70, {5, (short)(nScreenHeight-7)}, &dwBytesWritten);
    FillConsoleOutputAttribute(hConsole, health, 70, {5, (short)(nScreenHeight-8)}, &dwBytesWritten);

    FillConsoleOutputAttribute(hConsole, stamina, 70, {5, (short)(nScreenHeight-4)}, &dwBytesWritten);
    FillConsoleOutputAttribute(hConsole, stamina, 70, {5, (short)(nScreenHeight-5)}, &dwBytesWritten);
}


void renderGameStart()
{
    clearScreen();
    
    drawString(55, 10, "TERMINAL", 4);
    drawMenuOptions();
    if (bStartTop)
    {
        drawPointer(120, 50, 5, 5);
        drawPointer(120, 60, 5, 5, 0x2588);
        if (GetAsyncKeyState(VK_RETURN) & 0x0001)
        {
            PlaySound(TEXT("ambient.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
            setupStats();
            bStart = false;
        }
    }
    else
    {
        drawPointer(120, 60, 5, 5);
        drawPointer(120, 50, 5, 5, 0x2588);
        if (GetAsyncKeyState(VK_RETURN) & 0x0001)
        {
            bStart = false;
            bEnd = true;
        }
    }

    if (GetAsyncKeyState(VK_UP) & 0x0001 || GetAsyncKeyState(VK_DOWN) & 0x0001)
        bStartTop = !bStartTop;

    render();
}


void placeEntity(float& entityX, float& entityY, float excludeX = -1.0f, float excludeY = -1.0f, float minDistance = 0.0f)
{
    do
    {
        entityX = rand() % nMapWidth + 0.5f;
        entityY = rand() % nMapHeight + 0.5f;
    } 
    while (map[(int)entityY * nMapWidth + (int)entityX] == '#' || 
          (excludeX >= 0 && excludeY >= 0 && 
           pow(entityX - excludeX, 2) + pow(entityY - excludeY, 2) < minDistance * minDistance));
}


void populateOrbs()
{
    for (int nx = 0; nx < nMapWidth && orbs.size() < 100; nx++)
    {
        for (int ny = 0; ny < nMapHeight && orbs.size() < 100; ny++)
        {
            if (map[ny * nMapWidth + nx] == '.' && rand() % 4 == 1)
            {
                orbs.push_back({(float)nx + 0.5f, (float)ny + 0.5f});
            }
        }
    }
}


void resetGame()
{
    fPlayerX = 18.2f, fPlayerY = 15.5f, fPlayerAngle = -3.14159f / 2.0f, fPlayerPitch = 0.0f;
    fEnemyX = 17.5f, fEnemyY = 15.5f;
    fHealth = fConstHealth;
    fStamina = fConstStamina;
    bMoving = false, bFocused = false, bRecovery = false, bLocked = false, bStart = true, bStartTop = true, bEnd = false;
    orbs.clear();
    populateOrbs();
    placeEntity(fEnemyX, fEnemyY);  
    placeEntity(fPlayerX, fPlayerY, fEnemyX, fEnemyY, 10.0f);
    PlaySound(0, 0, 0);
}


void fixWindowSize()
{
    if(GetWindowRect(hwndTerminal, &rect))
    {
        width = (2*(rect.right - rect.left + 13))/10;
        height = (rect.bottom - rect.top - 28)/10;
    }

    if (width != nScreenWidth || height != nScreenHeight)
        SetWindowPos(hwndTerminal, NULL, 0, 0, 1487, 928, SWP_NOMOVE);
}


void renderPauseMenu()
{
    drawPauseMenuOptions();
                
    if (pauseSelection == 0)
    {
        drawPointer(105, 38, 5, 5);
        drawPointer(105, 72, 5, 5, 0x2588);
        drawPointer(105, 55, 5, 5, 0x2588);
        if (GetAsyncKeyState(VK_LEFT))
        {
            if (fSensitivityX > 0.0f)
            {
                fSensitivityX -= 0.00001;
                fSensitivityY -= 0.001;
            }
        }
        if (GetAsyncKeyState(VK_RIGHT))
        {
            if (fSensitivityX < 0.03f)
            {
                fSensitivityX += 0.00001;
                fSensitivityY += 0.001;
            }
        }
    }

    else if (pauseSelection == 1)
    {
        drawPointer(105, 55, 5, 5);
        drawPointer(105, 38, 5, 5, 0x2588);
        drawPointer(105, 72, 5, 5, 0x2588);
        if (GetAsyncKeyState(VK_LEFT))
        {
            if (fConstFOV > 0.5001f)
                fConstFOV -= 0.001;
        }
        if (GetAsyncKeyState(VK_RIGHT))
        {
            if (fConstFOV < 1.5f)
                fConstFOV += 0.001;
        }
        fFOV = fConstFOV;
    }

    else if (pauseSelection == 2)
    {
        drawPointer(105, 72, 5, 5);
        drawPointer(105, 38, 5, 5, 0x2588);
        drawPointer(105, 55, 5, 5, 0x2588);
        if (GetAsyncKeyState(VK_RETURN))
        {
            bEnd = true;
        }
    }

    if (GetAsyncKeyState(VK_UP) & 0x0001)
        pauseSelection -= 1;
    else if (GetAsyncKeyState(VK_DOWN) & 0x0001)
        pauseSelection += 1;

    pauseSelection = pauseSelection % 3;

    render();
}


void handleMouse(POINT cursorPos)
{
    GetCursorPos(&cursorPos);

    int deltaX = cursorPos.x - previousMouseX;
    int deltaY = cursorPos.y - previousMouseY;

    if (abs(deltaX) > 500)
        deltaX = 0;
    if (abs(deltaY) > 500)
        deltaY = 0;

    previousMouseX = cursorPos.x;
    previousMouseY = cursorPos.y;
    fPlayerAngle += deltaX * fSensitivityX;
    fPlayerPitch += deltaY * fSensitivityY;
    if (fPlayerPitch > nScreenHeight / 1.2) fPlayerPitch = nScreenHeight / 1.2;
    if (fPlayerPitch < -nScreenHeight / 1.2) fPlayerPitch = -nScreenHeight / 1.2;

    if (bFocused)
        {
            int right = GetSystemMetrics(SM_CXSCREEN);
            int bottom = GetSystemMetrics(SM_CYSCREEN);
            if (previousMouseX >= right-6)
                SetCursorPos(10, previousMouseY);
            else if (previousMouseX <= 0)
                SetCursorPos(right-9, previousMouseY);
            if (previousMouseY >= bottom-40)
                SetCursorPos(previousMouseX, 10);
            else if (previousMouseY <= 0)
                SetCursorPos(previousMouseX, bottom-50);
        }
}


void handleSprint(float fElapsedTime)
{
    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000 && bMoving && fStamina > 0.0f && !bRecovery) {
        fSpeed = fConstSpeed * 1.6;
        fStamina -= fStaminaDrain * fElapsedTime;
        if (fFOV < fConstFOV * 1.2)
            fFOV += 0.01;
    }
    else
    {
        fSpeed = fConstSpeed;
        if (fStamina < fConstStamina)
            fStamina += fStaminaGain * fElapsedTime;
        if (fStamina < 2.0f)
            bRecovery = true;
        else
            bRecovery = false;
        if (fFOV > fConstFOV)
            fFOV -= 0.01;
    }
}


void handleMovement(float fElapsedTime)
{
    bMoving = false;
    if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
        fPlayerX += sinf(fPlayerAngle) * fSpeed * fElapsedTime;
        fPlayerY += cosf(fPlayerAngle) * fSpeed * fElapsedTime;
        if (map[(int)(fPlayerY + cosf(fPlayerAngle) * fSpeed * fElapsedTime) * nMapWidth + (int)(fPlayerX + sinf(fPlayerAngle) * fSpeed * fElapsedTime)] == '#') {
            fPlayerX -= sinf(fPlayerAngle) * fSpeed * fElapsedTime;
            fPlayerY -= cosf(fPlayerAngle) * fSpeed * fElapsedTime;
        }
        bMoving = true;
    }
    if (GetAsyncKeyState((unsigned short)'A') & 0x8000) {
        fPlayerX -= cosf(fPlayerAngle) * fSpeed * fElapsedTime * 0.4f;
        fPlayerY += sinf(fPlayerAngle) * fSpeed * fElapsedTime * 0.4f;
        if (map[(int)(fPlayerY + sinf(fPlayerAngle) * fSpeed * fElapsedTime) * nMapWidth + (int)(fPlayerX - cosf(fPlayerAngle) * fSpeed * fElapsedTime)] == '#') {
            fPlayerX += cosf(fPlayerAngle) * fSpeed * fElapsedTime;
            fPlayerY -= sinf(fPlayerAngle) * fSpeed * fElapsedTime;
        }
        bMoving = true;
    }
    if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
        fPlayerX -= sinf(fPlayerAngle) * fSpeed * fElapsedTime;
        fPlayerY -= cosf(fPlayerAngle) * fSpeed * fElapsedTime;
        if (map[(int)(fPlayerY - cosf(fPlayerAngle) * fSpeed * fElapsedTime) * nMapWidth + (int)(fPlayerX - sinf(fPlayerAngle) * fSpeed * fElapsedTime)] == '#') {
            fPlayerX += sinf(fPlayerAngle) * fSpeed * fElapsedTime;
            fPlayerY += cosf(fPlayerAngle) * fSpeed * fElapsedTime;
        }
        bMoving = true;
    }
    if (GetAsyncKeyState((unsigned short)'D') & 0x8000) {
        fPlayerX += cosf(fPlayerAngle) * fSpeed * fElapsedTime * 0.4f;
        fPlayerY -= sinf(fPlayerAngle) * fSpeed * fElapsedTime * 0.4f;
        if (map[(int)(fPlayerY - sinf(fPlayerAngle) * fSpeed * fElapsedTime) * nMapWidth + (int)(fPlayerX + cosf(fPlayerAngle) * fSpeed * fElapsedTime)] == '#') {
            fPlayerX -= cosf(fPlayerAngle) * fSpeed * fElapsedTime;
            fPlayerY += sinf(fPlayerAngle) * fSpeed * fElapsedTime;
        }
        bMoving = true;
    }
}


char getMapChar(float x, float y, int direction) {
    switch (direction) {
        case UP: return map[(int(y) - 1) * nMapWidth + int(x)];
        case DOWN: return map[(int(y) + 1) * nMapWidth + int(x)];
        case LEFT: return map[int(y) * nMapWidth + int(x) - 1];
        case RIGHT: return map[int(y) * nMapWidth + int(x) + 1];
    }
    return WALL; // Default to wall if something goes wrong
}


void updateDirections() {
    directions.clear();
    if (getMapChar(fEnemyX, fEnemyY, UP) == OPEN_SPACE) directions.push_back(UP);
    if (getMapChar(fEnemyX, fEnemyY, DOWN) == OPEN_SPACE) directions.push_back(DOWN);
    if (getMapChar(fEnemyX, fEnemyY, LEFT) == OPEN_SPACE) directions.push_back(LEFT);
    if (getMapChar(fEnemyX, fEnemyY, RIGHT) == OPEN_SPACE) directions.push_back(RIGHT);
}


vector<Node> findPath(int startX, int startY, int endX, int endY)
{
    priority_queue<Node> openList;
    vector<vector<bool>> closedList(nMapHeight, vector<bool>(nMapWidth, false));
    vector<Node> path;

    Node start = { startX, startY, 0.0f, 0.0f, 0.0f, nullptr };
    Node goal = { endX, endY, 0.0f, 0.0f, 0.0f, nullptr };

    openList.push(start);

    auto heuristic = [](int x1, int y1, int x2, int y2) -> float {
        return abs(x1 - x2) + abs(y1 - y2);
    };

    while (!openList.empty())
    {
        Node current = openList.top();
        openList.pop();

        if (current.x == goal.x && current.y == goal.y)
        {
            while (current.parent)
            {
                path.push_back(current);
                current = *current.parent;
            }
            reverse(path.begin(), path.end());
            break;
        }

        closedList[current.y][current.x] = true;

        vector<pair<int, int>> neighbors = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
        for (auto [dx, dy] : neighbors)
        {
            int nx = current.x + dx;
            int ny = current.y + dy;

            if (nx < 0 || ny < 0 || nx >= nMapWidth || ny >= nMapHeight || closedList[ny][nx])
                continue;

            if (map[ny * nMapWidth + nx] == '#')
                continue;

            float gCost = current.gCost + 1.0f;
            float hCost = heuristic(nx, ny, goal.x, goal.y);
            float fCost = gCost + hCost;

            openList.push({ nx, ny, fCost, gCost, hCost, make_shared<Node>(current)});
        }
    }

    return path;
}


void moveRandom() {
    if (directions != lastDirections && fmod(fEnemyX - 0.5f, 1.0f) < 0.1f && fmod(fEnemyY - 0.5f, 1.0f) < 0.1f) 
    {
        lastDirections = directions;
        dir = directions[rand() % directions.size()];
    }
    else if (fmod(fEnemyX - 0.5f, 1.0f) > 0.1f && fmod(fEnemyY - 0.5f, 1.0f) > 0.1f) 
    {
        fEnemyX -= (fEnemyX - float(int(fEnemyX)) + 0.5f) / 10.0f;
        fEnemyY -= (fEnemyY - float(int(fEnemyY)) + 0.5f) / 10.0f;
        lastDirections.clear();
    }
    
    switch (dir) 
    {
        case UP: fEnemyY -= fEnemySpeed; break;
        case DOWN: fEnemyY += fEnemySpeed; break;
        case LEFT: fEnemyX -= fEnemySpeed; break;
        case RIGHT: fEnemyX += fEnemySpeed; break;
    }
}


bool enemySeesPlayer()
{
    for (int nAngle = 0; nAngle < 20; nAngle++)
    {
        float fAngle = nAngle * 3.14159 / 10;
        float fDistance = 0.0f;
        bool bHitWall = false;
        bool bHitPlayer = false;

        float fEyeX = sinf(fAngle);
        float fEyeY = cosf(fAngle);

        while (!bHitWall && !bHitPlayer && fDistance < fDepth)
        {
            fDistance += 0.1;

            int nTestX = (int)(fEnemyX + fEyeX * fDistance);
            int nTestY = (int)(fEnemyY + fEyeY * fDistance);
            float fTestX = (fEnemyX + fEyeX * fDistance);
            float fTestY = (fEnemyY + fEyeY * fDistance);

            if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
            {
                fDistance = fDepth;
                break;
            }
            else if (map[nTestY * nMapWidth + nTestX] == '#')
            {
                bHitWall = true;
            }
            else if (abs(fTestX - fPlayerX) < 3.0 && abs(fTestY - fPlayerY) < 3.0)
            {
                return true;
            }
        }
    }
    return false;
}


void moveHunting(float fElapsedTime) {
    lastDirections = directions;
    float fDirX = fEnemyX - fPlayerX;
    float fDirY = fEnemyY - fPlayerY;
    float fMagnitude = sqrt(fDirX * fDirX + fDirY * fDirY);

    fDirX = fDirX / fMagnitude;
    fDirY = fDirY / fMagnitude;

    float fResAngle = atan2(fDirY, fDirX);

    bool canMoveX = (map[(int)(fEnemyY) * nMapWidth + (int)(fEnemyX - cosf(fResAngle) * fEnemySpeed * fElapsedTime)] != '#');
    bool canMoveY = (map[(int)(fEnemyY - sinf(fResAngle) * fEnemySpeed * fElapsedTime) * nMapWidth + (int)(fEnemyX)] != '#');
    
    if (canMoveY && canMoveX) 
    {
        fEnemyX -= cosf(fResAngle) * fEnemySpeed * fElapsedTime;
        fEnemyY -= sinf(fResAngle) * fEnemySpeed * fElapsedTime;
    }
    else if (!canMoveX && canMoveY) 
    {
        fEnemyY -= sinf(fResAngle) * fEnemySpeed * fElapsedTime / abs(sinf(fResAngle));
    } 
    else if (!canMoveY && canMoveX) 
    {
        fEnemyX -= cosf(fResAngle) * fEnemySpeed * fElapsedTime / abs(cosf(fResAngle));
    }
    else
    {
        bLocked = false;
    }
}


bool isInVector(const vector<int>& vec, int value) {
    // Use std::find to search for the value in the vector
    auto it = find(vec.begin(), vec.end(), value);

    // If the iterator is not equal to vec.end(), the value is found
    return it != vec.end();
}


void moveActive(float fElapsedTime)
{

    auto path = findPath((int)fEnemyX, (int)fEnemyY, (int)fPlayerX, (int)fPlayerY);

    bool bHunting = enemySeesPlayer();

    if (bHunting)
    {
        moveHunting(fElapsedTime);
    }
    else if (!path.empty())
    {
        updateDirections();

        float nextX = (float)(path[0].x) + 0.5f;
        float nextY = (float)(path[0].y) + 0.5f;

        if (fEnemyX < nextX && isInVector(directions, RIGHT))
            fEnemyX += fEnemySpeed * fElapsedTime;
        else if (fEnemyX > nextX && isInVector(directions, LEFT))
            fEnemyX -= fEnemySpeed * fElapsedTime;

        if (fEnemyY < nextY && isInVector(directions, DOWN))
            fEnemyY += fEnemySpeed * fElapsedTime;
        else if (fEnemyY > nextY && isInVector(directions, UP))
            fEnemyY -= fEnemySpeed * fElapsedTime;
    }
    else
    {
        moveRandom();
    }
}


void updateEnemy(float fElapsedTime) {
    updateDirections();
    moveActive(fElapsedTime);
}


void renderOrb(float fOrbX, float fOrbY)
{
    if (fOrbX < 0 || fOrbX >= nMapWidth || fOrbY < 0 || fOrbY >= nMapHeight)
        return;

    float fAngleToOrb = (6.28318f - atan2f(fOrbY - fPlayerY, fOrbX - fPlayerX)) + 1.57f;
    float fSqrDistanceToOrb = (fPlayerX - fOrbX) * (fPlayerX - fOrbX) + (fPlayerY - fOrbY) * (fPlayerY - fOrbY);
    if (fSqrDistanceToOrb < 0.0001f) fSqrDistanceToOrb = 0.0001f;

    if (fSqrDistanceToOrb >= 36.0f)
        return;

    float fDistanceToOrb = sqrtf(fSqrDistanceToOrb);
    float fOrbScale = 0.08f;
    int nOrbRadius = (int)((nScreenHeight / fDistanceToOrb) * fOrbScale);
    float fOrbRadiusSqr = nOrbRadius * nOrbRadius;

    float fRelativeAngle = fPlayerAngle - fAngleToOrb;
    if (fRelativeAngle < -3.14159f) fRelativeAngle += 2 * 3.14159f;
    if (fRelativeAngle > 3.14159f)  fRelativeAngle -= 2 * 3.14159f;

    if (fabs(fRelativeAngle) > fFOV / 2.0f)
        return;

    int nOrbCenterX = (int)((nScreenWidth / 2.0f) - fRelativeAngle * nScreenWidth);
    int nOrbCenterY = (int)((nScreenHeight / 2.0f) - fPlayerPitch + (nScreenHeight / fDistanceToOrb) * 0.3f);

    int xMin = max(0, nOrbCenterX - nOrbRadius);
    int xMax = min(nScreenWidth - 1, nOrbCenterX + nOrbRadius);
    int yMin = max(0, nOrbCenterY - nOrbRadius);
    int yMax = min(nScreenHeight - 1, nOrbCenterY + nOrbRadius);

    wchar_t Shade[] = {0x2588, 0x2593, 0x2592, 0x2591};
    for (int py = yMin; py <= yMax; ++py)
    {
        for (int px = xMin; px <= xMax; ++px)
        {
            float fRelativeDistanceSqr = (py - nOrbCenterY) * (py - nOrbCenterY) + (px - nOrbCenterX) * (px - nOrbCenterX);

            if (fRelativeDistanceSqr <= fOrbRadiusSqr)
            {
                int index = min((int)((fRelativeDistanceSqr / fOrbRadiusSqr) * 4.0f), 3);
                if (renderBuffer[py * nScreenWidth + px] > fDistanceToOrb - 0.5f)
                    screen[py * nScreenWidth + px] = Shade[index];
            }
        }
    }
}


void renderEnemy()
{
    float fAngleToEnemy = (6.28318f - atan2f(fEnemyY - fPlayerY, fEnemyX - fPlayerX)) + 1.57f;

    float fDistanceToEnemy = sqrt((fPlayerX - fEnemyX) * (fPlayerX - fEnemyX) + (fPlayerY - fEnemyY) * (fPlayerY - fEnemyY));
    float fRelativeAngle = fPlayerAngle - fAngleToEnemy;
    if (fRelativeAngle < -3.14159f) fRelativeAngle += 2 * 3.14159f;
    if (fRelativeAngle > 3.14159f)  fRelativeAngle -= 2 * 3.14159f;

    if (fabs(fRelativeAngle) > 0.5f) return;

    int nEnemyCenterX = (int)((nScreenWidth / 2.0f) - fRelativeAngle * nScreenWidth);
    int nEnemyHeight = (int)(10.0f / fDistanceToEnemy);
    int nEnemyWidth = (int)(10.0f / fDistanceToEnemy);

    int nEnemyTop = (nScreenHeight / 2) - nEnemyHeight - fPlayerPitch;
    int nEnemyBottom = (nScreenHeight / 2) + nEnemyHeight - fPlayerPitch;
    int nEnemyLeft = nEnemyCenterX - nEnemyWidth / 2;
    int nEnemyRight = nEnemyCenterX + nEnemyWidth / 2;

    nEnemyTop = max(0, nEnemyTop);
    nEnemyBottom = min(nScreenHeight - 1, nEnemyBottom);
    nEnemyLeft = max(0, nEnemyLeft);
    nEnemyRight = min(nScreenWidth - 1, nEnemyRight);

    // Render the enemy
    bool bEnemyRendered = false;
    for (int y = nEnemyTop; y <= nEnemyBottom; ++y)
    {
        for (int x = nEnemyLeft; x <= nEnemyRight; ++x)
        {
            if (renderBuffer[y * nScreenWidth + x] > fDistanceToEnemy)
            {
                screen[y * nScreenWidth + x] = rand();
                bEnemyRendered = true;
            }
        }
    }
    if (bEnemyRendered && !bBossMusic)
    {
        PlaySound(TEXT("static.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        bBossMusic = true;
    }
    else if (!bEnemyRendered && bBossMusic)
    {
        PlaySound(TEXT("ambient.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        bBossMusic = false;
    }
}


void renderFlat(int x, int y, bool ceiling)
{
    float b;
    float relativeY = ((float)y - nScreenHeight / 2.0f + fPlayerPitch) / ((float)nScreenHeight / 2.0f);
    if (ceiling) b = 1.0f + relativeY;
    else b = 1.0f - relativeY;

    short nnShade = ' ';
    if (b < 0.07)      nnShade = 0x2587;
    else if (b < 0.2)  nnShade = 0x25A0;
    else if (b < 0.4)  nnShade = 0x25AA;
    else if (b < 0.6)  nnShade = 0x00B7;
    else               nnShade = ' ';
    screen[y * nScreenWidth + x] = nnShade;
    renderBuffer[y * nScreenWidth + x] = 100;
}


void calculateVerticalStrip(int x)
{
    float fRayAngle = (fPlayerAngle - fFOV / 2.0f) + ((float)x / (float)nScreenWidth * fFOV);

    float fDistanceToWall = 0.0f;
    bool bHitWall = false, bBoundary = false;

    float fEyeX = sinf(fRayAngle);
    float fEyeY = cosf(fRayAngle);

    while (!bHitWall && fDistanceToWall < fDepth)
    {
        fDistanceToWall += 0.01;

        //Values to check the map for collisions
        int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
        int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);
        float fTestX = (fPlayerX + fEyeX * fDistanceToWall);
        float fTestY = (fPlayerY + fEyeY * fDistanceToWall);

        //Check if the test locations are within the borders of the map
        if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
        {
            fDistanceToWall = fDepth;
            break;
        }
        else if (map[nTestY * nMapWidth + nTestX] == '#')
        {
            bHitWall = true;

            vector<pair<float, float>> p;

            for (int tx = 0; tx < 2; tx++)
                for (int ty = 0; ty < 2; ty++)
                {
                    float vy = (float)nTestY + ty - fPlayerY;
                    float vx = (float)nTestX + tx - fPlayerX;
                    float d = sqrt(vx*vx + vy*vy);
                    float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
                    p.push_back(make_pair(d, dot));
                }

            sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right) {return left.first < right.first;} );

            float fBound = 0.01 / fDistanceToWall;
            if (acos(p.at(0).second) < fBound) bBoundary = true;
            if (acos(p.at(1).second) < fBound) bBoundary = true;

        }
    }

    //Calculate the ceiling
    int nCeiling = (int)((nScreenHeight / 2.0f) - (nScreenHeight / fDistanceToWall) * 1/fFOV);
    int nFloor = nScreenHeight - nCeiling; 

    //Adjust for player pitch
    nCeiling -= fPlayerPitch;
    nFloor -= fPlayerPitch;

    for (int y = 0; y < nScreenHeight; y++)
    {
        if (y < nCeiling)
            renderFlat(x, y, 1);
        else if (y >= nCeiling && y <= nFloor)
        {
            short nShade = ' ';

            if (fDistanceToWall <= fDepth / 7.5f)       nShade = 0x2588;
            else if (fDistanceToWall < fDepth / 5.0f)   nShade = 0x2593;
            else if (fDistanceToWall < fDepth / 4.0f)   nShade = 0x2592;
            else if (fDistanceToWall < fDepth / 3.0f)   nShade = 0x2591;
            else                                        nShade = ' ';

            if (bBoundary)                              nShade = ' ';
            screen[y * nScreenWidth + x] = nShade;
            renderBuffer[y * nScreenWidth + x] = fDistanceToWall;
        }
        else
            renderFlat(x, y, 0);
    }
}


void checkForCollision()
{
    orbs.erase(remove_if(orbs.begin(), orbs.end(), [](const Orb& orb) 
    {
        return abs(fPlayerX - orb.x) < 0.1f && abs(fPlayerY - orb.y) < 0.1f;
    }),
    orbs.end());
}


void drawStats()
{
    for (int nx = 0; nx < (int)fConstStamina * 7; nx++) 
    {
        if (fHealth*7 > nx)
        {
            screen[(nScreenHeight-7) * nScreenWidth + nx + 5] = 0x2588;
            screen[(nScreenHeight-8) * nScreenWidth + nx + 5] = 0x2588;
        }
        else
        {
            screen[(nScreenHeight-7) * nScreenWidth + nx + 5] = ' ';
            screen[(nScreenHeight-8) * nScreenWidth + nx + 5] = ' ';
        }
        if (fStamina*7 > nx)
        {
            screen[(nScreenHeight-4) * nScreenWidth + nx + 5] = 0x2588;
            screen[(nScreenHeight-5) * nScreenWidth + nx + 5] = 0x2588;
            
        }
        else
        {
            screen[(nScreenHeight-4) * nScreenWidth + nx + 5] = ' ';
            screen[(nScreenHeight-5) * nScreenWidth + nx + 5] = ' ';
        }
            
    }
}


int main()
{
    srand(time(0));

    SetConsoleActiveScreenBuffer(hConsole);

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    int orbCount;

    POINT cursorPos;
    GetCursorPos(&cursorPos);
    previousMouseX = cursorPos.x;
    previousMouseY = cursorPos.y;

    resetGame();

    //MAIN GAME LOOP
    while (true) 
    {

        if (GetAsyncKeyState(VK_LSHIFT) & 0x8000 && GetAsyncKeyState(VK_LCONTROL) & 0x8000 && GetAsyncKeyState('B') & 0x0001 )
            debug = !debug;

        for (int i: renderBuffer)
            i = 1000;

        fixWindowSize();

        //Close game condition
        if (bEnd)
            break;


        //START MENU
        while (bStart)
            renderGameStart();


        //PAUSE MENU
        if (GetAsyncKeyState(VK_ESCAPE) & 0x0001)
        {
            clearScreen();

            while (true)
            {
                if (GetAsyncKeyState(VK_ESCAPE) & 0x0001)
                {
                    setupStats();
                    break;
                }
                
                renderPauseMenu();
                if (bEnd)
                    break;
            }
        }

        hwndFocus = GetForegroundWindow();
        hwndConsole = GetConsoleWindow();
        hwndTerminal = GetParent(hwndConsole);

        //Checking if terminal is focused
        bFocused = hwndFocus == hwndTerminal;

        //Delta Time
        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2-tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        //Loop angle around at 2pi
        fPlayerAngle = fmod(fPlayerAngle, 2 * M_PI);
        if (fPlayerAngle < 0)
            fPlayerAngle = 2 * M_PI - 0.01f;

        //Controls
        if (bFocused){

            handleMouse(cursorPos);

            handleSprint(fElapsedTime);

            handleMovement(fElapsedTime);

        }

        //Check for orb collisions
        checkForCollision();
        
        //Enemy
        updateEnemy(fElapsedTime);

        for (int x = 0; x < nScreenWidth; x++) 
            calculateVerticalStrip(x);

        renderEnemy();

        for (auto& orb: orbs)
        {
            renderOrb(orb.x, orb.y);    
        }

        drawStats();

        orbCount = orbs.size();
        drawNumber(270, 80, orbCount);

        if (debug)
        {
            swprintf_s(screen, 110, L"X=%3.2f, Y=%3.2f, eX=%3.2f, eY=%3.2f, eD=%i, A=%3.2f, w=%i, h=%i, W1=%wi, W2=%wi, S=%3.2f, L=%d FPS=%3.2f", 
                  fPlayerX, fPlayerY, fEnemyX, fEnemyY, dir, fPlayerAngle, width, height, hwndFocus, hwndTerminal, fStamina, (int)bLocked, 1/fElapsedTime);
            for (int nx = 0; nx < nMapWidth; nx++)
            {
                for (int ny = 0; ny < nMapHeight; ny++)
                {
                    screen[(4 + ny) * nScreenWidth + nx + 2] = map[ny * nMapWidth + nx];
                    if (nx == (int)fPlayerX && ny == (int)fPlayerY)
                        screen[(4 + ny) * nScreenWidth + nx + 2] = 'P';
                    if (nx == (int)fEnemyX && ny == (int)fEnemyY)
                        screen[(4 + ny) * nScreenWidth + nx + 2] = 'E';
                }
            }
            if (GetAsyncKeyState(VK_RSHIFT) & 0x0001)
                bLocked = !bLocked;
            if (GetAsyncKeyState(VK_DELETE) & 0x0001)
                orbs.clear();
        }


        render();

        if (abs(fPlayerX - fEnemyX) < 0.01f && abs(fPlayerY - fEnemyY) < 0.01f)
            fHealth -= (fHealthDrain * fElapsedTime);

        //GAME OVER - LOSE
        if (fHealth < 0.01f)
        {
            resetGame();
            clearScreen();
            drawString(56, 30, "YOU LOSE", 4);
            render();
            Sleep(3000);
            bStart = true;
        }

        //GAME OVER - WIN
        if (orbCount == 0)
        {
            resetGame();
            clearScreen();
            drawString(59, 30, "YOU WIN", 4);
            render();
            Sleep(3000);
            bStart = true;
        }
    }

    delete[] screen;
    return 0;
}
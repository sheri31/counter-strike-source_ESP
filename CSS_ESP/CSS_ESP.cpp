// CSS_ESP.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <D3d9.h>
#include <d3dx9.h>
#include <dwmapi.h>
#include <Psapi.h>
#include "CSS_ESP.h"
#include <math.h>

LPDIRECT3D9             g_pD3D = NULL; //Direct3D对象
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; //Direct3D设备对象
LPD3DXLINE              g_pLine[MAX_PLAYER] = {0}; //Direct3D线对象
D3DXVECTOR2*            g_pLineArr = NULL; //线段顶点 
CSS_LOCALINFO LocalInfo;
HMODULE g_ModBaseCSS_Client = 0;
HMODULE g_ModBaseCSS_Engine = 0;
HMODULE g_ModBaseCSS_Server = 0;
HANDLE g_hCSS = 0;
HWND g_hwndCSS = 0;
DWORD g_wndWidthCSS = 0;
DWORD g_wndHeightCSS = 0;




VOID Cleanup()
{
    // 释放Line对象
    for (UINT i = 0; i < MAX_PLAYER; i++) {
        if (g_pLine != NULL)
        {
            g_pLine[i]->Release();
        }
    }
    
    //释放Direct3D设备对象
    if (g_pd3dDevice != NULL)
        g_pd3dDevice->Release();

    //释放Direct3D对象
    if (g_pD3D != NULL)
        g_pD3D->Release();

    delete[] g_pLineArr;
}

bool InRange(double angle, PCSS_FOVRANGE pAngleRange) {
    if (angle > pAngleRange->lfSmallAngle && angle < pAngleRange->lfBigAngle) {
        return true;
    }
    return false;
}

HRESULT InitD3D(HWND hWnd)
{
    //创建Direct3D对象, 该对象用来创建Direct3D设备对象
    if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
        return E_FAIL;

    //设置D3DPRESENT_PARAMETERS结构, 准备创建Direct3D设备对象
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

    //创建Direct3D设备对象
    if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp, &g_pd3dDevice)))
    {
        return E_FAIL;
    }

    // 创建Direct3D线对象
    for (UINT i = 0; i < MAX_PLAYER; i++) {
        if (FAILED(D3DXCreateLine(g_pd3dDevice, &g_pLine[i])))
        {
            return E_FAIL;
        }
    }

    g_pLineArr = new D3DXVECTOR2[5 * MAX_PLAYER];

    return S_OK;
}

bool GetGameMem() {
    DWORD dwPlayer = 0;
    DWORD dwBlood = 0;
    DWORD dwRet = 0;

    ReadProcessMemory(g_hCSS, (LPCVOID)((DWORD)g_ModBaseCSS_Server + 0x589868), &LocalInfo.uOtherPlayerCount, 4, &dwRet);

    for (UINT i = 0; i < LocalInfo.uOtherPlayerCount + 1; i++) {
        ReadProcessMemory(g_hCSS, (LPCVOID)((DWORD)g_ModBaseCSS_Server + 0x4F3FCC + 0x10 * i), &dwPlayer, 4, &dwRet);
        ReadProcessMemory(g_hCSS, (LPCVOID)dwPlayer, &LocalInfo.Player[i], 0x1180, &dwRet);
    }

    ReadProcessMemory(g_hCSS, (LPCVOID)((DWORD)g_ModBaseCSS_Engine + 0x47F1B4), &LocalInfo.fMouseAngleY, 4, &dwRet);
    ReadProcessMemory(g_hCSS, (LPCVOID)((DWORD)g_ModBaseCSS_Engine + 0x47F1B8), &LocalInfo.fMouseAngleX, 4, &dwRet);
    ReadProcessMemory(g_hCSS, (LPCVOID)((DWORD)g_ModBaseCSS_Client + 0x5047C0), &LocalInfo.fFOV, 4, &dwRet);
    
    return true;
}

float GetOtherPlayerPosInScreenRadianDiffY(int nIndex) {

    float fTopMouseFOV = LocalInfo.fMouseAngleY + LocalInfo.fFOV / 2;
    float fBottomMouseFOV = LocalInfo.fMouseAngleY - LocalInfo.fFOV / 2;
    float fDiffX = LocalInfo.Player[nIndex].posVec.x - LocalInfo.Player[0].posVec.x;
    float fDiffY = LocalInfo.Player[nIndex].posVec.y - LocalInfo.Player[0].posVec.y;
    float fDiffZ = LocalInfo.Player[nIndex].posVec.z - LocalInfo.Player[0].posVec.z;
    float fDistanceXY = sqrt(fDiffX * fDiffX + fDiffY * fDiffY);
    double lfTargetToWorldAngleY = -atan2(fDiffZ, fDistanceXY) / PI * 180;

    CSS_FOVRANGE RangeMouseY = { 0 };
    RangeMouseY.Y.lfBottomAngle = LocalInfo.fMouseAngleY - LocalInfo.fFOV / 2;
    RangeMouseY.Y.lfTopAngle = LocalInfo.fMouseAngleY + LocalInfo.fFOV / 2;
    //printf("TargetToWorldAngleY = %f\r\n", lfTargetToWorldAngleY);

    //printf("lfTargetToWorldAngleY %f  LocalInfo.fMouseAngleY %f\r\n", lfTargetToWorldAngleY, LocalInfo.fMouseAngleY);
    if (InRange(lfTargetToWorldAngleY, &RangeMouseY) == false) {
        return ERROR_OUT_OF_SCREEN;
    }
    float fTargetRadianInScreenY = (lfTargetToWorldAngleY - LocalInfo.fMouseAngleY) / 180 * PI;

    return fTargetRadianInScreenY;
}

float GetOtherPlayerPosInScreenY(int nIndex) {

    float fTargetRadianInScreenY = GetOtherPlayerPosInScreenRadianDiffY(nIndex);
    if (fTargetRadianInScreenY == ERROR_OUT_OF_SCREEN) {
        return ERROR_OUT_OF_SCREEN;
    }

    float fScreenPosY = tan(fTargetRadianInScreenY) * g_wndHeightCSS / 2 + g_wndHeightCSS / 2;
    //printf("fScreenPosY = %f \r\n", fScreenPosY);
    return fScreenPosY;
}

float GetOtherPlayerPosInScreenRadianDiffX(int nIndex) {
    float fLeftMouseFOV = LocalInfo.fMouseAngleX + LocalInfo.fFOV / 2;
    float fRightMouseFOV = LocalInfo.fMouseAngleX - LocalInfo.fFOV / 2;
    float fDiffX = LocalInfo.Player[nIndex].posVec.x - LocalInfo.Player[0].posVec.x;
    float fDiffY = LocalInfo.Player[nIndex].posVec.y - LocalInfo.Player[0].posVec.y;
    double TargetToWorldAngleX = atan2(fDiffY, fDiffX) / PI * 180;
    //printf("angle = %f\r\n", TargetToWorldAngle);

    CSS_FOVRANGE RangeSameSignWithMouseX = { 0 };
    CSS_FOVRANGE RangeUnsameSignWithMouseX = { 0 };
    bool bBeyondPI = false;
    if (fLeftMouseFOV > 180) {
        bBeyondPI = true;
        RangeSameSignWithMouseX.X.lfLeftAngle = 180;
        RangeSameSignWithMouseX.X.lfRightAngle = fRightMouseFOV;
        RangeUnsameSignWithMouseX.X.lfLeftAngle = fLeftMouseFOV - 360;
        RangeUnsameSignWithMouseX.X.lfRightAngle = -180;
    }
    else if (fRightMouseFOV < -180) {
        bBeyondPI = true;
        RangeSameSignWithMouseX.X.lfLeftAngle = fLeftMouseFOV;
        RangeSameSignWithMouseX.X.lfRightAngle = -180;
        RangeUnsameSignWithMouseX.X.lfLeftAngle = 180;
        RangeUnsameSignWithMouseX.X.lfRightAngle = 360 + fRightMouseFOV;
    }
    else {
        bBeyondPI = false;
        RangeSameSignWithMouseX.X.lfLeftAngle = fLeftMouseFOV;
        RangeSameSignWithMouseX.X.lfRightAngle = fRightMouseFOV;
    }

    double lfTargetRadianInScreenX = ERROR_OUT_OF_SCREEN;
    if (InRange(TargetToWorldAngleX, &RangeSameSignWithMouseX) == true) {
        lfTargetRadianInScreenX = (TargetToWorldAngleX - LocalInfo.fMouseAngleX) / 180 * PI;
    }
    else {
        if (bBeyondPI) {
            if (InRange(TargetToWorldAngleX, &RangeUnsameSignWithMouseX) == true) {
                lfTargetRadianInScreenX = ((LocalInfo.fMouseAngleX < 0 ? -360 : 360) + TargetToWorldAngleX - LocalInfo.fMouseAngleX) / 180 * PI;
            }
        }
    }
    //printf("lfTargetAngleInScreen = %lf  \r\n", lfTargetRadianInScreenX * 180 / PI);

    return lfTargetRadianInScreenX;
}


float GetOtherPlayerPosInScreenX(int nIndex) {
    
    float lfTargetRadianInScreenX = GetOtherPlayerPosInScreenRadianDiffX(nIndex);
    if (lfTargetRadianInScreenX == ERROR_OUT_OF_SCREEN) {
        return ERROR_OUT_OF_SCREEN;//error value
    }

    float fScreenPosX = -tan(lfTargetRadianInScreenX) * g_wndWidthCSS / 2 + g_wndWidthCSS / 2;
    return fScreenPosX;
}


float GetOtherPlayerModelHeightInScreen(int nIndex) {
    float fDiffX = LocalInfo.Player[nIndex].posVec.x - LocalInfo.Player[0].posVec.x;
    float fDiffY = LocalInfo.Player[nIndex].posVec.y - LocalInfo.Player[0].posVec.y;
    float fDistanceXY = sqrt(fDiffX * fDiffX + fDiffY * fDiffY);

    return 400 / fDistanceXY * 100;
}

float GetOtherPlayerModelWidthInScreen(int nIndex) {
    float fDiffX = LocalInfo.Player[nIndex].posVec.x - LocalInfo.Player[0].posVec.x;
    float fDiffY = LocalInfo.Player[nIndex].posVec.y - LocalInfo.Player[0].posVec.y;
    float fDistanceXY = sqrt(fDiffX * fDiffX + fDiffY * fDiffY);

    return 150 / fDistanceXY * 100;
}


bool AutoAim() {

    float fMinScreenDistance = 5000.0f;
    int nIndex = -1;

    for (UINT i = 1; i < LocalInfo.uOtherPlayerCount + 1; i++) {
        if (LocalInfo.Player[i].dwBlood <= 1) {
            continue;
        }

        if (LocalInfo.Player[i].dwCamp == LocalInfo.Player[0].dwCamp) {
            continue;
        }

        float fScreenPosDiffX = GetOtherPlayerPosInScreenX(i) - g_wndWidthCSS / 2;
        float fScreenPosDiffY = GetOtherPlayerPosInScreenY(i) - g_wndHeightCSS / 2;
        if (fScreenPosDiffX != ERROR_OUT_OF_SCREEN && fScreenPosDiffY != ERROR_OUT_OF_SCREEN) {
            float fCurDisTance = sqrt(fScreenPosDiffX * fScreenPosDiffX + fScreenPosDiffY * fScreenPosDiffY);
            if (fMinScreenDistance >= fCurDisTance) {
                fMinScreenDistance = fCurDisTance;
                nIndex = i;
            }
        }
    }

    if (nIndex == -1) {
        //视角没敌人
        return false;
    }

    DWORD dwRet = 0;
    float fFixedMouseAngleX = LocalInfo.fMouseAngleX + GetOtherPlayerPosInScreenRadianDiffX(nIndex) * 180 / PI;
    float fFixedMouseAngleY = LocalInfo.fMouseAngleY + GetOtherPlayerPosInScreenRadianDiffY(nIndex) * 180 / PI + 0.6;
    if (fFixedMouseAngleX == ERROR_OUT_OF_SCREEN || fFixedMouseAngleX == ERROR_OUT_OF_SCREEN) {
        return false;
    }

    WriteProcessMemory(g_hCSS, (LPVOID)((DWORD)g_ModBaseCSS_Engine + 0x47F1B8), &fFixedMouseAngleX, 4, &dwRet);
    WriteProcessMemory(g_hCSS, (LPVOID)((DWORD)g_ModBaseCSS_Engine + 0x47F1B4), &fFixedMouseAngleY, 4, &dwRet);
    return true;
}


VOID Render()
{
    //清空后台缓冲区
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    memset(g_pLineArr, 0, sizeof(g_pLineArr));

    //开始在后台缓冲区绘制图形
    if (SUCCEEDED(g_pd3dDevice->BeginScene()))
    {
        GetGameMem();
        for (UINT i = 1; i < LocalInfo.uOtherPlayerCount + 1; i++) {
            if (LocalInfo.Player[i].dwBlood <= 1) {
                continue;
            }

            if (LocalInfo.Player[i].dwCamp == LocalInfo.Player[0].dwCamp) {
                continue;
            }
            float fScreenPosX = GetOtherPlayerPosInScreenX(i);
            float fScreenPosY = GetOtherPlayerPosInScreenY(i);
            float fModH = GetOtherPlayerModelHeightInScreen(i);
            float fModW = GetOtherPlayerModelWidthInScreen(i);
            //printf("fScreenPosX = %f  \r\n", fScreenPosX);
            if (fScreenPosX != ERROR_OUT_OF_SCREEN && fScreenPosY != ERROR_OUT_OF_SCREEN) {

                g_pLineArr[i * 5 + 0].x = fScreenPosX - fModW / 2;
                g_pLineArr[i * 5 + 0].y = fScreenPosY - fModH / 10;
                g_pLineArr[i * 5 + 1].x = fScreenPosX + fModW / 2;
                g_pLineArr[i * 5 + 1].y = fScreenPosY - fModH / 10;
                g_pLineArr[i * 5 + 2].x = fScreenPosX + fModW / 2;
                g_pLineArr[i * 5 + 2].y = fScreenPosY + fModH;
                g_pLineArr[i * 5 + 3].x = fScreenPosX - fModW / 2;
                g_pLineArr[i * 5 + 3].y = fScreenPosY + fModH;
                g_pLineArr[i * 5 + 4].x = fScreenPosX - fModW / 2;
                g_pLineArr[i * 5 + 4].y = fScreenPosY - fModH / 10;

                //绘制线
                D3DCOLOR color;
                g_pLine[i]->SetWidth(0.2f);
                g_pLine[i]->SetAntialias(TRUE);
                g_pLine[i]->Draw(g_pLineArr + 5 * i, 5, D3DCOLOR_ARGB(255, 255, 0, 0));

            }
        }
        Sleep(100);
        //结束在后台缓冲区渲染图形
        g_pd3dDevice->EndScene();
    }
    //将在后台缓冲区绘制的图形提交到前台缓冲区显示
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}



//-----------------------------------------------------------------------------
// Desc: 消息处理
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        //MARGINS margins;
        //DwmExtendFrameIntoClientArea(hWnd, &margins);

        RegisterHotKey(hWnd, 0x2333, 0, VK_F1);
        return 0;
    case WM_DESTROY:
        Cleanup();
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        Render();
        ValidateRect(hWnd, NULL);
        return 0;
    case WM_HOTKEY:
        if ((UINT)HIWORD(lParam) == VK_F1) {
            AutoAim();
        }
        return 0;
    }


    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main()
{
    HMODULE hModAry[256] = { 0 };
    DWORD dwPid = 0;

    g_hwndCSS = FindWindow(L"Valve001", L"Counter-Strike Source");
    GetWindowThreadProcessId(g_hwndCSS, &dwPid);
    g_hCSS = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
    DWORD cbNeeded = 0;
    EnumProcessModules(g_hCSS, hModAry, sizeof(hModAry), &cbNeeded);

    for (UINT i = 0; i < cbNeeded / sizeof(HMODULE); i++) {
        TCHAR wszModName[MAX_PATH];
        GetModuleFileNameEx(g_hCSS, hModAry[i], wszModName, MAX_PATH);

        if (wcsstr(wszModName, L"server.dll") != 0) {
            g_ModBaseCSS_Server = hModAry[i];
        }
        else if (wcsstr(wszModName, L"engine.dll") != 0) {
            g_ModBaseCSS_Engine = hModAry[i];
        }
        else if (wcsstr(wszModName, L"client.dll") != 0) {
            g_ModBaseCSS_Client = hModAry[i];
        }
        else {
            continue;
        }
    }

    RECT windowRect;
    BOOL b = GetWindowRect(g_hwndCSS, &windowRect);
    g_wndWidthCSS = windowRect.right - windowRect.left;
    g_wndHeightCSS = windowRect.bottom- windowRect.top - 33;

    //注册窗口类
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_VREDRAW | CS_HREDRAW, MsgProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, CreateSolidBrush(0), NULL,
                      L"ClassName", NULL };
    RegisterClassEx(&wc);


    //创建窗口
    HWND hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST, L"ClassName", L"ESP",
        WS_POPUP, windowRect.left, windowRect.top + 33, 2000, 2000, 0, 0, 0, 0);

    //初始化Direct3D
    if (SUCCEEDED(InitD3D(hWnd)))
    {
        //显示主窗口

        SetLayeredWindowAttributes(hWnd, NULL, 1, LWA_ALPHA);
        SetLayeredWindowAttributes(hWnd, NULL, 0, LWA_COLORKEY);
        ShowWindow(hWnd, SW_SHOW);

        //SetForegroundWindow();
        //进入消息循环
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                Render();  //渲染图形
            }
        }
    }
    UnregisterClass(L"ClassName", wc.hInstance);
}
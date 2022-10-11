﻿#include "pch.h"
// Example for D3D9 hook

// Include required libraries
#include "kiero.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <Psapi.h>

#include "imgui/imgui.h"
#include "impl/win32_impl.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "injector/injector.hpp"
#include "gui/gui.h"
#include "cheat/cheat.h"

HANDLE GameProcess = GetCurrentProcess();
DWORD base = cheat::GetBaseAddress(GameProcess);
static HWND window = (HWND)injector::ReadMemory<DWORD>(base + 0x19D504C);

// Create the type of function that we will hook
typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
static EndScene oEndScene = NULL;

typedef long(__stdcall* Reset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
static Reset oReset = NULL;

long __stdcall hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    long result = oReset(pDevice, pPresentationParameters);
    ImGui_ImplDX9_CreateDeviceObjects();

    return result;
}

// Declare the detour function

bool init = false;
long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
    if (!init)
    {
        D3DDEVICE_CREATION_PARAMETERS params;
        pDevice->GetCreationParameters(&params);

        impl::win32::init(params.hFocusWindow);

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(params.hFocusWindow);
        ImGui_ImplDX9_Init(pDevice);

        init = true;
    }

    gui::RenderGUI();
    cheat::HandleCheats();

    return oEndScene(pDevice);
}

int kieroExampleThread()
{
    if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
        switch (kiero::getRenderType())
        {
        case kiero::RenderType::D3D9:
            impl::d3d9::init();
            break;
        }

    return 0;
}

void impl::d3d9::init()
{
    assert(kiero::bind(16, (void**)&oReset, hkReset) == kiero::Status::Success);
    assert(kiero::bind(42, (void**)&oEndScene, hkEndScene) == kiero::Status::Success);
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{
    DisableThreadLibraryCalls(hInstance);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)kieroExampleThread, NULL, 0, NULL);
        break;
    }

    return TRUE;
}

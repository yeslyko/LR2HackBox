#include <ImGuiInjector/DinputHook.hpp>

#include <ImGuiInjector/ImGuiInjector.hpp>
#include <Helpers/Helpers.hpp>
#include <safetyhook.hpp>

#include <iostream>
#include <print>
#include <cassert>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

static void* GetDeviceStatePtr7(HMODULE hModule) {
    IDirectInput7A* pDirectInput = NULL;
    typedef HRESULT(__stdcall* tDirectInputCreateEx)(HINSTANCE hinst,
        DWORD dwVersion,
        REFIID riidltf,
        LPVOID* ppvOut,
        LPUNKNOWN punkOuter);
    tDirectInputCreateEx DirectInputCreateEx = (tDirectInputCreateEx)GetProcAddress(hModule, "DirectInputCreateEx");
    if (DirectInputCreateEx == nullptr) {
        std::println(std::cout, "DirectInputCreateEx not present in dinput.dll");
        return nullptr;
    }
    GUID IID_IDirectInput7A = { 0x9A4CB684,0x236D,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE };
    if (DirectInputCreateEx(GetModuleHandle(NULL), 0x0700, IID_IDirectInput7A, (LPVOID*)&pDirectInput, NULL) != DI_OK) {
        std::println(std::cout, "DirectInputCreateEx failed");
        return nullptr;
    }

    LPDIRECTINPUTDEVICE7A lpdiMouse;
    GUID GUID_SysMouse = { 0x6F1D2B60, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };
    GUID IID_IDirectInputDevice7A = { 0x57D7C6BC,0x2356,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE };
    if (pDirectInput->CreateDeviceEx(GUID_SysMouse, IID_IDirectInputDevice7A, (LPVOID*)&lpdiMouse, NULL) != DI_OK) {
        pDirectInput->Release();
        std::println(std::cout, "Error creating DirectInput7 device");
        return nullptr;
    }

    void** vTable = reinterpret_cast<void**>(mem::FindDMAAddy((uintptr_t)lpdiMouse, { 0x0 }));

    lpdiMouse->Release();
    pDirectInput->Release();

    return vTable[9];
}

static void* GetDeviceStatePtr8(HMODULE hModule) {
    IDirectInput8A* pDirectInput = NULL;
    decltype(DirectInput8Create)* DirectInput8Create = reinterpret_cast<decltype(DirectInput8Create)>(GetProcAddress(hModule, "DirectInput8Create"));
    if (DirectInput8Create == nullptr) {
        std::println(std::cout, "DirectInput8Create not present in dinput8.dll");
        return nullptr;
    }
    GUID IID_IDirectInput8A = { 0xBF798030,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00 };
    if (DirectInput8Create(GetModuleHandle(NULL), 0x0800, IID_IDirectInput8A, (LPVOID*)&pDirectInput, NULL) != DI_OK) {
        std::println(std::cout, "DirectInput8Create failed");
        return nullptr;
    }

    LPDIRECTINPUTDEVICE8A lpdiMouse;
    GUID GUID_SysMouse = { 0x6F1D2B60, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };
    if (pDirectInput->CreateDevice(GUID_SysMouse, &lpdiMouse, NULL) != DI_OK) {
        pDirectInput->Release();
        std::println(std::cout, "Error creating DirectInput8 device");
        return nullptr;
    }

    void** vTable = reinterpret_cast<void**>(mem::FindDMAAddy((uintptr_t)lpdiMouse, { 0x0 }));

    lpdiMouse->Release();
    pDirectInput->Release();

    return vTable[9];
}

int dinput::Hook(safetyhook::InlineHook& result, std::function<HRESULT __stdcall(IDirectInputDevice7A* pDevice, DWORD cbData, LPVOID lpvData)> destination) {
    void* pDestination = *destination.target<HRESULT(__stdcall*)(IDirectInputDevice7A*, DWORD, LPVOID)>();
    assert(pDestination && "Wrong destination prototype");
    int ver = 0;
    HMODULE dinputHandle7 = GetModuleHandle(L"dinput.dll");
    if (dinputHandle7) ver = 7;
    HMODULE dinputHandle8 = GetModuleHandle(L"dinput8.dll");
    if (dinputHandle8) ver = 8;

    void* pGetDeviceData = nullptr;
    switch (ver) {
    case 7: pGetDeviceData = GetDeviceStatePtr7(dinputHandle7); break;
    case 8: pGetDeviceData = GetDeviceStatePtr8(dinputHandle8); break;
    default: 
        std::println(std::cout, "DirectInput not found");
        return 0;
    }
    if (!pGetDeviceData) {
        std::println(std::cout, "Failed to hook DirectInput");
        return 0;
    }
    result = safetyhook::create_inline(pGetDeviceData, pDestination);
    return ver;
}
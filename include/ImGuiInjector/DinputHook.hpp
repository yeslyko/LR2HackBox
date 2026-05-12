#include <functional>
#include <windows.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <safetyhook.hpp>

namespace dinput {
	int Hook(safetyhook::InlineHook& result, std::function<HRESULT __stdcall(IDirectInputDevice7A* pDevice, DWORD cbData, LPVOID lpvData)> destination);
}
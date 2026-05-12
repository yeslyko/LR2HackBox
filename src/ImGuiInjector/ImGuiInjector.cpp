#include <ImGuiInjector/ImGuiInjector.hpp>

#include <iostream>
#include <windowsx.h>

#include <Helpers/Helpers.hpp>
#include <imgui/imgui.h>
#include <imgui_internal.h>
#include <kiero/kiero.h>
#include <minhook/include/MinHook.h>
#include <safetyhook.hpp>

#include <ImGuiInjector/DinputHook.hpp>

#pragma comment (lib, "BaseModels.lib")

#if defined _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

#if KIERO_INCLUDE_D3D9
# include "kiero/examples/imgui/impl/d3d9_impl.h"
#endif

#if KIERO_INCLUDE_D3D10
# include "kiero/examples/imgui/impl/d3d10_impl.h"
#endif

#if KIERO_INCLUDE_D3D11
# include "kiero/examples/imgui/impl/d3d11_impl.h"
#endif

#if KIERO_INCLUDE_D3D12
#endif

#if KIERO_INCLUDE_OPENGL
#endif

#if KIERO_INCLUDE_VULKAN
#endif

#if !KIERO_USE_MINHOOK
# error "The example requires that minhook be enabled!"
#endif

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

const LRESULT WINAPI ImGuiInjector::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ImGuiInjector& self = ImGuiInjector::Get();
    if (self.WndProcHandler(hWnd, msg, wParam, lParam)) {
    	return true;
    }
    return CallWindowProc(self.GetPreviousWndProc(), hWnd, msg, wParam, lParam);
};

ImGuiInjector& ImGuiInjector::Get() {
    static ImGuiInjector instance;
    return instance;
}

ImGuiInjector::ImGuiInjector() {
    ImGuiInjector::mMenus.clear();
    ImGuiInjector::Init();
}

static safetyhook::InlineHook GetDeviceState;
static HRESULT __stdcall OnGetDeviceState(IDirectInputDevice7A* pThis, DWORD cbData, LPVOID lpvData) {
    HRESULT result = GetDeviceState.stdcall<HRESULT>(pThis, cbData, lpvData);
    if (result == DI_OK) {
        ImGuiInjector& imgui = ImGuiInjector::Get();
        if (imgui.WantsMouseInput() && cbData == sizeof(DIMOUSESTATE2)) {
            DIMOUSESTATE2& mouse = *reinterpret_cast<LPDIMOUSESTATE2>(lpvData);
            mouse.rgbButtons[0] = 0;
            mouse.rgbButtons[1] = 0;
            mouse.lZ = 0;
        }
        if (imgui.WantsKeyboardInput() && cbData == 256) {
            memset(lpvData, 0, 256);
        }
    }
    return result;
}

int ImGuiInjector::Init() {
    if (kiero::init(kiero::RenderType::Auto) == kiero::Status::Success)
    {
        switch (kiero::getRenderType())
        {
#if KIERO_INCLUDE_D3D9
        case kiero::RenderType::D3D9:
            std::cout << "Kiero DX9 initialised\n";
            impl::d3d9::init();
            break;
#endif
#if KIERO_INCLUDE_D3D10
        case kiero::RenderType::D3D10:
            std::cout << "Kiero DX10 initialised\n";
            impl::d3d10::init();
            break;
#endif
#if KIERO_INCLUDE_D3D11
        case kiero::RenderType::D3D11:
            std::cout << "Kiero DX11 initialised\n";
            impl::d3d11::init();
            break;
#endif
        case kiero::RenderType::D3D12:
            // TODO: D3D12 implementation?
            break;
        case kiero::RenderType::OpenGL:
            // TODO: OpenGL implementation?
            break;
        case kiero::RenderType::Vulkan:
            // TODO: Vulkan implementation?
            break;
        }
        mDinputVer = dinput::Hook(GetDeviceState, OnGetDeviceState);
        return 1;
    }
    
    mLogger.LogOut("Failed to initialize ImGui");

    return 0;
}

void ImGuiInjector::SetWndProcHook(HWND hWnd) {
    ImGuiInjector::mWindowHandle = hWnd;
    ImGuiInjector::mWndProcPtr = reinterpret_cast<LONG_PTR>(WndProc);
    ImGuiInjector::mPreviousWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(ImGuiInjector::mWindowHandle, GWLP_WNDPROC, ImGuiInjector::mWndProcPtr));
}

HWND ImGuiInjector::GetWindowHandle() {
    return ImGuiInjector::mWindowHandle;
}

WNDPROC ImGuiInjector::GetPreviousWndProc() {
    return ImGuiInjector::mPreviousWndProc;
}

bool ImGuiInjector::WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LPARAM imgui_lParam = lParam;
    if(msg == WM_MOUSEMOVE || msg == WM_NCMOUSEMOVE) {
        RECT r;
        GetClientRect(hWnd, &r);
        int windowRes[2] = { r.right - r.left, r.bottom - r.top };
        float scaling_factor[2] = { mCanvasSize.x / windowRes[0], mCanvasSize.y / windowRes[1] };
        POINT mouse_pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        mouse_pos.x *= scaling_factor[0];
        mouse_pos.y *= scaling_factor[1];

        float canvasAR = mCanvasSize.x / mCanvasSize.y;
        float outputAR = mOutputSize.x / mOutputSize.y;
        if (canvasAR != outputAR) {
            bool horizontal = canvasAR > outputAR;
            auto& pos = horizontal ? mouse_pos.y : mouse_pos.x;
            auto& size = horizontal ? mOutputSize.y : mOutputSize.x;
            float scale = size / (size / canvasAR);
            float& scalePos = horizontal ? scaling_factor[1] : scaling_factor[0];
            pos *= scale;
            pos -= (size - size / canvasAR) / 2 * scalePos * scale;
        }

        imgui_lParam = MAKELPARAM(mouse_pos.x, mouse_pos.y);
    }
    for (auto& menu : mMenus) {
        menu->MessageHandler(hWnd, msg, wParam, imgui_lParam);
    }
    if (IsMenuRunning()) ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, imgui_lParam);

    if (ImGuiInjector::WantsMouseInput()) {
        switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
            return true;
        }
    }

    if (ImGuiInjector::WantsKeyboardInput()) {
        switch (msg) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            return true;
        }
    }

    return false;
}

void ImGuiInjector::AddMenu(ImGuiMenu* pMenu) {
    ImGuiInjector::mMenus.push_back(pMenu);
}

void ImGuiInjector::RunMenus() {
    bool drawImGuiCursor = false;
    for (auto& menu : mMenus) {
        if (menu->IsOpen()) {
            drawImGuiCursor = true;
            menu->Loop();
        }
    }
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = drawImGuiCursor;
}

bool ImGuiInjector::IsMenuRunning() {
    bool isMenuRunning = false;
    for (auto& menu : mMenus) {
        isMenuRunning |= menu->IsOpen();
    }
    return isMenuRunning;
}

bool ImGuiInjector::WantsMouseInput() {
    return mWantsMouseInput;
}

bool ImGuiInjector::WantsKeyboardInput() {
    return mWantsKeyboardInput;
}

void ImGuiInjector::ResetInput() {
    ImGuiIO& io = ImGui::GetIO();
    io.WantCaptureMouse = false;
    io.WantCaptureKeyboard = false;
    mWantsMouseInput = false;
    mWantsKeyboardInput = false;
}

void ImGuiInjector::UpdateInput() {
    ImGuiIO& io = ImGui::GetIO();
    mWantsMouseInput = io.WantCaptureMouse;
    mWantsKeyboardInput = io.WantCaptureKeyboard;
}

void ImGuiInjector::LoadJapaneseFont() {
    ImGuiIO& io = ImGui::GetIO();
    try {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msgothic.ttc", 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    }
    catch (const std::exception& e) {
        std::cout << "Couldn't load font at path C:\\Windows\\Fonts\\msgothic.ttc: " << e.what() << "\n" << std::flush;
    }
}

void ImGuiInjector::SetStartingStyle(const ImGuiStyle& style) {
    mStartingStyle = style;
}

void ImGuiInjector::SetCanvasSize(const ImVec2& size) {
    mCanvasSize = size;
}

void ImGuiInjector::SetOutputSize(const ImVec2& size) {
    mOutputSize = size;
}

void ImGuiInjector::UpdateGlobalScale() {
    if (!mScaleChanged) return;
    mScaleChanged = false;
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiStyle tmpStyle = mStartingStyle;
    float scale = mGlobalScale;
    tmpStyle.ScaleAllSizes(scale);
    tmpStyle.FontScaleMain = scale;
    tmpStyle.MouseCursorScale = scale;

    style.WindowPadding = tmpStyle.WindowPadding;
    style.WindowRounding = tmpStyle.WindowRounding;
    style.WindowMinSize = tmpStyle.WindowMinSize;
    style.WindowBorderHoverPadding = tmpStyle.WindowBorderHoverPadding;
    style.ChildRounding = tmpStyle.ChildRounding;
    style.PopupRounding = tmpStyle.PopupRounding;
    style.FramePadding = tmpStyle.FramePadding;
    style.FrameRounding = tmpStyle.FrameRounding;
    style.ItemSpacing = tmpStyle.ItemSpacing;
    style.ItemInnerSpacing = tmpStyle.ItemInnerSpacing;
    style.CellPadding = tmpStyle.CellPadding;
    style.TouchExtraPadding = tmpStyle.TouchExtraPadding;
    style.IndentSpacing = tmpStyle.IndentSpacing;
    style.ColumnsMinSpacing = tmpStyle.ColumnsMinSpacing;
    style.ScrollbarSize = tmpStyle.ScrollbarSize;
    style.ScrollbarRounding = tmpStyle.ScrollbarRounding;
    style.GrabMinSize = tmpStyle.GrabMinSize;
    style.GrabRounding = tmpStyle.GrabRounding;
    style.LogSliderDeadzone = tmpStyle.LogSliderDeadzone;
    style.ImageBorderSize = tmpStyle.ImageBorderSize;
    style.TabRounding = tmpStyle.TabRounding;
    style.TabCloseButtonMinWidthSelected = tmpStyle.TabCloseButtonMinWidthSelected;
    style.TabCloseButtonMinWidthUnselected = tmpStyle.TabCloseButtonMinWidthUnselected;
    style.TabBarOverlineSize = tmpStyle.TabBarOverlineSize;
    style.TreeLinesRounding = tmpStyle.TreeLinesRounding;
    style.SeparatorTextPadding = tmpStyle.SeparatorTextPadding;
    style.DisplayWindowPadding = tmpStyle.DisplayWindowPadding;
    style.DisplaySafeAreaPadding = tmpStyle.DisplaySafeAreaPadding;
    style.MouseCursorScale = tmpStyle.MouseCursorScale;
    style.FontScaleMain = tmpStyle.FontScaleMain;
}

void ImGuiInjector::SetGlobalScale(float scale) {
    if (std::abs(mGlobalScale - scale) >= std::numeric_limits<float>::epsilon()) mScaleChanged = true;
    mGlobalScale = scale;
}

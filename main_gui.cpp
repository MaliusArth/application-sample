// Dear ImGui: standalone example application for DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imnodes.h"
#include "pipeline.h"
#include <d3d11.h>
#include <tchar.h>
#include <random>

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define MA_SEED __LINE__
#define MA_ID(_str) ImHashStr(_str, strlen(_str), MA_SEED)
#define MA_ID2(_str, _seed) ImHashStr(_str, strlen(_str), _seed)

// void AlignRight(const char* text)
// {
// 	auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(text).x
// 	    - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
// 	if(posX > ImGui::GetCursorPosX())
// 	  ImGui::SetCursorPosX(posX);
// }

struct FileNodeData
{
	int id;
	int attr_input_write;
	int attr_output_read;
	size_t file_path_size;
	char* file_path;
};

void ShowFileNode(FileNodeData* node, const char* name, ImGuiID seed)
{
	node->id = MA_ID2(name, seed);
	// ImGui::DebugLog("node id: %d\n", node->id);
	ImNodes::BeginNode(node->id);

	ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("File");
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginStaticAttribute(MA_ID2("File Path", seed));
		ImGui::SetNextItemWidth(MAX_PATH);
		ImGui::InputText("File Path", node->file_path, node->file_path_size/* , ImGuiInputTextFlags_CallbackEdit */);
	ImNodes::EndStaticAttribute();

	// ImGui::Columns(2);
	// TODO: ImGui::Table
	node->attr_input_write = MA_ID2("Write", seed);
	ImNodes::BeginInputAttribute(node->attr_input_write);
		ImGui::Text("Write");
	ImNodes::EndInputAttribute();
	ImGui::SameLine(); // TODO
	node->attr_output_read = MA_ID2("Read", seed);
	ImNodes::BeginOutputAttribute(node->attr_output_read);
		// std::string text = "Read";
		// TODO: AlignRight(text.c_str());
		// ImGui::Text("%s", text);
		ImGui::Text("Read");
	ImNodes::EndOutputAttribute();

	ImNodes::SetNodeDraggable(node->id, false);
	ImNodes::EndNode();
}

struct RandomIntNodeData
{
	int id;
	int attr_output_int;
	unsigned int* seed;
};

void ShowRandomIntNode(RandomIntNodeData* node, const char* name, ImGuiID seed)
{
	node->id = MA_ID2(name, seed);
	ImNodes::BeginNode(node->id);

	ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("Random Int");
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginStaticAttribute(MA_ID2("Seed", seed));
		ImGui::SetNextItemWidth(240);
		ImGui::InputScalar("Seed", ImGuiDataType_U32, node->seed, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
	ImNodes::EndStaticAttribute();

	node->attr_output_int = MA_ID2("Int", seed);
	ImNodes::BeginOutputAttribute(node->attr_output_int);
		ImGui::Text("Int");
	ImNodes::EndOutputAttribute();

	ImNodes::SetNodeDraggable(node->id, false);
	ImNodes::EndNode();
}

struct AddNodeData
{
	int id;
	int attr_input_term_a, attr_input_term_b;
	int attr_output_result;
};

void ShowAddNode(AddNodeData* node, const char* name, ImGuiID seed)
{
	node->id = MA_ID2(name, seed);
	ImNodes::BeginNode(node->id);

	ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("Add");
	ImNodes::EndNodeTitleBar();

	node->attr_input_term_a = MA_ID2("A", seed);
	ImNodes::BeginInputAttribute(node->attr_input_term_a);
		ImGui::Text("A");
	ImNodes::EndInputAttribute();

	node->attr_input_term_b = MA_ID2("B", seed);
	ImNodes::BeginInputAttribute(node->attr_input_term_b);
		ImGui::Text("B");
	ImNodes::EndInputAttribute();

	node->attr_output_result = MA_ID2("Result", seed);
	ImNodes::BeginOutputAttribute(node->attr_output_result);
		ImGui::Text("Result");
	ImNodes::EndOutputAttribute();

	ImNodes::SetNodeDraggable(node->id, false);
	ImNodes::EndNode();
}

// Main code
int main(int, char**)
{
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Pipeline", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	/* ImNodes::ImNodesContext* context=*/ImNodes::CreateContext();
	// ImNodes::EditorContextResetPanning({0,0});
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;
	//io.ConfigViewportsNoDefaultParent = true;
	//io.ConfigDockingAlwaysTabBar = true;
	//io.ConfigDockingTransparentPayload = true;
	//io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
	//io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	static const float MARGIN = 10;
	bool show_message = false;
	// char message_buffer[MAX_PATH] = {};
	std::string pipeline_error = {};
	char input_file_path_buffer[MAX_PATH] = "data.json";
	char output_file_path_buffer[MAX_PATH] = "result.txt";
	unsigned int random_seed = 0x5702135;

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Handle window being minimized or screen locked
		if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
			continue;
		}
		g_SwapChainOccluded = false;

		// Handle window resize (we don't resize directly in the WM_SIZE handler)
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		auto dockspace_id = ImGui::GetID("Dock");
		ImGui::DockSpaceOverViewport(dockspace_id, nullptr,
			// ImGuiDockNodeFlags_CentralNode |
			ImGuiDockNodeFlags_NoUndocking |
			ImGuiDockNodeFlags_NoResize |
			ImGuiDockNodeFlags_HiddenTabBar
		);

		ImGui::DockBuilderDockWindow("Pipeline", dockspace_id);
		ImGui::DockBuilderFinish(dockspace_id);

		{
			ImGui::Begin("Pipeline");
			bool execute_pipeline = false;
			if (ImGui::Button("Execute pipeline"))
			{
				show_message = false;
				execute_pipeline = true;
			}

			if (show_message)
			{
				if (pipeline_error.empty())
				{
					ImGui::SameLine();
					ImGui::TextColored(ImColor(0, 200, 0, 255).Value, "Success!");
				}
				else
				{
					ImGui::SameLine();
					ImGui::TextColored(ImColor(200, 0, 0, 255).Value, pipeline_error.data());
				}
			}
			// ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

			ImNodes::BeginNodeEditor();

			FileNodeData node_input_file = {};
			node_input_file.file_path_size = IM_ARRAYSIZE(input_file_path_buffer);
			node_input_file.file_path = input_file_path_buffer;
			ShowFileNode(&node_input_file, "Input File Node", MA_SEED);
			ImNodes::SetNodeGridSpacePos(node_input_file.id, {MARGIN, MARGIN});

			RandomIntNodeData node_random_int = {};
			node_random_int.seed = &random_seed;
			ShowRandomIntNode(&node_random_int, "Random Node", MA_SEED);
			ImNodes::SetNodeGridSpacePos(node_random_int.id, {MARGIN, 100});

			AddNodeData node_add = {};
			ShowAddNode(&node_add, "Add Node", MA_SEED);
			ImVec2 node_input_file_dimensions = ImNodes::GetNodeDimensions(node_input_file.id);
			ImVec2 node_random_int_dimensions = ImNodes::GetNodeDimensions(node_random_int.id);
			ImVec2 node_add_dimensions = ImNodes::GetNodeDimensions(node_add.id);
			ImVec2 node_add_position = {};
			node_add_position.x = max(node_input_file_dimensions.x, node_random_int_dimensions.x)+3*MARGIN;
			node_add_position.y = (MARGIN+(node_input_file_dimensions.y+node_random_int_dimensions.y+2*MARGIN)/2.0f)-(node_add_dimensions.y/2.0f);
			ImNodes::SetNodeGridSpacePos(node_add.id, node_add_position);

			FileNodeData node_output_file = {};
			node_output_file.file_path_size = IM_ARRAYSIZE(output_file_path_buffer);
			node_output_file.file_path = output_file_path_buffer;
			ShowFileNode(&node_output_file, "Output File Node", MA_SEED);
			ImVec2 node_output_file_position = {};
			node_output_file_position.x = node_add_position.x+node_add_dimensions.x+2*MARGIN;
			node_output_file_position.y = node_add_position.y;
			ImNodes::SetNodeGridSpacePos(node_output_file.id, node_output_file_position);

			ImNodes::Link(MA_ID("Link"), node_input_file.attr_output_read, node_add.attr_input_term_a);
			ImNodes::Link(MA_ID("Link"), node_random_int.attr_output_int, node_add.attr_input_term_b);
			ImNodes::Link(MA_ID("Link"), node_add.attr_output_result, node_output_file.attr_input_write);

			ImNodes::EndNodeEditor();
			ImGui::End();

			if (execute_pipeline)
			{
				// char error_message_buffer[256];
				show_message = true;
				pipeline_error = ExecutePipeline(
					{node_input_file.file_path, node_input_file.file_path_size},
					node_random_int.seed,
					{node_output_file.file_path, node_output_file.file_path_size}
				);
			}
		}

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		// Present
		HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
		//HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
		g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImNodes::DestroyContext();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	case WM_DPICHANGED:
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
		{
			//const int dpi = HIWORD(wParam);
			//printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
			const RECT* suggested_rect = (RECT*)lParam;
			::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

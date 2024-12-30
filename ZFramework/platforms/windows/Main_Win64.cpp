#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "shellapi.h"
#include <commctrl.h>
#include <fstream>

#include "helpers/StringHelpers.h"
#include "helpers/CommandLineParser.h"
#include "helpers/Registry.h"
#include "ZDebug.h"
#include "helpers/Logger.h"
#include "ZInput.h"
#include "ZTickManager.h"
#include "ZGraphicSystem.h"
#include "ZScreenBuffer.h"
#include "ZTimer.h"
#include "ZMainWin.h"
#include "ZAnimator.h"

const char* szAppClass = "ZImageViewer";

namespace ZFrameworkApp
{
    extern bool InitRegistry(std::filesystem::path userDataPath);
    extern bool Initialize(int argc, char* argv[], std::filesystem::path userDataPath);
    extern void Shutdown();
};

extern ZTickManager     gTickManager;

using namespace std;


ATOM					MyRegisterClass(HINSTANCE, LPTSTR);
BOOL					WinInitInstance(int argc, char* argv[]);
extern bool             gbGraphicSystemResetNeeded; 
bool                    gbApplicationExiting = false;
bool                    gbApplicationRestart = false;
ZRect                   grFullArea;
ZRect                   grWindowedArea;
ZRect                   grFullScreenArea;


extern bool             gbPaused = false;
bool                    gbWindowSizeChanged = false;
bool                    gbRenderingEnabled = true;
HINSTANCE               g_hInst;				// The current instance
HWND                    ghWnd;

LRESULT CALLBACK        WndProc(HWND, UINT, WPARAM, LPARAM);

const uint64_t          kMinUSBetweenLoops = 8333;  // 120 fps max?

ZInput                  gInput;
// For resizing the window... it scales the mouse movements
float                   gfMouseMultX = 1.0f;
float                   gfMouseMultY = 1.0f;

ZDebug                  gDebug;


void HandleWindowSizeChanged();
void SwitchFullscreen(bool bFullscreen);


class Win64AppMessageHandler : public IMessageTarget
{
public:
    std::string GetTargetName() { return "Win64AppMessageHandler"; }
    bool        ReceiveMessage(const ZMessage& message)
    {
        string type = message.GetType();
        if (type == "toggle_fullscreen")
        {
            SwitchFullscreen(!gGraphicSystem.mbFullScreen);
        }
        return true;
    }
};


namespace fs = std::filesystem;

int processFolder(const fs::path& folderPath) 
{
    int nCount = 0;
    for (const auto& entry : fs::directory_iterator(folderPath) )
    {
        try 
        {
            if (fs::is_directory(entry.status())) 
            {
                std::cout << entry.path() << std::endl;
                nCount++;
                // You can perform additional operations on the folder here
            }
        }
        catch (const std::exception& ex) 
        {
            std::cerr << "Error processing " << entry.path() << ": " << ex.what() << std::endl;
        }
    }
    return nCount;
}



int main(int argc, char* argv[])
{

//#define TEST_MESSAGE_POSTING
#ifdef TEST_MESSAGE_POSTING


    uint64_t nStart = gTimer.GetUSSinceEpoch();
    const int kIters = 1 * 1000 * 1000;


    int64_t count = 0;

    for (int i = 0; i < kIters; i++)
    {
        gMessageSystem.Post(ZMessage("cursor_msg", &appMessageHandler, "subtype", "move", "x", i, "y", i));

        count++;

/*        ZMessage cursorMessage;
        cursorMessage.SetType("cursor_msg");
        cursorMessage.SetParam("subtype", "move");
        cursorMessage.SetParam("x", SH::FromInt(i));
        cursorMessage.SetParam("y", SH::FromInt(i));
        cursorMessage.SetTarget("blah");

        gMessageSystem.Post(cursorMessage);*/

        




    }

    uint64_t nEnd = gTimer.GetUSSinceEpoch();

    uint64_t nDelta = nEnd - nStart;


    cout << "messages: " << count << " time:" << nDelta << "\n";

    int stophere = 5;

#endif




    // Enable exception handling
    SetUnhandledExceptionFilter([](PEXCEPTION_POINTERS exceptionInfo) -> LONG {
        std::cerr << "Unhandled exception: " << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
        ZERROR("Unhandled exception: ", exceptionInfo->ExceptionRecord->ExceptionCode, "\n");
        gLogger.Flush();
        //while (1);
        return EXCEPTION_EXECUTE_HANDLER;
        });

    string sUserPath(getenv("APPDATA"));
    std::filesystem::path userDataPath(sUserPath);

    gLogger.msLogFilename = sUserPath + "/ZImageViewer/" + szAppClass + ".log";

    ZFrameworkApp::InitRegistry(userDataPath);

    // Perform application initialization:
    if (!WinInitInstance(argc, argv))
        return FALSE;




    if (!ZFrameworkApp::Initialize(argc, argv, userDataPath))
        return FALSE;


    uint64_t nTimeStamp = 0;
    uint64_t nLogFlushTimeStamp = gTimer.GetUSSinceEpoch();

    Win64AppMessageHandler appMessageHandler;
    gMessageSystem.AddNotification("toggle_fullscreen", &appMessageHandler);

    // Main message loop:
    MSG msg;
    memset(&msg, 0, sizeof(msg));
    while (msg.message != WM_QUIT && !gbApplicationExiting)
    {
        if (gbPaused)
        {
            gMessageSystem.Process();  // Process messages internally even when paused

            GetMessage(&msg, NULL, 0, 0);
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            std::this_thread::sleep_for(std::chrono::microseconds(20000));
        }
        else
        {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (!gbPaused && !gbApplicationExiting)
        {
            if (gbWindowSizeChanged)
                HandleWindowSizeChanged();

            gMessageSystem.Process();
            gTickManager.Tick();

            if (!gbApplicationExiting)  // have to check again because the state may have changed during the messagesystem process
            {
                gInput.Process();

                ZScreenBuffer* pScreenBuffer = gGraphicSystem.GetScreenBuffer();
                if (pScreenBuffer && gbRenderingEnabled)
                {
                    // Copy to our window surface
                    pScreenBuffer->BeginRender();

                    if (pScreenBuffer->DoesVisibilityNeedComputing())
                    {
                        int64_t nStartTime = gTimer.GetUSSinceEpoch();
                        pScreenBuffer->ResetVisibilityList();
                        if (gpMainWin->ComputeVisibility())
                            pScreenBuffer->SetVisibilityComputingFlag(false);   // Only clear the flag if all visibility was computed successfully

                        int64_t nEndTime = gTimer.GetUSSinceEpoch();

                        // ZOUT("Computing visibility took time:%lld us. Rects:%d\n", nEndTime - nStartTime, pScreenBuffer->GetVisibilityCount());
                    }


                    static int64_t nTotalRenderTime = 0;
                    static int64_t nTotalFrames = 0;

                    int64_t nStartRenderVisible = gTimer.GetUSSinceEpoch();

/*                    if (gAnimator.HasFullScreenDrawObjects())
                    {
                        gAnimator.Paint();
                    }
                    else
                    {*/
                    //pScreenBuffer->Fill(0xFF0000FF); // debug fill
                        int32_t nRenderedCount = pScreenBuffer->RenderVisibleRects();
                        gAnimator.Paint(pScreenBuffer);
                        gInput.Paint(pScreenBuffer);    // tooltips or any other overlays
                        pScreenBuffer->PaintToSystem();
                    //}
                    int64_t nEndRenderVisible = gTimer.GetUSSinceEpoch();

                    int64_t nDelta = nEndRenderVisible - nStartRenderVisible;
                    nTotalRenderTime += nDelta;
                    nTotalFrames += 1;
                    // ZOUT("render took time:%lld us. Rects:%d/%d. Total Frames:%d, avg frame time:%lld us\n", nEndRenderVisible - nStartRenderVisible, nRenderedCount, pScreenBuffer->GetVisibilityCount(), nTotalFrames, (nTotalRenderTime/nTotalFrames));

                    pScreenBuffer->EndRender();
                    InvalidateRect(gpGraphicSystem->GetMainHWND(), NULL, false);
                }

                if (gbGraphicSystemResetNeeded && !gbPaused && gbRenderingEnabled)
                {
                    ZDEBUG_OUT("calling HandleModeChanges\n");

                    if (gpGraphicSystem->HandleModeChanges(grFullArea))
                    {
                        gbGraphicSystemResetNeeded = false;
                    }
                }
            }

            gDebug.Flush();
            //            FlushDebugOutQueue();
        }
        uint64_t nNewTime = gTimer.GetUSSinceEpoch();
        uint64_t nTimeSinceLastLoop = nNewTime - nTimeStamp;
        //            ZOUT("US since last loop:%lld\n", nTimeSinceLastLoop);
        nTimeStamp = nNewTime;

        if (nTimeSinceLastLoop < kMinUSBetweenLoops)
        {
            uint64_t nUSToSleep = kMinUSBetweenLoops - nTimeSinceLastLoop;
            //                ZOUT("Maintaining min frame time. Sleeping for %lldus.\n", nUSToSleep);
            std::this_thread::sleep_for(std::chrono::microseconds(nUSToSleep));
            //                nTimeStamp = gTimer.GetUSSinceEpoch();
        }

        uint64_t nTimeSinceLastLogFlush = nLogFlushTimeStamp - nNewTime;
        if (nTimeSinceLastLogFlush > 1000000)
        {
            nTimeSinceLastLogFlush = nNewTime;
            gLogger.Flush();
        }
    }


    ZFrameworkApp::Shutdown();

    gDebug.Flush();

    if (gbApplicationRestart)
    {
        ShellExecute(0, "open", argv[0], nullptr, 0, SW_SHOWNORMAL);
    }


    return 0;
}


//#define GENERATE_FRAMEWORK_FONTS
#ifdef GENERATE_FRAMEWORK_FONTS

void GenerateFrameworkFonts()
{
    for (auto fontName : frameworkFontNames)
    {
        for (auto fontSize : frameworkFontSizes)
        {
            string sFontFilename;
            Sprintf(sFontFilename, "res/default_resources/%s_%d.zfont", fontName.c_str(), fontSize);

            std::ifstream fontFile;    // open and set pointer at end
            fontFile.open(sFontFilename.c_str(), ios::in | ios::binary | ios::ate);

            if (!fontFile.is_open())
            {
                ZFontParams params;

                params.nHeight = fontSize;
                params.bItalic = false;
                params.bUnderline = false;
                params.bStrikeOut = false;
                params.bFixedWidth = false;
                params.sFacename = fontName;
                if (fontSize < 24)
                {
                    params.nTracking = 0;
                    params.nWeight = 200;
                }
                else
                {
                    params.nTracking = (int64_t)(sqrt(fontSize / 10));
                    params.nWeight = 600;
                }

                tZFontPtr pFont = gpFontSystem->CreateFont(params);

                pFont->SaveFont(sFontFilename);
            }
            else
            {
                ZOUT("Font already exists! %s\n", sFontFilename.c_str());
            }
        }
    }
}



#endif //  GENERATE_FRAMEWORK_FONTS



void SizeWindowToClientArea(HWND hWnd, int left, int top, int nWidth, int nHeight)
{
    RECT rcClient, rcWindow;
    POINT ptDiff;
    GetClientRect(hWnd, &rcClient);
    GetWindowRect(hWnd, &rcWindow);
    ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
    ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
    MoveWindow(hWnd, left, top, nWidth + ptDiff.x, nHeight + ptDiff.y, TRUE);
}


int WinMain(HINSTANCE hInst, HINSTANCE hPrev, PSTR cmdline, int cmdshow)
{
    return main(__argc, __argv);
}


BOOL WinInitInstance(int argc, char* argv[])
{
    g_hInst = GetModuleHandle(nullptr);

	WNDCLASS	wc;
	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hInst;
	wc.hIcon = LoadIcon(g_hInst, szAppClass);
	wc.hCursor = LoadCursor(g_hInst, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = szAppClass;
	RegisterClass(&wc);

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

    // Default width/height
//    gInitWindowSize.x = GetSystemMetrics(SM_CXSCREEN) * 3 / 4;
//    gInitWindowSize.y = GetSystemMetrics(SM_CYSCREEN) * 3 / 4;
    grFullScreenArea.SetRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    grWindowedArea.SetRect(0, 0, GetSystemMetrics(SM_CXSCREEN) * 3 / 4, GetSystemMetrics(SM_CYSCREEN) * 3 / 4);
    grWindowedArea.MoveRect((GetSystemMetrics(SM_CXSCREEN) - grWindowedArea.Width()) / 2, (GetSystemMetrics(SM_CYSCREEN) - grWindowedArea.Height()) / 2);



    // if registry settings persisted
    if (!gRegistry.Get("appwin", "window_l", grWindowedArea.left))
        gRegistry.SetDefault("appwin", "window_l", grWindowedArea.left);
    if (!gRegistry.Get("appwin", "window_t", grWindowedArea.top))
        gRegistry.SetDefault("appwin", "window_t", grWindowedArea.top);
    if (!gRegistry.Get("appwin", "window_r", grWindowedArea.right))
        gRegistry.SetDefault("appwin", "window_r", grWindowedArea.right);
    if (!gRegistry.Get("appwin", "window_b", grWindowedArea.bottom))
        gRegistry.SetDefault("appwin", "window_b", grWindowedArea.bottom);

    if (!gRegistry.Get("appwin", "full_l", grFullScreenArea.left))
        gRegistry.SetDefault("appwin", "full_l", grFullScreenArea.left);
    if (!gRegistry.Get("appwin", "full_t", grFullScreenArea.top))
        gRegistry.SetDefault("appwin", "full_t", grFullScreenArea.top);
    if (!gRegistry.Get("appwin", "full_r", grFullScreenArea.right))
        gRegistry.SetDefault("appwin", "full_r", grFullScreenArea.right);
    if (!gRegistry.Get("appwin", "full_b", grFullScreenArea.bottom))
        gRegistry.SetDefault("appwin", "full_b", grFullScreenArea.bottom);

    if (!gRegistry.Get("appwin", "fullscreen", gGraphicSystem.mbFullScreen))
        gRegistry.SetDefault("appwin", "fullscreen", gGraphicSystem.mbFullScreen);


    // Finally if any command line overrides
    CLP::CommandLineParser parser;
    int64_t width = 0;
    int64_t height = 0;
    parser.RegisterParam(CLP::ParamDesc("width", &width, CLP::kNamed));
    parser.RegisterParam(CLP::ParamDesc("height", &height, CLP::kNamed));
    parser.RegisterParam(CLP::ParamDesc("fullscreen", &gGraphicSystem.mbFullScreen, CLP::kNamed));
    parser.Parse(argc, argv);
    if (parser.GetParamWasFound("width"))
        grWindowedArea.right = grWindowedArea.left + width;
    if (parser.GetParamWasFound("height"))
        grWindowedArea.bottom = grWindowedArea.top + height;





    if (gGraphicSystem.mbFullScreen)
    {
        ghWnd = CreateWindow(szAppClass, szAppClass, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_hInst, NULL);
        grFullArea.SetRect(0, 0, grFullScreenArea.Width(), grFullScreenArea.Height());
        SizeWindowToClientArea(ghWnd, (int)grFullScreenArea.left, (int)grFullScreenArea.top, (int)grFullArea.Width(), (int)grFullArea.Height());
    }
    else
    {
        ghWnd = CreateWindow(szAppClass, szAppClass, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_hInst, NULL);
        grFullArea.SetRect(0, 0, grWindowedArea.Width(), grWindowedArea.Height());
        SizeWindowToClientArea(ghWnd, (int)grWindowedArea.left, (int)grWindowedArea.top, (int)grWindowedArea.Width(), (int)grWindowedArea.Height());
    }

    if (!ghWnd)
    {
        return FALSE;
    }

	gGraphicSystem.SetHWND(ghWnd);


    HCURSOR hCursor = ::LoadCursor(NULL, IDC_ARROW);
    ::SetCursor(hCursor);

	ShowWindow(ghWnd, SW_SHOW);
	UpdateWindow(ghWnd);

	return TRUE;
}

void SwitchFullscreen(bool bFullscreen)
{
    if (gGraphicSystem.mbFullScreen == bFullscreen)
        return;

    gGraphicSystem.mbFullScreen = bFullscreen;

//    PAINTSTRUCT ps;
//    HDC hdc = BeginPaint(ghWnd, &ps);
    if (gGraphicSystem.mbFullScreen)
    {
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);

        HMONITOR h = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
        GetMonitorInfo(h, &mi);
        grFullScreenArea.SetRect(mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom);

        DWORD nStyle = GetWindowLong(ghWnd, GWL_STYLE);
        DWORD dwRemove = WS_OVERLAPPEDWINDOW;
        nStyle &= ~dwRemove;

        SetWindowLongPtr(ghWnd, GWL_STYLE, nStyle);
        MoveWindow(ghWnd, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, TRUE);

    }
    else
    {
        RECT rW;
        rW.left = (LONG)grWindowedArea.left;
        rW.top = (LONG)grWindowedArea.top;
        rW.right = (LONG)grWindowedArea.right;
        rW.bottom = (LONG)grWindowedArea.bottom;

        DWORD nStyle = GetWindowLong(ghWnd, GWL_STYLE);
        DWORD dwAdd = WS_OVERLAPPEDWINDOW;
        nStyle |= dwAdd;

        SetWindowLongPtr(ghWnd, GWL_STYLE, nStyle);
        MoveWindow(ghWnd, (int)grWindowedArea.left, (int)grWindowedArea.top, (int)grWindowedArea.Width(), (int)grWindowedArea.Height(), TRUE);
    }

    gbWindowSizeChanged = true;
//    HandleWindowSizeChanged();
//    EndPaint(ghWnd, &ps);
}

void HandleWindowSizeChanged()
{
    if (!gpGraphicSystem || !gpGraphicSystem->GetScreenBuffer() || !gpMainWin)
        return;

    gbWindowSizeChanged = false;

    RECT rW;
    GetWindowRect(ghWnd, &rW);
    if (gGraphicSystem.mbFullScreen)
    {
        grFullScreenArea.left = rW.left;
        grFullScreenArea.top = rW.top;
        grFullScreenArea.right = rW.right;
        grFullScreenArea.bottom = rW.bottom;

        gRegistry.Set("appwin", "full_l", grFullScreenArea.left);
        gRegistry.Set("appwin", "full_t", grFullScreenArea.top);
        gRegistry.Set("appwin", "full_r", grFullScreenArea.right);
        gRegistry.Set("appwin", "full_b", grFullScreenArea.bottom);
    }
    else
    {
        grWindowedArea.left = rW.left;
        grWindowedArea.top = rW.top;
        grWindowedArea.right = rW.right;
        grWindowedArea.bottom = rW.bottom;

        gRegistry.Set("appwin", "window_l", grWindowedArea.left);
        gRegistry.Set("appwin", "window_t", grWindowedArea.top);
        gRegistry.Set("appwin", "window_r", grWindowedArea.right);
        gRegistry.Set("appwin", "window_b", grWindowedArea.bottom);
    }

    RECT rC;
    GetClientRect(ghWnd, &rC);

    // if minimized, stop rendering
    if (rC.right - rC.left == 0 || rC.bottom - rC.top == 0)
        gbRenderingEnabled = false;
    else
        gbRenderingEnabled = true;

    if (gbRenderingEnabled)
    {
        if (grFullArea.right != rC.right || grFullArea.bottom != rC.bottom)
        {
            grFullArea.SetRect(0, 0, rC.right, rC.bottom);
            ZGUI::ComputeLooks();

            gRegistry.Set("appwin", "fullscreen", gGraphicSystem.mbFullScreen);

            gpGraphicSystem->HandleModeChanges(grFullArea);
            gpMainWin->SetArea(grFullArea);
            gGraphicSystem.GetScreenBuffer()->SetVisibilityComputingFlag(true);
        }
    }

    static int count = 0;
    ZOUT("done handlesizechanged:", count++, "\n");
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{   
	if (gbApplicationExiting)
		return 0;

	int wmId, wmEvent;

    static int count = 0;
	switch (message) 
	{
    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = 1600;
        lpMMI->ptMinTrackSize.y = 1024;
    }
    break;
    case WM_WINDOWPOSCHANGED:
    {
        WINDOWPOS* pPos = (WINDOWPOS*)lParam;
        gbWindowSizeChanged = true;
    }
        break;
    case WM_SYSCOMMAND:
        DefWindowProc(hWnd, message, wParam, lParam);
        if (wParam == SC_MAXIMIZE || wParam == SC_RESTORE)
        {
            gbWindowSizeChanged = true;
//            HandleWindowSizeChanged();
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_ENTERSIZEMOVE:
//        gbSizing = true;
        break;
    case WM_EXITSIZEMOVE:
        DefWindowProc(hWnd, message, wParam, lParam);
        gbWindowSizeChanged = true;
        break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{	
		case IDOK:
			SendMessage (hWnd, WM_CLOSE, 0, 0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		return TRUE;
	case WM_DESTROY:
//        ZFrameworkApp::Shutdown();
        gbApplicationExiting = true;
		break;
	case WM_CHAR:
        gInput.OnChar((uint32_t)wParam);
        break;
    case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
        gInput.OnKeyDown((uint32_t)wParam);
        if (wParam == VK_RETURN && gInput.IsKeyDown(VK_MENU))  // alt-enter
        {
            SwitchFullscreen(!gGraphicSystem.mbFullScreen);
        }
        else if (wParam == VK_OEM_3)    // ~
        {
            gMessageSystem.Post(ZMessage("toggleconsole"));
        }
        else if (wParam == 'f' || wParam == 'F')
        {
            if (gInput.IsKeyDown(VK_CONTROL))
                SwitchFullscreen(!gGraphicSystem.mbFullScreen);
        }
        else if (wParam == 'O')
        {
            gMessageSystem.Post(ZMessage("{toggleoverlay;target=MainAppMessageTarget}"));
        }
#ifdef _WIN32
        else if (wParam == 'L' && gInput.IsKeyDown(VK_CONTROL))
        {
            string sURL;
            ShellExecute(0, "open", gLogger.msLogFilename.c_str(), 0, 0, SW_SHOWNORMAL);

            return true;
        }
#endif
        break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        gInput.OnKeyUp((uint32_t)wParam);
		break;
	case WM_LBUTTONDOWN:
        gInput.OnLButtonDown((int64_t)((float)LOWORD(lParam) / gfMouseMultX), (int64_t)((float)HIWORD(lParam) / gfMouseMultY));
		break;
	case WM_LBUTTONUP:
        gInput.OnLButtonUp((int64_t)((float)LOWORD(lParam) / gfMouseMultX), (int64_t)((float)HIWORD(lParam) / gfMouseMultY));
        break;
    case WM_RBUTTONDOWN:
        gInput.OnRButtonDown((int64_t)((float)LOWORD(lParam) / gfMouseMultX), (int64_t)((float)HIWORD(lParam) / gfMouseMultY));
        break;
    case WM_RBUTTONUP:
        gInput.OnRButtonUp((int64_t)((float)LOWORD(lParam) / gfMouseMultX), (int64_t)((float)HIWORD(lParam) / gfMouseMultY));
        break;
    case WM_MOUSEMOVE:
        gInput.OnMouseMove((int64_t)((float)LOWORD(lParam) / gfMouseMultX), (int64_t)((float)HIWORD(lParam) / gfMouseMultY));
		break;
	case WM_MOUSEWHEEL:
		{
			// Wheel messages return mouse position in screen coords..... we need relative to client coords

			POINT clientTopLeft;
			clientTopLeft.x = 0;
			clientTopLeft.y = 0;
			ClientToScreen(hWnd, &clientTopLeft);
            gInput.OnMouseWheel((int64_t)((GET_X_LPARAM(lParam) - clientTopLeft.x) / gfMouseMultX), (int64_t)((GET_Y_LPARAM(lParam) - clientTopLeft.y) / gfMouseMultY), (int64_t)-GET_WHEEL_DELTA_WPARAM(wParam));	// we use negative delta since wheel up is a positive number
    }
		break;
	case WM_KILLFOCUS:
//		ZDEBUG_OUT("WM_KILLFOCUS\n");
		gbPaused = true;
		gTimer.Stop();
		break;
	case WM_SETFOCUS:
    {
        ZScreenBuffer* pSB = gGraphicSystem.GetScreenBuffer();
        GetKeyboardState((BYTE*) gInput.keyState);   // set up keystate for any keys up or down while window didn't have focus

        if (pSB)
            pSB->SetVisibilityComputingFlag(true);

//        if (gpMainWin)
//            gpMainWin->InvalidateChildren();
        gbPaused = false;
        gTimer.Start();
        gMessageSystem.Post(ZMessage("pause", "set", "0"));
    }
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			gMessageSystem.Post(ZMessage("pause", "set", "1"));
            gbPaused = true;
			gTimer.Stop();
		}
		else
		{
            gMessageSystem.Post(ZMessage("pause", "set", "0"));
            gbPaused = false;
			gTimer.Start();
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

#endif // _WIN64
#ifdef _WIN64
#include "windows.h"
#include <commctrl.h>
#include <fstream>

#include "helpers/StringHelpers.h"
#include "helpers/CommandLineParser.h"
#include "helpers/Registry.h"
#include "ZDebug.h"
#include "ZInput.h"
#include "ZTickManager.h"
#include "ZGraphicSystem.h"
#include "ZScreenBuffer.h"
#include "ZTimer.h"
#include "ZMainWin.h"
#include "ZAnimator.h"

namespace ZFrameworkApp
{
    extern bool InitRegistry(std::filesystem::path userDataPath);
    extern bool Initialize(int argc, char* argv[], std::filesystem::path userDataPath);
    extern void Shutdown();
};

extern ZTickManager     gTickManager;
extern ZAnimator        gAnimator;

using namespace std;


ATOM					MyRegisterClass(HINSTANCE, LPTSTR);
BOOL					WinInitInstance(int argc, char* argv[]);
bool                    gbFullScreen = true;
extern bool             gbGraphicSystemResetNeeded; 
extern bool             gbApplicationExiting = false;
ZRect                   grFullArea;
//ZPoint                  gInitWindowSize;
//ZPoint                  gWindowedModeOffset;    // for switching between full/windowed

ZRect                   grWindowArea;


extern bool             gbPaused = false;
HINSTANCE               g_hInst;				// The current instance
HWND                    ghWnd;

LRESULT CALLBACK        WndProc(HWND, UINT, WPARAM, LPARAM);

const uint64_t          kMinUSBetweenLoops = 8333;  // 120 fps max?

ZInput                  gInput;
// For resizing the window... it scales the mouse movements
float                   gfMouseMultX = 1.0f;
float                   gfMouseMultY = 1.0f;

ZDebug                  gDebug;


void HandleWindowSizeChanged(bool bSwitchingFullScreen);

class Win64AppMessageHandler : public IMessageTarget
{
public:
    std::string GetTargetName() { return "Win64AppMessageHandler"; }
    bool        ReceiveMessage(const ZMessage& message)
    {
        if (message.GetType() == "toggle_fullscreen")
        {
            gbFullScreen = !gbFullScreen;
            HandleWindowSizeChanged(true);
        }
        return true;
    }
};


int main(int argc, char* argv[])
{

    // Enable exception handling
    SetUnhandledExceptionFilter([](PEXCEPTION_POINTERS exceptionInfo) -> LONG {
        std::cerr << "Unhandled exception: " << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
//        while (1);
        return EXCEPTION_EXECUTE_HANDLER;
        });

    string sUserPath(getenv("APPDATA"));
    std::filesystem::path userDataPath(sUserPath);


    ZFrameworkApp::InitRegistry(userDataPath);

    // Perform application initialization:
    if (!WinInitInstance(argc, argv))
        return FALSE;




    if (!ZFrameworkApp::Initialize(argc, argv, userDataPath))
        return FALSE;
    //cout << "***AFTER SandboxInitialize. \n";


    uint64_t nTimeStamp = 0;

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

            gMessageSystem.Process();
            gTickManager.Tick();

            if (!gbApplicationExiting)  // have to check again because the state may have changed during the messagesystem process
            {
                gInput.Process();

                ZScreenBuffer* pScreenBuffer = gGraphicSystem.GetScreenBuffer();
                if (pScreenBuffer)
                {
                    // Copy to our window surface
                    pScreenBuffer->BeginRender();

                    if (pScreenBuffer->DoesVisibilityNeedComputing())
                    {
                        int64_t nStartTime = gTimer.GetUSSinceEpoch();
                        pScreenBuffer->SetVisibilityComputingFlag(false);
                        pScreenBuffer->ResetVisibilityList();
                        gpMainWin->ComputeVisibility();

                        int64_t nEndTime = gTimer.GetUSSinceEpoch();

                        //						ZOUT("Computing visibility took time:%lld us. Rects:%d\n", nEndTime - nStartTime, pScreenBuffer->GetVisibilityCount());
                    }


                    static int64_t nTotalRenderTime = 0;
                    static int64_t nTotalFrames = 0;


                    bool bAnimatorActive = gAnimator.Paint();

                    int64_t nStartRenderVisible = gTimer.GetUSSinceEpoch();
                    int32_t nRenderedCount = pScreenBuffer->RenderVisibleRects();
                    int64_t nEndRenderVisible = gTimer.GetUSSinceEpoch();
                    static char  buf[512];

                    int64_t nDelta = nEndRenderVisible - nStartRenderVisible;
                    nTotalRenderTime += nDelta;
                    nTotalFrames += 1;

                    //				    ZOUT("render took time:%lld us. Rects:%d/%d. Total Frames:%d, avg frame time:%lld us\n", nEndRenderVisible - nStartRenderVisible, nRenderedCount, pScreenBuffer->GetVisibilityCount(), nTotalFrames, (nTotalRenderTime/nTotalFrames));


                    pScreenBuffer->EndRender();
                    InvalidateRect(gpGraphicSystem->GetMainHWND(), NULL, false);

                }

                if (gbGraphicSystemResetNeeded && !gbPaused)
                {
                    ZDEBUG_OUT("calling HandleModeChanges\n");

                    if (gpGraphicSystem->HandleModeChanges())
                    {
                        gbGraphicSystemResetNeeded = false;
                    }
                }
            }

            gDebug.Flush();
            //            FlushDebugOutQueue();

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
        }
    }


    ZFrameworkApp::Shutdown();
    gDebug.Flush();
    //    FlushDebugOutQueue();

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


const char* szAppClass = "ZImageViewer";
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
    grWindowArea.SetRect(0, 0, GetSystemMetrics(SM_CXSCREEN) * 3 / 4, GetSystemMetrics(SM_CYSCREEN) * 3 / 4);
    grWindowArea.MoveRect((GetSystemMetrics(SM_CXSCREEN) - grWindowArea.Width()) / 2, (GetSystemMetrics(SM_CYSCREEN) - grWindowArea.Height()) / 2);



    // if registry settings persisted
    if (!gRegistry.Get("appwin", "window_l", grWindowArea.left))
        gRegistry.SetDefault("appwin", "window_l", grWindowArea.left);
    if (!gRegistry.Get("appwin", "window_t", grWindowArea.top))
        gRegistry.SetDefault("appwin", "window_t", grWindowArea.top);
    if (!gRegistry.Get("appwin", "window_r", grWindowArea.right))
        gRegistry.SetDefault("appwin", "window_r", grWindowArea.right);
    if (!gRegistry.Get("appwin", "window_b", grWindowArea.bottom))
        gRegistry.SetDefault("appwin", "window_b", grWindowArea.bottom);

    if (!gRegistry.Get("appwin", "fullscreen", gbFullScreen))
        gRegistry.SetDefault("appwin", "fullscreen", gbFullScreen);


    // Finally if any command line overrides
    CLP::CommandLineParser parser;
    int64_t width = 0;
    int64_t height = 0;
    parser.RegisterParam(CLP::ParamDesc("width", &width, CLP::kNamed));
    parser.RegisterParam(CLP::ParamDesc("height", &height, CLP::kNamed));
    parser.RegisterParam(CLP::ParamDesc("fullscreen", &gbFullScreen, CLP::kNamed));
    parser.Parse(argc, argv);
    if (parser.GetParamWasFound("width"))
        grWindowArea.right = grWindowArea.left + width;
    if (parser.GetParamWasFound("height"))
        grWindowArea.bottom = grWindowArea.top + height;





    if (gbFullScreen)
    {
        ghWnd = CreateWindow(szAppClass, szAppClass, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_hInst, NULL);
        grFullArea.SetRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        SizeWindowToClientArea(ghWnd, 0, 0, grFullArea.Width(), grFullArea.Height());
    }
    else
    {
        ghWnd = CreateWindow(szAppClass, szAppClass, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_hInst, NULL);
        grFullArea.SetRect(0, 0, grWindowArea.Width(), grWindowArea.Height());
        SizeWindowToClientArea(ghWnd, grWindowArea.left, grWindowArea.top, grWindowArea.Width(), grWindowArea.Height());
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

void HandleWindowSizeChanged(bool bSwitchingFullScreen = false)
{
    if (!gpGraphicSystem || !gpMainWin)
        return;

    gbPaused = true;

    if (gbFullScreen)
    {
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);

        HMONITOR h = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
        GetMonitorInfo(h, &mi);

        grFullArea.SetRect(0, 0, mi.rcMonitor.right- mi.rcMonitor.left, mi.rcMonitor.bottom- mi.rcMonitor.top);
        DWORD nStyle = GetWindowLong(ghWnd, GWL_STYLE);
        DWORD dwRemove = WS_OVERLAPPEDWINDOW;
        nStyle &= ~dwRemove;
        SetWindowLongPtr(ghWnd, GWL_STYLE, nStyle);
        MoveWindow(ghWnd, mi.rcMonitor.left, mi.rcMonitor.top, grFullArea.Width(), grFullArea.Height(), TRUE);
    }
    else
    {
        RECT rW;
        rW.left = grWindowArea.left;
        rW.top = grWindowArea.top;
        rW.right = grWindowArea.right;
        rW.bottom = grWindowArea.bottom;
        grFullArea.SetRect(0, 0, grWindowArea.Width(), grWindowArea.Height());

        DWORD nStyle = GetWindowLong(ghWnd, GWL_STYLE);
        DWORD dwAdd = WS_OVERLAPPEDWINDOW;
        nStyle |= dwAdd;
        SetWindowLongPtr(ghWnd, GWL_STYLE, nStyle);

//        SetWindowLongPtr(ghWnd, GWL_STYLE, WS_SYSMENU);
        if (bSwitchingFullScreen)
            AdjustWindowRect(&rW, dwAdd, FALSE);
        MoveWindow(ghWnd, rW.left, rW.top, rW.right - rW.left, rW.bottom - rW.top, TRUE);

//        gRegistry.Set("appwin", "width", gInitWindowSize.x);
//        gRegistry.Set("appwin", "height", gInitWindowSize.y);
        gRegistry.Set("appwin", "window_l", grWindowArea.left);
        gRegistry.Set("appwin", "window_t", grWindowArea.top);
        gRegistry.Set("appwin", "window_r", grWindowArea.right);
        gRegistry.Set("appwin", "window_b", grWindowArea.bottom);
    }

    gRegistry.Set("appwin", "fullscreen", gbFullScreen);


    gpGraphicSystem->HandleModeChanges();
    gpMainWin->SetArea(grFullArea);
    gpMainWin->InvalidateChildren();
    gGraphicSystem.GetScreenBuffer()->SetVisibilityComputingFlag(true);
    gbPaused = false;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (gbApplicationExiting)
		return 0;

	int wmId, wmEvent;

    static bool bSizing = false;

    static int count = 0;
	switch (message) 
	{
    case WM_WINDOWPOSCHANGED:
    {
        if (!gbFullScreen)
        {
            POINT pt;
            pt.x = 0;
            pt.y = 0;
            ClientToScreen(ghWnd, &pt);
            grWindowArea.MoveRect(pt.x, pt.y);

            WINDOWPOS* pWP = (WINDOWPOS*)lParam;
            if (pWP->cx != grWindowArea.Width() || pWP->cy != grWindowArea.Height())
            {
                grWindowArea.right = grWindowArea.left + pWP->cx;
                grWindowArea.bottom = grWindowArea.top + pWP->cy;
                bSizing = true;
            }
        }

        if (gpMainWin)
            gpMainWin->InvalidateChildren();
    }
        break;
    case WM_SYSCOMMAND:
        DefWindowProc(hWnd, message, wParam, lParam);
        if (wParam == SC_MAXIMIZE || wParam == SC_RESTORE)
            HandleWindowSizeChanged();
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_EXITSIZEMOVE:
        if (bSizing)
        {
            bSizing = false;
            HandleWindowSizeChanged();
        }
        DefWindowProc(hWnd, message, wParam, lParam);
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
        {
            if (wParam == '`')
            {
                gMessageSystem.Post("toggleconsole");
            }
            else if (wParam == 'f' || wParam == 'F')
            {
                gbFullScreen = !gbFullScreen;
                HandleWindowSizeChanged(true);
            }
        }
        break;
    case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
        gInput.OnKeyDown((uint32_t)wParam);
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

        if (gpMainWin)
            gpMainWin->InvalidateChildren();
        gbPaused = false;
        gTimer.Start();
        gMessageSystem.Post("pause;set=0");
    }
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			gMessageSystem.Post("pause;set=1");
            gbPaused = true;
			gTimer.Stop();
		}
		else
		{
            gMessageSystem.Post("pause;set=0");
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
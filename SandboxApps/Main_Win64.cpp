#ifdef _WIN64
#include "windows.h"
#include <commctrl.h>
#include <fstream>

#include "Main_Sandbox.h"
#include "helpers/StringHelpers.h"
#include "helpers/Registry.h"
#include "ZDebug.h"

using namespace std;


ATOM					MyRegisterClass(HINSTANCE, LPTSTR);
BOOL					WinInitInstance(HINSTANCE, int);
bool                    gbFullScreen = true;
HINSTANCE		        g_hInst;				// The current instance
HWND			        ghWnd;

LRESULT CALLBACK		WndProc(HWND, UINT, WPARAM, LPARAM);
void					Window_OnLButtonUp(UINT nX, UINT nY);
void					Window_OnLButtonDown(UINT nX, UINT nY);
void					Window_OnRButtonDown(UINT nX, UINT nY);
void					Window_OnRButtonUp(UINT nX, UINT nY);
void					Window_OnMouseMove(UINT nX, UINT nY);
void					Window_OnMouseWheel(UINT nX, UINT nY, INT nDelta);
void					CheckMouseForHover();

const uint64_t          kMinUSBetweenLoops = 8333;  // 120 fps max?
uint64_t                gnLastMouseMoveTime = 0;
uint32_t                kMouseHoverInitialTime = 333;
bool                    gbMouseHoverPosted = true;		// flag for knowing whether a "hover" message had been posted to the window currently under the mouse
// For resizing the window... it scales the mouse movements
float                   gfMouseMultX = 1.0f;
float                   gfMouseMultY = 1.0f;

std::string             gsRegistryFile;
ZDebug                  gDebug;



int WINAPI WinMain(	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	memset(&msg, 0, sizeof(msg));

	// Perform application initialization:
	if (!WinInitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}
    
	uint64_t nTimeStamp = 0;

	// Main message loop:
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

		if (!gbPaused && !gbApplicationExiting ) 
		{
           
            gMessageSystem.Process();
			gTickManager.Tick();

			if (!gbApplicationExiting)  // have to check again because the state may have changed during the messagesystem process
			{
				CheckMouseForHover();

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

                    int64_t nDelta = nEndRenderVisible-nStartRenderVisible;
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
                uint64_t nUSToSleep = kMinUSBetweenLoops-nTimeSinceLastLoop;
//                ZOUT("Maintaining min frame time. Sleeping for %lldus.\n", nUSToSleep);
                std::this_thread::sleep_for(std::chrono::microseconds(nUSToSleep));
//                nTimeStamp = gTimer.GetUSSinceEpoch();
            }
		}
    }


    Sandbox::SandboxShutdown();
    gDebug.Flush();
//    FlushDebugOutQueue();

    gRegistry.Save(gsRegistryFile);
    return (int) msg.wParam;
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


const char* szAppClass = "ProcessImage";
BOOL WinInitInstance(HINSTANCE hInstance, int nCmdShow)
{
    string sUserPath(getenv("APPDATA"));
    std::filesystem::path fullPath(sUserPath);
    fullPath += "/ZSandbox/prefs.json";
    gsRegistryFile = fullPath.make_preferred().string();

    if (!gRegistry.Load(gsRegistryFile))
    {
        ZDEBUG_OUT("No registry file at:%s creating path for one.");
        std::filesystem::path regPath(gsRegistryFile);
        std::filesystem::create_directories(regPath.parent_path());
    }


	g_hInst = hInstance;		// Store instance handle in our global variable

	WNDCLASS	wc;

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, "Application Name");
	wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = szAppClass;

	RegisterClass(&wc);

    DPI_AWARENESS_CONTEXT oldContext = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	if (gbFullScreen)
	{
		ghWnd = CreateWindow(szAppClass, szAppClass, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	}
	else
	{
		ghWnd = CreateWindow(szAppClass, szAppClass, WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	}
	if (!ghWnd)
	{	
		return FALSE;
	}
	//When the main window is created using CW_USEDEFAULT the height of the menubar (if one
	// is created is not taken into account). So we resize the window after creating it
	// if a menubar is present
	RECT rc;
	GetWindowRect(ghWnd, &rc);
	int nScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (gbFullScreen)
		grFullArea.SetRect(0, 0, nScreenWidth, nScreenHeight);
	else
		grFullArea.SetRect(0, 0, (long) (nScreenWidth * 0.75), (long)(nScreenHeight * 0.75));




	SizeWindowToClientArea(ghWnd, (int) (nScreenWidth - grFullArea.Width())/2, (int) (nScreenHeight-grFullArea.Height())/2, (int) grFullArea.Width(), (int) grFullArea.Height());

	gGraphicSystem.SetHWND(ghWnd);

#ifdef GENERATE_FRAMEWORK_FONTS
    Sandbox::SandboxInitializeFonts();
    GenerateFrameworkFonts();
#endif


    Sandbox::SandboxInitialize();

	ShowWindow(ghWnd, nCmdShow);
	UpdateWindow(ghWnd);

	return TRUE;
}

void CheckMouseForHover()
{
	uint64_t nCurTime = gTimer.GetElapsedTime();

	if (!gbMouseHoverPosted && nCurTime - gnLastMouseMoveTime > kMouseHoverInitialTime)
	{
		ZWin* pMouseOverWin = gpMainWin->GetChildWindowByPoint(gLastMouseMove.x, gLastMouseMove.y);
		if (pMouseOverWin)
			pMouseOverWin->OnMouseHover((int64_t) ((float) gLastMouseMove.x / gfMouseMultX), (int64_t) ((float) gLastMouseMove.y / gfMouseMultY));

		gbMouseHoverPosted = true;
	}
}

void CheckForMouseOverNewWindow()
{
	ZWin* pMouseOverWin = gpMainWin->GetChildWindowByPoint(gLastMouseMove.x, gLastMouseMove.y);

	// Check whether a "cursor out" message needs to be sent.  (i.e. a previous window had the cursor over it but no longer)
	if (gpMouseOverWin != pMouseOverWin)
	{
		gbMouseHoverPosted = false;	// Only post a mouse hover message if the cursor has moved into a new window

		// Mouse Out of old window
		if (gpMouseOverWin)
			gpMouseOverWin->OnMouseOut();

		gpMouseOverWin = pMouseOverWin;

		// Mouse In to new window
		if (pMouseOverWin)
			pMouseOverWin->OnMouseIn();
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (gbApplicationExiting)
		return 0;

	int wmId, wmEvent;

	switch (message) 
	{
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
		Sandbox::SandboxShutdown();
		break;
	case WM_CHAR:
		{
            if (wParam == '`')
            {
                gMessageSystem.Post(ZMessage("toggleconsole"));
            }
            else
            {
                ZMessage message("chardown");
                message.SetParam("code", StringHelpers::FromInt(wParam));
                gMessageSystem.Post(message);
            }
		}
	case WM_KEYDOWN:
		{
			ZMessage message("keydown");
			message.SetParam("code", StringHelpers::FromInt(wParam));
			gMessageSystem.Post(message);
		}
		break;
	case WM_KEYUP:
		{
			ZMessage message("keyup");
			message.SetParam("code", StringHelpers::FromInt(wParam));
			gMessageSystem.Post(message);
		}
		break;
	case WM_LBUTTONDOWN:
		Window_OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONUP:
		Window_OnLButtonUp(LOWORD(lParam), HIWORD(lParam));
		break;
    case WM_RBUTTONDOWN:
        Window_OnRButtonDown(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_RBUTTONUP:
        Window_OnRButtonUp(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_MOUSEMOVE:
		Window_OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_MOUSEWHEEL:
		{
			// Wheel messages return mouse position in screen coords..... we need relative to client coords

			POINT clientTopLeft;
			clientTopLeft.x = 0;
			clientTopLeft.y = 0;
			ClientToScreen(hWnd, &clientTopLeft);
			Window_OnMouseWheel(GET_X_LPARAM(lParam) - clientTopLeft.x, GET_Y_LPARAM(lParam) - clientTopLeft.y, -GET_WHEEL_DELTA_WPARAM(wParam));	// we use negative delta since wheel up is a positive number
		}
		break;
	case WM_KILLFOCUS:
//		ZDEBUG_OUT("WM_KILLFOCUS\n");
		gbPaused = true;
		gTimer.Stop();
		break;
	case WM_SETFOCUS:
        gGraphicSystem.GetScreenBuffer()->SetVisibilityComputingFlag(true);
		gbPaused = false;
		gTimer.Start();
		gMessageSystem.Post("type=pause;set=0");
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			gMessageSystem.Post("type=pause;set=1");
			gbPaused = true;
			gTimer.Stop();
		}
		else
		{
			gMessageSystem.Post("type=pause;set=0");
			gbPaused = false;
			gTimer.Start();
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void Window_OnLButtonDown(UINT nX, UINT nY)
{
	ZMessage cursorMessage;
	cursorMessage.SetType("cursor_msg");
	cursorMessage.SetParam("subtype", "l_down");
	cursorMessage.SetParam("x", StringHelpers::FromInt((int) ((float) nX / gfMouseMultX)));
	cursorMessage.SetParam("y", StringHelpers::FromInt((int) ((float) nY / gfMouseMultY)));
	if (gpCaptureWin)
		cursorMessage.SetTarget(gpCaptureWin->GetTargetName());
	else
		cursorMessage.SetTarget(gpMainWin->GetTargetName());

	gMessageSystem.Post(cursorMessage);
}

void Window_OnLButtonUp(UINT nX, UINT nY)
{
	ZMessage cursorMessage;
	cursorMessage.SetType("cursor_msg");
	cursorMessage.SetParam("subtype", "l_up");
	cursorMessage.SetParam("x", StringHelpers::FromInt((int) ((float) nX / gfMouseMultX)));
	cursorMessage.SetParam("y", StringHelpers::FromInt((int) ((float) nY / gfMouseMultY)));
	if (gpCaptureWin)
		cursorMessage.SetTarget(gpCaptureWin->GetTargetName());
	else
		cursorMessage.SetTarget(gpMainWin->GetTargetName());

	gMessageSystem.Post(cursorMessage);
}

void Window_OnRButtonDown(UINT nX, UINT nY)
{
    ZMessage cursorMessage;
    cursorMessage.SetType("cursor_msg");
    cursorMessage.SetParam("subtype", "r_down");
    cursorMessage.SetParam("x", StringHelpers::FromInt((int)((float)nX / gfMouseMultX)));
    cursorMessage.SetParam("y", StringHelpers::FromInt((int)((float)nY / gfMouseMultY)));
    if (gpCaptureWin)
        cursorMessage.SetTarget(gpCaptureWin->GetTargetName());
    else
        cursorMessage.SetTarget(gpMainWin->GetTargetName());

    gMessageSystem.Post(cursorMessage);
}

void Window_OnRButtonUp(UINT nX, UINT nY)
{
    ZMessage cursorMessage;
    cursorMessage.SetType("cursor_msg");
    cursorMessage.SetParam("subtype", "r_up");
    cursorMessage.SetParam("x", StringHelpers::FromInt((int)((float)nX / gfMouseMultX)));
    cursorMessage.SetParam("y", StringHelpers::FromInt((int)((float)nY / gfMouseMultY)));
    if (gpCaptureWin)
        cursorMessage.SetTarget(gpCaptureWin->GetTargetName());
    else
        cursorMessage.SetTarget(gpMainWin->GetTargetName());

    gMessageSystem.Post(cursorMessage);
}

void Window_OnMouseMove(UINT nX, UINT nY)
{
	{
#ifdef FLOATLINES_WINDOW
//		::SetCursor(NULL);
#else
		HCURSOR hCursor = ::LoadCursor(NULL, IDC_ARROW);
		::SetCursor(hCursor);
#endif
	}

    if (gLastMouseMove.x != nX || gLastMouseMove.y != nY)
    {
        gLastMouseMove.Set(nX, nY);
        gnLastMouseMoveTime = gTimer.GetElapsedTime();

        ZMessage cursorMessage;
        cursorMessage.SetType("cursor_msg");
        cursorMessage.SetParam("subtype", "move");
        cursorMessage.SetParam("x", StringHelpers::FromInt((int)((float)nX / gfMouseMultX)));
        cursorMessage.SetParam("y", StringHelpers::FromInt((int)((float)nY / gfMouseMultY)));
        if (gpCaptureWin)
            cursorMessage.SetTarget(gpCaptureWin->GetTargetName());
        else
            cursorMessage.SetTarget(gpMainWin->GetTargetName());

        gMessageSystem.Post(cursorMessage);

        CheckForMouseOverNewWindow();
    }
}

void Window_OnMouseWheel(UINT nX, UINT nY, INT nDelta)
{
	ZMessage cursorMessage;
	cursorMessage.SetType("cursor_msg");
	cursorMessage.SetParam("subtype", "wheel");
	cursorMessage.SetParam("x", StringHelpers::FromInt((int) ((float) nX / gfMouseMultX)));
	cursorMessage.SetParam("y", StringHelpers::FromInt((int) ((float) nY / gfMouseMultY)));
	cursorMessage.SetParam("delta", StringHelpers::FromInt(nDelta));
	if (gpCaptureWin)
		cursorMessage.SetTarget(gpCaptureWin->GetTargetName());
	else
		cursorMessage.SetTarget(gpMainWin->GetTargetName());

	gMessageSystem.Post(cursorMessage);
}

#endif // _WIN64
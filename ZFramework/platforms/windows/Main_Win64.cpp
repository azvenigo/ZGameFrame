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
    extern bool Initialize(int argc, char* argv[], std::filesystem::path userDataPath);
    extern void Shutdown();
};

extern ZTickManager            gTickManager;
extern ZAnimator            gAnimator;

using namespace std;


ATOM					MyRegisterClass(HINSTANCE, LPTSTR);
BOOL					WinInitInstance(HINSTANCE, int);
bool                    gbFullScreen = true;
extern bool             gbGraphicSystemResetNeeded; 
extern bool                    gbApplicationExiting = false;
ZRect                   grFullArea;

extern bool                    gbPaused = false;
HINSTANCE		        g_hInst;				// The current instance
HWND			        ghWnd;

LRESULT CALLBACK		WndProc(HWND, UINT, WPARAM, LPARAM);

const uint64_t          kMinUSBetweenLoops = 8333;  // 120 fps max?

ZInput                  gInput;
// For resizing the window... it scales the mouse movements
float                   gfMouseMultX = 1.0f;
float                   gfMouseMultY = 1.0f;

ZDebug                  gDebug;


int main(int argc, char* argv[])
{

    // Enable exception handling
    SetUnhandledExceptionFilter([](PEXCEPTION_POINTERS exceptionInfo) -> LONG {
        std::cerr << "Unhandled exception: " << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
//        while (1);
        return EXCEPTION_EXECUTE_HANDLER;
        });

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    MSG msg;
    memset(&msg, 0, sizeof(msg));

    // Perform application initialization:
    if (!WinInitInstance(hInstance, SW_SHOWNORMAL))
        return FALSE;


    string sUserPath(getenv("APPDATA"));
    std::filesystem::path userDataPath(sUserPath);





    if (!ZFrameworkApp::Initialize(argc, argv, userDataPath))
        return FALSE;
    //cout << "***AFTER SandboxInitialize. \n";


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


const char* szAppClass = "ProcessImage";
BOOL WinInitInstance(HINSTANCE hInstance, int nCmdShow)
{
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


    HCURSOR hCursor = ::LoadCursor(NULL, IDC_ARROW);
    ::SetCursor(hCursor);

	ShowWindow(ghWnd, nCmdShow);
	UpdateWindow(ghWnd);

	return TRUE;
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
        ZFrameworkApp::Shutdown();
		break;
	case WM_CHAR:
        gInput.OnChar((uint32_t)wParam);
        {
            if (wParam == '`')
            {
                gMessageSystem.Post("toggleconsole");
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
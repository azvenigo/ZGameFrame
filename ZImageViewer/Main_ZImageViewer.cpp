#include "Main_ZImageViewer.h"
#include "helpers/StringHelpers.h"
#include "helpers/Registry.h"
#include "helpers/CommandLineParser.h"
#include "helpers/Logger.h"
#include "helpers/ZZFileAPI.h"
#include "helpers/FileHelpers.h"


#include "ZWinDebugConsole.h"
#include "Resources.h"
#include "ImageViewer.h"
#include "ZThumbCache.h"
#include "ImageMeta.h"


using namespace std;

// Global Variables:
extern ZRect            grFullArea;
ZMainWin*               gpMainWin;
ZWinDebugConsole*       gpDebugConsole;
ZGraphicSystem          gGraphicSystem;
ZGraphicSystem*         gpGraphicSystem = &gGraphicSystem;
ZRasterizer             gRasterizer;
bool                    gbGraphicSystemResetNeeded(false);
ZTimer                  gTimer(true);
int64_t                 gnFramesPerSecond = 0;
ZFontSystem*            gpFontSystem = nullptr;
std::list<string>       gDebugOutQueue;
std::mutex              gDebugOutMutex;
std::list<std::string>  gDebugOutHistory;
std::mutex              gDebugOutHistoryMutex;
size_t                  gDebugHistoryMaxSize = 512;
size_t                  gDebugHistoryCounter = 0;


ZMessageSystem          gMessageSystem;
ZTickManager            gTickManager;
ZAnimator               gAnimator;
ZWin*                   gpCaptureWin = nullptr;
ZWin*                   gpMouseOverWin = nullptr;
REG::Registry           gRegistry;
LOG::Logger             gLogger;


void ZImageViewer::InitControlPanel()
{

    int64_t panelW = grFullArea.Width() / 8;    
    int64_t panelH = grFullArea.Height() / 2;
    ZRect rControlPanel(grFullArea.right - panelW, 0, grFullArea.right, panelH);     // upper right for now

    gpDebugConsole = new ZWinDebugConsole();
    gpDebugConsole->SetArea(ZRect(grFullArea.left, grFullArea.top, grFullArea.right, grFullArea.Height() / 2));
    gpMainWin->ChildAdd(gpDebugConsole, false);

}

void ZImageViewer::DeleteAllButControlPanelAndDebugConsole()
{
    gpMainWin->ChildRemove(gpDebugConsole);
    gpMainWin->ChildDeleteAll();
}


void ZImageViewer::InitChildWindows()
{
    DeleteAllButControlPanelAndDebugConsole();

    string sImageFilename = gRegistry.GetValue("zimageviewer", "imageviewer_filename");

    ImageViewer* pWin = new ImageViewer();
    pWin->SetArea(grFullArea);
    gpMainWin->ChildAdd(pWin);
//    if (!sImageFilename.empty())
//        pWin->ViewImage(sImageFilename);

    gpMainWin->ChildAdd(gpDebugConsole, false);
}



class MainAppMessageTarget : public IMessageTarget
{
public:
	MainAppMessageTarget()
	{
		gMessageSystem.RegisterTarget(this);
        gMessageSystem.AddNotification("toggleconsole", this);
	}

	string GetTargetName() { return "MainAppMessageTarget"; }
	bool ReceiveMessage(const ZMessage& message)
	{
		string sType = message.GetType();
		if (sType == "initchildwindows")
		{
            ZImageViewer::InitChildWindows();
		}
        else if (sType == "toggleconsole")
        {
            gpDebugConsole->SetVisible(!gpDebugConsole->mbVisible);
        }

		return true;
	}
};

MainAppMessageTarget gMainAppMessageTarget;


bool ZFrameworkApp::InitRegistry(std::filesystem::path userDataPath)
{
    filesystem::path appDataPath(userDataPath);
    appDataPath += "/ZImageViewer/";

    filesystem::path prefsPath = appDataPath.make_preferred().string() + "prefs.json";
    gRegistry.SetFilename(prefsPath.string());
    if (!gRegistry.Load())
    {
        ZDEBUG_OUT("No registry file at:%s creating path for one.");
        std::filesystem::create_directories(prefsPath.parent_path());
    }

    return true;
}


bool ReadCFG(const string& sCFGPath, string& sVersion, string& sBuildURL)
{
    tZZFilePtr filePtr;
    if (cZZFile::Open(sCFGPath, false, filePtr, LOG::gnVerbosityLevel > LVL_DEFAULT))
    {
        uint64_t nSize = filePtr->GetFileSize();
        uint8_t* pBuf = new uint8_t[nSize];
        filePtr->Read(pBuf, nSize);
        string sCFG((const char*)pBuf, nSize);

        size_t nStart = sCFG.find("appver=");
        if (nStart != string::npos)
        {
            nStart += 7; // skip "appver="
            size_t nEndVer = sCFG.find('\r', nStart+1);
            if (nEndVer != string::npos)
            {
                sVersion = sCFG.substr(nStart, nEndVer - nStart);
            }
        }

        nStart = sCFG.find("buildurl=");
        if (nStart != string::npos)
        {
            nStart += 9; // skip "buildurl="
            size_t nEndVer = sCFG.find('\r', nStart+1);
            if (nEndVer != string::npos)
            {
                sBuildURL = sCFG.substr(nStart, nEndVer - nStart);
            }

        }

        delete[] pBuf;

    }

    return !sVersion.empty() && !sBuildURL.empty();
}


bool ZFrameworkApp::Initialize(int argc, char* argv[], std::filesystem::path userDataPath)
{
    gGraphicSystem.SetArea(grFullArea);
    if (!gGraphicSystem.Init())
    {
        assert(false);
        return false;
    }

    filesystem::path appDataPath(userDataPath);
    appDataPath += "/ZImageViewer/";


#ifndef _DEBUG


    string sCurVersion = FH::GetFileVersion(argv[0]);
    string sCurURL;

    if (ReadCFG("install/install.cfg", sCurVersion, sCurURL))
    {
        gRegistry.Set("app", "version", sCurVersion);
    }


    string sNewVersion;
    string sNewURL;

    gbSkipCertCheck = true;
    string sURL;
    Sprintf(sURL, "https://www.azvenigo.com/zimageviewerbuild/install.cfg?cur=%s", SH::URL_Encode(sCurVersion).c_str());
    if (ReadCFG(sURL, sNewVersion, sNewURL))
    {
        gRegistry.Set("app", "newversion", sNewVersion);
        gRegistry.Set("app", "newurl", sNewURL);

        if (sNewVersion != sCurVersion)
        {
            ZOUT("Current build:", sCurVersion, " New Build Available:", sNewVersion, " URL:", sURL, "\n");
        }
    }
#endif


    std::filesystem::path appPath(argv[0]);
    gRegistry["apppath"] = appPath.parent_path().string();
    gRegistry["appDataPath"] = appDataPath.string();


    string sUserPath(getenv("APPDATA"));
    std::filesystem::path thumbPath(sUserPath);
    thumbPath.append("ZImageViewer/thumbcache/");

    if (!std::filesystem::exists(thumbPath))
        std::filesystem::create_directories(thumbPath);
    gThumbCache.Init(thumbPath);

    if (!std::filesystem::exists(gLogger.msLogFilename))
    {
        ofstream logFile;
        logFile.open(gLogger.msLogFilename);
        logFile.close();
    }

    gImageMeta.basePath = appDataPath;
    gImageMeta.basePath += "contests/";

/*    if (std::filesystem::exists(gImageMeta.basePath))
        gImageMeta.LoadAll();
    else
        std::filesystem::create_directories(gImageMeta.basePath);*/
    if (!std::filesystem::exists(gImageMeta.basePath))
        std::filesystem::create_directories(gImageMeta.basePath);





    std::string sImageFilename;
    CLP::CommandLineParser parser;
    parser.RegisterAppDescription("ZImageViewer");
    parser.RegisterParam(CLP::ParamDesc("PATH", &sImageFilename, CLP::kOptional | CLP::kPositional, "Filename of image to load."));

    if (!parser.Parse(argc, argv))
    {
        ZERROR("ERROR parsing commandline.");
        return false;
    }

    if (!sImageFilename.empty())
        gRegistry["ZImageViewer"]["image"] = sImageFilename;


    filesystem::path resourcesPath(appPath.parent_path());
    resourcesPath.append("res/");
    gResources.Init(resourcesPath.string());// todo, move this define elsewhere?

    ZGUI::ComputeLooks();


    gpFontSystem = new ZFontSystem();
    filesystem::path font_cache(appDataPath);
    font_cache.append("font_cache");


    std::filesystem::create_directory(font_cache);    // ensure font cache exists...keep in working folder or move elsewhere?
    gpFontSystem->SetCacheFolder(font_cache.string());
    gpFontSystem->SetDefaultFont(gDefaultTextFont);

    gpFontSystem->Init();

    gpMainWin = new ZMainWin();
    gpMainWin->SetArea(grFullArea);
    gpMainWin->Init();

    ZImageViewer::InitControlPanel();

    ZImageViewer::InitChildWindows();

    return true;
}

void ZFrameworkApp::Shutdown()
{
    if (gpMainWin)
    {
        gpMainWin->Shutdown();
        delete gpMainWin;
        gpMainWin = nullptr;
    }

    if (gpFontSystem)
    {
        delete gpFontSystem;
        gpFontSystem = nullptr;
    }

    gResources.Shutdown();
    gGraphicSystem.Shutdown();
    gAnimator.KillAllObjects();

    gRegistry.Save();
}
#include "Main_ZImageViewer.h"
#include "helpers/StringHelpers.h"
#include "helpers/Registry.h"
#include "helpers/CommandLineParser.h"
#include "helpers/Logger.h"
#include "helpers/ZZFileAPI.h"


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
ZPoint                  gLastMouseMove;
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


    string sCurBuild(__DATE__ " " __TIME__);
    gRegistry.Set("app", "version", sCurBuild);


    std::filesystem::path appPath(argv[0]);
    gRegistry["apppath"] = appPath.parent_path().string();
    gRegistry["appDataPath"] = appDataPath.string();

    tZZFilePtr filePtr;
    gbSkipCertCheck = true;
    string sURL;
    Sprintf(sURL, "https://www.azvenigo.com/zimageviewerbuild/release.txt?cur=%s", SH::URL_Encode(sCurBuild).c_str());
    if (cZZFile::Open(sURL, false, filePtr, LOG::gnVerbosityLevel > LVL_DEFAULT))
    {
        int64_t nSize = filePtr->GetFileSize();
        if (nSize < 256)
        {
            uint8_t verFile[256];
            if (filePtr->Read(verFile, nSize) == nSize)
            {
                string sLine((const char*) verFile, nSize);

                size_t versionStart = sLine.find('[');
                if (versionStart != string::npos)
                {
                    size_t versionEnd = sLine.find(']', versionStart + 1);
                    if (versionEnd != string::npos)
                    {
                        string sNewBuild = sLine.substr(versionStart+1, versionEnd-versionStart-1);
                        string sURL = sLine.substr(versionEnd + 1);

                        gRegistry.Set("app", "newversion", sNewBuild);
                        gRegistry.Set("app", "newurl", sURL);

                        if (sNewBuild != sCurBuild)
                        {
                            ZOUT("Current build:", sCurBuild, " New Build Available:", sNewBuild, " URL:", sURL, "\n");
                        }
                    }
                }
            }
            else
                ZDEBUG_OUT("Unable to retrieve version file.\n");
        }
        else
            ZOUT("Version file size larger than allowed (256b)\n");
    }
    else
        ZDEBUG_OUT("Unable to retrieve version file.\n");


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
    resourcesPath.append("res/default_resources/");
    gResources.Init(resourcesPath.string());// todo, move this define elsewhere?

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
    gRegistry.Save();

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
}
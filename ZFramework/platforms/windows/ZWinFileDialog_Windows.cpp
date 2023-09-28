#include "helpers/StringHelpers.h"

#ifdef _WIN64        // for open file dialong

#include <windows.h>
#include <shobjidl.h> 
#endif

using namespace std;

namespace ZWinFileDialog
{
    bool ShowLoadDialog(std::string fileTypeDescription, std::string fileTypes, std::string& sFilenameResult, std::string sDefaultFolder)
    {
        wstring sFileTypeDescription(SH::string2wstring(fileTypeDescription));
        wstring sFileTypes(SH::string2wstring(fileTypes));



        bool bSuccess = false;

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileOpenDialog* pFileOpen;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

            if (SUCCEEDED(hr))
            {
                COMDLG_FILTERSPEC rgSpec[] =
                {
                    //                    { L"Supported Image Formats", L"*.jpg;*.jpeg;*.bmp;*.gif;*.png;*.tiff;*.exif;*.wmf;*.emf" },
                                        { sFileTypeDescription.c_str(), sFileTypes.c_str() },
                                        { L"All Files", L"*.*" }
                };

                pFileOpen->SetFileTypes(2, rgSpec);

                DWORD dwOptions;
                hr = pFileOpen->GetOptions(&dwOptions);
                if (SUCCEEDED(hr))
                {
                    hr = pFileOpen->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);
                }

                if (!sDefaultFolder.empty())
                {
                    wstring sDefaultFolderW(SH::string2wstring(sDefaultFolder));
                    IShellItem* pShellItem = nullptr;
                    SHCreateItemFromParsingName(sDefaultFolderW.c_str(), nullptr, IID_IShellItem, (void**)(&pShellItem));
                    if (pShellItem)
                    {
                        pFileOpen->SetFolder(pShellItem);
                        pShellItem->Release();
                    }
                }

                // Show the Open dialog box.
                hr = pFileOpen->Show(NULL);

                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    IShellItem* pItem;
                    hr = pFileOpen->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath = nullptr;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                        if (SUCCEEDED(hr) && pszFilePath)
                        {
                            wstring wideFilename(pszFilePath);
                            sFilenameResult = SH::wstring2string(wideFilename);
                        }

                        pItem->Release();
                        bSuccess = true;
                    }
                }
                pFileOpen->Release();
            }
            CoUninitialize();
        }

        return bSuccess;
    }

    bool ShowMultiLoadDialog(std::string fileTypeDescription, std::string fileTypes, std::list<std::string>& resultFilenames, std::string sDefaultFolder)
    {
        wstring sFileTypeDescription(SH::string2wstring(fileTypeDescription));
        wstring sFileTypes(SH::string2wstring(fileTypes));

        bool bSuccess = false;

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileOpenDialog* pFileOpen;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

            if (SUCCEEDED(hr))
            {
                COMDLG_FILTERSPEC rgSpec[] =
                {
//                    { L"Supported Image Formats", L"*.jpg;*.jpeg;*.bmp;*.gif;*.png;*.tiff;*.exif;*.wmf;*.emf" },
                    { sFileTypeDescription.c_str(), sFileTypes.c_str() },
                    { L"All Files", L"*.*" }
                };

                pFileOpen->SetFileTypes(2, rgSpec);

                DWORD dwOptions;
                // Specify multiselect.
                hr = pFileOpen->GetOptions(&dwOptions);

                if (SUCCEEDED(hr))
                {
                    hr = pFileOpen->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);
                }

                if (!sDefaultFolder.empty())
                {
                    wstring sDefaultFolderW(SH::string2wstring(sDefaultFolder));
                    IShellItem* pShellItem = nullptr;
                    SHCreateItemFromParsingName(sDefaultFolderW.c_str(), nullptr, IID_IShellItem, (void**)(&pShellItem));
                    if (pShellItem)
                    {
                        pFileOpen->SetFolder(pShellItem);
                        pShellItem->Release();
                    }
                }


                // Show the Open dialog box.
                hr = pFileOpen->Show(NULL);

                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    IShellItemArray* pItems;
                    hr = pFileOpen->GetResults(&pItems);
                    if (SUCCEEDED(hr))
                    {
                        DWORD nCount = 0;
                        pItems->GetCount(&nCount);
                        for (DWORD i = 0; i < nCount; i++)
                        {
                            IShellItem* pItem;
                            pItems->GetItemAt(i, &pItem);

                            PWSTR pszFilePath;
                            pItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFilePath);

                            if (SUCCEEDED(hr) && pszFilePath)
                            {
                                wstring wideFilename(pszFilePath);
                                resultFilenames.push_back(SH::wstring2string(wideFilename));
                            }
                            pItem->Release();
                            bSuccess = true;
                        }
                    }
                }
                pFileOpen->Release();
            }
            CoUninitialize();
        }

        return bSuccess;
    }

    bool ShowSelectFolderDialog(std::string& sFilenameResult, std::string sDefaultFolder)
    {
        bool bSuccess = false;

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileOpenDialog* pFileOpen;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

            if (SUCCEEDED(hr))
            {
                COMDLG_FILTERSPEC rgSpec[] =
                {
                          { L"All Files", L"*.*" }
                };

                pFileOpen->SetFileTypes(1, rgSpec);

                DWORD dwOptions;
                hr = pFileOpen->GetOptions(&dwOptions);
                if (SUCCEEDED(hr))
                {
                    hr = pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
                }

                if (!sDefaultFolder.empty())
                {
                    wstring sDefaultFolderW(SH::string2wstring(sDefaultFolder));
                    IShellItem* pShellItem = nullptr;
                    SHCreateItemFromParsingName(sDefaultFolderW.c_str(), nullptr, IID_IShellItem, (void**)(&pShellItem));

                    pFileOpen->SetFolder(pShellItem);
                    pShellItem->Release();
                }

                // Show the Open dialog box.
                hr = pFileOpen->Show(NULL);

                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    IShellItem* pItem;
                    hr = pFileOpen->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                        if (SUCCEEDED(hr) && pszFilePath)
                        {
                            wstring wideFilename(pszFilePath);
                            sFilenameResult = SH::wstring2string(wideFilename);
                        }

                        pItem->Release();
                        bSuccess = true;
                    }
                }
                pFileOpen->Release();
            }
            CoUninitialize();
        }

        return bSuccess;
    }



    bool ShowSaveDialog(std::string fileTypeDescription, std::string fileTypes, std::string& sFilenameResult, std::string sDefaultFolder)
    {
        wstring sPresetFilename(SH::string2wstring(sFilenameResult));
        wstring sFileTypeDescription(SH::string2wstring(fileTypeDescription));
        wstring sFileTypes(SH::string2wstring(fileTypes));

        bool bSuccess = false;
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileSaveDialog* pDialog;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pDialog));

            if (SUCCEEDED(hr))
            {
                COMDLG_FILTERSPEC rgSpec[] =
                {
                    //{ L"Supported Image Formats", L"*.jpg;*.jpeg;*.bmp;*.gif;*.png;*.tiff;*.tif" },
                    { sFileTypeDescription.c_str(), sFileTypes.c_str() },
                    { L"All Files", L"*.*" }
                };

                pDialog->SetFileTypes(2, rgSpec);
                if (!sPresetFilename.empty())
                    pDialog->SetFileName(sPresetFilename.c_str());


                if (!sDefaultFolder.empty())
                {
                    wstring sDefaultFolderW(SH::string2wstring(sDefaultFolder));
                    IShellItem* pShellItem = nullptr;
                    SHCreateItemFromParsingName(sDefaultFolderW.c_str(), nullptr, IID_IShellItem, (void**)(&pShellItem));
                    if (pShellItem)
                    {
                        pDialog->SetFolder(pShellItem);
                        pShellItem->Release();
                    }
                }

                // Show the Open dialog box.
                hr = pDialog->Show(NULL);

                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    IShellItem* pItem;
                    hr = pDialog->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                        if (SUCCEEDED(hr) && pszFilePath)
                        {
                            wstring wideFilename(pszFilePath);
                            sFilenameResult = SH::wstring2string(wideFilename);
                        }

                        pItem->Release();
                        bSuccess = true;
                    }
                }
                pDialog->Release();
            }
            CoUninitialize();
        }

        return bSuccess;
    }
};

#if defined(_WIN32)
// Astuce permettant de démarrer une application sans console directement sur la fonction main()
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")       // Application fenêtrée
#pragma comment(linker, "/ENTRY:mainCRTStartup")    // Point d'entrée positionné sur l'initialisation de la Runtime Windows
#pragma comment(lib, "user32.lib")
#endif

#include <DataPacker.h>
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Commctrl.h>

#include "../Resources/resource.h"
#include "./FileManager/FileManager.h"
#include "./TreeView/TreeView.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HIMAGELIST InitImageList(HMODULE hinstance);

int main(char* argc, char** argv)
{
    // register TreeView from comctl32.dll before creating windows
    INITCOMMONCONTROLSEX commonCtrls;
    commonCtrls.dwSize = sizeof(commonCtrls);
    commonCtrls.dwICC = ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx(&commonCtrls);
    
    // Initilize COM
    OleInitialize(nullptr);

    HWND hwnd = CreateDialog(nullptr, MAKEINTRESOURCE(IDD_MAIN_WINDOW), nullptr, (DLGPROC)WindowProc);

    DataPackerGUI::TreeView tree_view(GetDlgItem(hwnd, IDC_TREE));
    HIMAGELIST image_list = InitImageList(GetModuleHandle(nullptr));
    tree_view.SetImageList(image_list);

    DataPackerGUI::FileManager& file_manager = DataPackerGUI::FileManager::CreateInstance(hwnd, &tree_view);


    while(true) 
    {
        MSG msg;
        if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_QUIT)
            {
                // Si le message WM_QUIT est reçu, on signale la fermeture de l'application
                break;

            }else{

                //Gestion des messages de fenêtre
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    file_manager.DestroyInstance();
    OleUninitialize();

    return 0;
}

/**
 * Create an ImageList (https://docs.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-imagelist_create) to handle TreeView icons
 * @param hinstance Windows Application Instance
 * @return ImageList handle
 */
HIMAGELIST InitImageList(HMODULE hinstance)
{
    HIMAGELIST image_list = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 6, 1);

    ImageList_AddIcon(image_list, LoadIcon(hinstance, MAKEINTRESOURCE(IDI_MAIN_ICON)));
    // ImageList_AddIcon(image_list, LoadIcon(hinstance, MAKEINTRESOURCE(IDI_FOLDER_OPENED)));
    ImageList_AddIcon(image_list, LoadIcon(hinstance, MAKEINTRESOURCE(IDI_FOLDER_CLOSED)));
    ImageList_AddIcon(image_list, LoadIcon(hinstance, MAKEINTRESOURCE(IDI_MESH)));
    ImageList_AddIcon(image_list, LoadIcon(hinstance, MAKEINTRESOURCE(IDI_MATERIAL)));
    ImageList_AddIcon(image_list, LoadIcon(hinstance, MAKEINTRESOURCE(IDI_TEXTURE)));

    return image_list;
}



LRESULT WINAPI WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) 
    {
        case EVENT_SYSTEM_DRAGDROPSTART:

        // User closed window
        case WM_CLOSE :
        {
            // Ask user for saving before closing, then terminate application
            if(DataPackerGUI::FileManager::GetInstance().CloseApplication())
                PostQuitMessage(0);
            return TRUE;
        }

        // Initialize window
        case WM_INITDIALOG :
        {
            HICON hIcon = LoadIcon(GetModuleHandleW(nullptr), MAKEINTRESOURCE(IDI_MAIN_ICON));
            if(hIcon) SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            return TRUE;
        }

        // Redraw window
        case WM_PAINT :
        {
            // Default behavior
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        // Resize window
        case WM_SIZE :
        {
            HWND tree = GetDlgItem(hwnd, IDC_TREE);
            constexpr int margin = 11;
            MoveWindow(tree, margin, margin, LOWORD(lParam) - margin * 2, HIWORD(lParam) - margin * 2, TRUE);
            return TRUE;
        }
 
        case WM_COMMAND:
        {
            switch(LOWORD(wParam)) 
            {
                case ID_FILE_LEAVE :
                    // Ask user for saving before closing, then terminate application
                    if(DataPackerGUI::FileManager::GetInstance().CloseApplication())
                        PostQuitMessage(0);
                    return TRUE;

                case ID_FILE_NEW :
                    DataPackerGUI::FileManager::GetInstance().NewFile();
                    return TRUE;

                case ID_FILE_SAVE :
                    DataPackerGUI::FileManager::GetInstance().SaveFile();
                    return TRUE;

                case ID_FILE_SAVE_AS :
                    DataPackerGUI::FileManager::GetInstance().SaveFile(true);
                    return TRUE;

                case ID_FILE_OPEN :
                    DataPackerGUI::FileManager::GetInstance().OpenFile();
                    return TRUE;

                case ID_FILE_CLOSE :
                {
                    DataPackerGUI::FileManager::GetInstance().CloseFile();
                    HWND tree = GetDlgItem(hwnd, IDC_TREE);
                    EnableWindow(tree, FALSE);
                    return TRUE;
                }

                case ID_CREATE_ROOT_FOLDER :
                    DataPackerGUI::FileManager::GetInstance().CreateFolder();
                    return TRUE;

                case ID_CREATE_FOLDER :
                {
                    DataPackerGUI::TreeView& treeview = DataPackerGUI::FileManager::GetInstance().GetLinkedTreeView();
                    std::string path = treeview.GetPath(treeview.GetSelectedItem());
                    DataPackerGUI::FileManager::GetInstance().CreateFolder(path);
                    return TRUE;
                }

                case ID_DELETE_BINARY :
                case ID_DELETE_FOLDER :
                {
                    DataPackerGUI::TreeView& treeview = DataPackerGUI::FileManager::GetInstance().GetLinkedTreeView();
                    std::string path = treeview.GetPath(treeview.GetSelectedItem());
                    DataPackerGUI::FileManager::GetInstance().DeleteObject(path);
                    return TRUE;
                }

                case ID_RENAME_BINARY :
                case ID_RENAME_FOLDER :
                {
                    HTREEITEM item = DataPackerGUI::FileManager::GetInstance().GetLinkedTreeView().GetSelectedItem();
                    DataPackerGUI::FileManager::GetInstance().GetLinkedTreeView().EditLabel(item);
                    return TRUE;
                }

                case ID_SETTYPE_IMAGE :
                {
                    DataPackerGUI::TreeView& treeview = DataPackerGUI::FileManager::GetInstance().GetLinkedTreeView();
                    std::string path = treeview.GetPath(treeview.GetSelectedItem());
                    DataPackerGUI::FileManager::GetInstance().SetNodeType(path, DataPacker::Packer::DATA_TYPE::IMAGE_FILE);
                    return TRUE;
                }

                case ID_SETTYPE_MESH :
                {
                    DataPackerGUI::TreeView& treeview = DataPackerGUI::FileManager::GetInstance().GetLinkedTreeView();
                    std::string path = treeview.GetPath(treeview.GetSelectedItem());
                    DataPackerGUI::FileManager::GetInstance().SetNodeType(path, DataPacker::Packer::DATA_TYPE::MESH_DATA);
                    return TRUE;
                }

                case ID_SETTYPE_BONETREE :
                {
                    DataPackerGUI::TreeView& treeview = DataPackerGUI::FileManager::GetInstance().GetLinkedTreeView();
                    std::string path = treeview.GetPath(treeview.GetSelectedItem());
                    DataPackerGUI::FileManager::GetInstance().SetNodeType(path, DataPacker::Packer::DATA_TYPE::BONE_TREE);
                    return TRUE;
                }

                case ID_SETTYPE_MATERIAL :
                {
                    DataPackerGUI::TreeView& treeview = DataPackerGUI::FileManager::GetInstance().GetLinkedTreeView();
                    std::string path = treeview.GetPath(treeview.GetSelectedItem());
                    DataPackerGUI::FileManager::GetInstance().SetNodeType(path, DataPacker::Packer::DATA_TYPE::MATERIAL_DATA);
                    return TRUE;
                }
            } 
        }
    } 
    return FALSE; 
}
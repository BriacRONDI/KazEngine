#include "FileManager.h"

namespace DataPackerGUI
{
    FileManager* FileManager::instance = nullptr;

    FileManager& FileManager::CreateInstance(HWND& hwnd, TreeView* tree_view)
    {
        if(FileManager::instance == nullptr) {
            FileManager::instance = new FileManager;
            FileManager::instance->hwnd = hwnd;
            FileManager::instance->tree_view = tree_view;
            tree_view->AddListener(FileManager::instance);
        }

        return *FileManager::instance;
    }

    void FileManager::DestroyInstance()
    {
        if(FileManager::instance != nullptr) {
            if(FileManager::instance->tree_view != nullptr) {
                FileManager::instance->tree_view->RemoveListener(FileManager::instance);
                FileManager::instance->tree_view = nullptr;
            }
            delete FileManager::instance;
        }
        FileManager::instance = nullptr;
    }

    void FileManager::NewFile()
    {
        this->CloseFile();
        
        this->need_save = true;
        this->UpdateTitle();
        EnableWindow(this->tree_view->GetHwnd(), TRUE);

        this->tree_view->InsertItem("root");
    }

    void FileManager::OpenFile()
    {
        this->CloseFile();

        // Prepare a Windows OpenFileDialog
        TCHAR szFile[1024];
        szFile[0] = '\0';

	    OPENFILENAME ofn = {sizeof(OPENFILENAME)};
	    ofn.lStructSize = sizeof(OPENFILENAME);
	    ofn.hwndOwner = this->hwnd; 
	    ofn.lpstrFilter = L"KazEngine Package\0*.kea";
	    ofn.nFilterIndex = 1;
	    ofn.lpstrFile = szFile;
	    ofn.nMaxFile = sizeof(szFile);
	    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        ofn.lpstrDefExt = L"kea";

        // Open Save File Dialog
	    if(GetOpenFileName(&ofn)) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            this->file_path = converter.to_bytes(szFile);
        }else{
            // User just closed the dialog box
            return;
        }

        // An empty path means something went wrong
        if(this->file_path.empty()) {
            MessageBox(nullptr, L"Unable to open file.", L"KazEngine DataPacker", MB_ICONERROR);
            return;
        }

        // Update window title
        this->UpdateTitle();

        // Load data and fill TreeView
        this->data = Tools::GetBinaryFileContents(this->file_path);
        this->RefreshTreeView();

        // Enable TreeView
        EnableWindow(this->tree_view->GetHwnd(), TRUE);
        SetFocus(this->tree_view->GetHwnd());
    }

    void FileManager::UpdateTitle()
    {
        if(this->file_path.empty()) {
            if(this->need_save) SetWindowText(this->hwnd, L"KazEngine DataPacker [new.kea*]");
            else SetWindowText(this->hwnd, L"KazEngine DataPacker");
        }else{
            std::string filename = Tools::GetFileName(this->file_path);
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring asterisk = this->need_save ? L"*" : L"";
            std::wstring utf16_filename = L"KazEngine DataPacker [" + converter.from_bytes(filename) + asterisk + L"]";
            SetWindowText(this->hwnd, utf16_filename.c_str());
        }
    }

    void FileManager::BuildTreeFromPackage(HTREEITEM parent, std::vector<DataPacker::Packer::DATA> const& package)
    {
        for(auto const& pack : package) {
            int image = -1;
            if(pack.type == DataPacker::Packer::PARENT_NODE) image = 1;
            else if(pack.type == DataPacker::Packer::MESH_DATA) image = 2;
            else if(pack.type == DataPacker::Packer::MATERIAL_DATA) image = 3;
            else if(pack.type == DataPacker::Packer::IMAGE_FILE) image = 4;

            HTREEITEM item = this->tree_view->InsertItem(pack.name, parent, TVI_LAST, image, image);
            this->BuildTreeFromPackage(item, pack.children);
        }
    }

    void FileManager::CloseFile()
    {
        if(this->need_save) {
            bool must_save = MessageBox(nullptr, L"Save changes before closing ?", L"KazEngine DataPacker", MB_YESNO) == IDYES;
            if(must_save && !this->SaveFile()) return;
            this->need_save = false;
        }

        this->file_path.clear();
        this->data.clear();
        this->tree_view->Clear();
        this->UpdateTitle();
    }

    bool FileManager::CloseApplication()
    {
        int user_reply = IDNO;
        if(this->need_save) {
            user_reply = MessageBox(nullptr, L"Save changes before closing ?", L"KazEngine DataPacker", MB_YESNOCANCEL);
            if(user_reply == IDYES && !this->SaveFile()) return false;
        }

        if(user_reply == IDCANCEL) return false;
        this->data.clear();
        this->tree_view->Clear();
        return true;
    }

    bool FileManager::SaveFile(bool save_as)
    {
        if(save_as || this->file_path.empty()) {

            // Prepare a Windows SaveFileDialog
            TCHAR szFile[1024];
            szFile[0] = '\0';

	        OPENFILENAME ofn = {sizeof(OPENFILENAME)};
	        ofn.lStructSize = sizeof(OPENFILENAME);
	        ofn.hwndOwner = this->hwnd; 
	        ofn.lpstrFilter = L"KazEngine Package\0*.kea";
	        ofn.nFilterIndex = 1;
	        ofn.lpstrFile = szFile;
	        ofn.nMaxFile = sizeof(szFile);
	        ofn.Flags = OFN_PATHMUSTEXIST;
            ofn.lpstrDefExt = L"kea";

            // Open Save File Dialog
	        if(GetSaveFileName(&ofn)) {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                this->file_path = converter.to_bytes(szFile);
            }

            // User closed dialog box
            if(this->file_path.empty()) return false;
        }

        // Error during data writing
        if(!Tools::WriteToFile(this->data, this->file_path)) {
            MessageBox(nullptr, L"Unable to save file.", L"KazEngine DataPacker", MB_ICONERROR);
            return false;
        }

        // Change title and state
        this->need_save = false;
        this->UpdateTitle();

        // Success
        return true;
    }

    void FileManager::OnContextMenu(std::string const& path, Tools::Point<uint32_t> const& position)
    {
        // There's no context menu in white area
        if(path.empty()) return;

        // Get context menu target type
        DataPacker::Packer::DATA_TYPE item_type = DataPacker::Packer::GetNodeType(this->data, path);
        uint32_t menu_id;
        if(item_type == DataPacker::Packer::DATA_TYPE::ROOT_NODE) menu_id = IDR_TREE_CONTEXT_ROOT;
        else if(item_type == DataPacker::Packer::DATA_TYPE::PARENT_NODE) menu_id = IDR_TREE_CONTEXT_FOLDER;
        else menu_id = IDR_TREE_CONTEXT_BINARY;

        // Compute context menu position
        POINT pt = {static_cast<LONG>(position.x), static_cast<LONG>(position.y)};
        ClientToScreen(this->tree_view->GetHwnd(), &pt);

        // Load context menu
        HMENU hmenu = LoadMenu(NULL, MAKEINTRESOURCE(menu_id));
        hmenu = GetSubMenu(hmenu, 0);

        // Open context menu
        TrackPopupMenu(hmenu, TPM_LEFTBUTTON, pt.x, pt.y, 0, this->hwnd, NULL);
    }

    void FileManager::OnLabelEdit(std::string const& path, std::string const& label)
    {
        // Get item reference
        HTREEITEM item = this->tree_view->FindItem(path);

        // Root label cannot be changed
        if(this->tree_view->IsRootItem(item)) return;

        // Check if name already exist
        std::string new_path = path;
        if(new_path[new_path.size() - 1] == '/') new_path = new_path.substr(0, new_path.size() - 1);
        size_t pos = new_path.find_last_of('/');
        new_path = new_path.substr(0, pos) + "/" + label;
        HTREEITEM already_exist = this->tree_view->FindItem(new_path);
        if(already_exist != nullptr) {
            MessageBox(nullptr, L"Name must be unique", L"Error", MB_ICONERROR);
            return;
        }

        // Change item label and node name
        this->tree_view->SetItemName(item, label);
        DataPacker::Packer::SetNodeName(this->data, path, label);
        this->need_save = true;
        this->UpdateTitle();
    }

    void FileManager::OnDropItem(std::string const& source_path, std::string const& dest_path)
    {
        bool moved = DataPacker::Packer::MoveNode(this->data, source_path, dest_path);
        if(moved) {
            this->RefreshTreeView();
            this->need_save = true;
            this->UpdateTitle();
        }
    }

    void FileManager::RefreshTreeView()
    {
        this->tree_view->Clear();
        std::vector<DataPacker::Packer::DATA> package = DataPacker::Packer::UnpackMemory(this->data);
        HTREEITEM root = this->tree_view->InsertItem("root");
        this->BuildTreeFromPackage(root, package);
    }

    void FileManager::OnDropFiles(std::vector<std::wstring> const& files_path, std::string const& dest_path)
    {
        // Unpack data
        std::vector<DataPacker::Packer::DATA> data_table = DataPacker::Packer::UnpackMemory(this->data);

        // Find destination node
        DataPacker::Packer::DATA data = DataPacker::Packer::FindPackedItem(data_table, dest_path);

        // Destination node must be a container
        if(!DataPacker::Packer::IsContainer(data.type)) return;

        for(auto const& path : files_path) {

            // Check file extension
            std::wstring lower_path = path;
            std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), std::tolower);
            size_t ext_pos = lower_path.find_last_of('.');
            if(ext_pos != std::wstring::npos) {
                std::wstring extension = lower_path.substr(ext_pos + 1, lower_path.size() - ext_pos - 1);

                // File extension is FBX
                if(extension == L"fbx") {

                    Log::Terminal::Open();

                    // Get file content
                    std::vector<char> fbx_data = Tools::GetBinaryFileContents(path);

                    // Get file name without extension
                    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                    std::string filename = converter.to_bytes(Tools::GetFileName(path.substr(0, ext_pos))).data();

                    // Texture directory is where the file is located
                    std::string texture_path = converter.to_bytes(path).data();
                    texture_path = Tools::GetFileDirectory(texture_path);
                    
                    // Parse FBX
                    FbxParser parser;
                    std::vector<char> package = parser.ParseData(fbx_data, texture_path);

                    // Pack data to destination path
                    std::unique_ptr<char> package_ptr(package.data());
                    DataPacker::Packer::PackToMemory(this->data, dest_path, DataPacker::Packer::DATA_TYPE::MODEL_TREE, filename, package_ptr, static_cast<uint32_t>(package.size()));
                    package_ptr.release();
                    this->need_save = true;

                } else {

                    // Get file name without extension
                    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                    std::string filename = converter.to_bytes(Tools::GetFileName(path.substr(0, ext_pos))).data();

                    // Get file content
                    std::vector<char> kea_data = Tools::GetBinaryFileContents(path);

                    DataPacker::Packer::DATA_TYPE data_type;
                    if(extension == L"kea") data_type = DataPacker::Packer::DATA_TYPE::MODEL_TREE;
                    else if(extension == L"png" || extension == L"bmp" || extension == L"jpg") data_type = DataPacker::Packer::DATA_TYPE::IMAGE_FILE;
                    else DataPacker::Packer::DATA_TYPE::BINARY_DATA;

                    // Copy data to destination path
                    std::unique_ptr<char> package_ptr(kea_data.data());
                    DataPacker::Packer::PackToMemory(this->data, dest_path, data_type, filename, package_ptr, static_cast<uint32_t>(kea_data.size()));
                    package_ptr.release();
                    this->need_save = true;
                    
                }
            }
        }

        Log::Terminal::Close();

        // Refresh treeview control and title
        this->RefreshTreeView();
        this->UpdateTitle();
    }

    void FileManager::CreateFolder(std::string const& path)
    {
        std::string folder_name = "New Folder";
        HTREEITEM already_exist = this->tree_view->FindItem(path + "/" + folder_name);
        uint8_t i = 1;
        while(already_exist != nullptr) {
            folder_name = "New Folder" + std::to_string(i);
            already_exist = this->tree_view->FindItem(path + "/" + folder_name);
            if(i == UINT8_MAX) {
                MessageBox(nullptr, L"Are you serious ?", L"Error", MB_ICONERROR);
                return;
            }
            i++;
        }

        DataPacker::Packer::PackToMemory(this->data, path, DataPacker::Packer::PARENT_NODE, folder_name, {}, 0);
        HTREEITEM parent = this->tree_view->FindItem(path);
        HTREEITEM folder = this->tree_view->InsertItem(folder_name, parent, TVI_SORT, 1, 1);
        this->tree_view->Expand(parent);
        this->need_save = true;
        this->UpdateTitle();
    }

    void FileManager::DeleteObject(std::string const& path)
    {
        HTREEITEM item = this->tree_view->FindItem(path);
        this->tree_view->DeleteItem(item);
        DataPacker::Packer::RemoveNode(this->data, path);
        this->need_save = true;
        this->UpdateTitle();
    }

    void FileManager::SetNodeType(std::string const& path, DataPacker::Packer::DATA_TYPE type)
    {
        DataPacker::Packer::SetNodeType(this->data, path, type);

        int image = -1;
        if(type == DataPacker::Packer::PARENT_NODE) image = 1;
        else if(type == DataPacker::Packer::MESH_DATA) image = 2;
        else if(type == DataPacker::Packer::MATERIAL_DATA) image = 3;
        else if(type == DataPacker::Packer::IMAGE_FILE) image = 4;

        HTREEITEM item = this->tree_view->FindItem(path);
        this->tree_view->SetItemImage(item, image);
    }
}
#include "TreeView.h"

namespace DataPackerGUI
{
    HHOOK TreeView::hook_handle[2] = {};
    std::vector<TreeView*> TreeView::treeviews;

    TreeView::TreeView(HWND hwnd)
    {
        // Hook TreeView messages
        if(TreeView::hook_handle[0] == nullptr) TreeView::hook_handle[0] = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, nullptr, GetCurrentThreadId());
        if(TreeView::hook_handle[1] == nullptr) TreeView::hook_handle[1] = SetWindowsHookEx(WH_CALLWNDPROC, Hookproc, nullptr, GetCurrentThreadId());

        this->refcount = 0;
        HRESULT drag_register = RegisterDragDrop(hwnd, this);

        TreeView::treeviews.push_back(this);

        this->hwnd          = hwnd;
        this->image_list    = nullptr;
        this->root          = nullptr;
        this->drag_image    = nullptr;
        this->dragged_item  = nullptr;
    }

    TreeView::~TreeView()
    {
        if(TreeView::treeviews.size() == 1) {
            TreeView::treeviews.clear();
            UnhookWindowsHookEx(TreeView::hook_handle[0]);
            UnhookWindowsHookEx(TreeView::hook_handle[1]);
            return;
        }

        RevokeDragDrop(this->hwnd);

        for(auto it = TreeView::treeviews.begin(); it != TreeView::treeviews.end(); it++) {
            if(*it == this) {
                TreeView::treeviews.erase(it);
                return;
            }
        }
    }

    void TreeView::SetImageList(HIMAGELIST image_list)
    {
        this->image_list = image_list;
        TreeView_SetImageList(this->hwnd, image_list, TVSIL_NORMAL);
    };

    HTREEITEM TreeView::InsertItem(std::string const& text, HTREEITEM parent, HTREEITEM insert_after, int image, int selected_image, const void* data)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring item_text = converter.from_bytes(text);

        uint32_t mask = TVIF_TEXT | TVIF_STATE;
        if(image != -1) mask |= TVIF_IMAGE;
        if(selected_image != -1) mask |= TVIF_SELECTEDIMAGE;
        if(data != nullptr) mask |= TVIF_PARAM;

        TCHAR pszText[64] = {};
        TVINSERTSTRUCT insert = {};
        std::memcpy(pszText, item_text.data(), item_text.size() * sizeof(wchar_t));
        insert.hParent = parent;
        insert.hInsertAfter = insert_after;
        insert.item.mask = mask;
        insert.item.pszText = pszText;
        insert.item.cchTextMax = sizeof(pszText);
        insert.item.iImage = image;
        insert.item.iSelectedImage = selected_image;
        insert.item.lParam = reinterpret_cast<LPARAM>(data);
        insert.item.state = TVIS_EXPANDED;

        if(parent == TVI_ROOT || parent == nullptr) insert.item.state |= TVIS_SELECTED | TVIS_BOLD;
        insert.item.stateMask = insert.item.state;

        HTREEITEM item = TreeView_InsertItem(this->hwnd, &insert);

        if(parent == TVI_ROOT || parent == nullptr) {
            this->root = item;
            SetFocus(this->hwnd);
        }

        return item;
    }

    void TreeView::Clear()
    {
        TreeView_DeleteAllItems(this->hwnd);
    }

    TV_ITEM TreeView::GetItem(HTREEITEM handle, std::wstring& pszText)
    {
        TV_ITEM item = {};
        item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_PARAM;
        item.hItem = handle;
        item.pszText = &pszText[0];
        item.cchTextMax = static_cast<int>(pszText.size() * sizeof(TCHAR));
        TreeView_GetItem(this->hwnd, &item);

        return item;
    }

    std::string TreeView::GetPath(HTREEITEM handle)
    {
        if(handle == this->root) return "/";

        std::wstring pszText;
        pszText.resize(64);
        TV_ITEM item = this->GetItem(handle, pszText);
        HTREEITEM parent = TreeView_GetParent(this->hwnd, handle);

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string name = converter.to_bytes(pszText).c_str();

        if(parent == this->root) return "/" + name;
        else return this->GetPath(parent) + "/" + name;
    }

    HTREEITEM TreeView::FindItem(std::string const& path)
    {
        return this->FindItem(root, path);
    }

    HTREEITEM TreeView::FindItem(HTREEITEM parent, std::string path)
    {
        // Path is root
        if(!path.size() || path == "/") return this->root;

        // Take first element of path
        size_t pos = path.find_first_of('/');
        if(pos == 0) {
            path = path.substr(1);
            pos = path.find_first_of('/');
        }

        std::string name;
        if(pos == std::string::npos) name = path;
        else name = path.substr(0, pos);

        // Search for target element inside current element children
        std::vector<HTREEITEM> children = this->GetChildren(parent);
        for(auto child : children) {

            // Get the child name
            std::string child_name = this->GetItemName(child);

            // Element found
            if(child_name == name) {
                
                // If end of path is reached, return element
                if(pos == std::string::npos) return child;

                // If end of path is not reached, explore children
                return this->FindItem(child, path.substr(pos + 1));
            }
        }

        return nullptr;
    }

    TreeView* TreeView::GetTreeView(HWND hwnd)
    {
        for(auto tree_view : TreeView::treeviews)
            if(tree_view->hwnd == hwnd)
                return tree_view;

        return nullptr;
    }

    void TreeView::SetItemName(HTREEITEM handle, std::string const& name)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring pszText = converter.from_bytes(name);

        TV_ITEM item = {};
        item.hItem = handle;
        item.mask = TVIF_TEXT;
        item.pszText = &pszText[0];
        item.cchTextMax = static_cast<int>(pszText.size() * sizeof(pszText[0]));

        TreeView_SetItem(this->hwnd, &item);
    }

    void TreeView::SetItemImage(HTREEITEM handle, int image)
    {
        TV_ITEM item = {};
        item.hItem = handle;
        item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        item.iImage = image;
        item.iSelectedImage = image;

        TreeView_SetItem(this->hwnd, &item);
    }

    std::string TreeView::GetItemName(HTREEITEM handle)
    {
        std::wstring pszText(64, 0);

        TV_ITEM item = {};
        item.hItem = handle;
        item.mask = TVIF_TEXT;
        item.pszText = &pszText[0];
        item.cchTextMax = static_cast<int>(pszText.size() * sizeof(pszText[0]));

        TreeView_GetItem(this->hwnd, &item);

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(pszText).c_str();
    }

    std::vector<HTREEITEM> TreeView::GetChildren(HTREEITEM handle)
    {
        std::vector<HTREEITEM> children;
        HTREEITEM child = TreeView_GetChild(this->hwnd, handle);

        if(child == nullptr) return children;
        else children.push_back(child);

        while(child != nullptr) {
            child = TreeView_GetNextSibling(this->hwnd, child);
            children.push_back(child);
        }

        return children;
    }

    HRESULT TreeView::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
    {
        SetFocus(this->hwnd);
        return S_OK;
    }

    HRESULT TreeView::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
    {
        // find out if the pointer is on a item
        // If so, highlight the item as a drop target.
        TVHITTESTINFO hit_test;
        hit_test.pt = {pt.x, pt.y};
        ScreenToClient(this->hwnd, &hit_test.pt);
        HTREEITEM targetItem = TreeView_HitTest(this->hwnd, &hit_test);
        if(hit_test.hItem != nullptr) this->SelectItem(hit_test.hItem);

        return S_OK;
    }

    HRESULT TreeView::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
    {
        TVHITTESTINFO hit_test;
        hit_test.pt = {pt.x, pt.y};
        ScreenToClient(this->hwnd, &hit_test.pt);
        HTREEITEM targetItem = TreeView_HitTest(this->hwnd, &hit_test);

        // Find the drop target
        if(hit_test.hItem != nullptr) {
            this->SelectItem(hit_test.hItem);
            std::string dest_path = this->GetPath(hit_test.hItem);

            // Retrieve files from IDataObject
            // Source : https://devblogs.microsoft.com/oldnewthing/20100503-00/?p=14183
            FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            STGMEDIUM stgm;
            if(SUCCEEDED(pDataObj->GetData(&fmte, &stgm))) {
                HDROP hdrop = reinterpret_cast<HDROP>(stgm.hGlobal);
                UINT cFiles = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0);
                std::vector<std::wstring> files_path;
                for(UINT i = 0; i < cFiles; i++) {
                    std::wstring szFile(MAX_PATH, 0);
                    UINT cch = DragQueryFile(hdrop, i, &szFile[0], MAX_PATH);
                    if(cch > 0 && cch < MAX_PATH) files_path.push_back(szFile.data());
                }
                ReleaseStgMedium(&stgm);

                // Call listeners
                for(auto& listener : this->Listeners) listener->OnDropFiles(files_path, dest_path);
            }

        }

        return S_OK;
    }

    HRESULT TreeView::QueryInterface(REFIID riid, void **ppvObject)
    {
        if(riid == IID_IUnknown || riid == IID_IDropTarget) {
            *ppvObject = reinterpret_cast<IUnknown*>(this);
            this->AddRef();
            return S_OK;
        }

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    LRESULT CALLBACK TreeView::Hookproc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if(nCode == HC_ACTION) {

            CWPSTRUCT& uMsg = *reinterpret_cast<CWPSTRUCT*>(lParam);

            if(uMsg.message == WM_NOTIFY)
            {
                LPNMHDR nmhdr = (LPNMHDR)uMsg.lParam;
                DataPackerGUI::TreeView* target = DataPackerGUI::TreeView::GetTreeView(nmhdr->hwndFrom);

                // Irrelevant message, just leave
                if(target == nullptr) return CallNextHookEx(TreeView::hook_handle[1], nCode, wParam, lParam);

                switch(nmhdr->code)
                {
                    case TVN_ENDLABELEDIT :
                    {
                        NMTVDISPINFO& disp_info = *reinterpret_cast<NMTVDISPINFO*>(uMsg.lParam);
                        
                        if(disp_info.item.hItem == nullptr || disp_info.item.pszText == nullptr) break;
                        std::string path = disp_info.item.hItem != nullptr ? target->GetPath(disp_info.item.hItem) : std::string();
                        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                        std::string label = converter.to_bytes(disp_info.item.pszText).c_str();

                        for(auto& listener : target->Listeners) listener->OnLabelEdit(path, label);
                        break;
                    }

                    case TVN_BEGINDRAG :
                    {
                        NMTREEVIEW& nm_treeview = *reinterpret_cast<NMTREEVIEW*>(uMsg.lParam);
                        target->drag_image = TreeView_CreateDragImage(target->hwnd, nm_treeview.itemNew.hItem);
                        target->dragged_item = nm_treeview.itemNew.hItem;
                        ImageList_BeginDrag(target->drag_image, 0, 0, 0);
                        ImageList_DragEnter(target->hwnd, nm_treeview.ptDrag.x, nm_treeview.ptDrag.y);
                        ShowCursor(false);
                        break;
                    }

                    case TVN_ITEMCHANGED :
                    {
                        NMTVITEMCHANGE & item_change = *reinterpret_cast<NMTVITEMCHANGE *>(uMsg.lParam);
                        int a = 0;
                        break;
                    }
                }
            }
        }

        return CallNextHookEx(TreeView::hook_handle[1], nCode, wParam, lParam); 
    }

    LRESULT CALLBACK TreeView::GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if(nCode == HC_ACTION) {

            MSG& uMsg = *reinterpret_cast<MSG*>(lParam);

            // Check if message is sent to a treeview
            TreeView* target = TreeView::GetTreeView(uMsg.hwnd);

            // Irrelevant message, just leave
            if(target == nullptr) return CallNextHookEx(TreeView::hook_handle[0], nCode, wParam, lParam);

            switch(uMsg.message) {

                /*case WM_DROPFILES :
                {
                    MessageBox(nullptr, L"Filed dropped !", L"Message", MB_OK);
                    break;
                }*/

                case WM_RBUTTONUP :
                {
                    if(IsWindowEnabled(uMsg.hwnd)) {
                        TVHITTESTINFO hit_test;
                        hit_test.pt = {LOWORD(uMsg.lParam), HIWORD(uMsg.lParam)};
                        TreeView_HitTest(uMsg.hwnd, &hit_test);

                        std::string path;
                        if(hit_test.hItem != nullptr) {
                            path = target->GetPath(hit_test.hItem);
                            target->SelectItem(hit_test.hItem);
                        }
                        
                        Engine::Point<uint32_t> position = {static_cast<uint32_t>(hit_test.pt.x), static_cast<uint32_t>(hit_test.pt.y)};
                        for(auto& listener : target->Listeners) listener->OnContextMenu(path, position);
                    }
                    break;
                }

                case WM_LBUTTONUP :
                {
                    if(target->drag_image != nullptr) {
                        ImageList_EndDrag();
                        ImageList_Destroy(target->drag_image);
                        ShowCursor(true);
                        target->drag_image = nullptr;

                        POINT point = {LOWORD(uMsg.lParam), HIWORD(uMsg.lParam)};
                        TVHITTESTINFO hit_test;
                        hit_test.pt = point;
                        TreeView_HitTest(uMsg.hwnd, &hit_test);
                        if(hit_test.hItem != nullptr) {
                            target->SelectItem(hit_test.hItem);
                            std::string source_path = target->GetPath(target->dragged_item);
                            std::string dest_path = target->GetPath(hit_test.hItem);
                            target->dragged_item = nullptr;
                            for(auto& listener : target->Listeners) listener->OnDropItem(source_path, dest_path);
                        }
                    }
                }

                case WM_MOUSEMOVE :
                {
                    if(target->drag_image != nullptr) {

                        // drag the item to the current the cursor position
                        POINT point = {LOWORD(uMsg.lParam), HIWORD(uMsg.lParam)};
                        ImageList_DragMove(point.x, point.y);

                        // hide the dragged image, so the background can be refreshed
                        ImageList_DragShowNolock(false);

                        // find out if the pointer is on a item
                        // If so, highlight the item as a drop target.
                        TVHITTESTINFO hit_test;
                        hit_test.pt = point;
                        HTREEITEM targetItem = TreeView_HitTest(uMsg.hwnd, &hit_test);
                        if(hit_test.hItem != nullptr) target->SelectItem(hit_test.hItem);

                        // show the dragged image
                        ImageList_DragShowNolock(true);
                    }
                    break;
                }
            }
        }

        return CallNextHookEx(TreeView::hook_handle[0], nCode, wParam, lParam);
    }
}
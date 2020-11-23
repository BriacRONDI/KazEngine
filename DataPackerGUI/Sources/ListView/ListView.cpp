#include "ListView.h"

namespace DataPackerGUI
{
    HHOOK ListView::hook_handle = nullptr;
    std::vector<ListView*> ListView::listviews;
    uint32_t ListView::next_subclass_id = 0;

    ListView::ListView(HWND hwnd)
    {
        this->hwnd = hwnd;
        this->subcalss_id = ListView::next_subclass_id;
        ListView::next_subclass_id++;

        if(ListView::hook_handle == nullptr) ListView::hook_handle = SetWindowsHookEx(WH_CALLWNDPROC, ListView::Hookproc, nullptr, GetCurrentThreadId());
        SetWindowSubclass(hwnd, &ListView::SubClassProc, this->subcalss_id, 0);
        ListView::listviews.push_back(this);

        ListView_SetExtendedListViewStyle(this->hwnd, LVS_EX_FULLROWSELECT);
        this->MouseOverLink = false;
    }

    ListView::~ListView()
    {
        if(this->hwnd != nullptr) RemoveWindowSubclass(this->hwnd, &ListView::SubClassProc, this->subcalss_id);

        if(ListView::listviews.size() == 1) {
            ListView::listviews.clear();
            UnhookWindowsHookEx(ListView::hook_handle);
            return;
        }

        for(auto it = ListView::listviews.begin(); it != ListView::listviews.end(); it++) {
            if(*it == this) {
                ListView::listviews.erase(it);
                return;
            }
        }
    }

    void ListView::Reset()
    {
        this->Clear();
        HWND header = ListView_GetHeader(this->hwnd);
        int column_count = Header_GetItemCount(header);
        while(column_count > 0) {
            Header_DeleteItem(header, 0);
            column_count = Header_GetItemCount(header);
        }
    }

    ListView* ListView::GetListView(HWND hwnd)
    {
        for(auto list_view : ListView::listviews)
            if(list_view->hwnd == hwnd)
                return list_view;

        return nullptr;
    }

    void ListView::Insert(std::string field, std::string value, int position)
    {
        TCHAR pszText[64] = {};

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring item_text = converter.from_bytes(field);
        std::memcpy(pszText, item_text.data(), item_text.size() * sizeof(wchar_t));

        LV_ITEM item = {};
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = pszText;
        item.iItem = position;
        ListView_InsertItem(this->hwnd, &item);

        item_text = converter.from_bytes(value);
        std::memset(pszText, 0, sizeof(pszText));
        std::memcpy(pszText, item_text.data(), item_text.size() * sizeof(wchar_t));
        item.iSubItem = 1;
        ListView_SetItem(this->hwnd, &item);
    }

    void ListView::InsertColumn(std::string name, int width)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring item_text = converter.from_bytes(name);

        TCHAR pszText[64] = {};
        std::memcpy(pszText, item_text.data(), item_text.size() * sizeof(wchar_t));

        HWND header = ListView_GetHeader(this->hwnd);
        if(header == nullptr) return;
        int column_id = Header_GetItemCount(header);
        if(column_id < 0) return;

        LV_COLUMN column = {};
        column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        column.fmt = LVCFMT_LEFT;
        column.pszText = pszText;
        column.cx = width;
        column.iSubItem = column_id;
        ListView_InsertColumn(this->hwnd, column_id, &column);
    }

    void ListView::Display(std::shared_ptr<Model::Mesh> mesh)
    {
        this->Insert("Type",        "Mesh",         0);
        this->Insert("Name",        mesh->name,     1);
        this->Insert("Texture",     mesh->texture,  2);
        this->Insert("Vertices",    "#buffer",      3);
        this->Insert("Indices",     "#buffer",      4);
        this->Insert("UV",          "#buffer",      5);
        this->Insert("Dependances", "#buffer",      6);
        if(!mesh->skeleton.empty()) this->Insert("Skeleton", mesh->skeleton, 7);

        /*for(uint8_t i=0; i<mesh->materials.size(); i++) {
            this->Insert("Material", "#" + mesh->materials[i].first, i + 5);
        }*/
    }

    void ListView::Display(std::vector<Maths::Vector2> buffer)
    {
        this->Reset();

        this->InsertColumn("ID", 75);
        this->InsertColumn("X", 132);
        this->InsertColumn("Y", 133);

        for(uint32_t i=0; i<buffer.size(); i++)
        {
            TCHAR pszText[64] = {};

            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring item_text = converter.from_bytes(std::to_string(i));
            std::memcpy(pszText, item_text.data(), item_text.size() * sizeof(wchar_t));

            LV_ITEM item = {};
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = pszText;
            item.iItem = i;
            ListView_InsertItem(this->hwnd, &item);

            for(uint8_t j=0; j<2; j++) {
                std::memset(pszText, 0, item_text.size() * sizeof(wchar_t));
                item_text = converter.from_bytes(std::to_string(buffer[i].value[j]));
                std::memcpy(pszText, item_text.data(), item_text.size() * sizeof(wchar_t));
                item.iSubItem = j+1;
                ListView_SetItem(this->hwnd, &item);
            }
        }
    }

    void ListView::Display(std::vector<Maths::Vector3> buffer)
    {
        this->Reset();

        this->InsertColumn("ID", 75);
        this->InsertColumn("X", 88);
        this->InsertColumn("Y", 88);
        this->InsertColumn("Z", 89);

        for(uint32_t i=0; i<buffer.size(); i++)
        {
            TCHAR pszText[64] = {};

            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring item_text = converter.from_bytes(std::to_string(i));
            std::memcpy(pszText, item_text.data(), item_text.size() * sizeof(wchar_t));

            LV_ITEM item = {};
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = pszText;
            item.iItem = i;
            ListView_InsertItem(this->hwnd, &item);

            for(uint8_t j=0; j<3; j++) {
                std::memset(pszText, 0, item_text.size() * sizeof(wchar_t));
                item_text = converter.from_bytes(std::to_string(buffer[i].value[j]));
                std::memcpy(pszText, item_text.data(), item_text.size() * sizeof(wchar_t));
                item.iSubItem = j+1;
                ListView_SetItem(this->hwnd, &item);
            }
        }
    }

    void ListView::Display(std::vector<uint32_t> buffer)
    {
        this->Reset();

        this->InsertColumn("ID", 75);
        this->InsertColumn("Value", 265);

        for(uint32_t i=0; i<buffer.size(); i++)
            this->Insert(std::to_string(i), std::to_string(buffer[i]), i);
    }

    /*void ListView::Display(Model::Mesh::MATERIAL material)
    {
        this->Reset();

        this->InsertColumn("Property", 75);
        this->InsertColumn("Value", 265);

        this->Insert("Ambient", std::to_string(material.ambient.x) + ", " + std::to_string(material.ambient.y) + ", " + std::to_string(material.ambient.z) + ", " + std::to_string(material.ambient.w), 0);
        this->Insert("Diffuse", std::to_string(material.diffuse.x) + ", " + std::to_string(material.diffuse.y) + ", " + std::to_string(material.diffuse.z) + ", " + std::to_string(material.diffuse.w), 1);
        this->Insert("Specular", std::to_string(material.specular.x) + ", " + std::to_string(material.specular.y) + ", " + std::to_string(material.specular.z) + ", " + std::to_string(material.specular.w), 2);
        this->Insert("Transparency", std::to_string(material.transparency), 3);
        if(!material.texture.empty()) this->Insert("Texture", material.texture, 4);
    }*/

    void ListView::Display(std::pair<std::string, std::vector<uint32_t>> material)
    {
        this->Reset();

        if(material.second.empty()) {
            this->InsertColumn("Property", 75);
            this->InsertColumn("Value", 265);

            this->Insert("Name", material.first, 0);
            this->Insert("Scope", "Global", 1);

        } else {
            this->InsertColumn("ID", 75);
            this->InsertColumn("Vertex Index", 265);

            for(uint32_t i=0; i<material.second.size(); i++)
                this->Insert(std::to_string(i), std::to_string(material.second[i]), i);
        }
    }

    void ListView::Display(std::vector<std::string> buffer)
    {
        this->Reset();

        this->InsertColumn("ID", 75);
        this->InsertColumn("Path", 265);

        for(uint8_t i=0; i<buffer.size(); i++)
            this->Insert(std::to_string(i), buffer[i], i);
    }

    bool ListView::IsObject(int row)
    {
        std::wstring pszText(64, 0);

        LV_ITEM item = {};
        item.iItem = row;
        item.iSubItem = 1;
        item.mask = LVIF_TEXT;
        item.pszText = &pszText[0];
        item.cchTextMax = static_cast<int>(pszText.size() * sizeof(pszText[0]));
        ListView_GetItem(this->hwnd, &item);
        return pszText.substr(0, 1) == L"#";
    }

    std::string ListView::GetSubItemText(int item, int sub_item)
    {
        std::wstring pszText(64, 0);

        LV_ITEM lvitem = {};
        lvitem.iItem = item;
        lvitem.iSubItem = sub_item;
        lvitem.mask = LVIF_TEXT;
        lvitem.pszText = &pszText[0];
        lvitem.cchTextMax = static_cast<int>(pszText.size() * sizeof(pszText[0]));
        ListView_GetItem(this->hwnd, &lvitem);
        
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(pszText).data();
    }

    LRESULT ListView::ProcessCustomDraw(LPARAM lParam)
    {
        LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

        ListView* target = ListView::GetListView(lplvcd->nmcd.hdr.hwndFrom);
        if(target == nullptr) return CDRF_DODEFAULT;

        switch(lplvcd->nmcd.dwDrawStage) 
        {
            case CDDS_PREPAINT :
                return CDRF_NOTIFYITEMDRAW;

            case CDDS_ITEMPREPAINT:
                return CDRF_NOTIFYSUBITEMDRAW;
            
            case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
            {
                int item_index = static_cast<int>(lplvcd->nmcd.dwItemSpec);
                int subitem_index = static_cast<int>(lplvcd->iSubItem);
                if(subitem_index == 1 && target->IsObject(item_index)) {

                    HFONT hf = CreateFont(14, 0, 0, 0, 0, 0, TRUE, 0, 0, 0, 0, 0, 0, L"Arial");
                    SelectObject(lplvcd->nmcd.hdc, hf);
                    lplvcd->clrText   = RGB(0,0,255);
                    return CDRF_NEWFONT;
                }
            }
        }
        return CDRF_DODEFAULT;
    }

    LRESULT CALLBACK ListView::Hookproc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if(nCode == HC_ACTION) {

            CWPSTRUCT& uMsg = *reinterpret_cast<CWPSTRUCT*>(lParam);

            if(uMsg.message == WM_NOTIFY)
            {
                LPNMHDR nmhdr = (LPNMHDR)uMsg.lParam;
                DataPackerGUI::ListView* target = DataPackerGUI::ListView::GetListView(nmhdr->hwndFrom);

                // Irrelevant message, just leave
                if(target == nullptr) return CallNextHookEx(ListView::hook_handle, nCode, wParam, lParam);

                switch(nmhdr->code)
                {
                    case NM_CLICK :
                    {
                        LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)uMsg.lParam;
                        for(auto& listener : target->Listeners)
                            listener->OnLvItemSelect(*target, lpnmitem->iItem);
                        break;
                    }
                }
            }
        }

        return CallNextHookEx(ListView::hook_handle, nCode, wParam, lParam); 
    }

    LRESULT CALLBACK ListView::SubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        switch(uMsg)
        {
            case WM_SETCURSOR :
            {
                ListView* target = ListView::GetListView(hWnd);
                if(target != nullptr && target->MouseOverLink) {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                    return TRUE;
                }
                break;
            }

            case WM_MOUSEMOVE :
            {
                ListView* target = ListView::GetListView(hWnd);
                if(target != nullptr) {
                    POINT point = {LOWORD(lParam), HIWORD(lParam)};
                    LVHITTESTINFO hit_test;
                    hit_test.pt = point;
                    ListView_SubItemHitTest(hWnd, &hit_test);
                    if(target->IsObject(hit_test.iItem)) {
                        if(!target->MouseOverLink) target->MouseOverLink = true;
                    }else{
                        if(target->MouseOverLink) {
                            target->MouseOverLink = false;
                            SetCursor(LoadCursor(nullptr, IDC_ARROW));
                        }
                    }
                    return TRUE;
                }
                break;
            }

            case WM_NCDESTROY :
            {
                RemoveWindowSubclass(hWnd, &ListView::SubClassProc, 1);
                return TRUE;
            }
        }

        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
}
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Commctrl.h>
#include <string>
#include <locale>
#include <codecvt>
#include <vector>
#include <ole2.h>
#include <shellapi.h>

#include "../../Resources/resource.h"
#include "../../Tools/Sources/EventEmitter.hpp"
#include "ITreeViewListener.h"

namespace DataPackerGUI
{
    class TreeView : public Tools::EventEmitter<ITreeViewListener>, public IDropTarget
    {
        public :
            /**
             * Initialize TreeView
             * @param hwnd Handle to TreeView dialog window
             */
            TreeView(HWND hwnd = nullptr);

            /// Free Memory
            ~TreeView();

            /**
             * Combine an ImageList with a TreeView
             * @param image_list ImageList to combine
             */
            void SetImageList(HIMAGELIST image_list);

            /// Retrieve TreeView dialog window handle
            inline HWND& GetHwnd() { return this->hwnd; }

            /**
             * Insert an item in the TreeView
             * @pram text Item name
             * @param parent Parent handle, TVI_ROOT for root node
             * @param insert_after TVI_FIRST : insert as first, TVI_LAST : insert at last, TVI_SORT : sort in alphabetical order
             * @return Handle of inserted item
             */
            HTREEITEM InsertItem(std::string const& text, HTREEITEM parent = TVI_ROOT, HTREEITEM insert_after = TVI_LAST,
                                 int image = -1, int selected_image = -1, const void* data = nullptr);

            /**
             * Get absolute path to node
             * @param handle Target item handle
             * @return Absolute item path
             */
            std::string GetPath(HTREEITEM handle);

            /**
             * Find an item by path
             * @param path Target item absolute path
             * @return Item reference, nullptr if not found
             */
            HTREEITEM FindItem(std::string const& path);

            /**
             * Get element children
             * @param handle Parent
             * @return Vector of children handles
             */
            std::vector<HTREEITEM> GetChildren(HTREEITEM handle);

            /**
             * Expand item
             * @param handle Item to expand
             */
            inline void Expand(HTREEITEM handle) { TreeView_Expand(this->hwnd, handle, TVE_EXPAND); }

            /**
             * Set label to edit mode
             * @param handle Item to edit
             */
            inline void EditLabel(HTREEITEM handle) { TreeView_EditLabel(this->hwnd, handle); }

            /**
             * Get a TreeView from handled list
             * @param hwnd TreeVIew dialog windows handle
             */
            static TreeView* GetTreeView(HWND hwnd);

            /**
             * Set the label of an item
             * @param handle Reference to item
             * @param name Desired value
             */
            void SetItemName(HTREEITEM handle, std::string const& name);

            /**
             * Remove specified item and all its children
             * @param handle Reference to item
             */
            inline void DeleteItem(HTREEITEM handle) { TreeView_DeleteItem(this->hwnd, handle); }

            /**
             * Get the label of an item
             * @param handle Reference to item
             * @return Label value
             */
            std::string GetItemName(HTREEITEM handle);

            /**
             * Check if this item is root
             * @param handle Reference to item
             * @retval true Item is root
             * @retval false Item is not root
             */
            inline bool IsRootItem(HTREEITEM handle) { return handle == this->root; }

            /// Get a reference to the selected item
            inline HTREEITEM GetSelectedItem() { return TreeView_GetSelection(this->hwnd); }

            /**
             * Select the given item
             * @param handle Reference to item
             */
            inline void SelectItem(HTREEITEM handle) { TreeView_SelectItem(this->hwnd, handle); }
            
            /// Clear all items
            void Clear();

            ///////////////////////////
            // IDropTarget interface //
            ///////////////////////////

            HRESULT DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
            inline HRESULT DragLeave() { return S_OK; };
            HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
            HRESULT Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

            HRESULT QueryInterface(REFIID riid, void **ppvObject);
            inline ULONG AddRef() { return InterlockedIncrement(&this->refcount); }
            inline ULONG Release() { return InterlockedDecrement(&this->refcount); }

        private :

            /// TreeView dialog window handle
            HWND hwnd;

            /// Combined ImageList handle
            HIMAGELIST image_list;

            /// Root item handle
            HTREEITEM root;

            /// Item being dragged
            HTREEITEM dragged_item;

            /// ImageList used for drag and drop operations
            HIMAGELIST drag_image;

            /**
             * Get item object from handle
             * @param handle Item handle
             * @param pszText Output buffer for item name
             * @return Item details
             */
            TV_ITEM GetItem(HTREEITEM handle, std::wstring& pszText);

            /**
             * Find an item inside a tree
             * @param parent Item from which to search
             * @param path relative path to item
             * @retval nullptr If not found
             * @retval Reference to found item if success
             */
            HTREEITEM FindItem(HTREEITEM parent, std::string path);

            // HTREEITEM GetItem(std::string path);

            /// Hook handle to intercept messages sent to a TreeView dialog window
            static HHOOK hook_handle[2];

            /// List of hooked TreeView dialog windows
            static std::vector<TreeView*> treeviews;

            // Reference count used by IDropTarget interface
            ULONG refcount;

            /// Hook callback procedure for WH_GETMESSAGE
            static LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);

            /// Hook callback procedure for WH_CALLWNDPROCRET
            static LRESULT CALLBACK Hookproc(int nCode, WPARAM wParam, LPARAM lParam);
    };
}
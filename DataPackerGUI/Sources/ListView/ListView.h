#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Commctrl.h>
#include <string>
#include <locale>
#include <codecvt>
#include <vector>

#include <DataPacker.h>
#include <Model.h>
#include <EventEmitter.hpp>

#include "IListViewListener.h"

namespace DataPackerGUI
{
    class ListView : public Tools::EventEmitter<IListViewListener>
    {
        public :
            
            /// Initialize
            ListView(HWND hwnd = nullptr);

            /// Destructor
            ~ListView();

            /**
             * Get a ListView from handled list
             * @param hwnd ListView dialog windows handle
             */
            static ListView* GetListView(HWND hwnd);

            /// Retrieve List control window handle
            inline HWND& GetHwnd() { return this->hwnd; }

            /// Remove all items
            inline void Clear() { ListView_DeleteAllItems(this->hwnd); }

            /// Remove all items and columns
            void Reset();

            /// Insert string in the list
            void Insert(std::string field, std::string value, int position = -1);

            /// Add a column to header
            void InsertColumn(std::string name, int width);

            /// Process ListView custom redraw message in order to set custom graphic style
            static LRESULT ProcessCustomDraw(LPARAM lParam);

            /// Display mesh properties
            void Display(std::shared_ptr<Model::Mesh> mesh);

            /// Display a vec2 buffer
            void Display(std::vector<Maths::Vector2> buffer);

            /// Display a vec3 buffer
            void Display(std::vector<Maths::Vector3> buffer);

            /// Display a material
            void Display(Model::Mesh::MATERIAL material);

            /// Display an uint buffer
            void Display(std::vector<uint32_t> buffer);

            /// Display a material index buffer
            void Display(std::pair<std::string, std::vector<uint32_t>> material);

            /// Display dependancy buffer
            void Display(std::vector<std::string> buffer);

            /// Make the listview visible
            inline void Show() { ShowWindow(this->hwnd, TRUE); }

            /// Make the listview invisible
            inline void Hide() { ShowWindow(this->hwnd, FALSE); }

            /// Get field value for row "item" and column "sub_item"
            std::string GetSubItemText(int item, int sub_item);

        private :
            
            /// ListVIew window handle
            HWND hwnd;

            /// Is this row a buffer
            bool IsObject(int row);

            /// The mouse is currently over a link
            bool MouseOverLink;

            /// Subclass identifier
            uint32_t subcalss_id;

            /// List of hooked ListView dialog windows
            static std::vector<ListView*> listviews;

            /// Hook handle to intercept messages sent to a ListView dialog window
            static HHOOK hook_handle;

            /// Hook callback procedure for WH_CALLWNDPROC
            static LRESULT CALLBACK Hookproc(int nCode, WPARAM wParam, LPARAM lParam);

            /// Component subclassing procedure
            static LRESULT CALLBACK SubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

            /// Unique subclass ID
            static uint32_t next_subclass_id;
    };
}
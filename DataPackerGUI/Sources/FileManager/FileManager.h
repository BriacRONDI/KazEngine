#pragma once

#include <locale>
#include <codecvt>
#include <Windows.h>
// #include <algorithm>

// #include <Tools.h>
#include <Types.hpp>
#include "../../Resources/resource.h"
#include "../../DataPacker/Sources/DataPacker.h"
#include "../TreeView/TreeView.h"
#include "../Import/Import.h"
#include "../ListView/ListView.h"

namespace DataPackerGUI
{
    /**
     * Allow GUI to open/close/save files
     */
    class FileManager : public ITreeViewListener, public IListViewListener
    {
        public :

            /// Singleton instance creation
            static FileManager& CreateInstance(HWND& hwnd, TreeView* tree_view, ListView* list_view_main, ListView* list_view_inspect);

            /// Singleton instance access
            static inline FileManager& GetInstance() { return *FileManager::instance; }

            /// Singleton instance destruction
            static void DestroyInstance();

            /// Get the linked TreeView
            inline TreeView& GetLinkedTreeView() { return *this->tree_view; }

            /// Get the linked ListView
            inline ListView& GetMainListView() { return *this->list_view_main; }

            /// Open a file
            void OpenFile();

            /**
             * Save data to file
             * @param save_as Prompt user to choose path
             * @retval true Success
             * @retval false Failure
             */
            bool SaveFile(bool save_as = false);

            /// Close current file
            void CloseFile();

            /**
             * Attempt to close application
             * @retval true Application can be closed
             * @retval false Cancel closing application
             */
            bool CloseApplication();

            /// New File
            void NewFile();

            /**
             * Create a container node at specified location
             * @param path Folder location
             */
            void CreateFolder(std::string const& path = {});

            /**
             * Delete the specified object from data
             * @param path Object location
             */
            void DeleteObject(std::string const& path);

            /// Complete rebuild of TreeView from raw data
            void RefreshTreeView();

            /**
             * Change the data type of the specified node
             * @param path Object location
             * @param type New type
             */
            void SetNodeType(std::string const& path, DataPacker::Packer::DATA_TYPE type);

            ///////////////////////
            // ITreeViewListener //
            ///////////////////////

            virtual void OnContextMenu(std::string const& path, Engine::Point<uint32_t> const& position);
            virtual void OnLabelEdit(std::string const& path, std::string const& label);
            virtual void OnDropItem(std::string const& source_path, std::string const& dest_path);
            virtual void OnDropFiles(std::vector<std::wstring> const& files_path, std::string const& dest_path);
            virtual void OnTvItemSelect(std::string const& path);

            ///////////////////////
            // IListViewListener //
            ///////////////////////

            virtual void OnLvItemSelect(ListView& list_view, int item_index);

        private :

            /// Singleton instance
            static FileManager* instance;

            /// Raw data
            std::vector<char> data;

            /// File path
            std::string file_path;

            /// File needs to be saved
            bool need_save = false;

            /// Main window handle
            HWND hwnd = nullptr;

            /// Linked TreeView
            TreeView* tree_view = nullptr;

            /// Object properties ListView
            ListView* list_view_main = nullptr;

            /// Properties values ListView
            ListView* list_view_inspect = nullptr;

            /// Display filename in window title
            void UpdateTitle();

            /// Fill the TreeView with parsed data
            void BuildTreeFromPackage(HTREEITEM parent, std::vector<DataPacker::Packer::DATA> const& package);

            /// Singleton constructor
            FileManager() = default;

            /// Singleton destructor
            ~FileManager() = default;

    };
}
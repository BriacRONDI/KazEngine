#pragma once

#include <Types.hpp>

namespace DataPackerGUI
{
    /// Interface : Allow classes to intercept TreeView events
    class ITreeViewListener
    {
        public :
            
            /**
             * Open context menu
             * @param path of target item
             * @param position Mouse cursor position
             */
            virtual void OnContextMenu(std::string const& path, Engine::Point<uint32_t> const& position) = 0;

            /**
             * A label has been edited
             * @param path of target item
             * @param label Label value
             */
            virtual void OnLabelEdit(std::string const& path, std::string const& label) = 0;

            /**
             * An item has been drag-dropped to an other
             * @param source_path Source item path
             * @param dest_path Target item path
             */
            virtual void OnDropItem(std::string const& source_path, std::string const& dest_path) = 0;

            /**
             * Files have been drag-dropped from an external application
             * @param files_path Absolute source files path
             * @param dest_path Target item path
             */
            virtual void OnDropFiles(std::vector<std::wstring> const& files_path, std::string const& dest_path) = 0;

            /**
             * An item has been selected
             * @param path of target item
             */
            virtual void OnTvItemSelect(std::string const& path) = 0;
    };
}

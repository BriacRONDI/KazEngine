#pragma once

#include <string>
#include "ListView.h"

namespace DataPackerGUI
{
    class ListView;

    /// Interface : Allow classes to intercept ListView events
    class IListViewListener
    {
        public :

            /**
             * An item has been selected
             * @param item_index Selected index
             */
            virtual void OnLvItemSelect(ListView& list_view, int item_index) = 0;
    };
}

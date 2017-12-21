/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "config.h"

#include "UnitTestUtils/EWK2UnitTestBase.h"
#include "UnitTestUtils/EWK2UnitTestEnvironment.h"
#include <Ecore.h>

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

static bool testFinished = false;

static const Ewk_Context_Menu_Item_Type customItemType = EWK_ACTION_TYPE;
static const Ewk_Context_Menu_Item_Action customItemTag = EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG;
static const char customItemTitle[] = "Custom Item";
static const bool customItemChecked = false;
static const bool customItemEnabled = true;

static const char webkitItemTitle1[] = "Open Link";
static const char webkitItemTitle2[] = "Open Link in New Window";
static const char webkitItemTitle3[] = "Download Linked File";
static const char webkitItemTitle4[] = "Copy Link Location";
static const char webkitItemNewTitle[] = "Copy Link";

class EWK2ContextMenuTest : public EWK2UnitTestBase {
public:
    static void checkBasicContextMenuItem(Ewk_Context_Menu_Item* item, Ewk_Context_Menu_Item_Type type, Ewk_Context_Menu_Item_Action action, const char* title, Eina_Bool checked, Eina_Bool enabled)
    {
        ASSERT_TRUE(item);

        EXPECT_EQ(type, ewk_context_menu_item_type_get(item));
        EXPECT_EQ(action, ewk_context_menu_item_action_get(item));
        EXPECT_STREQ(title, ewk_context_menu_item_title_get(item));
    EXPECT_EQ(checked, ewk_context_menu_item_checked_get(item));
    EXPECT_EQ(enabled, ewk_context_menu_item_enabled_get(item));
    }

    static Eina_Bool customItemSelected(Ewk_View_Smart_Data*, Ewk_Context_Menu_Item* item)
    {
        checkBasicContextMenuItem(item, customItemType, static_cast<Ewk_Context_Menu_Item_Action>(customItemTag), customItemTitle, customItemChecked, customItemEnabled);

        return true;
    }

    /**
     * This callback receives context menu called upon link with items generated by WebKit2.
     * Items have their properties modified, and one of items is selected.
     */
    static Eina_Bool testContextMenu(Ewk_View_Smart_Data*, Evas_Coord, Evas_Coord, Ewk_Context_Menu* contextMenu)
    {
        const Eina_List* list = ewk_context_menu_items_get(contextMenu);
        EXPECT_EQ(4, eina_list_count(list));

        Ewk_Context_Menu_Item* item = static_cast<Ewk_Context_Menu_Item*>(eina_list_nth(list, 0));
        checkBasicContextMenuItem(item, EWK_ACTION_TYPE, EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK, webkitItemTitle1, false, true);

        item = static_cast<Ewk_Context_Menu_Item*>(eina_list_nth(list, 1));
        checkBasicContextMenuItem(item, EWK_ACTION_TYPE, EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW, webkitItemTitle2, false, true);
        ewk_context_menu_item_enabled_set(item, false);
        EXPECT_FALSE(ewk_context_menu_item_enabled_get(item));

        item = static_cast<Ewk_Context_Menu_Item*>(eina_list_nth(list, 2));
        checkBasicContextMenuItem(item, EWK_ACTION_TYPE, EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_LINK_TO_DISK, webkitItemTitle3, false, true);
        ewk_context_menu_item_checked_set(item, true);
        EXPECT_TRUE(ewk_context_menu_item_checked_get(item));

        item = static_cast<Ewk_Context_Menu_Item*>(eina_list_nth(list, 3));
        checkBasicContextMenuItem(item, EWK_ACTION_TYPE, EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD, webkitItemTitle4, false, true);
        ewk_context_menu_item_title_set(item, webkitItemNewTitle);
        EXPECT_STREQ(webkitItemNewTitle, ewk_context_menu_item_title_get(item));

        // When context menu is created by WebKit, an item should have parent menu.
        EXPECT_EQ(contextMenu, ewk_context_menu_item_parent_menu_get(item));

        item = static_cast<Ewk_Context_Menu_Item*>(eina_list_nth(list, 0));

        EXPECT_TRUE(ewk_context_menu_item_select(contextMenu, item));

        return true;
    }

    /**
     * This callback receives context menu called upon link with items generated by WebKit2.
     * Received items are removed, then custom item is created and selected.
     */
    static Eina_Bool testContextMenuForRemoveAndAppend(Ewk_View_Smart_Data*, Evas_Coord, Evas_Coord, Ewk_Context_Menu* contextMenu)
    {
        const Eina_List* list = ewk_context_menu_items_get(contextMenu);
        EXPECT_EQ(4, eina_list_count(list));

        const Eina_List* listIterator;
        void* data;
        EINA_LIST_FOREACH(list, listIterator, data)
            ewk_context_menu_item_remove(contextMenu, static_cast<Ewk_Context_Menu_Item*>(data));

        list = ewk_context_menu_items_get(contextMenu);
        EXPECT_EQ(0, eina_list_count(list));

        Ewk_Context_Menu_Item* newItem = ewk_context_menu_item_new(customItemType, customItemTag, customItemTitle, customItemChecked, customItemEnabled);

        // When context menu item is created using ewk_context_menu_item_new, it should not have parent menu.
        EXPECT_EQ(0, ewk_context_menu_item_parent_menu_get(newItem));
        ewk_context_menu_item_append(contextMenu, newItem);

        // When context menu item is added to menu using ewk_context_menu_item_append, it should have parent menu.
        EXPECT_EQ(contextMenu, ewk_context_menu_item_parent_menu_get(newItem));

        // When non submenu type context menu item was created using ewk_context_menu_item_new, submenu should be 0.
        EXPECT_EQ(0, ewk_context_menu_item_submenu_get(newItem));

        EXPECT_TRUE(ewk_context_menu_item_select(contextMenu, newItem));

        ewk_context_menu_hide(contextMenu);

        return true;
    }

    /**
     * This callback receives context menu called upon link with items generated by WebKit2.
     * New submenus are created, one of submenu's item is selected.
     */
    static Eina_Bool testContextMenuForSubmenu(Ewk_View_Smart_Data*, Evas_Coord, Evas_Coord, Ewk_Context_Menu* contextMenu)
    {
        Eina_List* subMenuItemList = 0;
        Ewk_Context_Menu_Item* subMenuItem1 = ewk_context_menu_item_new(customItemType, customItemTag, customItemTitle, customItemChecked, customItemEnabled);
        Ewk_Context_Menu_Item* subMenuItem2 = ewk_context_menu_item_new(EWK_ACTION_TYPE, static_cast<Ewk_Context_Menu_Item_Action>(customItemTag + 1), "New SubMenu Item 2", false, true);
        Ewk_Context_Menu_Item* subMenuItem3 = ewk_context_menu_item_new(EWK_ACTION_TYPE, static_cast<Ewk_Context_Menu_Item_Action>(customItemTag + 2), "New SubMenu Item 3", false, true);
        subMenuItemList = eina_list_append(subMenuItemList, subMenuItem1);
        subMenuItemList = eina_list_append(subMenuItemList, subMenuItem2);
        subMenuItemList = eina_list_append(subMenuItemList, subMenuItem3);
        Ewk_Context_Menu* subMenu = ewk_context_menu_new_with_items(subMenuItemList);

        // When context menu is created using ewk_context_menu_new_with_items, items should have parent menu.
        EXPECT_EQ(subMenu, ewk_context_menu_item_parent_menu_get(subMenuItem1));

        Ewk_Context_Menu_Item* newItem = ewk_context_menu_item_new_with_submenu(EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG, "New Custom Item", true, subMenu);
        // When context menu item is created using ewk_context_menu_item_new_with_submenu, it should not have parent menu.
        EXPECT_EQ(0, ewk_context_menu_item_parent_menu_get(newItem));
        ewk_context_menu_item_append(contextMenu, newItem);

        // Context menu received from ewk_context_menu_item_submenu_get should be the same as one used to create sub menu.
        EXPECT_EQ(subMenu, ewk_context_menu_item_submenu_get(newItem));

        // When context menu item is created using ewk_context_menu_item_new_with_submenu and added using ewk_context_menu_item_append it should have parent menu.
        EXPECT_EQ(contextMenu, ewk_context_menu_item_parent_menu_get(newItem));

        Ewk_Context_Menu* subMenu2 = ewk_context_menu_new();
        Ewk_Context_Menu_Item* newItem2 = ewk_context_menu_item_new_with_submenu(EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG, "New Custom Item 2", true, subMenu2);
        ewk_context_menu_item_append(contextMenu, newItem2);

        EXPECT_TRUE(ewk_context_menu_item_select(ewk_context_menu_item_parent_menu_get(subMenuItem1), subMenuItem1));

        ewk_context_menu_hide(ewk_context_menu_item_parent_menu_get(subMenuItem1));

        return true;
    }

    static Eina_Bool finishTest(Ewk_View_Smart_Data*)
    {
        testFinished = true;

        return true;
    }

protected:
    void showContextMenu()
    {
        mouseClick(10, 20, /*Right*/ 3);
    }
};

TEST_F(EWK2ContextMenuTest, ewk_context_menu_item_select)
{
    const char* itemSelectHTML =
        "<html>"
        "<body><a href=http://www.google.com>Test Link</a></body>"
        "</html>";

    ewkViewClass()->context_menu_show = testContextMenu;

    ewk_view_html_string_load(webView(), itemSelectHTML, "file:///", 0);
    ASSERT_TRUE(waitUntilLoadFinished());
    showContextMenu();
    ASSERT_TRUE(waitUntilLoadFinished());
}

TEST_F(EWK2ContextMenuTest, ewk_context_menu_custom_items)
{
    const char* itemSelectHTML =
        "<html>"
        "<body><a href=http://www.google.com>Test Link</a></body>"
        "</html>";

    ewkViewClass()->custom_item_selected = customItemSelected;
    ewkViewClass()->context_menu_show = testContextMenuForRemoveAndAppend;
    ewkViewClass()->context_menu_hide = finishTest;

    ewk_view_html_string_load(webView(), itemSelectHTML, "file:///", 0);
    ASSERT_TRUE(waitUntilLoadFinished());
    showContextMenu();
    testFinished = false;
    ASSERT_TRUE(waitUntilTrue(testFinished));

    ewkViewClass()->context_menu_show = testContextMenuForSubmenu;
    showContextMenu();
    testFinished = false;
    ASSERT_TRUE(waitUntilTrue(testFinished));
}

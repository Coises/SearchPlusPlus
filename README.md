# Search++: An enhanced search plugin for Notepad++.

**Search++** is an enhanced search plugin for Notepad++.

**Search++** aims to present the full Unicode search I developed for [Columns++](https://github.com/Coises/ColumnsPlusPlus) in a more flexible and user-friendly manner.

**The current version is a work in progress. It is not feature-complete and has not undergone thorough testing yet. Use with caution.**

Like Notepad++, this software is released under the GNU General Public License (either version 3 of the License, or, at your option, any later version). Some files which do not contain any dependencies on Notepad++ are released under the [MIT License](https://www.opensource.org/licenses/MIT); see the individual source files for details.

Search++ uses the [C++ Mathematical Expression Toolkit Library
(ExprTk)](https://github.com/ArashPartow/exprtk) by Arash Partow (https://www.partow.net/) and [JSON for Modern C++](https://github.com/nlohmann/json) by Niels Lohmann (https://nlohmann.me), which are released under the [MIT License](https://www.opensource.org/licenses/MIT); the [Boost.Regex library](https://github.com/boostorg/regex/), which is released under the [Boost Software License, Version 1.0](https://www.boost.org/LICENSE_1_0.txt); and [ICU4C](https://unicode-org.github.io/icu/userguide/icu4c/), which is released under the [Unicode License](https://www.unicode.org/license.txt). Search++ was built using [NppCppMSVS](https://github.com/Coises/NppCppMSVS).

## Features

* Search++ can be used as a docking dialog or an ordinary dialog. An advantage of the docking dialog is that search results and context will never wind up hidden behind the dialog. As an ordinary dialog, Search++ can be oriented either horizontally or vertically. By default, Search++ opens docked to the right side of Notepad++. You can change that with the first option in the **Settings...** dialog. (As with any docking window in Notepad++, you *can* float the docking dialog by dragging its title bar into the main Notepad++ area; however, Search++ as a regular dialog, chosen from the Settings dialog, is likely to be ”better behaved” than a floating dockable dialog.)

* The Find and Replace fields in Search++ are Scintilla controls. This allows for easy inclusion and visualization of multiple lines, line ending characters and special characters, as well as zooming the fields as desired. Find and replace text is shown using the same font as in the main Notepad++ editing window.

* Search++ can search within selections (including column selections and multiple selections) and within marked text. Commands are available to Find All (send a list of matches to a search results window), Select, Mark or Show (hide all lines, then unhide lines with matches and mark the matches).

* Regular expression searches in Search++ perform a fully Unicode-based search using a customized combination of Boost.Regex and ICU4C. In particular, this produces fewer “surprising” results with Unicode characters above 0xFFFF (including most emoji) and when searching in documents using a DBCS code page (which in Notepad++ can be Chinese, Japanese or Korean files that are in the system default encoding instead of in Unicode).

* Regular expression replacements in Search++ can include numeric calculations. The replacement syntax is similar to the one introduced in Columns++.

* There is a [help file](https://coises.github.io/SearchPlusPlus/help.htm).

## Quirks and features with poor discoverability

* The Find and Replace boxes and the Search++ Results window have right-click menus. There are various commands and shortcuts you won’t discover until you right-click. In particular, history for the Find and Replace windows is on their right-click menus.

* The main command buttons (Find, Count, etc.) have drop-down menus. You can select an alternate action from those menus. If you Shift+click an item on the menu, that becomes the new action for clicking the button.

* The **Tools** button at the top right of the Find box opens a menu of handy functions you might want to use in conjunction with search.

* If you assign a keyboard shortcut to **Search...** on the **Search++** plugin menu, you can use that to open a search dialog or to switch keyboard focus to it when it is already open. You can use Ctrl+O (think “Other”) to switch between the Find and Replace boxes or to switch from the Search Results list to the Find box. You can use Ctrl+N (think “Notepad++”) to switch keyboard focus back to the current Notepad++ editing window, and Ctrl+Shift+N to close the window you are in (either Search or Search Results) and switch focus back to Notepad++.

* You can navigate the Search++ Results window by keyboard using the Tab key to move to the next match (or Shift+Tab to move to the previous match). The Enter key will locate in the document window the current cursor position or selection in the Search++ Results window. Shift+Enter will locate *and* move keyboard focus to the document window. (So you can use Tab to navigate and Enter to see any particular match in context; then when you want to move to that match for editing, use Shift+Enter.)

* Formulas use the format **(?=...)** within the replacement box. You can use multiple formulas in one replacement and mix them with other, ordinary replacement strings. Within a formula, use **$1** for the first capture group, **$2** for the second, etc.; use **$** or **$0** for the entire match. The match or sub-match is interpreted as a number, and you can use ordinary arithmetic. For example, if the Find string matches a number, **(?=$+100)** would be replaced by that number plus 100.

* The **Settings...** dialog (available from the Search++ plugin menu or the Tools menu) has quite a few options you might want to review.

* The **ICU** button at the top is there mostly for testing. It uses the regular expression engine built into ICU, which has different syntax than the familiar Boost.Regex engine and does not integrate as well with Scintilla. Replace is not implemented for this search engine, and it only works on Unicode documents. It will probably be removed when Search++ reaches version 1.0, as it really isn’t very useful except as a check on the results from the main Regex engine (since I’ve meddled with the main Regex engine quite a lot, and I haven’t modified the ICU engine in any way).

## Missing and Planned Features, and things I know don’t work quite right yet

* **Search++** does not yet support Find or Replace in all open documents or in files in a directory. I plan to add those capabilities, but I have not determined how I can/will do it.

* I plan to add a **Save** function that will let you save searches you might want to use again. Of course, once it is possible to save, it has to be possible to delete and rename and edit and organize... I haven’t designed a user interface for any of that yet.

* The Search++ Results window doesn’t show matches clearly in dark mode. I might be able to fix that without requiring user intervention; but depending on user experience, I suspect I might need to add the ability to customize colors and perhaps other details of all the Scintilla windows (Find/Replace and Search Results).

* If you edit a document that has results in the Search++ Results window, the results will no longer be “in sync” with the document and you won’t be able to navigate accurately to matches following the edit point. I don’t know if it is possible to overcome this, but I would like to do so if I find that I can.

* I’ve implemented a sort of “collision detection” when doing stepwise Find or Replace with the regular (non-docking) dialog or locating matches from the search results, so the document will be scrolled as needed to avoid the found text being obscured by the search window. I know it doesn’t work as well as it could yet. (It’s a surprisingly tricky problem to solve, and I couldn’t find any examples of it being solved already.)

* I hope to add more features to the regular expression search. The current version is almost identical to the search in Columns++, but presented in what is hopefully a more flexible and user-friendly interface. It should be more accurate for Unicode-derived properties since it uses ICU4C directly instead of working from the home-grown parse of Unicode tables used in Columns++. If I can work out a way, I hope to add Unicode word breaks and more Unicode properties. 

## Installation

Download the x86 or x64 zip file for the latest [release](https://github.com/Coises/SearchPlusPlus/releases), depending on whether you're using 32-bit or 64-bit Notepad++. Unzip the file to a folder named **Search++** (the name must be exactly that, or Notepad++ will not load the plugin) and copy that folder into the plugins directory where Notepad++ is installed (usually **C:\Program Files (x86)\Notepad++\plugins** for 32-bit versions or **C:\Program Files\Notepad++\plugins** for 64-bit versions).


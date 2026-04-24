# Search++: An enhanced search plugin for Notepad++

## Version 0.5.5 -- April 24th, 2026

* Fix Replace All with ICU search engine should give "Command not implemented" message.
* Copy current line indicator and caret settings from active document to Find and Replace box and search results list.
* Use a more distinct symbol for "in selection" scope on command buttons.
* Make messages for Mark and Show commands more accurate when there are null matches.

## Version 0.5.4 -- April 18th, 2026

* Fix bookmarks and Show command not working with ICU search engine.
* Fix unwanted control character inserted when focus is in Find or Replace box and a keyboard shortcut is used to activate a Tools menu command that opens a dialog.
* Use a custom font for button symbols.

## Version 0.5.3 -- April 11th, 2026

* Avoid a crash when searching **in Marked Text in Open Documents** or **in Marked Text in Documents in this View** and a document has no marked text.
* Fix button menus overlapping the button when the button is near the bottom of the screen. This could cause inadvertent activation of the last menu option.
* Make the Scintilla controls (Find box, Replace box and Results list) change colors when dark mode changes. Fix some visibility problems for caret and found text indicators in dark mode.
* Make **Show All** on the **Tools** menu scroll current position or selection into view.
* Add toggles **Bookmark lines when marking text** and **Jump to next match after Replace** to **Tools** menu.
* Add keyboard shortcuts for most **Tools** menu items.
* Condense the **Replace** button menu and coordinate with the new **Jump to next match after Replace** toggle.

## Version 0.5.2 -- April 8th, 2026

* Fix display of **Settings**: **Mark style** drop-down in dark mode.
* Add a dialog to resist accidentally clicking the Tools menu options to remove marks from documents in view or from all open documents.
* Rearrange [Settings](https://coises.github.io/SearchPlusPlus/help.htm#settings) dialog and add four new settings:
    - Fill after Search... menu command or shortcut even if search dialog is already visible.
    - Allow default Select command to Select in Selection.
    - When eligible selection and marks are present, default search is within selection.
    - Allow default Mark command to Mark in Marked Text.
* Make commands that explicitly specify in Selection or in Marked Text show a failure message (rather than fall back to Whole Document) if there is no selection or marked text.
* Remove **Before** and **After** commands without an explicit scope from command button menus.
* Convert active document to marks (when that setting is checked) only on successful match.
* Change names of stepwise replace commands to **Replace (then find)** and **Replace (then wait)**.
* Update and clarify documentation, particularly the [Search commands](https://coises.github.io/SearchPlusPlus/help.htm#commands) section.
* Fix adding to selection not working when the selection is a rectangular selection. Add note to help regarding unexpected results when adding to existing selections.

## Version 0.5.1 -- April 5th, 2026

* Fix Select Before and After not working.
* Enhance clearing behavior (whether implicit or checked in Settings dialog): don't clear selections, unmark text or hide lines if no matches are found.
* Fix Select Marked Text (formerly Mark -> Selections) on Tools menu including an extra, empty selection.
* Add items to the Tools menu to copy marked text and to remove marks from open documents and documents in this view. Rename some items, hopefully making them more clear. Disable some items when there is nothing to which to apply them.

## Version 0.5 -- April 2nd, 2026

* Add "in Open Documents" and "in Documents in this View" variants of Count, Find All, Mark and Replace All.
* Add ellipsis and balloon tip on hover for status messages that don't fit; some improvements in the text of status messages.
* Don't restrict default ("smart") Mark commands to marked text.
* Miscellaneous changes to wording and layout on command button menus to try to make it make more sense.
* Documentation updates.

## Version 0.4 -- March 30th, 2026

* Add wide layout for better top/bottom docking. Improve layout handling and make sure non-docking dialogs aren't sized too small.
* Add settings (checked by default) to focus the document after Select commands and after Show commands.
* Make behavior of general navigation commands (Ctrl/Ctrl+Shift + H/N/O) more consistent.
* Update documentation.

## Version 0.3 -- March 28th, 2026

* Fix error that caused backward searches to fail in some cases.
* Fix replaced marked text not being marked in some cases.
* Make Shift+click on Find and Replace buttons reverse direction for Plain text searches.
* Add functions to clear the search results list.
* Clarify the search results list context menu a bit.
* Update help file for changes. Clarify the effect of stepwise search in selection.

## Version 0.2 -- March 27th, 2026

* Fix failure to reset `match` counter after a Replace All.
* Add missing logic to make Replace Before and Replace After work properly after stepwise Find or Replace.
* Remember the contents of the Find and Replace boxes between Notepad++ sessions. Sychronize Find and Replace box contents and all settings accurately when switching between the docking dialog and the non-docking dialog.
* Add to the help file to clarify the special cases for Replace Before/After and what does and doesn’t reset the `match` counter. Correct broken links to this project on GitHub in the help file.
* Fix some mistakes in the GitHub project setup: some ICU4C files that were needed were not included, and some that were not needed were included.

## Version 0.1 -- March 25th, 2026

* First release. Still a work in progress.

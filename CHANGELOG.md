# Search++: An enhanced search plugin for Notepad++

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

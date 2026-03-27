# Search++: An enhanced search plugin for Notepad++

## Version 0.2 -- March 27th, 2026

* Fix failure to reset `match` counter after a Replace All.
* Add missing logic to make Replace Before and Replace After work properly after stepwise Find or Replace.
* Remember the contents of the Find and Replace boxes between Notepad++ sessions. Sychronize Find and Replace box contents and all settings accurately when switching between the docking dialog and the non-docking dialog.
* Add to the help file to clarify the special cases for Replace Before/After and what does and doesn’t reset the `match` counter. Correct broken links to this project on GitHub in the help file.
* Fix some mistakes in the GitHub project setup: some ICU4C files that were needed were not included, and some that were not needed were included.

## Version 0.1 -- March 25th, 2026

* First release. Still a work in progress.

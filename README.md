![TTM Banner](/GUI/resources/TTM_Banner.png?raw=true "TTM Banner")


[<img alt="TTM download button" src="/GUI/resources/TTM_download_button.png" />](https://github.com/TobiasM95/WoW-Talent-Tree-Manager/releases/download/v1.3.7/TalentTreeManager.zip)


## What is the WoW Talent Tree Manager?
For the return of talent trees in the upcoming World of Warcraft expansion Dragonflight, this project aims to give players the possibility to easily create, change and share talent tree setups for all classes and even create completely new and custom talent trees for fun. It does provide a simple and clean interface to make the exploration of new talent setups as smooth as possible.
## Features
Features of WoW TTM:
* Talent tree editor
    * View tree meta information
    * Provide/change overall tree description
    * Add/edit/delete talents
    * Reorganize tree structure
    * Load presets
    * Save/load custom trees
    * Export tree as TTM trees (and SimC in the future)
* Loadout editor
    * Manage your loadout, i.e. a selection of multiple skillsets
    * View selected talents in a skillsets
    * Provide/change a loadout description
    * Add/change/delete skillset
    * Export skillset as TTM skillsets (and SimC in the future)
* Loadout solver
    * Find all possible combinations in the currently active talent tree
    * Filter trees by talent points, must-have talents, must-not-have talents
    * Transfer results to loadout for fine tuning

## Gallery
Tree editor main window (dark theme):
![Tree editor main window](/GUI/resources/gallery/TreeEditor1.png?raw=true "Tree editor main window")

Tree editor talent window (dark theme):
![Tree editor talent window](/GUI/resources/gallery/TreeEditor2.png?raw=true "Tree editor talent window")

Loadout editor window (grey theme):
![Loadout editor window](/GUI/resources/gallery/LoadoutEditor.png?raw=true "Loadout editor window")

Loadout solver window (light theme):
![Loadout solver window](/GUI/resources/gallery/LoadoutSolver.png?raw=true "Loadout solver window")

## Feedback & Contribution
Since this is currently a solo project there will be some rough edges, issues and bugs as well as features that might be missing or aren't well thought out. You can provide TTM feedback via Discord ("About" tab in program for more details) and via issues here on Github.
Additionally, if you want to contribute (there's a lot of code cleanup and extension to do) then you are more than welcome to do so.

## For developers
To build this repository, it's best to use VS 2022 (since that is what I'm using).
Updater.cpp includes an internal GUI presets file `TTMGUIPresetsInternal.h`. This file includes a single `std::string PASTEBIN_API_DEV_KEY = "..."` in the namespace `Presets` which is to either provide with your own Pastebin api dev key or left empty (which leads to the Pastebin export functionality not working).

## Credits
Developed by [Tobias Mielich](https://github.com/TobiasM95).

Credits to [Dear ImGui](https://github.com/ocornut/imgui) which is the foundation of the whole GUI and very recommended!

Also used in this project: [libcurl](https://curl.se/libcurl/) & [stb](https://github.com/nothings/stb)

Credits to [Bloodmallet](https://bloodmallet.com/) for giving me the whole idea and talking about this.

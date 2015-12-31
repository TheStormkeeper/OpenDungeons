/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "modes/MenuModeEditorNew.h"

#include "utils/Helper.h"
#include "render/Gui.h"
#include "modes/ModeManager.h"
#include "sound/MusicPlayer.h"
#include "gamemap/GameMap.h"
#include "render/ODFrameListener.h"
#include "network/ODServer.h"
#include "network/ODClient.h"
#include "network/ServerMode.h"
#include "utils/LogManager.h"
#include "gamemap/MapLoader.h"
#include "utils/ResourceManager.h"
#include "utils/ConfigManager.h"
#include "game/Seat.h"

#include <CEGUI/CEGUI.h>

const std::string TEXT_LOADING = "LoadingText";
const std::string BUTTON_LAUNCH = "LevelWindowFrame/LaunchEditorButton";
const std::string BUTTON_BACK = "LevelWindowFrame/BackButton";
const std::string LIST_LEVEL_TYPES = "LevelWindowFrame/LevelTypeSelect";

MenuModeEditorNew::MenuModeEditorNew(ModeManager* modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_EDITOR_NEW)
{
    CEGUI::Window* window = modeManager->getGui().getGuiSheet(Gui::guiSheet::editorNewMenu);

    // Fills the Level type combo box with the available level types.
    const CEGUI::Image* selImg = &CEGUI::ImageManager::getSingleton().get("OpenDungeonsSkin/SelectionBrush");
    CEGUI::Combobox* levelTypeCb = static_cast<CEGUI::Combobox*>(window->getChild(LIST_LEVEL_TYPES));
    levelTypeCb->resetList();

    CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem("Skirmish Level", 0);
    item->setSelectionBrushImage(selImg);
    levelTypeCb->addItem(item);

    item = new CEGUI::ListboxTextItem("Multiplayer Level", 1);
    item->setSelectionBrushImage(selImg);
    levelTypeCb->addItem(item);

    addEventConnection(
        window->getChild(Gui::EDM_BUTTON_LAUNCH)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeEditorNew::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::EDM_BUTTON_BACK)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&AbstractApplicationMode::goBack,
                                     static_cast<AbstractApplicationMode*>(this))
        )
    );
    addEventConnection(
        window->getChild("LevelWindowFrame/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&AbstractApplicationMode::goBack,
                                     static_cast<AbstractApplicationMode*>(this))
        )
    );
}

void MenuModeEditorNew::activate()
{
    // Loads the corresponding Gui sheet.
    getModeManager().getGui().loadGuiSheet(Gui::editorNewMenu);

    giveFocus();

    // Play the main menu music
    MusicPlayer::getSingleton().play(ConfigManager::getSingleton().getMainMenuMusic());

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);

    CEGUI::Combobox* levelTypeCb = static_cast<CEGUI::Combobox*>(getModeManager().getGui().
                                       getGuiSheet(Gui::editorNewMenu)->getChild(LIST_LEVEL_TYPES));
    levelTypeCb->setItemSelectState(static_cast<size_t>(0), true);
}

bool MenuModeEditorNew::launchSelectedButtonPressed(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::guiSheet::editorNewMenu);

    window->getChild(TEXT_LOADING)->setText("Creating Map...");

    // Creating the filename
    CEGUI::Combobox* levelTypeCb = static_cast<CEGUI::Combobox*>(getModeManager().getGui().
                                       getGuiSheet(Gui::editorNewMenu)->getChild(LIST_LEVEL_TYPES));
    std::string levelPath;
    size_t selection = levelTypeCb->getItemIndex(levelTypeCb->getSelectedItem());
    switch (selection)
    {
        default:
        case 0:
            levelPath = ResourceManager::getSingleton().getUserLevelPathSkirmish();
            break;
        case 1:
            levelPath = ResourceManager::getSingleton().getUserLevelPathMultiplayer();
            break;
    }
    CEGUI::Editbox* editWin = static_cast<CEGUI::Editbox*>(window->getChild("LevelWindowFrame/LevelFilenameEdit"));
    // TODO: Check for invalid data
    std::string level = levelPath + "/" + editWin->getText().c_str();

    // Get the map size.
    editWin = static_cast<CEGUI::Editbox*>(window->getChild("LevelWindowFrame/LevelWidthEdit"));
    std::string width_str = editWin->getText().c_str();
    uint32_t width = Helper::toUInt32(width_str);
    editWin = static_cast<CEGUI::Editbox*>(window->getChild("LevelWindowFrame/LevelHeightEdit"));
    std::string height_str = editWin->getText().c_str();
    uint32_t height = Helper::toUInt32(height_str);

    if (width == 0 || height == 0) {
        window->getChild(TEXT_LOADING)->setText("Invalid size...");
        return true;
    }

    // Create the level before opening it.
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);
    gameMap->createNewMap(width, height);
    gameMap->setLevelFileName(level);
    editWin = static_cast<CEGUI::Editbox*>(window->getChild("LevelWindowFrame/LevelTitleEdit"));
    gameMap->setLevelName(editWin->getText().c_str());
    CEGUI::MultiLineEditbox* meditWin = static_cast<CEGUI::MultiLineEditbox*>(
        window->getChild("LevelWindowFrame/LevelDescriptionEdit"));
    gameMap->setLevelDescription(meditWin->getText().c_str());

    // TODO: Dehardcode this part
    gameMap->setTileSetName(""); // default one.
    gameMap->setLevelMusicFile("Searching_yd.ogg");
    gameMap->setLevelFightMusicFile("TheDarkAmulet_MP.ogg");
    Seat* seat = new Seat(gameMap);
    if(!gameMap->addSeat(seat))
    {
        OD_LOG_WRN("Couldn't add seat id=" + Helper::toString(seat->getId()));
        delete seat;
        window->getChild(TEXT_LOADING)->setText("Invalid seat data...");
        return true;
    }

    MapLoader::writeGameMapToFile(level, *gameMap);

    // In editor mode, we act as a server
    window->getChild(TEXT_LOADING)->setText("Loading...");
    ConfigManager& config = ConfigManager::getSingleton();
    std::string nickname = config.getGameValue(Config::NICKNAME, std::string(), false);
    if(!ODServer::getSingleton().startServer(nickname, level, ServerMode::ModeEditor, false))
    {
        OD_LOG_ERR("Could not start server for editor!!!");
        window->getChild(TEXT_LOADING)->setText("ERROR: Could not start server for editor!!!");
    }

    int port = ODServer::getSingleton().getNetworkPort();
    uint32_t timeout = ConfigManager::getSingleton().getClientConnectionTimeout();
    std::string replayFilename = ResourceManager::getSingleton().getReplayDataPath()
        + ResourceManager::getSingleton().buildReplayFilename();
    if(!ODClient::getSingleton().connect("localhost", port, timeout, replayFilename))
    {
        OD_LOG_ERR("Could not connect to server for editor!!!");
        window->getChild(TEXT_LOADING)->setText("Error: Couldn't connect to local server!");
        return true;
    }
    return true;
}

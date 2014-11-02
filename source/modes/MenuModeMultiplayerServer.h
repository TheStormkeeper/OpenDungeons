/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#ifndef MENUMODEMULTIPLAYERSERVER_H
#define MENUMODEMULTIPLAYERSERVER_H

#include "AbstractApplicationMode.h"

class MenuModeMultiplayerServer: public AbstractApplicationMode
{
public:
    MenuModeMultiplayerServer(ModeManager*);

    virtual ~MenuModeMultiplayerServer();

    virtual bool mouseMoved     (const OIS::MouseEvent &arg);
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool keyPressed     (const OIS::KeyEvent &arg);
    virtual bool keyReleased    (const OIS::KeyEvent &arg);
    virtual void handleHotkeys  (OIS::KeyCode keycode);

    void onFrameStarted(const Ogre::FrameEvent& evt) {};
    void onFrameEnded(const Ogre::FrameEvent& evt) {};

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate();

    void serverButtonPressed();
    void updateDescription();

    void listLevelsClicked();
    void listLevelsDoubleClicked();

    virtual bool waitForGameStart()
    {
        return mReadyToStartGame;
    }

private:
    std::vector<std::string> mFilesList;
    std::vector<std::string> mDescriptionList;

    //! \brief Tells whether the menu is ready to start the game.
    bool mReadyToStartGame;
};

#endif // MENUMODEMULTIPLAYERSERVER_H
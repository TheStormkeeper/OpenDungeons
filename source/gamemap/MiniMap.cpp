/*!
 * \file   MiniMap.cpp
 * \date   13 April 2011
 * \author StefanP.MUC
 * \brief  Contains everything that is related to the minimap
 *
 *
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

#include "gamemap/MiniMap.h"

#include "entities/Tile.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"

#include <OgrePrerequisites.h>
#include <OgreTextureManager.h>

#include <CEGUI/BasicImage.h>
#include <CEGUI/Image.h>
#include <CEGUI/ImageManager.h>
#include <CEGUI/PropertyHelper.h>
#include <CEGUI/RendererModules/Ogre/Renderer.h>
#include <CEGUI/Size.h>
#include <CEGUI/System.h>
#include <CEGUI/Texture.h>
#include <CEGUI/Window.h>
#include <CEGUI/WindowManager.h>

MiniMap::MiniMap(CEGUI::Window* miniMapWindow) :
    mMiniMapWindow(miniMapWindow),
    mTopLeftCornerX(0),
    mTopLeftCornerY(0),
    mGrainSize(4),
    mWidth(static_cast<unsigned int>(mMiniMapWindow->getPixelSize().d_width)
           + mGrainSize - (static_cast<unsigned int>(mMiniMapWindow->getPixelSize().d_width) % mGrainSize)),
    mHeight(static_cast<unsigned int>(mMiniMapWindow->getPixelSize().d_height)
            + mGrainSize - (static_cast<unsigned int>(mMiniMapWindow->getPixelSize().d_height) & mGrainSize)),
    mTiles(mHeight, TileColorRow_t(mWidth, Color(0,0,0))),
    mPixelBox(mWidth, mHeight, 1, Ogre::PF_R8G8B8),
    mMiniMapOgreTexture(Ogre::TextureManager::getSingletonPtr()->createManual(
            "miniMapOgreTexture",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,
            mWidth, mHeight, 0, Ogre::PF_R8G8B8,
            Ogre::TU_DYNAMIC_WRITE_ONLY)),
    mPixelBuffer(mMiniMapOgreTexture->getBuffer())
{
    CEGUI::Texture& miniMapTextureGui = static_cast<CEGUI::OgreRenderer*>(CEGUI::System::getSingletonPtr()
                                            ->getRenderer())->createTexture("miniMapTextureGui", mMiniMapOgreTexture);

    CEGUI::BasicImage& imageset = dynamic_cast<CEGUI::BasicImage&>(CEGUI::ImageManager::getSingletonPtr()->create("BasicImage", "MiniMapImageset"));
    imageset.setArea(CEGUI::Rectf(CEGUI::Vector2f(0.0, 0.0),
                                      CEGUI::Size<float>(
                                          static_cast<float>(mWidth), static_cast<float>(mHeight)
                                      )
                                  ));

    // Link the image to the minimap
    imageset.setTexture(&miniMapTextureGui);
    mMiniMapWindow->setProperty("Image", CEGUI::PropertyHelper<CEGUI::Image*>::toString(&imageset));

    mMiniMapOgreTexture->load();

    mTopLeftCornerX = mMiniMapWindow->getUnclippedOuterRect().get().getPosition().d_x;
    mTopLeftCornerY = mMiniMapWindow->getUnclippedOuterRect().get().getPosition().d_y;
}

MiniMap::~MiniMap()
{
    mMiniMapWindow->setProperty("Image", "");
    Ogre::TextureManager::getSingletonPtr()->remove("miniMapOgreTexture");
    CEGUI::ImageManager::getSingletonPtr()->destroy("MiniMapImageset");
    CEGUI::System::getSingletonPtr()->getRenderer()->destroyTexture("miniMapTextureGui");
}

Ogre::Vector2 MiniMap::camera_2dPositionFromClick(int xx, int yy)
{
    Ogre::Real mm, nn, oo, pp;
    // Compute move and normalise
    mm = (xx - mTopLeftCornerX) / static_cast<double>(mWidth) - 0.5;
    nn = (yy - mTopLeftCornerY) / static_cast<double>(mHeight) - 0.5;
    // Applying rotation
    oo = nn * mSinRotation + mm * mCosRotation;
    pp = nn * mCosRotation - mm * mSinRotation;
    // Apply result to camera
    mCamera_2dPosition.x += static_cast<Ogre::Real>(oo * mWidth / mGrainSize);
    mCamera_2dPosition.y -= static_cast<Ogre::Real>(pp * mHeight / mGrainSize);

    return mCamera_2dPosition;
}

void MiniMap::swap()
{
    mPixelBuffer->lock(mPixelBox, Ogre::HardwareBuffer::HBL_NORMAL);

    Ogre::uint8* pDest;
    pDest = static_cast<Ogre::uint8*>(mPixelBuffer->getCurrentLock().data) - 1;

    for (const TileColorRow_t& row : mTiles)
    {
        for(const Color& color : row)
        {
            drawPixelToMemory(pDest, color.RR, color.GG, color.BB);
        }
    }

    mPixelBuffer->unlock();
}

void MiniMap::draw(const GameMap& gameMap)
{
    for (int ii = 0, mm = mCamera_2dPosition.x - mWidth / (2 * mGrainSize); ii < static_cast<int>(mWidth); ++mm, ii += mGrainSize)
    {
        //NOTE: (0,0) is in the bottom left in the game map, top left in textures, so we are reversing y order here.
        for (int jj = static_cast<int>(mHeight) - static_cast<int>(mGrainSize), nn = mCamera_2dPosition.y - mHeight / (2 * mGrainSize);
             jj >= 0; ++nn, jj -= mGrainSize)
        {
            // Applying rotation
            int oo = mCamera_2dPosition.x + static_cast<int>((mm - mCamera_2dPosition.x) * mCosRotation - (nn - mCamera_2dPosition.y) * mSinRotation);
            int pp = mCamera_2dPosition.y + static_cast<int>((mm - mCamera_2dPosition.x) * mSinRotation + (nn - mCamera_2dPosition.y) * mCosRotation);

            /*FIXME: even if we use a THREE byte pixel format (PF_R8G8B8),
             * for some reason it only works if we have FOUR increments
             * (the empty one is the unused alpha channel)
             * this is not how it is intended/expected
             */
            Tile* tile = gameMap.getTile(oo, pp);
            if(tile == nullptr)
            {
                drawPixel(ii, jj, 0x00, 0x00, 0x00);
                continue;
            }

            if (tile->getMarkedForDigging(gameMap.getLocalPlayer()))
            {
                drawPixel(ii, jj, 0xFF, 0xA8, 0x00);
                continue;
            }

            if(tile->isClaimed())
            {
                Seat* tempSeat = tile->getSeat();
                if (tempSeat != nullptr)
                {
                    Ogre::ColourValue color = tempSeat->getColorValue();
                    if (tile->getFullness() <= 0.0)
                        drawPixel(ii, jj, color.r*200.0, color.g*200.0, color.b*200.0);
                    else
                        drawPixel(ii, jj, color.r*255.0, color.g*255.0, color.b*255.0);
                }
                else
                {
                    if (tile->getFullness() <= 0.0)
                        drawPixel(ii, jj, 0x5C, 0x37, 0x1B);
                    else
                        drawPixel(ii, jj, 0x86, 0x50, 0x28);
                }
                continue;
            }

            switch (tile->getType())
            {
            case TileType::water:
                drawPixel(ii, jj, 0x21, 0x36, 0x7A);
                break;
            case TileType::lava:
                drawPixel(ii, jj, 0xB2, 0x22, 0x22);
                break;

            case TileType::dirt:
                if (tile->getFullness() <= 0.0)
                    drawPixel(ii, jj, 0x3B, 0x1D, 0x08);
                else
                    drawPixel(ii, jj, 0x5B, 0x2D, 0x0C);
                break;

            case TileType::rock:
                if (tile->getFullness() <= 0.0)
                    drawPixel(ii, jj, 0x30, 0x30, 0x30);
                else
                    drawPixel(ii, jj, 0x41, 0x41, 0x41);
                break;

            case TileType::gold:
                if (tile->getFullness() <= 0.0)
                    drawPixel(ii, jj, 0x3B, 0x1D, 0x08);
                else
                    drawPixel(ii, jj, 0xB5, 0xB3, 0x2F);
                break;

            case TileType::nullTileType:
                drawPixel(ii, jj, 0x00, 0x00, 0x00);
                break;

            default:
                drawPixel(ii,jj,0x00,0xFF,0x7F);
                break;
            }
        }
    }

    // Draw creatures on map.
    // std::vector<Creature*>::iterator updatedCreatureIndex = mGameMap->creatures.begin();
    // for(; updatedCreatureIndex < mGameMap->creatures.end(); ++updatedCreatureIndex)
    // {
    //     if((*updatedCreatureIndex)->getIsOnMap())
    //     {
    //         double  ii = (*updatedCreatureIndex)->getPosition().x;
    //         double  jj = (*updatedCreatureIndex)->getPosition().y;

    //         drawPixel(ii, jj, 0x94, 0x0, 0x94);
    //     }
    // }
}

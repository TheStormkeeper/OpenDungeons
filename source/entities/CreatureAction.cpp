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

#include "entities/CreatureAction.h"

#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "utils/LogManager.h"

CreatureAction::CreatureAction(Creature& creature, const CreatureActionType actionType, bool forcedAction, GameEntity* entity, Tile* tile,
        CreatureSkillData* creatureSkillData, bool b) :
    mCreature(creature),
    mActionType(actionType),
    mForcedAction(forcedAction),
    mEntity(entity),
    mTile(tile),
    mCreatureSkillData(creatureSkillData),
    mBool(b),
    mNbTurns(0),
    mNbTurnsActive(0)
{
    if(mEntity != nullptr)
        mEntity->addGameEntityListener(this);

    // We check mandatory items according to action type
    switch(mActionType)
    {
        case CreatureActionType::digTile:
            OD_ASSERT_TRUE(mTile != nullptr);
            if(mTile != nullptr)
                mTile->addWorkerDigging(mCreature);
            break;
        case CreatureActionType::claimGroundTile:
        case CreatureActionType::claimWallTile:
            OD_ASSERT_TRUE(mTile != nullptr);
            if(mTile != nullptr)
                mTile->addWorkerClaiming(mCreature);
            break;
        case CreatureActionType::attackObject:
            OD_ASSERT_TRUE(mEntity != nullptr);
            OD_ASSERT_TRUE(mTile != nullptr);
            break;
        case CreatureActionType::grabEntity:
        case CreatureActionType::carryEntity:
            OD_ASSERT_TRUE(mEntity != nullptr);
            if(mEntity != nullptr)
                mEntity->setCarryLock(mCreature, true);
            break;

        default:
            break;
    }

    if(mCreature.getDefinition()->isWorker())
        mCreature.getSeat()->getPlayer()->notifyWorkerAction(mCreature, mActionType);
}

CreatureAction::~CreatureAction()
{
    if(mEntity != nullptr)
        mEntity->removeGameEntityListener(this);

    switch(mActionType)
    {
        case CreatureActionType::digTile:
            if(mTile != nullptr)
                mTile->removeWorkerDigging(mCreature);
            break;
        case CreatureActionType::claimGroundTile:
        case CreatureActionType::claimWallTile:
            if(mTile != nullptr)
                mTile->removeWorkerClaiming(mCreature);
            break;
        case CreatureActionType::grabEntity:
        case CreatureActionType::carryEntity:
            if(mEntity != nullptr)
                mEntity->setCarryLock(mCreature, false);
            break;

        default:
            break;
    }

    if(mCreature.getDefinition()->isWorker())
        mCreature.getSeat()->getPlayer()->notifyWorkerStopsAction(mCreature, mActionType);
}

std::string CreatureAction::toString(CreatureActionType actionType)
{
    switch (actionType)
    {
    case CreatureActionType::walkToTile:
        return "walkToTile";

    case CreatureActionType::fight:
        return "fight";

    case CreatureActionType::fightArena:
        return "fightArena";

    case CreatureActionType::searchTileToDig:
        return "searchTileToDig";

    case CreatureActionType::digTile:
        return "digTile";

    case CreatureActionType::searchWallTileToClaim:
        return "searchWallTileToClaim";

    case CreatureActionType::claimWallTile:
        return "claimWallTile";

    case CreatureActionType::searchGroundTileToClaim:
        return "searchGroundTileToClaim";

    case CreatureActionType::claimGroundTile:
        return "claimGroundTile";

    case CreatureActionType::attackObject:
        return "attackObject";

    case CreatureActionType::findHome:
        return "findHome";

    case CreatureActionType::sleep:
        return "sleep";

    case CreatureActionType::job:
        return "job";

    case CreatureActionType::eat:
        return "eat";

    case CreatureActionType::flee:
        return "flee";

    case CreatureActionType::searchEntityToCarry:
        return "searchEntityToCarry";

    case CreatureActionType::grabEntity:
        return "grabEntity";

    case CreatureActionType::carryEntity:
        return "carryEntity";

    case CreatureActionType::getFee:
        return "getFee";

    case CreatureActionType::leaveDungeon:
        return "leaveDungeon";

    default:
        assert(false);
        break;
    }

    return "unhandledAct=" + Helper::toString(static_cast<uint32_t>(actionType));
}

std::string CreatureAction::getListenerName() const
{
    return "Action" + mCreature.getName() + toString(mActionType);
}

bool CreatureAction::notifyDead(GameEntity* entity)
{
    if(entity == mEntity)
    {
        mEntity = nullptr;
        return false;
    }
    return true;
}

bool CreatureAction::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mEntity)
    {
        mEntity = nullptr;
        return false;
    }
    return true;
}

bool CreatureAction::notifyPickedUp(GameEntity* entity)
{
    if(entity == mEntity)
    {
        mEntity = nullptr;
        return false;
    }
    return true;
}

bool CreatureAction::notifyDropped(GameEntity* entity)
{
    // That should not happen. For now, we only require events for attacked creatures. And when they
    // are picked up, we should have cleared the action queue
    OD_LOG_ERR("name=" + mCreature.getName() + ", entity=" + entity->getName());
    return true;
}

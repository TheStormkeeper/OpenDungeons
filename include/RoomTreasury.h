#ifndef ROOMTREASURY_H
#define ROOMTREASURY_H

#include "Room.h"

class RoomTreasury : public Room
{
	public:
		RoomTreasury();

		// Functions overriding virtual functions in the Room base class.
		void doUpkeep();
		void addCoveredTile(Tile* t);
		void removeCoveredTile(Tile* t);
		void clearCoveredTiles();

		// Functions specific to this class.
		int getTotalGold();
		int depositGold(int gold, Tile *tile);
		int withdrawGold(int gold);

	private:
		enum TreasuryTileFullness {empty, bag, chest, overfull};
		TreasuryTileFullness getTreasuryTileFullness(int gold);
		string getMeshNameForTreasuryTileFullness(TreasuryTileFullness fullness);

		void updateMeshesForTile(Tile *t);
		void createMeshesForTile(Tile *t);
		void destroyMeshesForTile(Tile *t);

		static const int maxGoldWhichCanBeStoredInABag = 3000;
		static const int maxGoldWhichCanBeStoredInAChest = 5000;

		map<Tile*,int> goldInTile;
		map<Tile*,TreasuryTileFullness> fullnessOfTile;
};

#endif


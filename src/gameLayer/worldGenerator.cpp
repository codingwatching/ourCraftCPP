#include <platformTools.h>
#include "worldGenerator.h"
#include "FastNoiseSIMD.h"
#include "FastNoise/FastNoise.h"
#include <cmath>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <profilerLib/include/profilerLib.h>

const int waterLevel = 65;


//riverValue is from 0 to 1 where 1 means river
void calculateBlockPass1(int height, Block *startPos, Biome &biome, bool road, float roadValue,
	float randomNumber, bool sandShore, bool stonePatch, float riverValue)
{
	
	BlockType surfaceBlock = biome.surfaceBlock;
	BlockType secondBlock = biome.secondaryBlock;

	if (sandShore)
	{
		surfaceBlock = BlockTypes::sand;
		secondBlock = BlockTypes::sand;
	}

	if (stonePatch)
	{
		surfaceBlock = BlockTypes::stone;
		secondBlock = BlockTypes::stone;

		if (sandShore)
		{
			surfaceBlock = BlockTypes::cobblestone;
		}
	}

	if (riverValue > 0.20)
	{
		BlockVariation roadShape;
		roadShape.block.push_back(BlockTypes::cobblestone);
		roadShape.block.push_back(BlockTypes::stone);
		roadShape.block.push_back(BlockTypes::gravel);
		roadShape.block.push_back(BlockTypes::coarseDirt);
		roadShape.block.push_back((BlockTypes)surfaceBlock);

		if (riverValue < 0.5)
		{
			roadShape.block.push_back((BlockTypes)surfaceBlock);
		}

		if (riverValue < 0.4)
		{
			roadShape.block.push_back((BlockTypes)surfaceBlock);
		}

		if (riverValue < 0.35)
		{
			roadShape.block.push_back((BlockTypes)surfaceBlock);
		}

		if (riverValue < 0.30)
		{
			roadShape.block.push_back((BlockTypes)surfaceBlock);
			roadShape.block.push_back((BlockTypes)surfaceBlock);
		}

		if (riverValue < 0.25)
		{
			roadShape.block.push_back((BlockTypes)surfaceBlock);
			roadShape.block.push_back((BlockTypes)surfaceBlock);
		}

		surfaceBlock = roadShape.getRandomBLock(randomNumber);
	}

#pragma region road
	if(road)
	{
		BlockVariation roadShape;
		roadShape.block.push_back(BlockTypes::grassBlock);
		roadShape.block.push_back(BlockTypes::gravel);
		roadShape.block.push_back(BlockTypes::coarseDirt);

		BlockType roadCenter = BlockTypes::coarseDirt;
		
		if (roadValue < 0.25)
		{
			surfaceBlock = roadCenter;
		}
		else
		{
			surfaceBlock = roadShape.getRandomBLock(randomNumber);
		}

	}
#pragma endregion


	int y = height;

	//find grass
	for (; y >= waterLevel; y--)
	{
		//if (startPos[y].getType() != BlockTypes::air)
		if (startPos[y].getType() == BlockTypes::stone)
		{
			startPos[y].setType(surfaceBlock);

			for (y--; y > height - 4; y--) //todo randomness here
			{
				if (startPos[y].getType() != BlockTypes::air)
				{
					startPos[y].setType(secondBlock);
				}
			}

			break;
		}
	}

	for (y = waterLevel; y >= 20; y--)
	{

		if (startPos[y].getType() == BlockTypes::stone && startPos[y+1].getType() == biome.waterType)
		{
			//water block
			if (riverValue > 0.2)
			{
				BlockVariation roadShape;

				if (riverValue > 0.9)
				{
					roadShape.block.push_back(BlockTypes::stone);
				}

				roadShape.block.push_back(BlockTypes::stone);
				roadShape.block.push_back(BlockTypes::gravel);
				roadShape.block.push_back(BlockTypes::gravel);
				roadShape.block.push_back(BlockTypes::cobblestone);
				roadShape.block.push_back(BlockTypes::cobblestone);

				startPos[y].setType(roadShape.getRandomBLock(randomNumber));
			}

		}else
		if (startPos[y].getType() == BlockTypes::air)
		{
			if(road && y == waterLevel)
			{
				startPos[y].setType(BlockTypes::wooden_plank);
			}
			else
			{
				startPos[y].setType(biome.waterType);
			}
		}
		else
		{
			break;
		}
	}

}

void generateChunk(Chunk &c, WorldGenerator &wg, StructuresManager &structuresManager,
	BiomesManager &biomesManager, std::vector<StructureToGenerate> &generateStructures)
{
	generateChunk(c.data, wg, structuresManager, biomesManager, generateStructures);
}


int fromFloatNoiseValToIntegers(float noise, int maxExclusive)
{
	if (noise >= 1) { noise = 0.999; }
	return noise * maxExclusive;
}

void generateChunk(ChunkData& c, WorldGenerator &wg, StructuresManager &structuresManager, BiomesManager &biomesManager,
	std::vector<StructureToGenerate> &generateStructures)
{
	//PL::Profiler profiler;
	//profiler.start();


	//super flat
	//for (int x = 0; x < CHUNK_SIZE; x++)
	//	for (int z = 0; z < CHUNK_SIZE; z++)
	//	{
	//		c.cachedBiomes[x][z] = 0;
	//
	//		for (int y = 0; y < 256; y++)
	//		{
	//			if (y <= 4)
	//			{
	//				c.unsafeGet(x, y, z).setType(BlockTypes::stone);
	//			}
	//			else if(y <= 7)
	//			{
	//				c.unsafeGet(x, y, z).setType(BlockTypes::dirt);
	//			}
	//			else if (y == 8)
	//			{
	//				c.unsafeGet(x, y, z).setType(BlockTypes::grassBlock);
	//			}
	//			else
	//			{
	//				c.unsafeGet(x, y, z).setType(BlockTypes::air);
	//			}
	//
	//		}
	//	}
	//return;

	float interpolateValues[16 * 16] = {};
	float borderingFactor[16 * 16] = {};
	float tightBorders[16 * 16] = {};
	float vegetationMaster = 0;
	float xCellValue = 0;
	float zCellValue = 0;
	int currentBiomeHeight = wg.getRegionHeightAndBlendingsForChunk(c.x, c.z,
		interpolateValues, borderingFactor, vegetationMaster, tightBorders, xCellValue, zCellValue);

	c.vegetation = vegetationMaster;
	c.regionCenterX = xCellValue;
	c.regionCenterZ = zCellValue;

	//vegetationMaster = 1.f;
	float vegetationPower = linearRemap(vegetationMaster, 0, 1, 1.2, 0.4);

	auto interpolator = [&](int *ptr, float value)
	{
		if (value >= 5.f)
		{
			return (float)ptr[5];
		}

		float rez = ptr[int(value)];
		float rez2 = ptr[int(value)+1];

		float interp = value - int(value);

		//return rez;
		return glm::mix(rez, rez2, interp);
	};
	                   //water    plains   hills
	//int startValues[] = {22, 45,  66,      72,     80, 140};
	//int maxlevels[] =   {40, 64,  71,      120,     170, 250};

	int startValues[] = {22, 45,  66,      72,     74, 100};
	int maxlevels[] =   {40, 64,  71,      120,     170, 240};
	int biomes[] = {BiomesManager::plains, BiomesManager::plains, 
		BiomesManager::plains, BiomesManager::forest,
		BiomesManager::snow, BiomesManager::snow};

	int valuesToAddToStart[] = {5, 5, 10,  20,  20,  20};
	int valuesToAddToMax[] = {5, 5, 5,  20,  10,  0};
	float peaksPower[] = {1,1, 0.5, 1, 1, 1};

	c.clear();
	
	int xPadd = c.x * 16;
	int zPadd = c.z * 16;

	c.vegetation = vegetationMaster;


#pragma region noises
	static alignas(32) float continentalness[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.continentalnessNoise->FillNoiseSet(continentalness, xPadd, 0, zPadd, CHUNK_SIZE, (1), CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		continentalness[i] += 1;
		continentalness[i] /= 2;
		continentalness[i] = powf(continentalness[i], wg.continentalPower);
		continentalness[i] = wg.continentalSplines.applySpline(continentalness[i]);
	}


	static alignas(32) float peaksAndValies[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.peaksValiesNoise->FillNoiseSet(peaksAndValies, xPadd, 0, zPadd, CHUNK_SIZE, (1), CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		peaksAndValies[i] += 1;
		peaksAndValies[i] /= 2;
		peaksAndValies[i] = powf(peaksAndValies[i], wg.peaksValiesPower);
		peaksAndValies[i] = wg.peaksValiesSplines.applySpline(peaksAndValies[i]);
	}


	static alignas(32) float wierdness[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.wierdnessNoise->FillNoiseSet(wierdness, xPadd, 0, zPadd, CHUNK_SIZE, (1), CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		wierdness[i] += 1;
		wierdness[i] /= 2;
		wierdness[i] = powf(wierdness[i], wg.wierdnessPower);
		wierdness[i] = wg.wierdnessSplines.applySpline(wierdness[i]);
	}


	static alignas(32) float densityNoise[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE] = {};
	wg.stone3Dnoise->FillNoiseSet(densityNoise, xPadd, 0, zPadd, CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT; i++)
	{
		densityNoise[i] += 1;
		densityNoise[i] /= 2;
		densityNoise[i] = powf(densityNoise[i], wg.stone3Dpower);
		densityNoise[i] = wg.stone3DnoiseSplines.applySpline(densityNoise[i]);
	}


	static alignas(32) float randomSand[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE] = {};
	wg.randomStonesNoise->FillNoiseSet(randomSand, xPadd, 0, zPadd, CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT; i++)
	{
		randomSand[i] += 1;
		randomSand[i] /= 2;
		randomSand[i] = powf(randomSand[i], wg.randomSandPower);
		randomSand[i] = wg.randomSandSplines.applySpline(randomSand[i]);
	}


	static alignas(32) float randomGravel[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE] = {};
	wg.randomStonesNoise->FillNoiseSet(randomGravel, xPadd, 300, zPadd, CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT; i++)
	{
		randomGravel[i] += 1;
		randomGravel[i] /= 2;
		randomGravel[i] = powf(randomGravel[i], wg.randomSandPower + 0.1);
		randomGravel[i] = wg.randomSandSplines.applySpline(randomGravel[i]);
	}
	

	static alignas(32) float randomClay[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE] = {};
	wg.randomStonesNoise->FillNoiseSet(randomClay, xPadd, 600, zPadd, CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT; i++)
	{	
		randomClay[i] += 1;
		randomClay[i] /= 2;
		randomClay[i] = powf(randomClay[i], wg.randomSandPower + 0.5);
		randomClay[i] = wg.randomSandSplines.applySpline(randomClay[i]);
	}


	static alignas(32) float spagettiNoise[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE] = {};
	wg.spagettiNoise->FillNoiseSet(spagettiNoise, xPadd, 0, zPadd, CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE, 1);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT; i++)
	{
		spagettiNoise[i] += 1;
		spagettiNoise[i] /= 2;
		spagettiNoise[i] = powf(spagettiNoise[i], wg.spagettiNoisePower);
		spagettiNoise[i] = wg.spagettiNoiseSplines.applySpline(spagettiNoise[i]);
	}


	const float SHIFT = 16;
	static alignas(32) float spagettiNoise2[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE] = {};
	wg.spagettiNoise->FillNoiseSet(spagettiNoise2, xPadd + SHIFT + 6, 0 + SHIFT + 3, zPadd + SHIFT,
		CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT; i++)
	{
		spagettiNoise2[i] += 1;
		spagettiNoise2[i] /= 2;
		spagettiNoise2[i] = powf(spagettiNoise2[i], wg.spagettiNoisePower);
		spagettiNoise2[i] = wg.spagettiNoiseSplines.applySpline(spagettiNoise2[i]);
	}

	static alignas(32) float randomStones[1] = {};
	wg.randomStonesNoise->FillNoiseSet(randomStones, xPadd, 0, zPadd, 1, 1, 1);
	*randomStones = std::pow(((*randomStones + 1.f) / 2.f)*0.5, 3.0);

	
	static alignas(32) float cavesNoise[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE] = {};
	wg.cavesNoise->FillNoiseSet(cavesNoise, xPadd, 0, zPadd, CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT; i++)
	{
		cavesNoise[i] += 1;
		cavesNoise[i] /= 2;
		cavesNoise[i] = powf(cavesNoise[i], wg.cavesPower);
		cavesNoise[i] = wg.cavesSpline.applySpline(cavesNoise[i]);
	}

	static alignas(32) float lakesNoise[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.lakesNoise->FillNoiseSet(lakesNoise, xPadd, 0, zPadd, CHUNK_SIZE, 1, CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		lakesNoise[i] += 1;
		lakesNoise[i] /= 2;
		lakesNoise[i] = powf(lakesNoise[i], wg.lakesPower);
		lakesNoise[i] = wg.lakesSplines.applySpline(lakesNoise[i]);
	}

	static alignas(32) float whiteNoise[(CHUNK_SIZE + 1) * (CHUNK_SIZE + 1)] = {};
	wg.whiteNoise->FillNoiseSet(whiteNoise, xPadd, 0, zPadd, CHUNK_SIZE + 1, (1), CHUNK_SIZE + 1);
	for (int i = 0; i < (CHUNK_SIZE + 1) * (CHUNK_SIZE + 1); i++)
	{
		whiteNoise[i] += 1;
		whiteNoise[i] /= 2;
	}

	static alignas(32) float whiteNoise2[(CHUNK_SIZE + 1) * (CHUNK_SIZE + 1)] = {};
	wg.whiteNoise2->FillNoiseSet(whiteNoise2, xPadd, 0, zPadd, CHUNK_SIZE + 1, (1), CHUNK_SIZE + 1);
	for (int i = 0; i < (CHUNK_SIZE + 1) * (CHUNK_SIZE + 1); i++)
	{
		whiteNoise2[i] += 1;
		whiteNoise2[i] /= 2;
	}

	static alignas(32) float whiteNoise3[(CHUNK_SIZE + 1) * (CHUNK_SIZE + 1)] = {};
	wg.whiteNoise2->FillNoiseSet(whiteNoise3, xPadd, 100, zPadd, CHUNK_SIZE + 1, (1), CHUNK_SIZE + 1);
	for (int i = 0; i < (CHUNK_SIZE + 1) * (CHUNK_SIZE + 1); i++)
	{
		whiteNoise3[i] += 1;
		whiteNoise3[i] /= 2;
	}


	static alignas(32) float stonePatches[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.stonePatchesNoise->FillNoiseSet(stonePatches, xPadd, 0, zPadd, CHUNK_SIZE, 1, CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		stonePatches[i] += 1;
		stonePatches[i] /= 2;
		stonePatches[i] = powf(stonePatches[i], wg.stonePatchesPower);
		stonePatches[i] = wg.stonePatchesSpline.applySpline(stonePatches[i]);
	}


	static alignas(32) float riversNoise[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.riversNoise->FillNoiseSet(riversNoise, xPadd, 0, zPadd, CHUNK_SIZE, (1), CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		riversNoise[i] += 1;
		riversNoise[i] /= 2;
		riversNoise[i] = powf(riversNoise[i], wg.riversPower);
		riversNoise[i] = wg.riversSplines.applySpline(riversNoise[i]);
	}


	static alignas(32) float hillsDropDownsNoise[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.hillsDropsNoise->FillNoiseSet(hillsDropDownsNoise, xPadd, 0, zPadd, CHUNK_SIZE, (1), CHUNK_SIZE);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		hillsDropDownsNoise[i] += 1;
		hillsDropDownsNoise[i] /= 2;
		hillsDropDownsNoise[i] = powf(hillsDropDownsNoise[i], wg.hillsDropsPower);
		hillsDropDownsNoise[i] = wg.hillsDropsSpline.applySpline(hillsDropDownsNoise[i]);
	}


	static alignas(32) float roadNoise[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.roadNoise->FillNoiseSet(roadNoise, xPadd, 0, zPadd, CHUNK_SIZE, (1), CHUNK_SIZE, 1);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		roadNoise[i] += 1;
		roadNoise[i] /= 2;
		roadNoise[i] = powf(roadNoise[i], wg.roadPower);
		roadNoise[i] = wg.roadSplines.applySpline(roadNoise[i]);
	}


	static alignas(32) float treeAmountNoise1[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.treesAmountNoise->FillNoiseSet(treeAmountNoise1, xPadd, 0, zPadd, CHUNK_SIZE, (1), CHUNK_SIZE, 1);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		treeAmountNoise1[i] += 1;
		treeAmountNoise1[i] /= 2;
		treeAmountNoise1[i] = powf(treeAmountNoise1[i], wg.treesAmountPower);
		treeAmountNoise1[i] = wg.treesAmountSpline.applySpline(treeAmountNoise1[i]);
	}

	static alignas(32) float treeAmountNoise2[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.treesAmountNoise->FillNoiseSet(treeAmountNoise2, xPadd + 10000, 1000, zPadd + 10000, CHUNK_SIZE, (1), CHUNK_SIZE, 2);

	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		treeAmountNoise2[i] += 1;
		treeAmountNoise2[i] /= 2;
		treeAmountNoise2[i] = powf(treeAmountNoise2[i], wg.treesAmountPower);
		treeAmountNoise2[i] = wg.treesAmountSpline.applySpline(treeAmountNoise2[i]);
	}

	static alignas(32) float treeTypeNoise1[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.treesTypeNoise->FillNoiseSet(treeTypeNoise1, xPadd, 0, zPadd, CHUNK_SIZE, (1), CHUNK_SIZE, 1);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		treeTypeNoise1[i] += 1;
		treeTypeNoise1[i] /= 2;
		treeTypeNoise1[i] = powf(treeTypeNoise1[i], wg.treesTypePower);
		treeTypeNoise1[i] = wg.treesTypeSpline.applySpline(treeTypeNoise1[i]);
	}


	static alignas(32) float treeTypeNoise2[CHUNK_SIZE * CHUNK_SIZE] = {};
	wg.treesTypeNoise->FillNoiseSet(treeTypeNoise2, xPadd + 10000, 1000, zPadd + 10000, CHUNK_SIZE, (1), CHUNK_SIZE, 1);
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		treeTypeNoise2[i] += 1;
		treeTypeNoise2[i] /= 2;
		treeTypeNoise2[i] = powf(treeTypeNoise2[i], wg.treesTypePower);
		treeTypeNoise2[i] = wg.treesTypeSpline.applySpline(treeTypeNoise2[i]);
	}
#pragma endregion


#pragma region gets

	auto getNoiseVal = [](int x, int y, int z)
	{
		return continentalness[x * CHUNK_SIZE * (1) + y * CHUNK_SIZE + z];
	};

	auto getRivers = [](int x, int z)
	{
		return riversNoise[x * CHUNK_SIZE + z];
	};

	auto getHillsDropDowns = [](int x, int z)
	{
		return hillsDropDownsNoise[x * CHUNK_SIZE + z];
	};

	auto getTreeAmount1 = [](int x, int z)
	{
		return treeAmountNoise1[x * CHUNK_SIZE + z];
	};
	
	auto getTreeAmount2 = [](int x, int z)
	{
		return treeAmountNoise2[x * CHUNK_SIZE + z];
	};

	auto getTreeType1 = [](int x, int z)
	{
		return treeTypeNoise1[x * CHUNK_SIZE + z];
	};

	auto getTreeType2 = [](int x, int z)
	{
		return treeTypeNoise2[x * CHUNK_SIZE + z];
	};

	auto getRoads = [](int x, int z)
	{
		return roadNoise[x * CHUNK_SIZE + z];
	};

	auto getPeaksAndValies = [](int x, int z)
	{
		return peaksAndValies[x * CHUNK_SIZE + z];
	};
	
	auto getWhiteNoiseVal = [](int x, int z)
	{
		return whiteNoise[x * (CHUNK_SIZE + 1) + z];
	};

	auto getLakesNoiseVal = [](int x, int z)
	{
		return lakesNoise[x * (CHUNK_SIZE) + z];
	};

	auto getWhiteNoise2Val = [](int x, int z)
	{
		return whiteNoise2[x * (CHUNK_SIZE + 1) + z];
	};

	auto getWhiteNoise3Val = [](int x, int z)
	{
		return whiteNoise3[x * (CHUNK_SIZE + 1) + z];
	};

	auto getStonePatches = [](int x, int z)
	{
		return stonePatches[x * CHUNK_SIZE + z];
	};

	auto getWhiteNoiseChance = [getWhiteNoiseVal](int x, int z, float chance)
	{
		return getWhiteNoiseVal(x, z) < chance;
	};

	auto getWhiteNoise2Chance = [getWhiteNoise2Val](int x, int z, float chance)
	{
		return getWhiteNoise2Val(x, z) < chance;
	};

	auto getDensityNoiseVal = [](int x, int y, int z) //todo more cache friendly operation here please
	{
		return densityNoise[x * CHUNK_SIZE * (CHUNK_HEIGHT) + y * CHUNK_SIZE + z];
	};

	auto getRandomSandVal = [](int x, int y, int z)
	{
		return randomSand[x * CHUNK_SIZE * (CHUNK_HEIGHT)+y * CHUNK_SIZE + z];
	};

	auto getRandomGravelVal = [](int x, int y, int z)
	{
		return randomGravel[x * CHUNK_SIZE * (CHUNK_HEIGHT)+y * CHUNK_SIZE + z];
	};

	auto getRandomClayVal = [](int x, int y, int z)
	{
		return randomClay[x * CHUNK_SIZE * (CHUNK_HEIGHT)+y * CHUNK_SIZE + z];
	};

	auto getSpagettiNoiseVal = [](int x, int y, int z) //todo more cache friendly operation here please
	{
		return spagettiNoise[x * CHUNK_SIZE * (CHUNK_HEIGHT)+y * CHUNK_SIZE + z];
	};

	auto getSpagettiNoiseVal2 = [](int x, int y, int z) //todo more cache friendly operation here please
	{
		return spagettiNoise2[x * CHUNK_SIZE * (CHUNK_HEIGHT)+y * CHUNK_SIZE + z];
	};

	auto getCavesNoiseVal = [](int x, int y, int z)
	{
		return cavesNoise[x * CHUNK_SIZE * (CHUNK_HEIGHT)+y * CHUNK_SIZE + z];
	};

	auto getWierdness = [](int x, int z)
	{
		return wierdness[x * CHUNK_SIZE + z];
	};

	auto getIntFromFloat = [](float f, int maxExclusive)
	{
		if (f >= 0.99) { f = 0.99; }
		return int(f * maxExclusive);
	};

#pragma endregion

	for (int x = 0; x < CHUNK_SIZE; x++)
		for (int z = 0; z < CHUNK_SIZE; z++)
		{

			//this will be some random places where there is stone instead of grass or sand or whatever
			float stonePatchesVal = getStonePatches(x, z);

			float wierdnessTresshold = 0.4;
			float localWierdness = getWierdness(x, z);
			//localWierdness = 0;

			float peaks = getPeaksAndValies(x, z);
			float continentalness = getNoiseVal(x, 0, z);

			float currentInterpolatedValue = interpolateValues[z + x * CHUNK_SIZE];
			int startLevel = interpolator(startValues, currentInterpolatedValue);
			int maxMountainLevel = interpolator(maxlevels, currentInterpolatedValue);
			
			float treeAmountVal1 = getTreeAmount1(x, z);
			float treeAmountVal2 = getTreeAmount2(x, z);

			float treeType1 = getTreeType1(x, z);
			float treeType2 = getTreeType2(x, z);

			float lakeNoiseVal = getLakesNoiseVal(x, z); //this one is 1 for lake 0 for no lake
			float localBorderingFactor = borderingFactor[z + x * CHUNK_SIZE];

			if (tightBorders[z + x * CHUNK_SIZE])
			{
				c.setBorderFlag(x, z);
			}

			//if (localBorderingFactor > 0.95)
			//{
			//	c.setBorderFlag(x, z);
			//	//std::cout << "YESSS ";
			//}

		#pragma region roads
			bool placeRoad = 0;
			float roadValue = 0;

			//plains roads
			if (currentBiomeHeight == 2)
			{
				roadValue = getRoads(x, z);

				if (roadValue < 0.6)
				{
					peaks = glm::mix(0.5f, peaks, roadValue);

					if (roadValue < 0.5)
					{
						placeRoad = true;
					}
				};


				//maxMountainLevel = glm::mix(maxMountainLevel-1, maxMountainLevel, roadValue);


				//localWierdness = glm::mix(1.f, localWierdness, roadValue);
				//wierdnessTresshold = glm::mix(0.f, wierdnessTresshold, roadValue);
				//wierdnessTresshold = 0;

				//peaks = glm::mix(0.5f, peaks, roadValue);
			}
			else if (currentBiomeHeight == 3 && localWierdness < 0.5f && continentalness < 0.4f)
			{
				roadValue = getRoads(x, z);

				if (roadValue < 0.6)
				{
					peaks = glm::mix(0.5f, peaks, roadValue);

					if (roadValue < 0.5)
					{
						placeRoad = true;
					}
				};

				//wierdnessTresshold = glm::mix(0.f, wierdnessTresshold, roadValue);
				//wierdnessTresshold = 0;

				//maxMountainLevel = glm::mix(maxMountainLevel - 1, maxMountainLevel, roadValue);
				//startLevel = glm::mix(startLevel - 1, startLevel, roadValue);

				//localWierdness = glm::mix(1.f, localWierdness, roadValue);
				//todo investigate wierdness

				//peaks = glm::mix(0.5f, peaks, roadValue);

			}
		#pragma endregion

		#pragma region borderings

			bool sandShore = 0;
			if (
				//currentBiomeHeight == 2 
				currentInterpolatedValue <= 1.9f && currentInterpolatedValue > 1.1f
				&& localBorderingFactor > 0.1)
			{
				sandShore = 1;
			}

		#pragma endregion

			auto screenBlend = [](float a, float b)
			{
				return 1.f - (1.f - a) * (1.f - b);
			};

		#pragma region lakes
			if (lakeNoiseVal < 0.1) 
			{
				lakeNoiseVal = 0;
			}

			if (currentBiomeHeight != 2)
			{
				lakeNoiseVal = 0;
			}
			else
			{
				lakeNoiseVal *= (1.f - localBorderingFactor);

				//if (lakeNoiseVal > 0.1)
				//{
				//	startLevel = glm::mix(startLevel, waterLevel - 5, lakeNoiseVal);
				//	maxMountainLevel = glm::mix(maxMountainLevel, waterLevel - 2, lakeNoiseVal);
				//}
			}
		#pragma endregion


		#pragma region rivers
			float underGroundRivers = 0;
			float riverChanceValue = 0;
			//plains rivers
			if (currentBiomeHeight == 2)
			{
				float rivers = getRivers(x, z);
				rivers *= (1 - lakeNoiseVal);

				riverChanceValue = 1 - rivers;

				startLevel = glm::mix(waterLevel - 5, startLevel, rivers);
				maxMountainLevel = glm::mix(waterLevel - 2, maxMountainLevel, rivers);
			}
			else if (currentBiomeHeight >= 3)
			{
				underGroundRivers = 1-getRivers(x, z);
				riverChanceValue = underGroundRivers;

			}
		#pragma endregion


		#pragma region hills drop downs
			//hills drop downs
			if (currentBiomeHeight == 3)
			{
				float dropDown = getHillsDropDowns(x, z);

				//decrease contribution in plains
				if (currentBiomeHeight == 2)
				{
					dropDown /= 2.f;
				}

				//make sure we don't do this near edges...
				if (interpolateValues[z + x * CHUNK_SIZE] > 3.f)
				{
					float interpolator = interpolateValues[z + x * CHUNK_SIZE] - int(interpolateValues[z + x * CHUNK_SIZE]);
					dropDown = screenBlend(dropDown, interpolator);
				}
				//else
				//if(interpolateValues[z + x * CHUNK_SIZE] < 2.8f)
				//{
				//	float interpolator = interpolateValues[z + x * CHUNK_SIZE] - int(interpolateValues[z + x * CHUNK_SIZE]);
				//	dropDown = dropDown * interpolator;
				//}

				if (dropDown < 0.9)
				{
					startLevel = glm::mix(66, startLevel, dropDown);
					maxMountainLevel = glm::mix(71, maxMountainLevel, dropDown);
				}
			}
		#pragma endregion


			int valueToAddToStart = valuesToAddToStart[currentBiomeHeight];
			int valueToAddToEnd = valuesToAddToMax[currentBiomeHeight];
				
			peaks = std::powf(peaks, peaksPower[currentBiomeHeight]);
			startLevel += (peaks * valueToAddToStart) - 3;
			maxMountainLevel += (peaks * valueToAddToEnd) - 3;

			if (startLevel >= maxMountainLevel)
			{
				startLevel = maxMountainLevel;
				startLevel--;
				maxMountainLevel++;
			}

			if (maxMountainLevel <= startLevel) { startLevel = maxMountainLevel - 1; }
			int heightDiff = maxMountainLevel - startLevel;

			int biomeIndex = biomes[currentBiomeHeight];
			auto biome = biomesManager.biomes[biomeIndex];

			//c.unsafeGetCachedBiome(x, z) = biomeIndex;

			constexpr int stoneNoiseStartLevel = 1;

			int height = int(startLevel + continentalness * heightDiff);

			float firstH = 1;
			for (int y = 0; y < 256; y++)
			{


				float density = 1;
				{
					//int heightOffset = height + wg.densityHeightoffset;
					//int difference = y - heightOffset;
					//float differenceMultiplier =
					//	glm::clamp(powf(std::abs(difference) / squishFactor, wg.densitySquishPower),
					//	1.f, 10.f);
					//density = getDensityNoiseVal(x, y, z);
					//if (difference > 0)
					//{
					//	density = powf(density, differenceMultiplier);
					//}
					//else if (difference < 0)
					//{
					//	density = powf(density, 1.f / (differenceMultiplier));
					//}

					density = getDensityNoiseVal(x, y, z);

					float heightNormalized = (y - startLevel) / (float)heightDiff;
					heightNormalized = glm::clamp(heightNormalized, 0.f, 1.f);
					float heightNormalizedRemapped = linearRemap(heightNormalized, 0, 1, 0.2, 7);

					heightNormalized = glm::mix(0.1f, heightNormalizedRemapped, localWierdness);
					//heightNormalized = 0.1f;


					density = std::powf(density, heightNormalized);
					density = glm::clamp(density, 0.f, 1.f);
						
					//density = linearRemap(density, 0, 1)
				}
				
			#pragma region other block patches
				BlockType block = BlockTypes::stone;

				float sandVal = getRandomSandVal(x, y, z);
				float gravelVal = getRandomGravelVal(x, y, z);
				float clayVal = getRandomClayVal(x, y, z);

				if (sandVal > 0.5 || gravelVal > 0.5 || clayVal > 0.5)
				{
					if (sandVal > gravelVal && sandVal > clayVal)
					{
						block = BlockTypes::sand;
					}
					else if (gravelVal > clayVal)
					{
						block = BlockTypes::gravel;
					}
					else
					{

						if (biome.isICy)
						{
							block = BlockTypes::ice;
						}
						else
						{
							block = BlockTypes::clay;
						}

					}

				}
			#pragma endregion


				if (y < stoneNoiseStartLevel)
				{
					c.unsafeGet(x, y, z).setType(block);
				}
				else
				{
					if (y < height)
					if (density > wierdnessTresshold)
					{
						firstH = y;
						c.unsafeGet(x, y, z).setType(block);
					}
					//else cave
				}

			}

			if (underGroundRivers > 0.2)
			{
				int water = waterLevel;
				int min = water - 4 * underGroundRivers;
				int max = water + 8 * underGroundRivers;

				for (int y = min; y < max; y++)
				{
					c.unsafeGet(x, y, z).setType(BlockTypes::air);
				}

				
			}

			float riverValueForPlacingRocks = screenBlend(riverChanceValue, lakeNoiseVal);
			if (currentBiomeHeight != 2 && currentBiomeHeight != 1)
			{
				riverValueForPlacingRocks = 0;
			}

			calculateBlockPass1(firstH, &c.unsafeGet(x, 0, z), biome, placeRoad, roadValue, 
				getWhiteNoiseVal(x,z), sandShore, stonePatchesVal > 0.5, 
				riverValueForPlacingRocks);

			//all caves
			for (int y = 2; y < firstH; y++)
			{
				
				auto caveDensity = getCavesNoiseVal(x, y, z);
				//auto caveDensityBellow = getCavesNoiseVal(x, y - 1, z);
				//auto caveDensityBellow2 = getCavesNoiseVal(x, y-2, z);
				//|| caveDensityBellow < 0.5 || caveDensityBellow2 < 0.5

				if (caveDensity < 0.5 )
				{
					//cave
					if (c.unsafeGet(x, y, z).getType() != BlockTypes::water)
					{
						c.unsafeGet(x, y, z).setType(BlockTypes::air);
					}
				}
				else
				{
					auto density = getSpagettiNoiseVal(x, y, z);
					float density2 = getSpagettiNoiseVal2(x, y, z);
					density = screenBlend(density, density2);

					float height = y / (CHUNK_HEIGHT-1);
					//height = height * height;
					float heightRemapped = linearRemap(height, 0, 1, 0.25, 0.1);


					//bias = powf(bias, wg.spagettiNoiseBiasPower);

					if (density > 0.75 - heightRemapped)
					{
						//stone
					}
					else
					{
						//spagetti cave
						if (c.unsafeGet(x, y, z).getType() != BlockTypes::water)
						{
							c.unsafeGet(x, y, z).setType(BlockTypes::air);
						}
					}
				}

				
			
			}

			auto generateTreeFunction = [&](unsigned char treeType)
			{
				//generate tree
				StructureToGenerate str;
				if (treeType == Biome::treeNormal)
				{
					str.type = Structure_Tree;
				}else if (treeType == Biome::treeNormalTall)
				{
					str.type = Structure_Tree;
					str.addRandomTreeHeight = true;
					str.replaceLogWith = BlockTypes::woodLog;
				}
				else if (treeType == Biome::treeSpruceTallOakCenter)
				{
					str.type = Structure_SpruceSlim;
					str.addRandomTreeHeight = true;
					str.replaceLogWith = BlockTypes::woodLog;
					str.replaceLeavesWith = BlockTypes::leaves;
				}
				else if (treeType == Biome::treeSpruceTallOakCenterRed)
				{
					str.type = Structure_SpruceSlim;
					str.addRandomTreeHeight = true;
					str.replaceLogWith = BlockTypes::woodLog;
					str.replaceLeavesWith = BlockTypes::spruce_leaves_red;
				}
				else if (treeType == Biome::treeSpruceTallOakCenterYellow)
				{
					str.type = Structure_SpruceSlim;
					str.addRandomTreeHeight = true;
					str.replaceLogWith = BlockTypes::woodLog;
					str.replaceLeavesWith = BlockTypes::birch_leaves;
				}
				else if (treeType == Biome::treeJungle)
				{
					str.type = Structure_JungleTree;
				}
				else if (treeType == Biome::treePalm)
				{
					str.type = Structure_PalmTree;
				}
				else if (treeType == Biome::treeBirch)
				{
					str.type = Structure_BirchTree;
				}
				else if (treeType == Biome::treeBirchTall)
				{	
					str.type = Structure_BirchTree;
					str.addRandomTreeHeight = true;
					str.replaceLogWith = BlockTypes::birch_log;
					str.replaceLeavesWith = BlockTypes::birch_leaves;
				}
				else if (treeType == Biome::treeRedBirchTall)
				{
					str.type = Structure_BirchTree;
					str.addRandomTreeHeight = true;
					str.replaceLogWith = BlockTypes::birch_log;
					str.replaceLeavesWith = BlockTypes::spruce_leaves_red;
				}
				else if (treeType == Biome::treeRedBirch)
				{
					str.type = Structure_BirchTree;
					//str.addRandomTreeHeight = true;
					//str.replaceLogWith = BlockTypes::birch_log;
					str.replaceLeavesWith = BlockTypes::spruce_leaves_red;
				}
				else if (treeType == Biome::treeSpruce)
				{
					str.type = Structure_Spruce;
				}
				else if (treeType == Biome::treeTallOak)
				{
					str.type = Structure_TallSlimTree;
					str.addRandomTreeHeight = true;
					str.replaceLogWith = BlockTypes::woodLog;
					str.replaceLeavesWith = BlockTypes::leaves;
				}
				else
				{
					assert(0);
				}

				str.pos = {x + xPadd, firstH, z + zPadd};
				str.replaceBlocks = false;
				str.randomNumber1 = getWhiteNoise3Val(x, z);
				str.randomNumber2 = getWhiteNoise3Val(x + 1, z);
				str.randomNumber3 = getWhiteNoise3Val(x + 1, z + 1);
				str.randomNumber4 = getWhiteNoise3Val(x, z + 1);

				generateStructures.push_back(str);
			};

			if (firstH < CHUNK_HEIGHT - 1)
			{
				auto b = c.unsafeGet(x, firstH, z).getType();

				bool generatedSomethingElse = 0;

				//random stones
				{

					float stonesChance = glm::mix(0.008, 0.04, stonePatchesVal);

					stonesChance *= randomStones[0];

					//more stones near rivers
					if (riverChanceValue > 0.2 && currentBiomeHeight == 2)
					{
						stonesChance += 0.002;
					}

					//if()

					if (getWhiteNoiseChance(x, z, stonesChance))
					{
						generatedSomethingElse = true;

						StructureToGenerate str;
						str.type = Structure_SmallStone;

						str.pos = {x + xPadd, firstH + 1, z + zPadd};
						str.randomNumber1 = getWhiteNoise3Val(x, z);
						str.randomNumber2 = getWhiteNoise3Val(x + 1, z);
						str.randomNumber3 = getWhiteNoise3Val(x + 1, z + 1);
						str.randomNumber4 = getWhiteNoise3Val(x, z + 1);

						generateStructures.push_back(str);
					}
				}

				auto generateOneFeature = [&](float treeAmount, 
					VegetationNoiseSettings &veg)
				{
					treeAmount = std::powf(treeAmount, vegetationPower);
					float noiseVal = treeAmount;

					//one distribution element, can be multiple things there tho
					for (auto &entry : veg.entry)
					{
						if (noiseVal >= entry.minTresshold && entry.maxTresshold >= noiseVal)
						{

							float chanceRemap = linearRemap(noiseVal, entry.minTresshold, entry.maxTresshold,
								entry.chanceRemap.x, entry.chanceRemap.y);

							//float chanceRemap = linearRemap(noiseVal, 0, 1,
							//	entry.chanceRemap.x, entry.chanceRemap.y);

							if (getWhiteNoiseChance(x, z, chanceRemap))
							{

								//pick the block to place
								auto &growThing = entry.growThing;

								bool canGrow = 0;

								for (auto &growOn : growThing.growOn)
								{
									if (b == growOn)
									{
										canGrow = true;
									}
								}

								if (canGrow)
								{
									int count = growThing.elements.size();
									float noiseVal = getWhiteNoise2Val(x, z);

									int index = fromFloatNoiseValToIntegers(noiseVal, count);
									auto &growElement = growThing.elements[index];

									assert(growElement.block || growElement.treeType);
									assert(!(growElement.block != 0 && growElement.treeType != 0));

									if (growElement.block)
									{
										c.unsafeGet(x, firstH + 1, z).setType(growElement.block);
										generatedSomethingElse = true;
									}
									else if (growElement.treeType)
									{

										//don't put trees too together...
										float noiseVal1 = getWhiteNoise2Val(x + 1, z);
										float noiseVal2 = getWhiteNoise2Val(x, z + 1);
										float noiseVal3 = getWhiteNoise2Val(x + 1, z + 1);

										if (0 &&
											noiseVal1 >= entry.minTresshold && entry.maxTresshold >= noiseVal1 ||
											noiseVal2 >= entry.minTresshold && entry.maxTresshold >= noiseVal2 ||
											noiseVal3 >= entry.minTresshold && entry.maxTresshold >= noiseVal3
											)
										{

											float chanceRemap1 = linearRemap(noiseVal1, entry.minTresshold, entry.maxTresshold,
												entry.chanceRemap.x, entry.chanceRemap.y);
											float chanceRemap2 = linearRemap(noiseVal2, entry.minTresshold, entry.maxTresshold,
												entry.chanceRemap.x, entry.chanceRemap.y);
											float chanceRemap3 = linearRemap(noiseVal3, entry.minTresshold, entry.maxTresshold,
												entry.chanceRemap.x, entry.chanceRemap.y);

											if (
												getWhiteNoiseChance(x + 1, z, chanceRemap1) ||
												getWhiteNoiseChance(x, z + 1, chanceRemap2) ||
												getWhiteNoiseChance(x + 1, z + 1, chanceRemap3)
												)
											{
												int index1 = fromFloatNoiseValToIntegers(noiseVal, count);
												int index2 = fromFloatNoiseValToIntegers(noiseVal, count);
												int index3 = fromFloatNoiseValToIntegers(noiseVal, count);

												if (index1 == index || index2 == index || index3 == index)
												{
													continue;
												}
											}
										}

										generateTreeFunction(growElement.treeType);
										generatedSomethingElse = true;
										break;
									}
								}

							}

						}

						if (generatedSomethingElse) { break; }
					}
				};

				if (!generatedSomethingElse)
				{
					int type = getIntFromFloat(treeType1, biomesManager.greenBiomesTrees.size());
					generateOneFeature(treeAmountVal1, biomesManager.greenBiomesTrees[type]);
				}

				if (!generatedSomethingElse)
				{
					int type = getIntFromFloat(treeType2, biomesManager.greenBiomesTrees.size());
					generateOneFeature(treeAmountVal2, biomesManager.greenBiomesTrees[type]);
				}

				if (!generatedSomethingElse)
				{
					//todo
					generateOneFeature(0.6, biomesManager.greenBiomesGrass[0]);
				}

				/*
				if(!generatedSomethingElse)
				for (int noiseIndex = 0; noiseIndex < biome.vegetationNoises.size(); noiseIndex++)
				{
					auto &noiseSettings = biome.vegetationNoises[noiseIndex];

					float noiseVal = 0;
					float noiseVal1 = 0;
					float noiseVal2 = 0;
					float noiseVal3 = 0;

					if (noiseIndex == 0)
					{
						noiseVal = getVegetationNoiseVal(x, z);
						noiseVal1 = getVegetationNoiseVal(x+1, z);
						noiseVal2 = getVegetationNoiseVal(x, z+1);
						noiseVal3 = getVegetationNoiseVal(x+1, z+1);
					}
					else if (noiseIndex == 1)
					{
						noiseVal = getVegetation2NoiseVal(x, z);
						noiseVal1 = getVegetation2NoiseVal(x+1, z);
						noiseVal2 = getVegetation2NoiseVal(x, z+1);
						noiseVal3 = getVegetation2NoiseVal(x+1, z+1);
					}
					else if(noiseIndex == 2)
					{
						noiseVal = getVegetation3NoiseVal(x, z);
						noiseVal1 = getVegetation3NoiseVal(x + 1, z);
						noiseVal2 = getVegetation3NoiseVal(x, z + 1);
						noiseVal3 = getVegetation3NoiseVal(x + 1, z + 1);
					}
					else if (noiseIndex == 3)
					{
						noiseVal = getVegetation4NoiseVal(x, z);
						noiseVal1 = getVegetation4NoiseVal(x + 1, z);
						noiseVal2 = getVegetation4NoiseVal(x, z + 1);
						noiseVal3 = getVegetation4NoiseVal(x + 1, z + 1);
					}

					bool generated = 0;

					//one distribution element, can be multiple things there tho
					for (auto &entry : noiseSettings.entry)
					{

						if (noiseVal >= entry.minTresshold && entry.maxTresshold >= noiseVal)
						{

							float chanceRemap = linearRemap(noiseVal, entry.minTresshold, entry.maxTresshold,
								entry.chanceRemap.x, entry.chanceRemap.y);
							//chanceRemap = noiseVal;

							if (getWhiteNoiseChance(x, z, chanceRemap))
							{

								//pick the block to place
								auto &growThing = entry.growThing;

								bool canGrow = 0;

								for (auto &growOn : growThing.growOn)
								{
									if (b == growOn)
									{
										canGrow = true;
									}
								}

								if (canGrow)
								{
									int count = growThing.elements.size();
									float noiseVal = getWhiteNoise2Val(x, z);

									int index = fromFloatNoiseValToIntegers(noiseVal, count);
									auto &growElement = growThing.elements[index];

									assert(growElement.block || growElement.treeType);
									assert(!(growElement.block != 0 && growElement.treeType != 0));

									if (growElement.block)
									{
										c.unsafeGet(x, firstH + 1, z).setType(growElement.block);
										generated = true;
									}
									else if (growElement.treeType)
									{

										//don't put trees too together...
										float noiseVal1 = getWhiteNoise2Val(x + 1, z);
										float noiseVal2 = getWhiteNoise2Val(x, z + 1);
										float noiseVal3 = getWhiteNoise2Val(x + 1, z + 1);

										if ( 0 &&
											noiseVal1 >= entry.minTresshold && entry.maxTresshold >= noiseVal1 ||
											noiseVal2 >= entry.minTresshold && entry.maxTresshold >= noiseVal2 ||
											noiseVal3 >= entry.minTresshold && entry.maxTresshold >= noiseVal3
											)
										{

											float chanceRemap1 = linearRemap(noiseVal1, entry.minTresshold, entry.maxTresshold,
												entry.chanceRemap.x, entry.chanceRemap.y);
											float chanceRemap2 = linearRemap(noiseVal2, entry.minTresshold, entry.maxTresshold,
												entry.chanceRemap.x, entry.chanceRemap.y);
											float chanceRemap3 = linearRemap(noiseVal3, entry.minTresshold, entry.maxTresshold,
												entry.chanceRemap.x, entry.chanceRemap.y);

											if (
												getWhiteNoiseChance(x + 1, z, chanceRemap1) ||
												getWhiteNoiseChance(x, z + 1, chanceRemap2) ||
												getWhiteNoiseChance(x + 1, z + 1, chanceRemap3)
												)
											{
												int index1 = fromFloatNoiseValToIntegers(noiseVal, count);
												int index2 = fromFloatNoiseValToIntegers(noiseVal, count);
												int index3 = fromFloatNoiseValToIntegers(noiseVal, count);

												if (index1 == index || index2 == index || index3 == index)
												{
													continue;
												}
											}
										}

										generateTreeFunction(growElement.treeType);
										generated = true;
										break;
									}
								}

							}

						}

						if (generated) { break; }
					}
					if (generated) { break; }


				}
				*/

			}



		}
	
	
	//profiler.end();
	//std::cout << "Time ms: " << profiler.rezult.timeSeconds * 1000 << "\n";


}


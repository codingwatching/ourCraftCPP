#include "chunkSystem.h"
#include <glm/vec2.hpp>
#include "threadStuff.h"
#include <algorithm>

Chunk* ChunkSystem::getChunkSafe(int x, int z)
{
	if (x < 0 || z < 0 || x >= squareSize || z >= squareSize)
	{
		return nullptr;
	}
	else
	{
		return loadedChunks[x * squareSize + z];
	}
}

void ChunkSystem::createChunks(int viewDistance, std::vector<int> &data)
{
	squareSize = viewDistance;

	loadedChunks.resize(squareSize * squareSize, nullptr);

	//for (int x = 0; x < squareSize; x++)
	//	for (int z = 0; z < squareSize; z++)
	//	{
	//		Chunk *chunk = new Chunk;
	//		chunk->create(x, z);
	//		loadedChunks.push_back(nullptr);
	//	}

	//for (int x = 0; x < squareSize; x++)
	//	for (int z = 0; z < squareSize; z++)
	//	{
	//		auto chunk = getChunkSafe(x, z);
	//		chunk->bake();
	//	}

}

void ChunkSystem::update(int x, int z, std::vector<int>& data)
{

	x /= CHUNK_SIZE;
	z /= CHUNK_SIZE;

	data.clear();

	glm::ivec2 minPos = glm::ivec2(x, z) - glm::ivec2(squareSize / 2, squareSize / 2);
	glm::ivec2 maxPos = glm::ivec2(x, z) + glm::ivec2(squareSize / 2 + squareSize % 2, squareSize / 2 + squareSize % 2);
	//exclusive max

	if (!created || lastX != x || lastZ != z)
	{

		std::vector<Chunk*> newChunkVector;
		newChunkVector.resize(squareSize * squareSize);

		std::vector<Chunk*> dirtyChunls;
		dirtyChunls.reserve(squareSize * squareSize);

		for (int i = 0; i < squareSize * squareSize; i++)
		{
			if (loadedChunks[i] == nullptr) { continue; }

			glm::ivec2 chunkPos;
			chunkPos.x = loadedChunks[i]->x;
			chunkPos.y = loadedChunks[i]->z;

			if (
				chunkPos.x >= minPos.x &&
				chunkPos.y >= minPos.y &&
				chunkPos.x < maxPos.x &&
				chunkPos.y < maxPos.y
				)
			{
				glm::ivec2 chunkPosRelToSystem = chunkPos - minPos;
				//loadedChunks[i]->dirty = false;

				newChunkVector[chunkPosRelToSystem.x * squareSize + chunkPosRelToSystem.y] = loadedChunks[i];

				loadedChunks[i] = nullptr;
			}
			else
			{
				delete loadedChunks[i];
				//loadedChunks[i]->dirty = true;
				//dirtyChunls.push_back(loadedChunks[i]);
				loadedChunks[i] = nullptr;
			}
		}

		std::vector<Task> tasks;
		for (int x = 0; x < squareSize; x++)
			for (int z = 0; z < squareSize; z++)
			{

				if (newChunkVector[x * squareSize + z] == nullptr)
				{
					//Chunk* chunk = dirtyChunls.back();
					//dirtyChunls.pop_back();
					//
					//chunk->create(x + minPos.x, z + minPos.y);
					//newChunkVector[x * squareSize + z] = chunk;
					Task t;

					t.type = Task::generateChunk;
					t.pos = glm::ivec3(x + minPos.x, 0, z + minPos.y);
					tasks.push_back(t);
				}
			}
		
		if (!tasks.empty())
		{
			submitTask(tasks);
		}


		//assert(dirtyChunls.empty());

		loadedChunks = std::move(newChunkVector);

	}

	created = 1;
	lastX = x;
	lastZ = z;

	auto recievedChunks = getChunks();

	for (auto i : recievedChunks)
	{

		int x = i->x - minPos.x;
		int z = i->z - minPos.y;

		if (x < 0 || z < 0 || x >= squareSize || z >= squareSize)
		{
			delete i; // ignore chunk, not of use anymore
			continue;
		}
		else
		{
			if (loadedChunks[x * squareSize + z] != nullptr)
			{
				delete i; //double request, ignore
			}
			else
			{
				//assert(loadedChunks[x * squareSize + z] == nullptr);
				loadedChunks[x * squareSize + z] = i;
			}
			
		}
	}

	int currentBaked = 0;
	const int maxToBake = 3; //this frame

	auto chunkVectorCopy = loadedChunks;

	std::sort(chunkVectorCopy.begin(), chunkVectorCopy.end(),
		[x, z](Chunk* a, Chunk* b) 
			{
				if (a == nullptr) { return false; }
				if (b == nullptr) { return true; }
				
				int ax = a->x - x;
				int az = a->z - z;
	
				int bx = b->x - x;
				int bz = b->z - z;
	
				unsigned long reza = ax * ax + az * az;
				unsigned long rezb = bx * bx + bz * bz;
	
				return reza < rezb;
			}
		);

	for (int i = 0; i < chunkVectorCopy.size(); i++)
	{
		auto chunk = chunkVectorCopy[i];
		if (chunk == nullptr) { continue; } //todo break? 

		int x = chunk->x - minPos.x;
		int z = chunk->z - minPos.y;

		if (currentBaked < maxToBake)
		{
			auto left = getChunkSafe(x - 1, z);
			auto right = getChunkSafe(x + 1, z);
			auto front = getChunkSafe(x, z + 1);
			auto back = getChunkSafe(x, z - 1);
		
			auto b = chunk->bake(left, right, front, back);
		
			if (b) { currentBaked++; }
		
			for (auto i : chunk->opaqueGeometry)
			{
				data.push_back(i);
			}
		}
		else
		{
			if (!chunk->dirty)
			{
				for (auto i : chunk->opaqueGeometry)
				{
					data.push_back(i);
				}
			}
		}

	}

	//for (int x = 0; x < squareSize; x++)
	//	for (int z = 0; z < squareSize; z++)
	//	{
	//		auto chunk = getChunkSafe(x, z);
	//
	//		if (chunk != nullptr)
	//		{
	//			if (currentBaked < maxToBake)
	//			{
	//				auto left = getChunkSafe(x - 1, z);
	//				auto right = getChunkSafe(x + 1, z);
	//				auto front = getChunkSafe(x, z + 1);
	//				auto back = getChunkSafe(x, z - 1);
	//
	//				auto b = chunk->bake(left, right, front, back);
	//
	//				if (b) { currentBaked++; }
	//
	//				for (auto i : chunk->opaqueGeometry)
	//				{
	//					data.push_back(i);
	//				}
	//			}
	//			else
	//			{
	//				if (!chunk->dirty)
	//				{
	//					for (auto i : chunk->opaqueGeometry)
	//					{
	//						data.push_back(i);
	//					}
	//				}
	//			}
	//			
	//		}
	//
	//	}
	//

}

Block* ChunkSystem::getBlockSafe(int x, int y, int z)
{
	if (y < 0 || y >= CHUNK_HEIGHT) { return nullptr; }

	auto cornerChunk = loadedChunks[0];
	int cornerX = cornerChunk->x;
	int cornerZ = cornerChunk->z;

	//todo

}



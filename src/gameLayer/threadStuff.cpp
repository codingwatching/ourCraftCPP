#include "threadStuff.h"
#include <thread>
#include <mutex>
#include <queue>
#include "worldGenerator.h"
#include <thread>
#include <unordered_map>
#include <iostream>

std::mutex taskMutex;
std::condition_variable taskCondition;
std::queue<Task> tasks;

void submitTask(Task& t)
{

	std::unique_lock<std::mutex> locker(taskMutex);
	tasks.push(t);
	locker.unlock();
	taskCondition.notify_one();

}

void submitTask(std::vector<Task>& t)
{
	std::unique_lock<std::mutex> locker(taskMutex);
	for (auto& i : t)
	{
		tasks.push(i);
	}
	locker.unlock();
	taskCondition.notify_one();
}

std::vector<Task> waitForTasks()
{
	std::unique_lock<std::mutex> locker(taskMutex);
	if (tasks.empty())
	{
		taskCondition.wait(locker);
	}

	auto size = tasks.size();
	std::vector<Task> retVector;
	retVector.reserve(size);

	for (int i = 0; i < size; i++)
	{
		retVector.push_back(tasks.front());
		tasks.pop();
	}

	locker.unlock();

	return retVector;
}

std::vector<Task> tryForTasks()
{
	std::unique_lock<std::mutex> locker(taskMutex);
	if (tasks.empty())
	{
		locker.unlock();
		return {};
	}

	auto size = tasks.size();
	std::vector<Task> retVector;
	retVector.reserve(size);

	for (int i = 0; i < size; i++)
	{
		retVector.push_back(tasks.front());
		tasks.pop();
	}

	locker.unlock();

	return retVector;
}

std::mutex chunkMutex;
std::queue<Chunk*> chunks;

void submitChunk(Chunk* c)
{
	chunkMutex.lock();

	chunks.push(c);

	chunkMutex.unlock();
}

std::vector<Chunk*> getChunks()
{
	std::vector<Chunk*> retVector;

	chunkMutex.lock();

	auto size = chunks.size();
	for (int i = 0; i < size; i++)
	{
		retVector.push_back(chunks.front());
		chunks.pop();
	}

	chunkMutex.unlock();

	return retVector;
}

struct ListNode;

struct SavedChunk
{
	//std::mutex mu;
	Chunk chunk;
	ListNode* node = nullptr;
};

//https://www.geeksforgeeks.org/how-to-create-an-unordered_map-of-user-defined-class-in-cpp/
struct ChunkHash
{
	size_t operator()(const glm::ivec2& in) const
	{
		int x = in.x;
		int z = in.y;

		size_t ret = 0;
		ret += x;
		ret += (z < 32);

		return ret;
	}
};


struct ListNode
{
	ListNode *prev = nullptr;
	glm::ivec2 chunkPos;
	ListNode *next = nullptr;
};


void serverFunction()
{

	std::unordered_map<glm::ivec2, SavedChunk*, ChunkHash> savedChunks;
	const int maxSavedChunks = 2000;
	ListNode *first = nullptr;
	ListNode *last = nullptr;

	std::queue<Task> waitingTasks;

	while (true)
	{
		std::vector<Task> tasks;
		if (waitingTasks.empty())
		{
			tasks = waitForTasks();
		}
		else
		{
			tasks = tryForTasks();
		}
		
		for (auto i : tasks)
		{
			waitingTasks.push(i);
		}

		int taskIndex = 0;
		int count = waitingTasks.size();
		for (;taskIndex < std::min(count, 10); taskIndex++)
		{
			auto &i = waitingTasks.front();

			if (i.type == Task::generateChunk)
			{
				glm::ivec2 pos = {i.pos.x, i.pos.z};
				auto foundPos = savedChunks.find(pos);
				if (foundPos != savedChunks.end())
				{
					auto nod = (*foundPos).second->node;

					auto left = nod->prev;
					auto right = nod->next;

					if (left == nullptr)
					{
						//nothing, this is the first element
					}
					else if(right == nullptr)
					{
						left->next = nullptr;
						last = left;

						nod->next = first;
						if (first)
						{
							first->prev = nod;
						}
						
						nod->prev = nullptr;
						first = nod;
					}
					else
					{
						//link left right:
						left->next = right;
						right->prev = left;

						nod->next = first;
						if (first)
						{
							first->prev = nod;
						}

						nod->prev = nullptr;
						first = nod;
					}

					submitChunk(new Chunk((*foundPos).second->chunk));
				}
				else
				{

					if (savedChunks.size() >= maxSavedChunks)
					{
						//std::cout << "bad";
						glm::ivec2 oldPos = last->chunkPos;
						auto foundPos = savedChunks.find(oldPos);
						assert(foundPos != savedChunks.end());//unlink of the 2 data structures

						SavedChunk *chunkToRecycle = foundPos->second;

						//here we can save the chunk to a file


						savedChunks.erase(foundPos);

						chunkToRecycle->chunk.x = pos.x;
						chunkToRecycle->chunk.z = pos.y;

						generateChunk(1234, chunkToRecycle->chunk);
						submitChunk(new Chunk(chunkToRecycle->chunk));

						assert(chunkToRecycle->node == last);
						last->chunkPos = pos;

						auto prev = last->prev;
						prev->next = nullptr;

						auto newFirst = last;
						last = prev;

						first->prev = newFirst;
						newFirst->prev = nullptr;
						newFirst->next = first;
						first = newFirst;

						savedChunks[pos] = chunkToRecycle;

					}
					else
					{

						SavedChunk *c = new SavedChunk;
						c->chunk.x = i.pos.x;
						c->chunk.z = i.pos.z;

						generateChunk(1234, c->chunk);
						//std::this_thread::sleep_for(std::chrono::milliseconds(1));
						submitChunk(new Chunk(c->chunk));
						
						ListNode *node = new ListNode;
						if (last == nullptr) { last = node; }
						
						node->prev = nullptr;
						node->next = first;
						node->chunkPos = {i.pos.x, i.pos.z};

						if (first)
						{
							first->prev = node;
						}

						first = node;

						c->node = node;
						savedChunks[{i.pos.x, i.pos.z}] = c;

					}
				}

			}

			waitingTasks.pop();
		}


	}

	

}
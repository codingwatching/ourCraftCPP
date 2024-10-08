#include "gameplay/mapEngine.h"
#include "gamePlayLogic.h"
#include "chunkSystem.h"
#include <platform/platformInput.h>

void MapEngine::close()
{
	for (auto &it : mapChunks)
	{
		it.second.cleanup();
	}

	mapChunks = {};
}

void MapEngine::open(ProgramData &programData, glm::ivec2 playerPos, 
	ChunkSystem &chunkSystem)
{
	centerPos = playerPos;

	camera.follow(centerPos, 10000, 0, 0, programData.ui.renderer2d.windowW,
		programData.ui.renderer2d.windowH);
	camera.zoom = 3;


}

void createMapChunkFromChunk(ChunkData &chunkData, MapEngine::MapChunk &chunk, ProgramData &programData)
{

	gl2d::Texture t;
	
	unsigned char textureData[CHUNK_SIZE * CHUNK_SIZE * 4] = {};

	for (int textureY = 0; textureY < CHUNK_SIZE; textureY++)
	{
		for (int textureX = 0; textureX < CHUNK_SIZE; textureX++)
		{

			unsigned char colorR = 0;
			unsigned char colorG = 0;
			unsigned char colorB = 0;
			unsigned char colorA = 255;

			bool bordering = chunkData.isBorder(textureX, textureY);
			bool center = chunkData.x == chunkData.regionCenterX && chunkData.z == chunkData.regionCenterZ;

			for (int y = CHUNK_HEIGHT - 1; y >= 0; y--)
			{
				auto &b = chunkData.unsafeGet(textureX, y, textureY);
				if (b.getType())
				{
					auto color = programData.blocksLoader.blocksColors[b.getType()];

					if (center)
					{
						color = glm::mix(color, glm::vec3{0.2, 0.2, 0.9}, 0.5f);
					}

					if (bordering)
					{
						color *= glm::vec3{0.9, 0.2, 0.2};
					}

					colorR = color.r * 255;
					colorG = color.g * 255;
					colorB = color.b * 255;
					break;
				}
			}

			textureData[(textureX + textureY * CHUNK_SIZE) * 4 + 0] = colorR;
			textureData[(textureX + textureY * CHUNK_SIZE) * 4 + 1] = colorG;
			textureData[(textureX + textureY * CHUNK_SIZE) * 4 + 2] = colorB;
			textureData[(textureX + textureY * CHUNK_SIZE) * 4 + 3] = colorA;

		}
	}

	t.createFromBuffer((char *)textureData, CHUNK_SIZE, CHUNK_SIZE, true, true);

	chunk.t = t;
}

void MapEngine::update(ProgramData &programData, float deltaTime,
	glm::ivec2 chunkPos, ChunkSystem &chunkSystem)
{

	camera.zoom += platform::getScroll() * 0.2;

	camera.zoom = glm::clamp(camera.zoom, 0.6f, 10.f);


	//programData.blocksLoader.
	auto &renderer = programData.ui.renderer2d;
	
	camera.follow(centerPos, deltaTime * 10, 0.01, 10, programData.ui.renderer2d.windowW,
		programData.ui.renderer2d.windowH);


	renderer.pushCamera(camera);
	{
		
		auto cameraBox = renderer.getViewRect();
		glm::vec2 mousePosInViewRect = glm::vec2(cameraBox);
		mousePosInViewRect += glm::vec2(cameraBox.z, cameraBox.w) * glm::vec2(platform::getRelMousePosition()) /
			glm::vec2(renderer.windowW, renderer.windowH);

		mouseHoveredId = UINT64_MAX;

		//first pass
		for (int z = 0; z < chunkSystem.squareSize; z++)
		{
			for (int x = 0; x < chunkSystem.squareSize; x++)
			{

				auto c = chunkSystem.getChunksInMatrixSpaceUnsafe(x, z);

				if (c)
				{
					int chunkX = c->data.x;
					int chunkZ = c->data.z;

					glm::vec4 pos = glm::vec4{x,z,1,1};
					pos.x += chunkSystem.cornerPos.x;
					pos.y += chunkSystem.cornerPos.y;
					pos *= (float)CHUNK_SIZE;

					if (glui::aabb(pos, mousePosInViewRect))
					{
						mouseHovered = glm::ivec2(pos);
						mouseHoveredId = getRegionId(c->data.regionCenterX, c->data.regionCenterZ); //todo
						//color = {0.5,0.5,0.5,0.5};
					}
				}
			}
		}

		for (int z = 0; z < chunkSystem.squareSize; z++)
		{
			for (int x = 0; x < chunkSystem.squareSize; x++)
			{

				auto c = chunkSystem.getChunksInMatrixSpaceUnsafe(x, z);

				if (c)
				{
					int chunkX = c->data.x;
					int chunkZ = c->data.z;

					auto found = mapChunks.find({chunkX, chunkZ});

					if (found == mapChunks.end())
					{
						MapChunk mapChunk;
						createMapChunkFromChunk(c->data, mapChunk, programData);

						mapChunks.insert({{chunkX, chunkZ}, mapChunk});

						found = mapChunks.find({chunkX, chunkZ});
					}

					glm::vec4 pos = glm::vec4{x,z,1,1};
					pos.x += chunkSystem.cornerPos.x;
					pos.y += chunkSystem.cornerPos.y;
					pos *= (float)CHUNK_SIZE;

					glm::vec4 color = Colors_White;

					//renderer.renderRectangle(pos * (float)CHUNK_SIZE,
					//	color);
					
					renderer.renderRectangle(pos, found->second.t, 
						color,
						{}, 0, {0,0,1,1});


					if (mouseHoveredId == getRegionId(c->data.regionCenterX, c->data.regionCenterZ))
					{
						renderer.renderRectangle(pos, glm::vec4{232, 207, 149, 125} / 255.f);

					}

				}
				

			}
		}

		//renderer.renderRectangle({mousePosInViewRect, 5, 5}, Colors_Red);

	}
	renderer.popCamera();

}

void MapEngine::MapChunk::cleanup()
{
	t.cleanup();

}

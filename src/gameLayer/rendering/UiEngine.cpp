#include "rendering/UiEngine.h"
#include <platform/platformInput.h>
#include <gameplay/items.h>
#include <blocksLoader.h>
#include <gameplay/life.h>
#include <gamePlayLogic.h>
#include <gameplay/player.h>
#include <gameplay/blocks/structureBaseBlock.h>

float determineTextSize(gl2d::Renderer2D &renderer, const std::string &str,
	gl2d::Font &f, glm::vec4 transform, bool minimize = true)
{
	float size = 4;

	auto s = renderer.getTextSize(str.c_str(), f, size);

	float ratioX = transform.z / s.x;
	float ratioY = transform.w / s.y;


	if (ratioX > 1 && ratioY > 1)
	{

		///keep size
		//return size;

		//else
		//{
		//	if (ratioX > ratioY)
		//	{
		//		return size * ratioY;
		//	}
		//	else
		//	{
		//		return size * ratioX;
		//	}
		//}

	}
	else
	{
		if (ratioX < ratioY)
		{
			size *= ratioX;
		}
		else
		{
			size *= ratioY;
		}
	}

	if (minimize)
	{
		size *= 0.9;
	}
	else
	{
		size *= 0.9;
	}

	return size;
}

void renderTextIntoBox(gl2d::Renderer2D &renderer, const std::string &str,
	gl2d::Font &f, glm::vec4 transform, glm::vec4 color, bool minimize = true, bool alignLeft = false)
{
	auto newS = determineTextSize(renderer, str, f, transform, minimize);

	glm::vec2 pos = glm::vec2(transform);

	if (!alignLeft)
	{
		pos.x += transform.z / 2.f;
		pos.y += transform.w / 3.f;
		renderer.renderText(pos, str.c_str(), f, color, newS);
	}
	else
	{
		//pos.x += transform.z * 0.02;
		pos.y += transform.w * 0.4;
		renderer.renderText(pos, str.c_str(), f, color, newS, 4, 3, false);
	}

}

glm::vec4 shrinkRectanglePercentage(glm::vec4 in, float perc)
{
	float shrinkX = in.z * perc;
	float shrinkY = in.w * perc;

	in.x += shrinkX / 2.f;
	in.y += shrinkY / 2.f;

	in.z -= shrinkX;
	in.w -= shrinkY;

	return in;
}

glm::vec4 shrinkRectanglePercentageMoveDown(glm::vec4 in, float perc)
{
	float shrinkX = in.z * perc;
	float shrinkY = in.w * perc;

	in.x += shrinkX / 2.f;
	in.y += shrinkY;

	in.z -= shrinkX;
	in.w -= shrinkY;

	return in;
}


void UiENgine::init()
{
	renderer2d.create();

	

}

void UiENgine::loadTextures(std::string path)
{
	if (!font.texture.id)
	{
		font.createFromFile((path + "font.ttf").c_str());
	}
	
	if(!uiTexture.id)
	uiTexture.loadFromFile((path + "ui0.png").c_str(), true, true);
	
	if(!buttonTexture.id)
	buttonTexture.loadFromFile((path + "button.png").c_str(), true, true);

	if (!itemsBar.id)
	{
		itemsBar.loadFromFile((path + "ui1.png").c_str(), true, true);
		itemsBarSize = itemsBar.GetSize();
	}

	if (!itemsHighlighter.id)
	{
		itemsHighlighter.loadFromFile((path + "ui2.png").c_str(), true, true);
		itemsHighlighterSize = itemsHighlighter.GetSize();
	}

	if (!itemsBarInventory.id)
	{
		itemsBarInventory.loadFromFile((path + "ui3.png").c_str(), true, true);
		itemsBarInventorySize = itemsBarInventory.GetSize();
	}

	if (!oneInventorySlot.id)
	{
		oneInventorySlot.loadFromFile((path + "ui4.png").c_str(), true, true);
		oneInventorySlotSize = oneInventorySlot.GetSize();
	}

	if (!playerCell.id)
	{
		playerCell.loadFromFile((path + "ui5.png").c_str(), true, true);
		playerCellSize = playerCell.GetSize();
	}

}

void UiENgine::clearOnlyTextures()
{
	font.cleanup();
	uiTexture.cleanup();
	buttonTexture.cleanup();

	itemsBar.cleanup();
	itemsBar = {};

	itemsHighlighter.cleanup();
	itemsHighlighter = {};

	itemsBarInventory.cleanup();
	itemsBarInventorySize = {};

	oneInventorySlot.cleanup();
	oneInventorySlotSize = {};

	playerCell.cleanup();
	playerCellSize = {};
}

const int INVENTORY_TAB_DEFAULT = 0;
const int INVENTORY_TAB_BLOCKS = 1;
const int INVENTORY_TAB_ITEMS = 2;

void UiENgine::renderGameUI(float deltaTime, int w, int h
	, int itemSelected, PlayerInventory &inventory, BlocksLoader &blocksLoader,
	bool insideInventory, int &cursorItemIndex, Item &itemToCraft,
	bool insideCraftingTable, int &currentInventoryTab, bool isCreative,
	unsigned short &selectedItem, Life &playerHealth, ProgramData &programData, LocalPlayer &player)
{

	if (!isCreative) { currentInventoryTab = INVENTORY_TAB_DEFAULT; }

	cursorItemIndex = -1;
	glm::vec4 cursorItemIndexBox = {};
	auto mousePos = platform::getRelMousePosition();

	auto renderOneItem = [&](glm::vec4 itemBox, Item & item, float in = 8.f / 22.f)
	{
		if (item.type == 0)return;

		if (item.type < BlocksCount)
		{

			gl2d::Texture t;
			t.id = blocksLoader.texturesIds[getGpuIdIndexForBlock(item.type, 0)];

			//we have a block
			renderer2d.renderRectangle(shrinkRectanglePercentage(itemBox, in), t);


		}
		else
		{
			//we have an item
			gl2d::Texture t;
			t.id = blocksLoader.texturesIdsItems[item.type - ItemsStartPoint];

			//we have a block
			renderer2d.renderRectangle(shrinkRectanglePercentage(itemBox, in), t);
		}

		if (item.counter != 1)
		{
			itemBox = shrinkRectanglePercentage(itemBox, in);
			itemBox.x += itemBox.z / 1.4;
			itemBox.y += itemBox.w / 1.4;

			std::string s = std::to_string(item.counter);
			if (item.counter < 10) { s = " " + s; }

			renderer2d.renderText({itemBox}, s.c_str(),
				font, {1,1,1,1}, 0.9 * (itemBox.z/100.f));

		}

	};


	if (w != 0 && h != 0)
	{

		glui::Frame f({0,0, w, h});

		if (insideInventory)
		{

			renderer2d.renderRectangle({0,0,w,h}, {0.1,0.1,0.1,0.1});

			float minDimenstion = std::min(w, h);

			float aspectIncrease = 0;

			if (insideCraftingTable || currentInventoryTab == INVENTORY_TAB_ITEMS || currentInventoryTab == INVENTORY_TAB_BLOCKS)
			{
				aspectIncrease = 0.10;
			}


			auto inventoryBox = glui::Box().xCenter().yCenter().yDimensionPixels(minDimenstion * 0.85f).
				xAspectRatio(1.f + aspectIncrease)();
			//{
			//	auto inventoryBox2 = glui::Box().xCenter().yCenter().xDimensionPixels(minDimenstion * 0.9f).
			//		yAspectRatio(1.f - aspectIncrease)();
			//
			//	if (inventoryBox2.w < inventoryBox.w)
			//	{
			//		inventoryBox = inventoryBox2;
			//	}
			//}


			//render inventory box
			renderer2d.render9Patch(inventoryBox,
				24, {1,1,1,1}, {}, 0.f, buttonTexture, GL2D_DefaultTextureCoords, {0.2,0.8,0.8,0.2});
			//renderer2d.renderRectangle(inventoryBox, {1,0,0,1});

			if (glui::aabb(inventoryBox, mousePos))
			{
				cursorItemIndex = -2;
			}




			int oneItemSize = 0;

			glui::Frame insideUiCell(inventoryBox);

			{

				glui::Frame insideInventoryLeft(glui::Box().xLeft().yTop().
					yDimensionPercentage(1.f).xAspectRatio(1.f)());

				auto checkInside = [&](int start, glm::vec4 box)
				{
					auto itemBox = box;
					itemBox.z = itemBox.w;
					for (int i = start; i < start + 9; i++)
					{
						itemBox.x = box.x + itemBox.z * (i - start);
						if (glui::aabb(itemBox, mousePos))
						{
							cursorItemIndex = i;
							cursorItemIndexBox = itemBox;
							renderer2d.renderRectangle(shrinkRectanglePercentage(itemBox, (2.f / 22.f)),
								{0.7,0.7,0.7,0.5});
						}
					}
				};

				auto checkInsideCreativeMenu = [&](int start, glm::vec4 box)
				{
					auto itemBox = box;
					itemBox.z = itemBox.w;
					for (int i = start; i < start + 9; i++)
					{
						if (!isItem(i) && !isBlock(i)) { continue; }

						itemBox.x = box.x + itemBox.z * (i - start);
						if (glui::aabb(itemBox, mousePos))
						{
							selectedItem = i;
							cursorItemIndexBox = itemBox;
							renderer2d.renderRectangle(shrinkRectanglePercentage(itemBox, (2.f / 22.f)),
								{0.7,0.7,0.7,0.5});
						}
					}
				};

				auto checkInsideOneCell = [&](int start, glm::vec4 box)
				{
					auto itemBox = box;
					itemBox.z = itemBox.w;

					if (glui::aabb(itemBox, mousePos))
					{
						cursorItemIndex = start;
						cursorItemIndexBox = itemBox;
						renderer2d.renderRectangle(shrinkRectanglePercentage(itemBox, (2.f / 22.f)),
							{0.7,0.7,0.7,0.5});
					}
				};

				auto hotBarBox = glui::Box().xCenter().yBottomPerc(-0.05).xDimensionPercentage(0.9).
					yAspectRatio(itemsBarInventorySize.y / itemsBarInventorySize.x)();

				renderer2d.renderRectangle(hotBarBox, itemsBarInventory);

				oneItemSize = hotBarBox.w;


				//render items
				auto renderItems = [&](int start, glm::ivec4 box)
				{
					auto itemBox = box;
					itemBox.z = itemBox.w;
					for (int i = start; i < start + 9; i++)
					{
						if (inventory.items[i].type)
						{
							itemBox.x = box.x + itemBox.z * (i - start);
							renderOneItem(itemBox, inventory.items[i], 4.f / 22.f);
						}
					}
				};


				glm::vec4 tabBox = inventoryBox;
				tabBox.z = oneItemSize;
				tabBox.w = oneItemSize / 2;
				tabBox.x += oneItemSize / 4.f;
				tabBox.y -= oneItemSize / 2.f;

				const int BARS_COUNT = 7;

				//render side bar
				auto renderSideSlider = [&](glm::ivec4 box)
				{
					glm::vec4 barBox = box;
					barBox.y -= box.w * (BARS_COUNT - 1);
					barBox.x += box.z;
					barBox.z = barBox.w;
					barBox.w = box.w * (BARS_COUNT);

					//renderer2d.renderRectangle(barBox, Colors_Red);

					glm::ivec4 topBox = barBox;
					topBox.w = topBox.z;

					glm::ivec4 bottomBox = barBox;
					bottomBox.y += barBox.w - barBox.z;
					bottomBox.w = bottomBox.z;

					int slider = 0;

					if (glui::drawButton(renderer2d, topBox, Colors_White, "",
						font, buttonTexture, platform::getRelMousePosition(),
						platform::isLMouseHeld(), platform::isLMouseReleased()))
					{
						slider--;
					}

					if (glui::drawButton(renderer2d, bottomBox, Colors_White, "",
						font, buttonTexture, platform::getRelMousePosition(),
						platform::isLMouseHeld(), platform::isLMouseReleased()))
					{
						slider++;
					}

					slider -= platform::getScroll();

					return slider;
				};

				if(currentInventoryTab == INVENTORY_TAB_DEFAULT)
				{

					//bottom part
					auto inventoryBars = glui::Box().xCenter().yBottomPerc(-0.17).xDimensionPercentage(0.9).
						yAspectRatio(itemsBarInventorySize.y / itemsBarInventorySize.x)();
					renderer2d.renderRectangle(inventoryBars, itemsBarInventory);

					auto inventoryBars2 = inventoryBars;
					inventoryBars2.y -= inventoryBars2.w;
					renderer2d.renderRectangle(inventoryBars2, itemsBarInventory);

					auto inventoryBars3 = inventoryBars2;
					inventoryBars3.y -= inventoryBars3.w;
					renderer2d.renderRectangle(inventoryBars3, itemsBarInventory);

					checkInside(9, inventoryBars);
					checkInside(18, inventoryBars2);
					checkInside(27, inventoryBars3);


					//upper part
					glui::Frame insideUpperPart(glui::Box().xCenter().yTopPerc(0.05).
						xDimensionPercentage(0.9).yDimensionPercentage(0.45)());
					

			

					//highlight
					//renderer2d.renderRectangle(glui::Box().xLeft().yTop().xDimensionPercentage(1.f).
					//	yDimensionPercentage(1.f)(), {1,0,0,0.5});

					//player stuff
					{
						auto armourBox = glui::Box().xLeft().yTopPerc(0.1).xDimensionPercentage(1.f / 9.f).
							yAspectRatio(1.f)();
						auto start = armourBox;
						glm::vec4 playerBox = armourBox;
						playerBox.x += playerBox.z;
						playerBox.w *= 4;
						playerBox.z = (playerBox.w / playerCellSize.y) * playerCellSize.x;

						//render armour stuff
						renderer2d.renderRectangle(armourBox, oneInventorySlot);

						armourBox.y += armourBox.w;
						renderer2d.renderRectangle(armourBox, oneInventorySlot);

						armourBox.y += armourBox.w;
						renderer2d.renderRectangle(armourBox, oneInventorySlot);

						armourBox.y += armourBox.w;
						renderer2d.renderRectangle(armourBox, oneInventorySlot);

						//render player
						renderer2d.renderRectangle(playerBox, playerCell);

						//render armour stuff to the right
						armourBox = start;
						armourBox.x = playerBox.x + playerBox.z;
						renderer2d.renderRectangle(armourBox, oneInventorySlot);

						armourBox.y += armourBox.w;
						renderer2d.renderRectangle(armourBox, oneInventorySlot);

						armourBox.y += armourBox.w;
						renderer2d.renderRectangle(armourBox, oneInventorySlot);

						armourBox.y += armourBox.w;
						renderer2d.renderRectangle(armourBox, oneInventorySlot);

					}


					//crafting table stuff
					if (insideCraftingTable)
					{

						glm::vec4 craftingStart = glui::Box().xLeftPerc(0.56).yTopPerc(0.2).xDimensionPercentage(1.f / 9.f).
							yAspectRatio(1.f)();
						renderer2d.renderRectangle(craftingStart, oneInventorySlot);
						glm::vec4 secondCrafting = craftingStart; secondCrafting.x += craftingStart.z;
						renderer2d.renderRectangle(secondCrafting, oneInventorySlot);
						glm::vec4 thirdCrafting = secondCrafting; thirdCrafting.x += craftingStart.z;
						renderer2d.renderRectangle(thirdCrafting, oneInventorySlot);


						glm::vec4 fourthCrafting = craftingStart; fourthCrafting.y += craftingStart.w;
						renderer2d.renderRectangle(fourthCrafting, oneInventorySlot);
						glm::vec4 fifthCrafting = fourthCrafting; fifthCrafting.x += craftingStart.z;
						renderer2d.renderRectangle(fifthCrafting, oneInventorySlot);
						glm::vec4 sixthCrafting = fifthCrafting; sixthCrafting.x += craftingStart.z;
						renderer2d.renderRectangle(sixthCrafting, oneInventorySlot);

						glm::vec4 seventhCrafting = fourthCrafting; seventhCrafting.y += craftingStart.w;
						renderer2d.renderRectangle(seventhCrafting, oneInventorySlot);
						glm::vec4 eighthCrafting = seventhCrafting; eighthCrafting.x += craftingStart.z;
						renderer2d.renderRectangle(eighthCrafting, oneInventorySlot);
						glm::vec4 ninethCrafting = eighthCrafting; ninethCrafting.x += craftingStart.z;
						renderer2d.renderRectangle(ninethCrafting, oneInventorySlot);


						//result
						glm::vec4 resultCrafting = craftingStart; resultCrafting.x += craftingStart.z * 4; resultCrafting.y += craftingStart.w * 1.f;
						renderer2d.renderRectangle(resultCrafting, oneInventorySlot);


						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX, craftingStart);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 1, secondCrafting);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 2, thirdCrafting);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 3, fourthCrafting);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 4, fifthCrafting);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 5, sixthCrafting);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 6, seventhCrafting);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 7, eighthCrafting);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 8, ninethCrafting);


						checkInsideOneCell(PlayerInventory::CRAFTING_RESULT_INDEX, resultCrafting);

						renderOneItem(craftingStart, inventory.crafting[0], 4.f / 22.f);
						renderOneItem(secondCrafting, inventory.crafting[1], 4.f / 22.f);
						renderOneItem(thirdCrafting, inventory.crafting[2], 4.f / 22.f);
						renderOneItem(fourthCrafting, inventory.crafting[3], 4.f / 22.f);
						renderOneItem(fifthCrafting, inventory.crafting[4], 4.f / 22.f);
						renderOneItem(sixthCrafting, inventory.crafting[5], 4.f / 22.f);
						renderOneItem(seventhCrafting, inventory.crafting[6], 4.f / 22.f);
						renderOneItem(eighthCrafting, inventory.crafting[7], 4.f / 22.f);
						renderOneItem(ninethCrafting, inventory.crafting[8], 4.f / 22.f);

						renderOneItem(resultCrafting, itemToCraft, 4.f / 22.f);

					}
					else
					{
						//crafting (normal)

						glm::vec4 craftingStart = glui::Box().xLeftPerc(0.56).yTopPerc(0.2).xDimensionPercentage(1.f / 9.f).
							yAspectRatio(1.f)();
						renderer2d.renderRectangle(craftingStart, oneInventorySlot);
						glm::vec4 secondCrafting = craftingStart; secondCrafting.x += craftingStart.z;
						renderer2d.renderRectangle(secondCrafting, oneInventorySlot);
						glm::vec4 thirdCrafting = craftingStart; thirdCrafting.y += craftingStart.w;
						renderer2d.renderRectangle(thirdCrafting, oneInventorySlot);
						glm::vec4 fourthCrafting = craftingStart; fourthCrafting.x += craftingStart.z; fourthCrafting.y += craftingStart.w;
						renderer2d.renderRectangle(fourthCrafting, oneInventorySlot);


						//result
						glm::vec4 resultCrafting = craftingStart; resultCrafting.x += craftingStart.z * 3; resultCrafting.y += craftingStart.w * 0.5;
						renderer2d.renderRectangle(resultCrafting, oneInventorySlot);


						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX, craftingStart);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 1, secondCrafting);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 2, thirdCrafting);
						checkInsideOneCell(PlayerInventory::CRAFTING_INDEX + 3, fourthCrafting);

						checkInsideOneCell(PlayerInventory::CRAFTING_RESULT_INDEX, resultCrafting);

						renderOneItem(craftingStart, inventory.crafting[0], 4.f / 22.f);
						renderOneItem(secondCrafting, inventory.crafting[1], 4.f / 22.f);
						renderOneItem(thirdCrafting, inventory.crafting[2], 4.f / 22.f);
						renderOneItem(fourthCrafting, inventory.crafting[3], 4.f / 22.f);
						
						renderOneItem(resultCrafting, itemToCraft, 4.f / 22.f);
					
					}
				

					renderItems(9, inventoryBars);
					renderItems(18, inventoryBars2);
					renderItems(27, inventoryBars3);


				}
				else if (currentInventoryTab == INVENTORY_TAB_BLOCKS)
				{

					auto inventoryBars = glui::Box().xCenter().yBottomPerc(-0.17).xDimensionPercentage(0.9).
						yAspectRatio(itemsBarInventorySize.y / itemsBarInventorySize.x)();

					static int currentStartRow = 0;
					currentStartRow += renderSideSlider(inventoryBars);
					currentStartRow = glm::clamp(currentStartRow, 0,
						(((int)BlocksCount / 9) - BARS_COUNT) + 1);
					if (currentStartRow < 0) { currentStartRow = 0; }

					//render items
					auto renderCreativeBlocks = [&](int start, glm::ivec4 box)
					{
						auto itemBox = box;
						itemBox.z = itemBox.w;
						for (int i = start; i < start + 9; i++)
						{
							if (i < BlocksCount)
							{
								itemBox.x = box.x + itemBox.z * (i - start);
								renderOneItem(itemBox, Item(i), 4.f / 22.f);
							}
						}
					};

					for (int i = 0; i < BARS_COUNT; i++)
					{
						renderer2d.renderRectangle(inventoryBars, itemsBarInventory);

						checkInsideCreativeMenu((6 - i) * 9 + 1 + currentStartRow * 9, inventoryBars);
						renderCreativeBlocks((6 - i) * 9 + 1 + currentStartRow * 9, inventoryBars);

						inventoryBars.y -= inventoryBars.w;
					}

				}
				else if (currentInventoryTab == INVENTORY_TAB_ITEMS)
				{

					//render items
					auto renderCreativeItems = [&](int start, glm::ivec4 box)
					{
						auto itemBox = box;
						itemBox.z = itemBox.w;
						for (int i = start; i < start + 9; i++)
						{
							if (i < lastItem)
							{
								itemBox.x = box.x + itemBox.z * (i - start);
								renderOneItem(itemBox, Item(i), 4.f / 22.f);
							}
						}
					};

					auto inventoryBars = glui::Box().xCenter().yBottomPerc(-0.17).xDimensionPercentage(0.9).
						yAspectRatio(itemsBarInventorySize.y / itemsBarInventorySize.x)();
						
					static int currentStartRow = 0;
					currentStartRow += renderSideSlider(inventoryBars);
					currentStartRow = glm::clamp(currentStartRow, 0,
						(((int)(lastItem - ItemsStartPoint) / 9) - BARS_COUNT) + 1);
					if (currentStartRow < 0) { currentStartRow = 0; }

					for (int i = 0; i < BARS_COUNT; i++)
					{
						renderer2d.renderRectangle(inventoryBars, itemsBarInventory);

						checkInsideCreativeMenu((6 - i) * 9 + ItemsStartPoint + currentStartRow*9, inventoryBars);
						renderCreativeItems((6-i) * 9 + ItemsStartPoint + currentStartRow * 9, inventoryBars);

						inventoryBars.y -= inventoryBars.w;
					}



				}

				checkInside(0, hotBarBox);
				renderItems(0, hotBarBox);


				if (isCreative)
				{

					GLuint textures[3] = {
						blocksLoader.texturesIdsItems[wooddenSword - ItemsStartPoint],
						blocksLoader.texturesIds[getGpuIdIndexForBlock(grassBlock, 0)],
						blocksLoader.texturesIdsItems[stick - ItemsStartPoint],
					};

					for (int i = 0; i < 3; i++)
					{
						glm::vec4 selected = {};
						if (i == currentInventoryTab)
						{
							selected = glm::vec4(0, 0, 0, 12);
						}

						glm::vec4 color = {0.8,0.8,0.8,1};

						if (i != currentInventoryTab && glui::aabb(tabBox, platform::getRelMousePosition()))
						{
							color = {1.2,1.2,1.2,1};
						}
						else if (i == currentInventoryTab)
						{
							color = {1,1,1,1};
						}


						renderer2d.render9Patch(tabBox + selected,
							24, color, {}, 0.f, buttonTexture,
							{0,1,1,0.5}, {0.2,0.8,0.8,0.5});


						if (glui::aabb(tabBox, platform::getRelMousePosition()) &&
							platform::isLMousePressed()
							)
						{
							currentInventoryTab = i;
						}

						{
							auto newBox = tabBox;

							gl2d::Texture t; t.id = textures[i];
							if (i == currentInventoryTab)
							{
								newBox.w *= 2;
								newBox = shrinkRectanglePercentage(newBox, 0.3);
								renderer2d.renderRectangle(newBox, t, Colors_White,
									{}, 0);
							}
							else
							{
								newBox = shrinkRectanglePercentageMoveDown(newBox, 0.3);
								renderer2d.renderRectangle(newBox, t, Colors_White,
									{}, 0, {0,1,1,0.5});
							}
						}


						tabBox.x += oneItemSize + oneItemSize * (0.1f);
					}

				}


			

				

			}

			if(aspectIncrease)
			{

				glui::Frame insideInventoryRight(glui::Box().xRight().yTop().
					yDimensionPercentage(1.f).xDimensionPercentage(aspectIncrease)());
				
				//renderer2d.renderRectangle(glui::Box().xLeft().yTop().yDimensionPercentage(1.f).
				//	xDimensionPercentage(1.f)(), {1,0,0,0.2});

			}


			//render held item
			glm::vec4 itemPos(mousePos.x - oneItemSize/2.f, mousePos.y - oneItemSize/2.f,
				oneItemSize, oneItemSize);
			renderOneItem(itemPos, inventory.heldInMouse, 0);

			//render hovered item stuff
			if (cursorItemIndex >= 0 && !inventory.heldInMouse.type)
			{

				auto item = inventory.getItemFromIndex(cursorItemIndex);

				if (item && item->type && item->metaData.size())
				{

					auto box = cursorItemIndexBox;

					box.x += box.z * 0.5;
					box.y += box.w * 0.8;
					box.w *= 1;
					box.z *= 1.8;

					renderer2d.render9Patch(box,
						24, {0.3,0.2,0.2,0.8}, {}, 0.f, buttonTexture, GL2D_DefaultTextureCoords, {0.2,0.8,0.8,0.2});

					std::string text = item->formatMetaDataToString();

					renderTextIntoBox(renderer2d, text, font, box, Colors_White, true, true);
					

				}

			}

		}
		else
		{
			//cross
			renderer2d.renderRectangle(
				glui::Box().xCenter().yCenter().xDimensionPixels(30).yAspectRatio(1.f),
				uiTexture, Colors_White, {}, 0,
				uiAtlas.get(2, 0)
			);



			//items
			auto itemsBarBox = glui::Box().xCenter().yBottom().xDimensionPercentage(0.35)
				.yAspectRatio(itemsBarSize.y / itemsBarSize.x)();
			renderer2d.renderRectangle(itemsBarBox, itemsBar, Colors_White, {}, 0);

			float itemBoxTexelSize = itemsBarBox.z / itemsBarSize.x;
			float itemBoxAdvance = (itemsBarBox.z - itemBoxTexelSize * 2) / 9.f;


			//icons
			auto itemBox = itemsBarBox;
			itemBox.z = itemBox.w;
			for (int i = 0; i < 9; i++)
			{
				if (inventory.items[i].type)
				{
					itemBox.x = itemsBarBox.x + itemBoxAdvance * i;
					renderOneItem(itemBox, inventory.items[i]);
				}
			}

			auto selectedBox = itemsBarBox;
			selectedBox.z = selectedBox.w;
			selectedBox.x += itemBoxAdvance * itemSelected;
			renderer2d.renderRectangle(selectedBox, itemsHighlighter, Colors_White, {}, 0);

			//if (!isCreative)
			{
				auto heartBox = itemsBarBox;
				heartBox.z = itemsBarBox.w / 2.5;
				heartBox.w = itemsBarBox.w / 2.5;
				heartBox.y -= heartBox.w;

				int background = 0;

				static Oscilator lifeFlash(0.1, 2);
				bool tookDamage = 0;

				if (player.justRecievedDamageTimer > 0)
				{
					player.justRecievedDamageTimer -= deltaTime;
					tookDamage = true;
					background = 1;
				}
				else if (player.justHealedTimer > 0)
				{
					player.justHealedTimer -= deltaTime;
					background = 2;
				}

				if (background == 1)
				{
					background = lifeFlash.currentFaze;
					lifeFlash.update(deltaTime);
				}
				else if (background == 2)
				{
					background = lifeFlash.currentFaze == 1 ? 2 : 0;
					lifeFlash.update(deltaTime);
				}else
				{
					lifeFlash.reset();
				}


				{
					auto heartBoxCopy = heartBox;
					for (int i = 0; i < playerHealth.maxLife; i += 2)
					{
						renderer2d.renderRectangle(heartBoxCopy, programData.heartsTexture, Colors_White, {}, 0,
							programData.heartsAtlas.get(background, 0));
						heartBoxCopy.x += heartBoxCopy.z;
					}
				}

				if (tookDamage)
				{
					auto heartBoxCopy = heartBox;
					for (int i = 0; i < playerHealth.maxLife; i += 2)
					{
						if (i < player.lastLife.life)
						{
							if (i + 1 >= player.lastLife.life)
							{
								renderer2d.renderRectangle(heartBoxCopy, programData.heartsTexture, {1,1,1,0.3}, {}, 0,
									programData.heartsAtlas.get(4, 0));
							}
							else
							{
								renderer2d.renderRectangle(heartBoxCopy, programData.heartsTexture, {1,1,1,0.3}, {}, 0,
									programData.heartsAtlas.get(3, 0));
							}
						}
						heartBoxCopy.x += heartBoxCopy.z;
					}
				}
				else
				{
					player.lastLife = player.life;
				}

				for (int i = 0; i < playerHealth.maxLife; i += 2)
				{
					

					if (i < playerHealth.life)
					{
						if (i + 1 >= playerHealth.life)
						{
							renderer2d.renderRectangle(heartBox, programData.heartsTexture, Colors_White, {}, 0,
								programData.heartsAtlas.get(4, 0));
						}
						else
						{
							renderer2d.renderRectangle(heartBox, programData.heartsTexture, Colors_White, {}, 0,
								programData.heartsAtlas.get(3, 0));
						}
					}

					heartBox.x += heartBox.z;
				}


			}

		}

	}


}

bool UiENgine::renderBaseBlockUI(float deltaTime, int w, int h,
	ProgramData &programData, BaseBlock &baseBlock)
{

	if (w != 0 && h != 0)
	{

		glui::Frame f({0,0, w, h});

		auto textBox = glui::Box().xCenter().yTopPerc(0.3).xDimensionPercentage(0.8).
			yDimensionPixels(150)();

		renderer2d.renderRectangle({0,0,w,h}, {0.1,0.1,0.1,0.1});



		//glui::renderTextInput(renderer2d, "", baseBlock.name,
		//	sizeof(baseBlock.name), platform::getTypedInput(), font,
		//	textBox, Colors_Gray, buttonTexture, false, true);

		menuRenderer.Begin(5391);
		menuRenderer.SetAlignModeFixedSizeWidgets({0,150});

		menuRenderer.InputText("##123", baseBlock.name, sizeof(baseBlock.name), Colors_Gray,
			buttonTexture);


		menuRenderer.sliderint8("Offset X", &baseBlock.offsetX, -16, 16, Colors_White, buttonTexture, Colors_Gray, buttonTexture, Colors_White);
		menuRenderer.sliderint8("Offset Y", &baseBlock.offsetY, -16, 16, Colors_White, buttonTexture, Colors_Gray, buttonTexture, Colors_White);
		menuRenderer.sliderint8("Offset Z", &baseBlock.offsetZ, -16, 16, Colors_White, buttonTexture, Colors_Gray, buttonTexture, Colors_White);

		menuRenderer.sliderUint8("Size X", &baseBlock.sizeX, 0, 120, Colors_White, buttonTexture, Colors_Gray, buttonTexture, Colors_White);
		menuRenderer.sliderUint8("Size Y", &baseBlock.sizeY, 0, 120, Colors_White, buttonTexture, Colors_Gray, buttonTexture, Colors_White);
		menuRenderer.sliderUint8("Size Z", &baseBlock.sizeZ, 0, 120, Colors_White, buttonTexture, Colors_Gray, buttonTexture, Colors_White);


		bool rez = menuRenderer.Button("Save", Colors_Gray, buttonTexture);

		menuRenderer.End();

		return rez;
	}

	return false;
}

void Oscilator::update(float deltaTime)
{
	currentTimer -= deltaTime;
	if (currentTimer < 0)
	{
		currentTimer = maxFazeTime;
		currentFaze++;

		if (currentFaze >= maxFazes)
		{
			currentFaze = 0;
		}
	}
}

void Oscilator::reset()
{
	currentTimer = 0;
	currentFaze = 0;
}

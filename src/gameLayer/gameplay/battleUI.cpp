#include <gameplay/battleUI.h>
#include <platform/platformInput.h>
#include <glui/glui.h>
#include <gameplay/entity.h>
#include <imgui.h>

void BattleUI::reset()
{
	*this = {};

}


HitResult BattleUI::update(Item &item, int inventorySlot, bool dontRun,
	UiENgine &ui, std::minstd_rand &rng, float deltaTime)
{

	static float debugSpeed = 1;
	ImGui::Begin("Test speed");
	ImGui::SliderFloat("Speed: ", &debugSpeed, 1, 10);
	ImGui::End();

	HitResult result;

	if (item.type != lastItemType || inventorySlot != lastInventorySlot || dontRun)
	{
		reset();
	}

	if (dontRun) { return result; }

	lastItemType = item.type;
	lastInventorySlot = inventorySlot;

	bool leftPressed = platform::isLMousePressed();
	bool rightPressed = platform::isRMousePressed();

	auto &renderer = ui.renderer2d;
	
	glui::Frame f({0, 0, renderer.windowW, renderer.windowH});

	{
	#pragma region set the ui box
		auto box1 = glui::Box().xCenter().yCenter().yDimensionPercentage(0.4).xAspectRatio(1)();
		auto box2 = glui::Box().xCenter().yCenter().xDimensionPercentage(0.4).yAspectRatio(1)();
		if (box2.z < box1.x) { box1 = box2; }
		glui::Frame f(box1);
		auto fullBox = glui::Box().xLeft(0).yTop(0).xDimensionPercentage(1).yDimensionPercentage(1)();
	#pragma endregion

		if (item.isSpear())
		{

			auto stats = item.getWeaponStats();
			stats.speed = debugSpeed;

			if (!started)
			{
				if (leftPressed || rightPressed)
				{
					//TODO attack here.
					started = true;

					result.hit = true;
					result.isSwipeAttack = rightPressed;
					result.hitCorectness = 1;

					
					timer = 0;
					
				}

			}
			else
			{
				timer -= deltaTime;

				if (timer <= 0)
				{
					auto range = stats.getTimerCulldownRangeForAttacks();
					timer = getRandomNumberFloat(rng, range.x, range.y);
					float speed = stats.getUIMoveSpeed();
					
					if (spearData.currentBallsCount < spearData.MAX_POSITIONS)
					{
						glm::vec2 vector = {1,0};
						vector = glm::rotate(vector, getRandomNumberFloat(rng, 0, 3.1415926*2.f));
						spearData.balls[spearData.currentBallsCount] = {};
						spearData.balls[spearData.currentBallsCount].position = vector;
						spearData.balls[spearData.currentBallsCount].velocity = -vector * speed;
						spearData.currentBallsCount++;
					}

				}

				float ballRelativeSize = 0.05;
				float hitRelativeSize = 0.1;

				for (int i = 0; i < spearData.currentBallsCount; i++)
				{
					auto &b = spearData.balls[i];
					
					if (b.passedCenter)
					{
						b.dieTimer -= deltaTime;
						if (b.dieTimer <= 0)
						{
							for (int j = i+1; j < spearData.currentBallsCount; j++)
							{
								spearData.balls[j - 1] = spearData.balls[j];
							}
							spearData.currentBallsCount--;
							continue;
						}
					}
					else
					{
						if (glm::length(b.position) < (ballRelativeSize+hitRelativeSize)/2.f)
						{
							b.passedCenter = true;
							b.dieTimer = 1;
						}
					}

					b.position += b.velocity * deltaTime;
				}
				
				if ((leftPressed || rightPressed) && spearData.currentBallsCount)
				{
					result.hit = true;

					float length = glm::length(spearData.balls[0].position);

					if (length > (ballRelativeSize + hitRelativeSize) * 2.f)
					{
						result.hitCorectness = 0;
					}
					else
					{
						if (length <= (ballRelativeSize + hitRelativeSize) / 2.f)
						{
							result.hitCorectness = 1;
							result.bonusCritChance = 1;
						}
						else
						{
							float hitCorectness = length - ((ballRelativeSize + hitRelativeSize) / 2.f);
							hitCorectness /= ((ballRelativeSize + hitRelativeSize) * 1.5f);
							hitCorectness = 1 - hitCorectness;
							hitCorectness = glm::clamp(hitCorectness, 0.f, 1.f);

							//todo flatten here but on the server
							result.bonusCritChance = (hitCorectness * 2.f) - 1;
							result.hitCorectness = hitCorectness;
							result.hitCorectness = std::max(result.hitCorectness, 0.1f);
						}

						

					}

				}

				//renderer.renderRectangle(fullBox, {1,1,1,0.2});


				renderer.renderRectangle(fullBox,
					ui.battleTextures[UiENgine::BattleTextures::circle]);

				renderer.renderRectangle(fullBox,
					ui.battleTextures[UiENgine::BattleTextures::leftButton], Colors_Red);

				renderer.renderRectangle(fullBox,
					ui.battleTextures[UiENgine::BattleTextures::rightButton], Colors_Blue);

				renderer.renderRectangle(fullBox,
					ui.battleTextures[UiENgine::BattleTextures::leftButtonFrontAttack]);

				renderer.renderRectangle(fullBox,
					ui.battleTextures[UiENgine::BattleTextures::rightButtonSwipeAttack]);

				renderer.renderCircleOutline(glm::vec2{glm::vec2(fullBox) 
					+ glm::vec2(fullBox.z/ 2.f,fullBox.w / 2.)}
				, Colors_White, fullBox.z * hitRelativeSize, 6);

				for (int i = 0; i < spearData.currentBallsCount; i++)
				{
					auto &b = spearData.balls[i];
					auto color = Colors_White;

					if (b.passedCenter) { color = Colors_Green; }

					float size = fullBox.z * ballRelativeSize;

					glm::vec2 position = (b.position + glm::vec2(1.f))/2.f;
					position.x = fullBox.x + fullBox.z * position.x;
					position.y = fullBox.y + fullBox.w * position.y;

					renderer.renderCircleOutline(glm::vec2{position}
						, color, size, 6);
				}

			}


		}

	}

	return result;
}

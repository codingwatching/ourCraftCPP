#include <gameplay/crafting.h>




static CraftingRecepie recepies[] =
{
	recepie(Item(ItemTypes::stick, 4), 
		{Item(),Item(),Item(),
		Item(BlockTypes::wooden_plank), Item(), Item(),
		Item(BlockTypes::wooden_plank), Item(), Item()})
};




Item craft4(Item items[4])
{

	Item newItems[9] = {};

	newItems[0] = items[0];
	newItems[1] = items[1];
	newItems[3] = items[2];
	newItems[4] = items[3];

	return craft9(newItems);
}

Item craft9(Item items[9])
{

	Item newItems[9] = {};

	int shiftLeft = 0;
	if ((!items[0].type) && (!items[3].type) && (!items[6].type))
	{
		shiftLeft = 1;
		if ((!items[1].type) && (!items[4].type) && (!items[7].type))
		{
			shiftLeft = 2;
		}
	}

	int shiftDown = 0;
	if ((!items[6].type) && (!items[7].type) && (!items[8].type))
	{
		shiftDown = 1;
		if ((!items[3].type) && (!items[4].type) && (!items[5].type))
		{
			shiftDown = 2;
		}
	}

	for (int i = shiftLeft, x = 0; i < 3; i++, x++)
		for (int j = shiftDown, y = 0; j < 3; j++, y++)
		{
			newItems[x + j * 3] = items[i + y * 3];
		}


	for (int r = 0; r < sizeof(recepies) / sizeof(recepies[0]); r++)
	{
		bool good = true;
		for (int i = 0; i < 9; i++)
		{

			if (newItems[i].type != recepies[r].items[i].type)
			{
				good = false;
				break;
			}

		}
		
		if (good)
		{
			return recepies[r].result;
		}

	}

	return Item();
}

CraftingRecepie recepie(Item result, std::array<Item, 9> items)
{
	CraftingRecepie ret;

	ret.result = result;
	
	for (int i = 0; i < 9; i++)
	{
		ret.items[i] = items[i];
	}

	return ret;
}

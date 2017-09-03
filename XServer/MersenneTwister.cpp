/* 
 * XenServer, an online game server
 * Copyright (C) 2008 XenServer team
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Shared.h"

#define NUMBER_OF_GENERATORS 5
Mutex* m_locks[NUMBER_OF_GENERATORS];
CRandomMersenne* m_generators[NUMBER_OF_GENERATORS];
unsigned int counter=0;

unsigned int GenerateSeed()
{
	unsigned int mstime = gmstime();
	unsigned int stime = (unsigned int)time(NULL);
	unsigned int rnd[2];
	rnd[0] = rand()*rand()*rand();
	rnd[1] = rand()*rand()*rand();

	uint32 val = mstime ^ rnd[0];
	val += stime ^ rnd[1];
	return val;
}

unsigned int RandomUInt(unsigned int max)
{
	unsigned int ret;
	unsigned int c;
	for(;;)
	{
		c=counter%NUMBER_OF_GENERATORS;
		if(m_locks[c]->AttemptAcquire())
		{
			ret = m_generators[c]->IRandom(0, max);
			m_locks[c]->Release();
			return ret;
		}

		++counter;
	}
}

unsigned int RandomUInt(unsigned int min, unsigned int max)
{
	return RandomUInt(max - min) + min;
}

void InitRandomNumberGenerators()
{
	srand(gmstime());
	for(uint32 i = 0; i < NUMBER_OF_GENERATORS; ++i)
	{
		m_generators[i] = new CRandomMersenne(GenerateSeed());
		m_locks[i] = new Mutex();
	}
}

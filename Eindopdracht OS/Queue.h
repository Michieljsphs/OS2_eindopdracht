#pragma once
#include "Block.h"
//#include "Audio.h"
#include <windows.h>
#include <fstream>
const int BUFLEN = 5; // aantal in de queue

class Queue
{
private:
	Block* buffer[BUFLEN]; 
	// Audio audio;
	// Twee indexen en de lengte bijhouden.
	// Redundant, maar lekker makkelijk!
	int treblePos, bassPos, outputPos, inputPos;
	int count;
	signed short sample;
	// Deze sync-dingen moet je gaan gebruiken in get() en put()
	CRITICAL_SECTION busy;
	HANDLE canTreble, canBass, canOutput, canInput;

public:
	Queue();

	Block* input();
	Block* treble(Block* block);
	Block* bass(Block* block);
	Block* output(Block* block);

	static unsigned long __stdcall input(void* pVoid);
	static unsigned long __stdcall treble(void* pVoid);
	static unsigned long __stdcall bass(void* pVoid);
	static unsigned long __stdcall output(void* pVoid);
};
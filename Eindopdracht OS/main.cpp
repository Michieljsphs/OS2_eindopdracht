#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <thread>
#include <math.h>

FILE* filepoint;

using namespace std;
const int BUFLEN = 5; // aantal in de queue
double bb0, bb1, bb2, ba1, ba2;
double tb0 , tb1, tb2, ta1, ta2;



void bassCoefficients(int intensity, double* b0, double* b1, double* b2, double* a1, double* a2)
{
	double frequency = 330;
	double qFactor = 0.5;
	double gain = intensity;
	double sampleRate = 44100;

	double pi = 4.0 * atan(1);
	double a = pow(10.0, gain / 40);
	double w0 = 2 * pi * frequency / sampleRate;
	double alpha = sin(w0) / (2.0 * qFactor);
	double a0 = (a + 1) + (a - 1) * cos(w0) + 2.0 * sqrt(a) * alpha;

	*a1 = -(-2.0 * ((a - 1) + (a + 1) * cos(w0))) / a0;
	*a2 = -((a + 1) + (a - 1) * cos(w0) - 2.0 * sqrt(a) * alpha) / a0;

	*b0 = (a * ((a + 1) - (a - 1) * cos(w0) + 2.0 * sqrt(a) * alpha)) / a0;
	*b1 = (2 * a * ((a - 1) - (a + 1) * cos(w0))) / a0;
	*b2 = (a * ((a + 1) - (a - 1) * cos(w0) - 2.0 * sqrt(a) * alpha)) / a0;
}

void trebleCoefficients(int intensity, double* b0, double* b1, double* b2, double* a1, double* a2)
{
	double frequency = 3300;
	double qFactor = 0.5;
	double gain = intensity;
	double sampleRate = 44100;

	double pi = 4.0 * atan(1);
	double a = pow(10.0, gain / 40);
	double w0 = 2 * pi * frequency / sampleRate;
	double alpha = sin(w0) / (2.0 * qFactor);
	double a0 = (a + 1) - (a - 1) * cos(w0) + 2.0 * sqrt(a) * alpha;

	*a1 = -(2.0 * ((a - 1) - (a + 1) * cos(w0))) / a0;
	*a2 = -((a + 1) - (a - 1) * cos(w0) - 2.0 * sqrt(a) * alpha) / a0;

	*b0 = (a * ((a + 1) + (a - 1) * cos(w0) + 2.0 * sqrt(a) * alpha)) / a0;
	*b1 = (-2.0 * a * ((a - 1) + (a + 1) * cos(w0))) / a0;
	*b2 = (a * ((a + 1) + (a - 1) * cos(w0) - 2.0 * sqrt(a) * alpha)) / a0;
}

class Block
{
public:
	Block() {
		orderNr = 0;
		for (int i = 0; i < 1024; i++) {
			sample[i] = new signed short;
			*sample[i] = 0;
		}
	}

	signed int orderNr;
	signed short *sample[1024];
};

Block* getBlock(FILE* filepoint, int blockNr)
{

	// File was opened, filepoint can be used to read the stream.
	fseek(filepoint, 0, SEEK_END);
	int size = ftell(filepoint);
	fseek(filepoint, 0, SEEK_SET);
	uint16_t* buf = (uint16_t*)malloc(size);
	signed short* f = (signed short*)malloc(size * sizeof(signed short) / 2);

	//read buffer
	fread(buf, sizeof(uint8_t), size, filepoint);
	//std::cout << size << std::endl;
	int blocks = size / 1024;			// calculate the amount of blocks needed
	//std::cout << blocks << std::endl;

	// creates the block
	int beginBuf = blockNr * 1024;			// the begin index for the new block
	int endBuf = (blockNr + 1) * 1024 - 1;	// the last index for the new block
	std::cout << beginBuf << " - " << endBuf << std::endl;

	int newBufIndex = 0;
	signed short* blockBuf = (signed short*)malloc(size);
	signed short temp1;

	for (int sectionIndex = beginBuf; sectionIndex < endBuf; sectionIndex++) {

		//convert to signed short:
		temp1 = buf[sectionIndex] | buf[sectionIndex + 1] << 8;
		f[sectionIndex / 2] = (signed short)temp1 / (signed short)32767;
		if (f[sectionIndex / 2] > 1)
			f[sectionIndex / 2] = 1;
		if (f[sectionIndex / 2] < -1)
			f[sectionIndex / 2] = -1;

		// put signed short into new buffer
		blockBuf[newBufIndex] = temp1;
		//std::cout << blockBuf[newBufIndex] << std::endl;

		newBufIndex++;
	}

	//cout << blockBuf[50] << endl;
	Block* block = new Block;
	for (int i = 0; i < 1024; i++) *block->sample[i] = blockBuf[i];
	//cout << *block->sample[50];
	return block;
}


void equalizer(signed short x[1024], char channel) {
	signed short y[1024];
	if (channel == 't') {
		for (int n = 2; n < 1024; n++) {
			//y[n] = new signed short;
			y[n] = signed short(tb0 * x[n] + tb1 * x[n - 1] + tb2 * x[n - 2] + ta1 * y[n - 1] + ta2 * y[n - 2]);
		}
	}
	else if (channel == 'b') {
		for (int n = 2; n < 1024; n++) {
			//y[n] = new signed short;
			y[n] = signed short(bb0 * x[n] + bb1 * x[n - 1] + bb2 * x[n - 2] + ba1 * y[n - 1] + ba2 * y[n - 2]);
		}
	}
	x = y;
}

class Queue
{
private:
	Block* inputBuffer[BUFLEN];
	Block* trebleBuffer[BUFLEN];
	Block* bassBuffer[BUFLEN];
	Block* outputBuffer[BUFLEN];
	// Twee indexen en de lengte bijhouden.
	// Redundant, maar lekker makkelijk!
	int treblePos, bassPos, outputPos, inputPos;
	int count, orderCount;
	int countInput, countTreble, countBass, countOutput;
	//int bufferPart;
	signed short sample;
	CRITICAL_SECTION busy;
	HANDLE canTreble, canBass, canOutput, canInput;

public:
	Queue() {
		// Hier worden ze goed geinitialiseerd
		InitializeCriticalSection(&busy);
		canTreble = CreateEvent(0, 0, 0, 0);
		canBass = CreateEvent(0, 0, 0, 0);
		canOutput = CreateEvent(0, 0, 0, 0);
		sample = 0;
		count = 0;
		orderCount = 0;
		countInput = 0, countTreble = 0, countBass = 0, countOutput  = 0;
		//bufferPart = BUFLEN / 4;
		inputPos = 0, treblePos = 0, bassPos = 0, outputPos = 0;

		//for (int i = 0; i < BUFLEN; i++) buffer[i] = 0;
	}

	void input() {
		EnterCriticalSection(&busy);
		while (countInput == 4) { // waiting till there is room
			//cout << "waiting for input room" << endl;
			LeaveCriticalSection(&busy);
			WaitForSingleObject(canInput, INFINITE);
			EnterCriticalSection(&busy);
		}
		Block *block = getBlock(filepoint, orderCount);
		block->orderNr = orderCount; // om de positie van het block in het geluidsfragment te onthouden
		orderCount++;
		cout << "input block created" << endl;
		
		// stop block in de buffer op de plek van treblePos
		trebleBuffer[inputPos] = block;

		// inputPos aanpassen
		inputPos = (inputPos + 1) % BUFLEN;
		
		countInput++;
		SetEvent(canTreble);	// set next step
		ResetEvent(canInput);	// reset current step
		LeaveCriticalSection(&busy);

		//return block;
	}

	void treble() {
		EnterCriticalSection(&busy);
		while (countInput == 0) { // waiting for object
			cout << "waiting for TREBLE object" << endl;
			LeaveCriticalSection(&busy);
			WaitForSingleObject(canTreble, INFINITE);
			EnterCriticalSection(&busy);
		}
		cout << "TREBLE object created" << endl;
		Block* block = new Block;
		block = trebleBuffer[treblePos];

		// VOER TREBLE SHIT UIT -------
		signed short* x[1024];
		*x = *block->sample;
		equalizer(*x, 't');
		
		*block->sample = *x;

		bassBuffer[treblePos] = block; // stop blok op de plek van bassPos
		treblePos = (treblePos + 1) % BUFLEN;
		
		countInput--;
		countTreble++;
		SetEvent(canBass);	// set next step
		ResetEvent(canTreble);	// reset current step
		LeaveCriticalSection(&busy);

		//return block;
	}
	void bass() {
		EnterCriticalSection(&busy);
		while (countTreble == 0) {
			cout << "waiting for BASS object" << endl;
			LeaveCriticalSection(&busy);
			WaitForSingleObject(canBass, INFINITE);
			EnterCriticalSection(&busy);
		}
		cout << "BASS object created" << endl;
		Block* block = new Block;
		block = bassBuffer[bassPos];

		// voer BASS shit uit ------
		signed short* x[1024];
		*x = *block->sample;
		equalizer(*x, 'b');

		*block->sample = *x;

		outputBuffer[bassPos] = block; // stop block in buffer op plek van outputPos
		bassPos = (bassPos + 1) % BUFLEN;

		countTreble--;
		countBass++;

		SetEvent(canOutput);	// set next step	
		ResetEvent(canBass);	// reset current step
		LeaveCriticalSection(&busy);

		//return block;
	}
	void output() {
		EnterCriticalSection(&busy);
		while (countBass == 0) {
			cout << "waiting for output object" << endl;
			LeaveCriticalSection(&busy);
			WaitForSingleObject(canOutput, INFINITE);
			EnterCriticalSection(&busy);
		}
		cout << "output object created" << endl;
		Block* block = new Block;
		block = outputBuffer[outputPos];

		// OUTPUT SCHRIJVEN

		outputPos = (outputPos + 1) % BUFLEN;
		countBass--;
		//countOutput++;
		cout << "block ordernummer:  " << block->orderNr << "  block sample: " << *block->sample[2] << endl;
		SetEvent(canInput);	// set next step (input, because the queue is smaller
		ResetEvent(canOutput);	// reset current step
		LeaveCriticalSection(&busy);


		//return block;
	}
};

Queue queue;

DWORD WINAPI input(void* arg)
{
	while (1) {
		queue.input();
	}
	return 0;
}

DWORD WINAPI treble(void* arg)
{
	while (1){
		queue.treble();
	}
	return 0;
}

DWORD WINAPI bass(void* arg)
{
	while (1){
		queue.bass();
	}
	return 0;
}

DWORD WINAPI output(void* arg)
{
	while (1){
		queue.output();
	}
	return 0;
}

void calculateCoefficients(int bassIntensity, int trebleIntensity) {
	bassCoefficients(bassIntensity, &bb0, &bb1, &bb2, &ba1, &ba2);
	trebleCoefficients(trebleIntensity, &tb0, &tb1, &tb2, &ta1, &ta2);
}

FILE* inputFile()
{
	errno_t err;


	if ((err = fopen_s(&filepoint, "you_and_i.pcm", "r")) != 0) { // open the file
		// File could not be opened. filepoint was set to NULL
		// error code is returned in err.
		// error message can be retrieved with strerror(err);
		fprintf(stderr, "cannot open file '%s': %s\n",
			"you_and_i.pcm", strerror(err));
	}
	else {
		std::cout << "open file" << std::endl;;
	}
	return filepoint;
}


int _tmain(int argc, _TCHAR* argv[]){
	FILE * filepoint = inputFile();
	//getBlock(filepoint, 50);
	_TCHAR p = 2;// *argv[0];	// number of threads
	_TCHAR b = 2;// *argv[1];	// bass intensity
	_TCHAR t = 2;// *argv[2];	// treble intensity
	//_TCHAR inputFile = *argv[3];
	//_TCHAR outputFile = *argv[4];
	
	calculateCoefficients(b, t);

	CreateThread(0, 0, input, nullptr, 0, 0);
	CreateThread(0, 0, treble, nullptr, 0, 0);
	CreateThread(0, 0, bass, nullptr, 0, 0);
	CreateThread(0, 0, output, nullptr, 0, 0);
	
	// Druk op een toets om af te breken...
	cin.get();
	return 0;
}



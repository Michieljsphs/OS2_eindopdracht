using namespace std;
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <thread>
#include <math.h>
#include <string>
#include <vector>

FILE* filepoint;
FILE* outputfilepoint;
signed short* outputBuf;
signed short* inputBuf ;

bool runningFlag = 1;

const int BUFLEN = 5; // aantal in de queue
double bb0, bb1, bb2, ba1, ba2;
double tb0, tb1, tb2, ta1, ta2;
int fileSize;
int outputOrder = 0;
int blockAmount = 0;

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
			//sample[i] = new signed short;
			sample[i] = 0;
		}
	}
	~Block() {

	}
	signed int orderNr;
	signed short sample[1024];
};



FILE* inputFile(string inputLocation)
{
	errno_t err;

	//you_and_i.pcm
	//inputLocation = "you_and_i.pcm";
	if ((err = fopen_s(&filepoint, inputLocation.c_str(), "r")) != 0) { // open the file
	// File could not be opened. filepoint was set to NULL
	// error code is returned in err.
	// error message can be retrieved with strerror(err);
		fprintf(stderr, "cannot open file '%s': %s\n",
			"you_and_i.pcm", strerror(err));
	}
	else {
		std::cout << "open file" << std::endl;;
	}
	fseek(filepoint, 0, SEEK_END);
	fileSize = ftell(filepoint); // size in bytes
	blockAmount = fileSize / 2 / 1024;
	inputBuf = (signed short *)malloc(fileSize);
	fread(inputBuf, sizeof(signed short), fileSize / 2, filepoint);

	return filepoint;
}

Block* getBlock(FILE* filepoint, int blockNr, Block *block)
{
	int size = fileSize;
	int blocks = size / 1024 / 2;			// calculate the amount of blocks needed

	// creates the block
	int beginBuf = blockNr * 1024;			// the begin index for the new block
	int endBuf = (blockNr + 1) * 1024 - 1;	// the last index for the new block

	int x = 0;
	for (int i = beginBuf; i < endBuf; i++) {
		block->sample[x] = inputBuf[i];
		x++;
	}
	//cout << "block nr " << blockNr << " beginBuf = " << beginBuf << " endBuf = " << endBuf << endl;
	return block;
}

FILE* outputFile(string outputLocation) {
	errno_t err;
	//you_and_i.pcm
	//inputLocation = "you_and_i.pcm";
	if ((err = fopen_s(&outputfilepoint, outputLocation.c_str(), "w")) != 0) { // open the file
	// File could not be opened. filepoint was set to NULL
	// error code is returned in err.
	// error message can be retrieved with strerror(err);
		fprintf(stderr, "cannot open file '%s': %s\n",
			"output.pcm", strerror(err));
	}
	else {
		std::cout << "open file" << std::endl;;
	}
	int size = fileSize;
	outputBuf = (signed short *)malloc(fileSize);
	return outputfilepoint;
}

void fillBuff(Block* block) {
	int currentOrderNr = block->orderNr;
	//cout << currentOrderNr;
	int offset = currentOrderNr * 1024;
	for (int i = 0; i < 1024; i++) {
		outputBuf[offset + i] = block->sample[i];
	}

}

void writeFile() {
	fwrite(outputBuf, sizeof(signed short), fileSize / 2, outputfilepoint); // division by 2 because 2 bytes are used per entity
}

void equalizer(signed short x[1024], char channel) {
	signed short y[1024];
	if (channel == 't') {
		for (int n = 2; n < 1024; n++) {
			// Formule die waarden toepast
			y[n] = tb0 * x[n] + tb1 * x[n - 1] + tb2 * x[n - 2] + ta1 * y[n - 1] + ta2 * y[n - 2];
		}
	}
	else if (channel == 'b') {
		for (int n = 2; n < 1024; n++) {
			// Formule die waarden toepast
			y[n] = bb0 * x[n] + bb1 * x[n - 1] + bb2 * x[n - 2] + ba1 * y[n - 1] + ba2 * y[n - 2];
		}
	}
	x = y;
}


class Queue
{
private:
	// vier buffers waarin de rij is opgeslagen
	Block inputBuffer[BUFLEN];
	Block trebleBuffer[BUFLEN];
	Block bassBuffer[BUFLEN];
	Block outputBuffer[BUFLEN];
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
		countInput = 0, countTreble = 0, countBass = 0, countOutput = 0;
		//bufferPart = BUFLEN / 4;
		inputPos = 0, treblePos = 0, bassPos = 0, outputPos = 0;

		//for (int i = 0; i < BUFLEN; i++) buffer[i] = 0;
	}

	void input() {
		EnterCriticalSection(&busy);
		while (countInput == (BUFLEN - 1)) { // waiting till there is room
			LeaveCriticalSection(&busy);
			WaitForSingleObject(canInput, INFINITE);
			EnterCriticalSection(&busy);
		}
		Block* block = new Block;
		if (orderCount < blockAmount - 1) {
			block = getBlock(filepoint, orderCount, block);

			block->orderNr = orderCount; // om de positie van het block in het geluidsfragment te onthouden
		}
		else {
			runningFlag = 0;
		}
		orderCount++;
		cout << "INPUT " << block->orderNr << endl;

		// stop block in de buffer op de plek van treblePos
		trebleBuffer[inputPos] = *block;
		delete block;
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
			cout << "TREBLE wait" << endl;
			LeaveCriticalSection(&busy);
			WaitForSingleObject(canTreble, INFINITE);
			EnterCriticalSection(&busy);
		}

		Block* block = new Block;
		*block = trebleBuffer[treblePos];

		cout << "TREBLE " << block->orderNr << endl;
		// VOER TREBLE SHIT UIT -------
		equalizer(block->sample, 't');

		bassBuffer[treblePos] = *block; // stop blok op de plek van bassPos
		treblePos = (treblePos + 1) % BUFLEN;

		delete block;
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
			cout << "BASS wait" << endl;
			LeaveCriticalSection(&busy);
			WaitForSingleObject(canBass, INFINITE);
			EnterCriticalSection(&busy);
		}
		Block* block = new Block;
		*block = bassBuffer[bassPos];
		cout << "BASS " << block->orderNr << endl;

		// voer BASS shit uit ------
		equalizer(block->sample, 'b');

		outputBuffer[bassPos] = *block;
		bassPos = (bassPos + 1) % BUFLEN;

		delete block;
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
			cout << "OUTPUT wait" << endl;
			LeaveCriticalSection(&busy);
			WaitForSingleObject(canOutput, INFINITE);
			EnterCriticalSection(&busy);
		}

		Block* block = new Block;
		*block = outputBuffer[outputPos];
		cout << "OUTPUT " << block->orderNr << endl;
		// OUTPUT SCHRIJVEN
		fillBuff(block);
		if (block->orderNr == 260) {
			//runningFlag = 0;
		}
		outputPos = (outputPos + 1) % BUFLEN;
		countBass--;

		SetEvent(canInput);	// set next step (input, because the queue is smaller
		ResetEvent(canOutput);	// reset current step
		delete block;
		LeaveCriticalSection(&busy);


		//return block;
	}
};

Queue queue;

DWORD WINAPI input(void* arg)
{
	while (runningFlag) {
		queue.input();
	}
	return 0;
}

DWORD WINAPI treble(void* arg)
{
	while (runningFlag) {
		queue.treble();
	}
	return 0;
}

DWORD WINAPI bass(void* arg)
{
	while (runningFlag) {
		queue.bass();
	}
	return 0;
}

DWORD WINAPI output(void* arg)
{
	while (runningFlag) {
		queue.output();
	}
	return 0;
}

void calculateCoefficients(int bassIntensity, int trebleIntensity) {
	bassCoefficients(bassIntensity, &bb0, &bb1, &bb2, &ba1, &ba2);
	trebleCoefficients(trebleIntensity, &tb0, &tb1, &tb2, &ta1, &ta2);
}



int _tmain(int argc, _TCHAR* argv[]) {
	int amountOfThreads = 0, lowFrequencySetting = 0, highFrequencySetting = 0;
	string inputLocation = "temp1", outputLocation = "temp2";
	int fileCounter = 0;


	for (int argcCount = 1; argcCount < argc; argcCount++) {
		std::string argvStr = argv[argcCount];
		const char* str = argv[argcCount];


		cout << argcCount << " - " << endl;
		printf("%s\n", argvStr.c_str());
		std::size_t found;
		if (argvStr.find("-p") == 0) {

			found = argvStr.find(":");
			amountOfThreads = stoi(argvStr.substr((found + 1), argvStr.size()));
		}
		else if (argvStr.find("-b") == 0) {
			found = argvStr.find(":");
			lowFrequencySetting = stoi(argvStr.substr((found + 1), argvStr.size()));
		}
		else if (argvStr.find("-t") == 0) {
			found = argvStr.find(":");
			highFrequencySetting = stoi(argvStr.substr((found + 1), argvStr.size()));
		}
		else if (argvStr.find(".") != std::string::npos) {
			if (fileCounter < 1) {
				inputLocation = argvStr;
			}
			else if (fileCounter < 2) {
				outputLocation = argvStr;
			}
			else {
				//error
			}
			fileCounter++;
		}
		else {
			cout << "Parameter" << endl;
		}
	}
	cout << amountOfThreads << " " << lowFrequencySetting << " " << highFrequencySetting << " " << inputLocation << " " << outputLocation << endl;
	inputLocation = "you_and_i.pcm";
	FILE * filepoint = inputFile(inputLocation);
	FILE* outputfilepoint = outputFile(outputLocation);

	_TCHAR threads = 10;// amountOfThreads;// *argv[0];	// number of threads
	_TCHAR basslv = lowFrequencySetting;// *argv[1];	// bass intensity
	_TCHAR treblelv = highFrequencySetting;// *argv[2];	// treble intensity
	//_TCHAR inputFile = *argv[3];
	//_TCHAR outputFile = *argv[4];
	calculateCoefficients(basslv, treblelv);
	//blockAmount = fileSize / 1024;
	for (int i = 0; i < threads; i++) {
		CreateThread(0, 0, input, nullptr, 0, 0);
		CreateThread(0, 0, treble, nullptr, 0, 0);
		CreateThread(0, 0, bass, nullptr, 0, 0);
		CreateThread(0, 0, output, nullptr, 0, 0);
	}
	cin.get();
	writeFile();
	return 0;
}
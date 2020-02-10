#define holdingSize 64
#define automaticTicking true

#include "Arduino.h"

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "esp_system.h"

class timingManager
{
public:
	enum RunType {
		MILISEC, CYCLEJOB
	};

	enum Core {
		CORE0 = 0, CORE1 = 1
	};

private:
	bool outputWork = false;
	bool coreReady[2] = { false, false };

	struct functionData
	{
		unsigned int goal;
		void (*functionReference)(void*);
		void* addressOfData;
		RunType typeOfCount;
		unsigned int lastTimeRan = 0;

		unsigned int runTimes = 0; //Number of times the task will run. 0 means it will run forever.
		unsigned int timesRan = 0; //This is the number of times the task has run so far.
	};

	struct FunctionNode {
		functionData data;
		FunctionNode* next;
		FunctionNode* prev;
	};

	int counter[2] = { 0, 0 };
	FunctionNode* linkedListCoreHead[2];

	static void performWork(timingManager* tmObj, Core core);
	static void secondCoreLoop(void* _tmObj);
	static void primaryCoreLoop(void* _tmObj);
	static bool isJobFinished(functionData* currJob);

public:
	timingManager(bool _outputWork);
	~timingManager();
	void addFunction(RunType type, unsigned int activator, void (*referencToFunction)(void*), void* _addressOfData, int offset = 0, Core kerne = CORE1, unsigned int runCount = 0);
	void tick(Core coreToTick);
	void cycle();
	void startHandlingPrimaryCore(bool killArduinoTask = false);
	void printChain();
};
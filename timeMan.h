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
	//TaskHandle_t opgaveHandle;

	bool secondaryCoreRunning = true; //A way of shutting down the second core when the timingManager destructor is called.

	struct functionData
	{
		int goal;
		void (*functionReference)(void*);
		void* addressOfData;
		RunType typeOfCount;
		int lastTimeRan = 0;

		int runTimes = -1; //Number of times the task will run. -1 means it will run forever.
		int timesRan = 0; //This is the number of times the task has run so far.
		int enabled = true;
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
	[[deprecated]]
	static void updateTimesRan(functionData* currJob, bool _outputWork);
	static bool isJobFinished(functionData* currJob);

public:
	timingManager(bool _outputWork);
	~timingManager();
	void addFunction(RunType type, int activator, void (*referencToFunction)(void*), void* _addressOfData, int offset = 0, Core kerne = CORE1, int runCount = -1);
	void tick(Core coreToTick);
	void cycle();
	void startHandlingPrimaryCore(bool killArduinoTask = false);
	void printChain();
};
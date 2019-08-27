#define holdingSize 64
#define automaticTicking true

#include "Arduino.h"

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "esp_system.h"

class timingManager
{
public:
	enum runType {
		milisec, cycleJob
	};

	enum core {
		core0 = 0, core1 = 1
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
		runType typeOfCount;
		int lastTimeRan = 0;

		int runTimes = -1; //Number of times the task will run. -1 means it will run forever.
		int timesRan = 0; //This is the number of times the task has run so far.
		int enabled = true;
	};

	int counter[2] = { 0, 0 };
	functionData holding[2][holdingSize];
	int currentIndex[2] = { 0, 0 };

	static void performWork(timingManager* tmObj, core kerne);
	static void secondCoreLoop(void* _tmObj);
	static void primaryCoreLoop(void* _tmObj);
	static void updateTimesRan(functionData* currJob, bool _outputWork);

public:
	timingManager(bool _outputWork);
	~timingManager();
	bool addFunction(runType type, int activator, void (*referencToFunction)(void*), void* _addressOfData, int offset = 0, core kerne = core1, int runCount = -1);
	void tick(core coreToTick);
	void cycle();
	void startHandlingPrimaryCore(bool killArduinoTask = false);
};
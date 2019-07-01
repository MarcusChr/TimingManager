#define holdingSize 64

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
    bool secondaryCoreReady = false;
    TaskHandle_t opgaveHandle;
	
	bool secondaryCoreRunning = true; //Standard til True!
    
    struct functionData
    {
      int goal;
      void (*functionReference)(void*);
      void *addressOfData;
      runType typeOfCount;
      int lastTimeRan = 0;

      int runTimes = -1; //Antal gange den skal køre
      int timesRan = 0; //Antal gange den har kørt
      int enabled = true;
    };
	
	int counter = 0;
    functionData holding[2][holdingSize];
    int currentIndex[2] = {0, 0};
	
	static void performWork(timingManager *tmObj, core kerne);
	static void secondCoreLoop(void * _tmObj);
	static void updateTimesRan(functionData* currJob, bool _outputWork);
	
	public:
	timingManager(bool _outputWork);
	~timingManager();
	void addFunction(runType type, int activator, void (*referencToFunction)(void*), void *_addressOfData, int offset = 0, core kerne = core1, int runCount = -1);
	void cycle();
};
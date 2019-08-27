#include "timeMan.h"

timingManager::timingManager(bool _outputWork = false)
{
	outputWork = _outputWork;
	memset((byte*)holding, 0x00, sizeof(holding));
}

timingManager::~timingManager()
{
	secondaryCoreRunning = false;
}

void timingManager::updateTimesRan(functionData* currJob, bool _outputWork)
{
	if (currJob->runTimes != -1) {
		if (currJob->runTimes <= ++currJob->timesRan)
		{
			if (_outputWork) Serial.println("0x" + String((long)& currJob) + " - task is now disabled..");
			currJob->enabled = false;
		}
	}
}

void timingManager::performWork(timingManager * tmObj, core kerne)
{
#if  automaticTicking
	++tmObj->counter[kerne];
#endif //  automaticTicking



	functionData* jobQueue = tmObj->holding[kerne];
	for (int i = 0; i < holdingSize; i++)
	{
		if (tmObj->holding[kerne][i].functionReference == nullptr) break;

		functionData * currJob = &jobQueue[i];

		if (currJob->enabled) {
			if (currJob->typeOfCount == cycleJob) {
				if (tmObj->counter[kerne] % currJob->goal == 0) {
					currJob->functionReference(currJob->addressOfData);
					updateTimesRan(currJob, tmObj->outputWork);
				}
			}
			else if (currJob->typeOfCount == milisec) {
				if (millis() >= (currJob->lastTimeRan + currJob->goal)) {
					currJob->lastTimeRan = millis(); //Using the newest time (BEFORE) the function has been executed. [Add a way to choose between before and after the execution?]
					if (tmObj->outputWork) Serial.println("\nPerformed 0x" + String((long)& currJob->functionReference) + " on core" + xPortGetCoreID());
					currJob->functionReference(currJob->addressOfData);
					updateTimesRan(currJob, tmObj->outputWork);
				}
			}
		}
	}
}

void timingManager::secondCoreLoop(void* _tmObj)
{
	timingManager* tmObj = (timingManager*)_tmObj;
	while (tmObj->coreReady[core0])//(tmObj->secondaryCoreRunning)
	{
		performWork(tmObj, core0);

		/////////////////////////////////////////////
		/*
		  Problem: Task watchdog got triggered. The following tasks did not reset the watchdog in time: [...]
		  Solution: https://github.com/espressif/esp-idf/issues/1646#issuecomment-413829778
		*/
		TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
		TIMERG0.wdt_feed = 1;
		TIMERG0.wdt_wprotect = 0;
		/////////////////////////////////////////////
	}
	vTaskDelete(NULL);
}

void timingManager::primaryCoreLoop(void* _tmObj) {
	timingManager* tmObj = (timingManager*)_tmObj;
	while (tmObj->coreReady[core1]) {
		performWork(tmObj, core1);
	}
	vTaskDelete(NULL);
}

void timingManager::startHandlingPrimaryCore(bool killArduinoTask) { //This will add a job 

	if (!coreReady[core1]) {
		coreReady[core1] = true;
		if (outputWork) Serial.println("Added task to primary core!");
		xTaskCreatePinnedToCore(primaryCoreLoop, "coreOneTask", 10000, this, 1, NULL, core1); //Description: Function to run - name - size of stack - parameter (Reference to the task holding) - priority of this coretask - reference to the taskHandle - the core.
		if (killArduinoTask) {
			vTaskDelete(NULL);
		}
	}
}


bool timingManager::addFunction(runType type, int activator, void (*referencToFunction)(void*), void* _addressOfData, int offset, core kerne, int runCount)
{
	functionData newJob;
	newJob.goal = activator;
	newJob.addressOfData = _addressOfData;
	newJob.functionReference = referencToFunction;
	newJob.typeOfCount = type;
	newJob.lastTimeRan = millis() + offset;
	newJob.runTimes = runCount;

	if (currentIndex[kerne] < holdingSize) {
		holding[kerne][currentIndex[kerne]++] = newJob; //Using the core's value to decide which core-holding to add the task to.
		if (kerne == core0 && !coreReady[core0]) {
			coreReady[core0] = true;
			xTaskCreatePinnedToCore(secondCoreLoop, "coreZeroTask", 10000, this, 0, NULL, core0); //Description: Function to run - name - size of stack - parameter (Reference to the task holding) - priority of this coretask - reference to the taskHandle - the core.
			if (outputWork) Serial.println("Added task to core0");
			return true;
		}
	}
	else {
		for (int i = 0; i < holdingSize; i++) {
			functionData* currJob = &holding[kerne][i];
			if (!currJob->enabled) {
				*currJob = newJob;
				return true;
			}
		}
		return false;
	}
}

void timingManager::tick(core coreToTick) {
	++counter[coreToTick];
}

void timingManager::cycle()
{
	performWork(this, core1);
}


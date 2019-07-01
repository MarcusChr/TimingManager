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
	functionData* jobQueue = tmObj->holding[kerne];
	for (int i = 0; i < holdingSize; i++)
	{
		if (tmObj->holding[kerne][i].functionReference == nullptr) break;

		functionData * currJob = &jobQueue[i];

		if (currJob->enabled) {
			if (currJob->typeOfCount == cycleJob) {
				if (tmObj->counter % currJob->goal == 0) {
					currJob->functionReference(currJob->addressOfData);
					updateTimesRan(currJob, tmObj->outputWork);
				}
			}
			else if (currJob->typeOfCount == milisec) {
				if (millis() >= (currJob->lastTimeRan + currJob->goal)) {
					currJob->lastTimeRan = millis(); //Man skal have den nyeste time, derfor genbruges tiden ikke fra tidligere senere.
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
	while (tmObj->secondaryCoreRunning)
	{
		performWork(tmObj, core0);

		/////////////////////////////////////////////
		/*
		  Problem: Task watchdog got triggered. The following tasks did not reset the watchdog in time: [...]
		  Løsning: https://github.com/espressif/esp-idf/issues/1646#issuecomment-413829778
		*/
		TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
		TIMERG0.wdt_feed = 1;
		TIMERG0.wdt_wprotect = 0;
		/////////////////////////////////////////////
	}
}


void timingManager::addFunction(runType type, int activator, void (*referencToFunction)(void*), void* _addressOfData, int offset, core kerne, int runCount)
{
	functionData newJob;
	newJob.goal = activator;
	newJob.addressOfData = _addressOfData;
	newJob.functionReference = referencToFunction;
	newJob.typeOfCount = type;
	newJob.lastTimeRan = millis() + offset;
	newJob.runTimes = runCount;

	holding[kerne][currentIndex[kerne]++] = newJob; //En enum er en integer "i forklædning", og den kan derfor også bruges som en integer (core0 = 0, core = 1)

	if (kerne == core0 && !secondaryCoreReady) {
		xTaskCreatePinnedToCore(secondCoreLoop, "coreZeroTask", 10000, this, 0, &opgaveHandle, 0); //Beskrivelse: Funktionen som skal køre - navn - størrelse af stakken - parameter (mit tilfælde en reference til min opgavekø) - prioritet - reference til opgaveHandle - kernen
		secondaryCoreReady = true;
		Serial.println("Added task to core0");
	}
}

void timingManager::cycle()
{
	++counter; //SPEEEEEED! ++cycle > cycle++ i hastighed
	performWork(this, core1);
}

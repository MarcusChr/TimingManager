#include "timeMan.h"

timingManager::timingManager(bool _outputWork = false)
{
	outputWork = _outputWork;
	//memset(holding, 0x00, sizeof(holding));
	memset(linkedListCoreHead, 0x00, sizeof(linkedListCoreHead));
}

timingManager::~timingManager()
{
	secondaryCoreRunning = false;
}

[[deprecated]]
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


bool timingManager::isJobFinished(functionData* currJob)
{
	bool finished = false;

	if (currJob->runTimes != -1) {
		if (currJob->runTimes <= currJob->timesRan)
		{
			finished = true;
		}
	}

	return finished;
}

void timingManager::performWork(timingManager* tmObj, core kerne)
{
#if  automaticTicking
	++tmObj->counter[kerne];
#endif //  automaticTicking

	FunctionNode* currentNode = tmObj->linkedListCoreHead[kerne];

	while (currentNode != nullptr && currentNode->data.functionReference != nullptr)
	{
		functionData* currJob = &currentNode->data;

		if (currJob->typeOfCount == cycleJob) {
			if (tmObj->counter[kerne] % currJob->goal == 0) {
				currJob->functionReference(currJob->addressOfData);
				++currJob->timesRan;
			}
		}
		else if (currJob->typeOfCount == milisec) {
			if (millis() >= (currJob->lastTimeRan + currJob->goal)) {
				currJob->lastTimeRan = millis(); //Using the newest time (BEFORE) the function has been executed. [Add a way to choose between before and after the execution?]
				if (tmObj->outputWork) Serial.println("\nPerformed 0x" + String((unsigned long)currentNode) + " (Function reference:0x" + String((long)&currJob->functionReference) + ") on core" + xPortGetCoreID());
				currJob->functionReference(currJob->addressOfData);
				++currJob->timesRan;
			}
		}

		if (isJobFinished(currJob)) {
			FunctionNode* nextNode = currentNode->next;

			//nextNode = currentNode->next;

			if(currentNode->prev != nullptr)
			{
				currentNode->prev->next = nextNode;

			} else {
				tmObj->linkedListCoreHead[kerne] = currentNode->next;
			}

			if (nextNode != nullptr) 
			{
				nextNode->prev = currentNode->prev;
			}
			
			if (tmObj->outputWork) Serial.println("0x" + String((unsigned long)currentNode) + " (Function reference:0x" + String((long)&currJob->functionReference) + ") - task is now disabled..");
			free(currentNode);

			if (nextNode != nullptr)
			{
				currentNode = nextNode;
			}
			else {
				currentNode = nullptr;
			}
		}
		else {
			currentNode = currentNode->next;
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

void timingManager::printChain()
{
	String startOfChainMessage = "#################Start of chain#################";
	Serial.println(startOfChainMessage);
	int totalMemPerNode = sizeof(functionData) + sizeof(FunctionNode);
	int freeHeapMem = xPortGetFreeHeapSize();

	Serial.println("Size of node: " + String(sizeof(FunctionNode)) + " bytes");
	Serial.println("Size of task: " + String(sizeof(functionData)) + " bytes");
	Serial.println("Total:        " + String(totalMemPerNode) + " bytes");
	Serial.println(String(freeHeapMem) + " bytes free");
	Serial.println("Theoretical limit: " + String(floor(freeHeapMem / totalMemPerNode)));

	for (int kerne = 0; kerne < 2; kerne++)
	{
		Serial.println("CORE " + String(kerne));
		FunctionNode* currentNode = linkedListCoreHead[kerne];
		while (currentNode != nullptr) {
			Serial.println("[prev] 0x" + String((unsigned long)currentNode->prev) + " -> [node] 0x" + String((unsigned long)currentNode) + " -> [next] 0x" + String((unsigned long)currentNode->next));
			currentNode = currentNode->next;
		}
	}
	Serial.print("\n");
	for (int i = 0; i < startOfChainMessage.length(); i++) Serial.print("#");
	Serial.print("\n");
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

	if (outputWork) Serial.println("Adding task to core" + String(kerne));
	FunctionNode* previousNode = linkedListCoreHead[kerne];

	FunctionNode* functionNodeToAdd = new FunctionNode();
	Serial.println("Created new object in HEAP memory.");
	functionNodeToAdd->data = newJob;

	Serial.print("\n");
	if (previousNode == nullptr) {
		if (outputWork) Serial.print("Set the head of the LinkedList");
	}
	else {
		functionNodeToAdd->next = linkedListCoreHead[kerne];
		linkedListCoreHead[kerne]->prev = functionNodeToAdd;

		Serial.println(String((unsigned long)functionNodeToAdd->prev) + ":|" + String((unsigned long)functionNodeToAdd) + "|:" + String((unsigned long)functionNodeToAdd->next));

		//linkedListCoreHead[kerne] = functionNodeToAdd;
		if (outputWork) Serial.print("Changed the head of the LinkedList");
	}
	Serial.println("\nCORE" + String(kerne) +" -> " + String((unsigned long)functionNodeToAdd->prev) + ":|" + String((unsigned long)functionNodeToAdd) + "|:" + String((unsigned long)functionNodeToAdd->next));
	linkedListCoreHead[kerne] = functionNodeToAdd;

	Serial.println("[" + String(kerne) + "]");

	if (kerne == core0 && !coreReady[core0]) {
		coreReady[core0] = true;
		xTaskCreatePinnedToCore(secondCoreLoop, "coreZeroTask", 10000, this, 0, NULL, core0); //Description: Function to run - name - size of stack - parameter (Reference to the task holding) - priority of this coretask - reference to the taskHandle - the core.
		if (outputWork) Serial.println("Added task to core0");
	}
}

void timingManager::tick(core coreToTick) {
	++counter[coreToTick];
}

void timingManager::cycle()
{
	performWork(this, core1);
}


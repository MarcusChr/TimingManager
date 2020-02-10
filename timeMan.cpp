#include "timeMan.h"

timingManager::timingManager(bool _outputWork = false)
{
	outputWork = _outputWork;
	memset(linkedListCoreHead, 0x00, sizeof(linkedListCoreHead));
}

timingManager::~timingManager()
{
	secondaryCoreRunning = false;
}

bool timingManager::isJobFinished(functionData* currJob)
{
	bool finished = false;

	if (currJob->runTimes != 0) {
		if (currJob->runTimes <= currJob->timesRan)
		{
			finished = true;
		}
	}

	return finished;
}

void timingManager::performWork(timingManager* tmObj, Core core)
{
#if  automaticTicking
	++tmObj->counter[core];
#endif //  automaticTicking

	FunctionNode* currentNode = tmObj->linkedListCoreHead[core];

	while (currentNode != nullptr && currentNode->data.functionReference != nullptr)
	{
		functionData* currJob = &currentNode->data;

		if (currJob->typeOfCount == CYCLEJOB) {
			if (tmObj->counter[core] % currJob->goal == 0) {
				currJob->functionReference(currJob->addressOfData);
				++currJob->timesRan;
			}
		}
		else if (currJob->typeOfCount == MILISEC) {
			if (millis() >= (currJob->lastTimeRan + currJob->goal)) {
				currJob->lastTimeRan = millis(); //Using the newest time (BEFORE) the function has been executed. [Add a way to choose between before and after the execution?]
				if (tmObj->outputWork) Serial.println("\nPerformed 0x" + String((unsigned long)currentNode) + " (Function reference:0x" + String((long)&currJob->functionReference) + ") on core" + xPortGetCoreID());
				currJob->functionReference(currJob->addressOfData);
				++currJob->timesRan;
			}
		}

		if (isJobFinished(currJob)) {
			FunctionNode* nextNode = currentNode->next;

			if (currentNode->prev != nullptr)
			{
				currentNode->prev->next = nextNode;

			}
			else {
				tmObj->linkedListCoreHead[core] = nextNode;//currentNode->next;
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
	while (tmObj->coreReady[CORE0])//(tmObj->secondaryCoreRunning)
	{
		performWork(tmObj, CORE0);

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
	while (tmObj->coreReady[CORE1]) {
		performWork(tmObj, CORE1);
	}
	vTaskDelete(NULL);
}

void timingManager::startHandlingPrimaryCore(bool killArduinoTask) { //This will add a job 

	if (!coreReady[CORE1]) {
		coreReady[CORE1] = true;
		if (outputWork) Serial.println("Added task to primary core!");
		xTaskCreatePinnedToCore(primaryCoreLoop, "coreOneTask", 10000, this, 1, NULL, CORE1); //Description: Function to run - name - size of stack - parameter (Reference to the task holding) - priority of this coretask - reference to the taskHandle - the core.
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
	Serial.println("Size of this: " + String(sizeof(*this)) + " bytes");

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

void timingManager::addFunction(RunType type, unsigned int activator, void (*referencToFunction)(void*), void* _addressOfData, int offset, Core kerne, unsigned int runCount)
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
	if (outputWork) Serial.println("Created new object in HEAP memory.");
	functionNodeToAdd->data = newJob;

	if (outputWork) Serial.print("\n");
	if (previousNode == nullptr) {
		if (outputWork) Serial.print("Set the head of the LinkedList");
	}
	else {
		functionNodeToAdd->next = linkedListCoreHead[kerne];
		linkedListCoreHead[kerne]->prev = functionNodeToAdd;

		if (outputWork) Serial.println(String((unsigned long)functionNodeToAdd->prev) + ":|" + String((unsigned long)functionNodeToAdd) + "|:" + String((unsigned long)functionNodeToAdd->next));
		if (outputWork) Serial.print("Changed the head of the LinkedList");
	}
	if (outputWork) Serial.println("\nCORE" + String(kerne) + " -> " + String((unsigned long)functionNodeToAdd->prev) + ":|" + String((unsigned long)functionNodeToAdd) + "|:" + String((unsigned long)functionNodeToAdd->next));
	linkedListCoreHead[kerne] = functionNodeToAdd;

	if (outputWork) Serial.println("[" + String(kerne) + "]");

	if (kerne == CORE0 && !coreReady[CORE0]) {
		coreReady[CORE0] = true;
		xTaskCreatePinnedToCore(secondCoreLoop, "coreZeroTask", 10000, this, 0, NULL, CORE0); //Description: Function to run - name - size of stack - parameter (Reference to the task holding) - priority of this coretask - reference to the taskHandle - the core.
		if (outputWork) Serial.println("Added task to CORE0");
	}
}

void timingManager::tick(Core coreToTick) {
	++counter[coreToTick];
}

void timingManager::cycle()
{
	performWork(this, CORE1);
}
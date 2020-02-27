//This example will show how to use both cores.
//Hint: Enable 'show timestamp' in the Serial Monitor instead of counting yourself. :)

#include <timeMan.h>

TimingManager* timeMan = TimingManager::getInstance();

void greetTheWorld(void * payload) //
{
  Serial.println("Hello, world!");
}

void farewellTheWorld(void * payload)
{
  Serial.println("See you later, world!");
}

void setup() {
  Serial.begin(500000);
  timeMan->setOutputWork(false);
  timeMan->addFunction(TimingManager::MILISEC, 3000, greetTheWorld, nullptr, 0, TimingManager::CORE0); //The type of job (milisec or cycleJob) | how many miliseconds between the runs | a pointer to the payload. In this case there isn't any | the off-set | the core to run on. core0 is the secondary core.
  timeMan->addFunction(TimingManager::MILISEC, 5000, farewellTheWorld, nullptr, 2500, TimingManager::CORE1); //Notice that the off-set is set to 2500 ms. It's set to 2500 ms. so they won't both write via Serial at the same time.
  
  //timeMan->startHandlingPrimaryCore(true); //Uncommenting this, will kill the Arduino task and make CORE1 focus on running its tasks.
}

void loop() {
  timeMan->cycle(); //This will cycle through the tasks on the primary core (The setup() and loop() runs on core1)
  delay(60000); //This delay will stop the primary core for 1 min / 60 sec / 60000 milliseconds. Notice how the 'farewellTheWorld only runs once a minute?'
}

//This example will show how to schedule a (simple) function to run every 5th second.
//Hint: Enable 'show timestamp' in the Serial Monitor instead of counting yourself. :)

#include <timeMan.h>

timingManager timeMan(false);

void greetTheWorld(void * payload) //
{
  Serial.println("Hello, world!");
}

void setup() {
  Serial.begin(9600);
  timeMan.addFunction(timingManager::milisec, 5000, greetTheWorld, nullptr); //The type of job (milisec or cycleJob) | how many miliseconds between the runs | a pointer to the payload. In this case there isn't any
}

void loop() {
  timeMan.cycle();
}

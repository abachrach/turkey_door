#include <Time.h> //depends on the time library: http://www.pjrc.com/teensy/td_libs_Time.html

// set pin mapping
#define PWM_A_PIN    3
#define DIR_A_PIN    12
#define BRAKE_A_PIN  9
#define SNS_A_PIN    A0

#define TOP_SWITCH_PIN       7
#define BOTTOM_SWITCH_PIN    4

namespace turkey_door {

class TurkeyDoorFsm {
public:
  TurkeyDoorFsm() :
      door_should_be_open_(false),
      top_switch_closed_(false),
      bottom_switch_closed_(false)
  {
    open_time_minutes_ = TimeToMinutes(7, 30); // 7:30am
    close_time_minutes_ = TimeToMinutes(20, 45); //8:45pm
  }
  void Initialize()
  {
    Serial.begin(9600);
    Serial.println("Initializing TurkeyDoorFsm\n");

    pinMode(BRAKE_A_PIN, OUTPUT);
    pinMode(DIR_A_PIN, OUTPUT);

    SetTime();

    Serial.println("Initialization done!");
  }

  void Step()
  {
    PrintTime(hour(), minute(), second());
    Serial.print(" -- Door should be: ");
    if (door_should_be_open_ && top_switch_closed_) {
      Serial.print("open!");
    }
    else if (door_should_be_open_ && !top_switch_closed_) {
      Serial.print("opening...");
    }
    else if (!door_should_be_open_ && !bottom_switch_closed_) {
      Serial.print("closing...");
    }
    else if (!door_should_be_open_ && bottom_switch_closed_) {
      Serial.print("closed!");
    }
    else{
      // this shouldn't happen
      Serial.print("broken :-/");
    }
    Serial.println();

    ReadInputs();
    UpdateDesiredState();
    SetOutputs();

    delay(1); // sleep for 1millisec
  }

private:
  void ReadInputs()
  {
    //TODO(abachrac): figure out whether this is pull up or pull down
    top_switch_closed_ = digitalRead(TOP_SWITCH_PIN);
    bottom_switch_closed_ = digitalRead(BOTTOM_SWITCH_PIN);
    if (bottom_switch_closed_ && top_switch_closed_) {
      Serial.println("ERROR: both switches are reporting closed... This shoulnd't happen!");
    }
  }
  void UpdateDesiredState()
  {
    int time_of_day_minutes = TimeToMinutes(hour(), minute());
    if (time_of_day_minutes >= open_time_minutes_ && time_of_day_minutes < close_time_minutes_) {
      door_should_be_open_ = true;
    }
    else {
      door_should_be_open_ = false;
    }
  }

  void SetOutputs()
  {
    //TODO(abachrac): check directions
    if (door_should_be_open_ && !top_switch_closed_) {
      digitalWrite(BRAKE_A_PIN, LOW);  // Turn off motor braking
      digitalWrite(DIR_A_PIN, HIGH);  // Make the motor will spin forward
      analogWrite(PWM_A_PIN, 255);  // Set the speed of the motor, 255 its the maximum value
    }
    else if (!door_should_be_open_ && !bottom_switch_closed_) {
      digitalWrite(BRAKE_A_PIN, LOW);  // Turn off motor braking
      digitalWrite(DIR_A_PIN, LOW);  // Make the motor will spin backwards
      analogWrite(PWM_A_PIN, 255);  // Set the speed of the motor, 255 its the maximum value
    }
    else {
      analogWrite(PWM_A_PIN, 0);     // Turn off the motor
      digitalWrite(BRAKE_A_PIN, HIGH);     // Enable Brake;
    }
  }

  void PrintTime(int hours, int minutes, int seconds)
  {
    PrintWithLeadingZeros(hours);
    Serial.print(":");
    PrintWithLeadingZeros(minutes);
    Serial.print(":");
    PrintWithLeadingZeros(seconds);
  }

  void PrintWithLeadingZeros(int digits)
  {
    if (digits < 10)
      Serial.print('0');
    Serial.print(digits);
  }

  int TimeToMinutes(int hours, int minutes)
  {
    return hours * 60 + minutes;
  }

  void SetTime()
  {
    bool got_time = false;
    while (!got_time) {
      Serial.flush();
      Serial.println("Please type in the time followed by a semi-colon eg: 20:03;");
      char buf[256] = { };

      Serial.setTimeout(60000); // timeout after 1 minute
      int hours = 06, minutes = 30, seconds = 00;
      if (Serial.readBytesUntil(';', buf, 256) <= 0) {
        Serial.println("WARNING: Timed out waiting to read a timestamp!");
        Serial.print("We're gonna assume that it is ");
        PrintTime(hours, minutes, seconds);
        Serial.println(", and the solar panel just powered us back up :-p");
      }
      else {
        int ret = sscanf(buf, "%d:%d:%d", &hours, &minutes, &seconds);
        if (ret < 2) {
          Serial.println("Ooops:");
          Serial.println(buf);
          Serial.print("could not be interpreted as a time... let't try again");
          continue;
        }

        Serial.print("Got: ");
        PrintTime(hours, minutes, seconds);
        Serial.println();
        if (hours < 0 || hours > 24 || minutes < 0 || minutes > 60 || seconds < 0 || seconds > 60) {
          Serial.print("Time was invalid... let't try again");
          continue;
        }
      }
      // pass in a dummy date cuz we only care about time
      int days = 1;
      int months = 1;
      int years = 2013;
      setTime(hours, minutes, seconds, days, months, years);
      got_time = true;
    }
  }

  // input times
  int open_time_minutes_; // time of day to open the door (in minutes)
  int close_time_minutes_; // time of day to close the door (in minutes)


  //state of the system
  bool door_should_be_open_;
  bool top_switch_closed_;
  bool bottom_switch_closed_;
};

} // namespace turkey_door

// basic arduino calls:
static turkey_door::TurkeyDoorFsm turkey_door_fsm;
void setup()
{
  turkey_door_fsm.Initialize();
}
void loop()
{
  turkey_door_fsm.Step();
}


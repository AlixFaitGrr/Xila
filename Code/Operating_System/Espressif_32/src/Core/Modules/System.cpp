#include "Core/Core.hpp"

#include "soc/rtc_wdt.h"

// -- ESP 32 update
#include "Update.h"

extern Software_Handle_Class Shell_Handle;

Xila_Class::System_Class::System_Class()
{
  State = Default;
  Task_Handle = NULL;
  strlcpy(Device_Name, Default_Device_Name, sizeof(Device_Name));
}

Xila_Class::System_Class::~System_Class()
{
}

Xila_Class::Event Xila_Class::System_Class::Load_Registry()
{
  File Temporary_File = Xila.Drive.Open((Registry("System")));
  DynamicJsonDocument System_Registry(512);
  if (deserializeJson(System_Registry, Temporary_File)) // error while deserialising
  {
    Temporary_File.close();
    return Error;
  }
  if (strcmp("System", System_Registry["Registry"] | "") != 0)
  {
    Temporary_File.close();
    return Error;
  }
  JsonObject Version = System_Registry["Version"];
  if (Version["Major"] != Version_Major || Version["Minor"] != Version_Minor || Version["Revision"] != Version_Revision)
  {
    Panic_Handler(Installation_Conflict);
  }
 strlcpy(Xila.System.Device_Name, System_Registry["Device Name"] | Default_Device_Name, sizeof(Xila.System.Device_Name));
  Xila.WiFi.setHostname(Xila.System.Device_Name); // Set hostname
  Temporary_File.close();
  return Success;
}

Xila_Class::Event Xila_Class::System_Class::Save_Registry()
{
  File Temporary_File = Xila.Drive.Open((Registry("System")), FILE_WRITE);
  DynamicJsonDocument System_Registry(256);
  System_Registry["Registry"] = "System";
  System_Registry["Device Name"] = Device_Name;
  JsonObject Version = System_Registry.createNestedObject("Version");
  Version["Major"] = Version_Major;
  Version["Minor"] = Version_Minor;
  Version["Revision"] = Version_Revision;
  if (serializeJson(System_Registry, Temporary_File) == 0)
  {
    Temporary_File.close();
    return Error;
  }
  Temporary_File.close();
  return Success;
}

inline uint32_t Xila_Class::System_Class::Get_Free_Heap()
{
  return ESP.getFreeHeap();
}

///
/// @brief
///
void Xila_Class::System_Class::Task(void *)
{
  uint32_t Last_Header_Refresh = 0;
  Xila.Power.Button_Counter = 0;
  while (1)
  {
    Xila.Software.Check_Watchdog(); // check if current running software is not frozen
    Xila.Power.Check_Button();
    Xila.Display.Loop();

    if ((Xila.Time.Milliseconds() - Last_Header_Refresh) > 5000) //
    {

      if (Xila.System.Get_Free_Heap() < Low_Memory_Threshold) // Check memory
      {
        Xila.System.Panic_Handler(Low_Memory);
      }

      if (Xila.Software.Openned[0] == NULL)
      {
        Xila.Task.Delay(100);
        if (Xila.Software.Openned[0] == NULL)
        {
          Xila.Software.Open(Shell_Handle);
          Xila.Software.Send_Instruction_Shell(Software_Class::Desk);
          Xila.Software.Maximize_Shell();
        }
      }

      Xila.Time.Synchronise(); // Time synchro
      Xila.System.Refresh_Header();

      Last_Header_Refresh = Xila.Time.Milliseconds();
    }

    Xila.Task.Delay(20);
  }
}

///
/// @brief Update Xila on the MCU
///
/// @param Update_File
/// @return Xila_Class::Event
Xila_Class::Event Xila_Class::System_Class::Load_Executable(File Executable_File)
{
  if (!Executable_File || Executable_File.isDirectory())
  {
    return Xila.Error;
  }
  if (Executable_File.isDirectory())
  {
    return Xila.Error;
  }
  if (Executable_File.size() == 0)
  {
    return Xila.Error;
  }
  if (!Update.begin(Executable_File.size(), U_FLASH))
  {
    return Xila.Error;
  }
  size_t Written = Update.writeStream(Executable_File);
  if (Written != Executable_File.size())
  {
    return Xila.Error;
  }
  if (!Update.end())
  {
    return Xila.Error;
  }
  if (!Update.isFinished())
  {
    return Xila.Error;
  }

  return Xila.Success;
}

/**
 * @enum Events
 * @brief All kinds of events returned by Core API.
 */
void Xila_Class::System_Class::Panic_Handler(Panic_Code Panic_Code)
{
  vTaskSuspendAll();
  Xila.Display.Set_Current_Page(F("Core_Panic"));
  char Temporary_String[23];
  printf(Temporary_String, "Error code %X", Panic_Code);

  Xila.Display.Set_Text(F("ERRORCODE_TXT"), Temporary_String);
  Xila.Task.Delay(10000);
  abort();
}

Xila_Class::Event Xila_Class::System_Class::Save_Dump()
{
  Xila.Software.Minimize(*Xila.Software.Openned[0]->Handle);

  File Dump_File = Xila.Drive.Open(Dump_Registry_Path, FILE_WRITE);

  if (!Dump_File)
  {
    return Error;
  }

  Dump_File.seek(0 * 24);
  Dump_File.write((uint8_t *)Xila.Software.Openned[2]->Handle->Name, sizeof(Xila.Software.Openned[2]->Handle->Name));
  Dump_File.seek(1 * 24);
  Dump_File.write((uint8_t *)Xila.Software.Openned[3]->Handle->Name, sizeof(Xila.Software.Openned[3]->Handle->Name));
  Dump_File.seek(2 * 24);
  Dump_File.write((uint8_t *)Xila.Software.Openned[4]->Handle->Name, sizeof(Xila.Software.Openned[4]->Handle->Name));
  Dump_File.seek(3 * 24);
  Dump_File.write((uint8_t *)Xila.Software.Openned[5]->Handle->Name, sizeof(Xila.Software.Openned[5]->Handle->Name));
  Dump_File.seek(4 * 24);
  Dump_File.write((uint8_t *)Xila.Software.Openned[6]->Handle->Name, sizeof(Xila.Software.Openned[6]->Handle->Name));
  Dump_File.seek(5 * 24);
  Dump_File.write((uint8_t *)Xila.Software.Openned[7]->Handle->Name, sizeof(Xila.Software.Openned[7]->Handle->Name));
  Dump_File.seek(6 * 24);
  Dump_File.write((uint8_t *)Xila.Account.Current_Username, sizeof(Xila.Account.Current_Username));
  Dump_File.close();

  return Success;
}

Xila_Class::Event Xila_Class::System_Class::Load_Dump()
{
  if (Xila.Drive.Exists(Dump_Registry_Path))
  {
    File Dump_File = Xila.Drive.Open((Dump_Registry_Path));
    char Temporary_Software_Name[24];
    for (uint8_t i = 0; i < 7; i++)
    {
      Dump_File.seek(i * 24);
      Dump_File.readBytes(Temporary_Software_Name, sizeof(Temporary_Software_Name));
      for (uint8_t ii = 0; ii < (sizeof(Xila.Software.Handle) / sizeof(Xila.Software.Handle[0])); ii++)
      {
        if (strcmp(Xila.Software.Handle[ii]->Name, Temporary_Software_Name) == 0)
        {
          Xila.Software.Open(*Xila.Software.Handle[ii]);
        }
      }
    }

    Dump_File.seek(6 * 24);
    Dump_File.readBytes(Xila.Account.Current_Username, sizeof(Xila.Account.Current_Username));
    return Success;
  }
  else
  {
    return Error;
  }
}

const char *Xila_Class::System_Class::Get_Device_Name()
{
  return Xila.System.Device_Name;
}

void Xila_Class::System_Class::Refresh_Header()
{
  if (Xila.Display.Get_State() == false) // if display sleep
  {
    return;
  }
  static char Temporary_Char_Array[6];

  // -- Update clock
  Temporary_Char_Array[0] = Xila.Time.Current_Time.tm_hour / 10;
  Temporary_Char_Array[0] += 48;
  Temporary_Char_Array[1] = Xila.Time.Current_Time.tm_hour % 10;
  Temporary_Char_Array[1] += 48;
  Temporary_Char_Array[2] = ':';
  Temporary_Char_Array[3] = Xila.Time.Current_Time.tm_min / 10;
  Temporary_Char_Array[3] += 48;
  Temporary_Char_Array[4] = Xila.Time.Current_Time.tm_min % 10;
  Temporary_Char_Array[4] += 48;
  Temporary_Char_Array[5] = '\0';

  Xila.Display.Set_Text(F("CLOCK_TXT"), Temporary_Char_Array);

  // Update connexion
  Temporary_Char_Array[5] = Xila.WiFi.RSSI();

  if (Xila.WiFi.status() == WL_CONNECTED)
  {
    if (Temporary_Char_Array[5] <= -70)
    {
      Xila.Display.Set_Text(F("CONNEXION_BUT"), Xila.Display.WiFi_Low);
    }
    if (Temporary_Char_Array[0] <= -50 && Temporary_Char_Array[0] > -70)
    {
      Xila.Display.Set_Text(F("CONNEXION_BUT"), Xila.Display.WiFi_Medium);
    }
    else
    {
      Xila.Display.Set_Text(F("CONNEXION_BUT"), Xila.Display.WiFi_High);
    }
  }
  else
  {
    Xila.Display.Set_Text(F("CONNEXION_BUT"), ' ');
  }

  // -- Update charge level
  Temporary_Char_Array[5] = Xila.Power.Get_Charge_Level();

  if (Temporary_Char_Array[5] <= 15)
  {
    Xila.Display.Set_Text(F("BATTERY_BUT"), Xila.Display.Battery_Empty);
  }
  else if (Temporary_Char_Array[5] <= 30 && Temporary_Char_Array[5] > 15)
  {
    Xila.Display.Set_Text(F("BATTERY_BUT"), Xila.Display.Battery_Low);
  }
  else if (Temporary_Char_Array[5] <= 70 && Temporary_Char_Array[5] > 30)
  {
    Xila.Display.Set_Text(F("BATTERY_BUT"), Xila.Display.Battery_Medium);
  }
  else // more than 70 %
  {
    Xila.Display.Set_Text(F("BATTERY_BUT"), Xila.Display.Battery_High);
  }

  // -- Update sound
  Temporary_Char_Array[5] = Xila.Sound.Get_Volume();

  if (Temporary_Char_Array[5] == 0)
  {
    Xila.Display.Set_Text(F("SOUND_BUT"), Xila.Display.Sound_Mute);
  }
  else if (Temporary_Char_Array[5] < 86)
  {
    Xila.Display.Set_Text(F("SOUND_BUT"), Xila.Display.Sound_Low);
  }
  else if (Temporary_Char_Array[5] < 172)
  {
    Xila.Display.Set_Text(F("SOUND_BUT"), Xila.Display.Sound_Medium);
  }
  else
  {
    Xila.Display.Set_Text(F("SOUND_BUT"), Xila.Display.Sound_High);
  }
}

///
/// @brief First
///
inline void Xila_Class::System_Class::First_Start_Routine()
{
  // -- Check if the power button was press or the power supply plugged.
  esp_sleep_enable_ext0_wakeup(POWER_BUTTON_PIN, LOW);
  esp_sleep_wakeup_cause_t Wakeup_Cause = esp_sleep_get_wakeup_cause();
  if (Wakeup_Cause != ESP_SLEEP_WAKEUP_EXT0 && Wakeup_Cause != ESP_SLEEP_WAKEUP_UNDEFINED)
  {
    Xila.Power.Deep_Sleep();
  }

  // -- Set power button interrupts
  Xila.GPIO.Set_Mode(POWER_BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(POWER_BUTTON_PIN), Xila.Power.Press_Button_Handler, FALLING);
  attachInterrupt(digitalPinToInterrupt(POWER_BUTTON_PIN), Xila.Power.Release_Button_Handler, RISING);

  // -- Initialize drive. -- //

#if Drive_Mode == 0
  Xila.GPIO.Set_Mode(14, INPUT_PULLUP);
  Xila.GPIO.Set_Mode(2, INPUT_PULLUP);
  Xila.GPIO.Set_Mode(4, INPUT_PULLUP);
  Xila.GPIO.Set_Mode(12, INPUT_PULLUP);
  Xila.GPIO.Set_Mode(13, INPUT_PULLUP);
#endif

  if (!Xila.Drive.Begin() || Xila.Drive.Type() == Xila.Drive.None)
  {
    Xila.Display.Set_Text(F("EVENT_TXT"), F("Failed to initialize drive."));
  }
  while (!Xila.Drive.Begin() || Xila.Drive.Type() == Xila.Drive.None)
  {
    Xila.Task.Delay(200);
  }
  Xila.Display.Set_Text(F("EVENT_TXT"), F(""));

  // -- Initialize display. -- //

  Xila.Display.Set_Callback_Function_Numeric_Data(Xila.Display.Incomming_Numeric_Data_From_Display);
  Xila.Display.Set_Callback_Function_String_Data(Xila.Display.Incomming_String_Data_From_Display);
  Xila.Display.Set_Callback_Function_Event(Xila.Display.Incomming_Event_From_Display);

  if (Xila.Display.Load_Registry() != Success)
  {
    Xila.Display.Save_Registry();
  }

  Xila.Display.Set_Current_Page(F("Core_Load")); // Play animation
  Xila.Display.Set_Trigger(F("LOAD_TIM"), true);

  // -- Check system integrity -- //

  if (!Xila.Drive.Exists(Xila_Directory_Path) || !Xila.Drive.Exists(Software_Directory_Path))
  {
    Xila.System.Panic_Handler(Xila.System.Missing_System_Files);
  }

  // -- Load system registry -- //
  if (Xila.System.Load_Registry() != Success)
  {
    Xila.System.Panic_Handler(Xila.System.Damaged_System_Registry);
  }

  // -- Load sound registry --

  if (Xila.Sound.Load_Registry() != Success)
  {
    Xila.Sound.Save_Registry();
  }
  Xila.Sound.Begin();

  // -- Play startup sound

  Xila.Sound.Play(Sounds("Startup.mp3"));

  // -- Load power registry

  if (Xila.Power.Load_Registry() != Success)
  {
    Xila.Power.Save_Registry();
  }

  // WiFi :

  if (Xila.WiFi.Load_Registry() != Success)
  {
    Xila.WiFi.Save_Registry();
  }

  // -- Time registry
  if (Xila.Time.Load_Registry() != Success)
  {
    Xila.Time.Save_Registry();
  }

  // -- Load account registry

  if (Xila.Account.Load_Registry() != Success)
  {
    Xila.Account.Set_Autologin(false);
  }

  // -- Load Keyboard Registry
  if (Xila.Keyboard.Load_Registry() != Success)
  {
    Xila.Keyboard.Save_Registry(); // recreate a keyboard registry with default values
  }
}

///
/// @brief
///
void Xila_Class::System_Class::Second_Start_Routine()
{
  Xila.System.Load_Dump();

#if Animations == 1
  Xila.Task.Delay(3000);
#endif

  Xila.Display.Set_Value(F("STATE_VAR"), 2);

#if Animations == 1
  Xila.Task.Delay(3000);
#endif

  Execute_Startup_Function();

  Xila.Task.Delay(500);

  if (!Xila.Drive.Exists(Users_Directory_Path))
  {
    File Temporary_File = Xila.Drive.Open(Display_Executable_Path);
    if (Xila.Display.Update(Temporary_File) != Xila.Display.Update_Succeed)
    {
      Panic_Handler(Failed_To_Update_Display);
    }
    Xila.Display.Wake_Up();
    Xila.Display.Set_Current_Page(F("Core_Load")); // Play animation
    Xila.Display.Set_Trigger(F("LOAD_TIM"), true);
  }

  Xila.Task.Create(Xila.System.Task, "Core Task", Memory_Chunk(4), NULL, Xila.Task.System_Task, &Xila.System.Task_Handle);

  Xila.Task.Delete(); // delete main task
}

/**
    * @brief Function handle deep-sleep wakeup, initialize the core, start software etc.
    * @param Software_Package Software that Xila need to load. 
    * @details Function steps :
    * 1) Check if the wakeup reasing is linked to a power button press, or undefined (power reset) and if not, go to sleep.
    * 2) Create an interrupt for the power button.
    * 3) Initalize display.
    * 4) Initalize system drive.
    * 5) Load registries (display, sound, keyboard, network, time).
    * 6) Play sound and animation.
    * 7) Load software handles.
    * 8) Execute software startup function (Shell and other software).
    */
void Xila_Class::System_Class::Start(Software_Handle_Class *Software_Package, uint8_t Size)
{
  if (Xila.System.Task_Handle != NULL) // Already started
  {
    return;
  }

  First_Start_Routine();

  // Restore attribute

  for (uint8_t i = 0; i < Size; i++)
  {
    Xila.Software.Add_Handle(Software_Package[i]);
  }

  Second_Start_Routine();
}

/**
     * @brief Function that handle deep-sleep wakeup, initialize the, start software etc.
     * @details Function steps :
     * 1) Cheeck if the wa
     */
void Xila_Class::System_Class::Start()
{
#if USB_Serial == 1
  Serial.begin(Default_USB_Serial_Speed);
#endif
  if (Xila.System.Task_Handle != NULL) // Already started
  {
    return;
  }

  First_Start_Routine();

  extern Software_Handle_Class Calculator_Handle;
  extern Software_Handle_Class Clock_Handle;
  extern Software_Handle_Class Internet_Browser_Handle;
  extern Software_Handle_Class Music_Player_Handle;
  extern Software_Handle_Class Oscilloscope_Handle;
  extern Software_Handle_Class Paint_Handle;
  extern Software_Handle_Class Periodic_Handle;
  extern Software_Handle_Class Piano_Handle;
  extern Software_Handle_Class Pong_Handle;
  extern Software_Handle_Class Simon_Handle;
  extern Software_Handle_Class Text_Editor_Handle;
  extern Software_Handle_Class Tiny_Basic_Handle;

  Xila.Software.Add_Handle(Calculator_Handle);
  Xila.Software.Add_Handle(Clock_Handle);
  Xila.Software.Add_Handle(Internet_Browser_Handle);
  Xila.Software.Add_Handle(Music_Player_Handle);
  Xila.Software.Add_Handle(Oscilloscope_Handle);
  Xila.Software.Add_Handle(Paint_Handle);
  Xila.Software.Add_Handle(Periodic_Handle);
  Xila.Software.Add_Handle(Piano_Handle);
  Xila.Software.Add_Handle(Pong_Handle);
  Xila.Software.Add_Handle(Simon_Handle);
  Xila.Software.Add_Handle(Text_Editor_Handle);
  Xila.Software.Add_Handle(Tiny_Basic_Handle);

  Second_Start_Routine();
}

///
/// @brief
///
void Xila_Class::System_Class::Execute_Startup_Function()
{

  (*Shell_Handle.Startup_Function_Pointer)();

  for (uint8_t i = 0; i < Maximum_Software; i++)
  {

    if (Xila.Software.Handle[i] != NULL)
    {
      if (Xila.Software.Handle[i]->Startup_Function_Pointer != NULL)
      {
        (*Xila.Software.Handle[i]->Startup_Function_Pointer)();
      }
    }
  }
}

///
/// @brief Function that shutdown Xila and it's connected devices.
///
void Xila_Class::System_Class::Shutdown()
{
  Verbose_Print_Line("Shutdown");
  Xila.Software.Send_Instruction_Shell(Software_Class::Shutdown);
  Xila.Software.Maximize_Shell();

  for (uint8_t i = 2; i < 8; i++)
  {
    if (Xila.Software.Openned[i] != NULL)
    {
      Xila.Software.Openned[i]->Send_Instruction(Software_Class::Shutdown);
      Xila.Task.Resume(Xila.Software.Openned[i]->Task_Handle);
      // -- Waiting for the software to close
      for (uint8_t j = 0; j <= 200; j++)
      {
        if (Xila.Software.Openned[i] == NULL)
        {
          break;
        }
        if (j == 200 && Xila.Software.Openned[i] != NULL)
        {
          Xila.Software.Force_Close(*Xila.Software.Openned[i]->Handle);
        }
        Xila.Task.Delay(20);
      }
    }
  }

  Second_Sleep_Routine();
}

void Xila_Class::System_Class::Second_Sleep_Routine()
{
  Xila.Display.Save_Registry();
  Xila.Keyboard.Save_Registry();
  Xila.Power.Save_Registry();
  Xila.Sound.Save_Registry();
  Xila.WiFi.Save_Registry();
  Xila.Time.Save_Registry();
  Xila.System.Save_Registry();

  // Shutdown screen
  Xila.Display.Set_Touch_Wake_Up(false);
  Xila.Display.Set_Serial_Wake_Up(true);

  Xila.Sound.Play(Sounds("Shutdown.wav"));

  Xila.Task.Delay(5000);

  //
  Xila.Display.Sleep();

  // Disconnect wifi
  Xila.WiFi.disconnect(true);

  //
  Xila.Drive.End();

  //Set deep sleep
  Xila.Power.Deep_Sleep();
}

void Xila_Class::System_Class::Hibernate()
{
  Xila.System.Save_Dump();

  Xila.Software.Send_Instruction_Shell(Software_Class::Hibernate);
  Xila.Software.Maximize_Shell();

  for (uint8_t i = 2; i < 8; i++)
  {
    if (Xila.Software.Openned[i] != NULL)
    {
      Xila.Software.Openned[i]->Send_Instruction(Software_Class::Hibernate);
      Xila.Task.Resume(Xila.Software.Openned[i]->Task_Handle);
      // -- Waiting for the software to close
      for (uint8_t j = 0; j <= 200; j++)
      {
        if (Xila.Software.Openned[i] == NULL)
        {
          break;
        }
        if (j == 200 && Xila.Software.Openned[i] != NULL)
        {
          Xila.Software.Force_Close(*Xila.Software.Openned[i]->Handle);
        }
        Xila.Task.Delay(20);
      }
    }
  }

  Second_Sleep_Routine();
}

void Xila_Class::System_Class::Restart()
{
  Xila.Software.Send_Instruction_Shell(Software_Class::Restart);
  Xila.Software.Maximize_Shell();

  for (uint8_t i = 2; i < 8; i++)
  {
    if (Xila.Software.Openned[i] != NULL)
    {
      Xila.Software.Openned[i]->Send_Instruction(Software_Class::Restart);
      Xila.Task.Resume(Xila.Software.Openned[i]->Task_Handle);
      // -- Waiting for the software to close
      for (uint8_t j = 0; j <= 200; j++)
      {
        if (Xila.Software.Openned[i] == NULL)
        {
          break;
        }
        if (j == 200 && Xila.Software.Openned[i] != NULL)
        {
          Xila.Software.Force_Close(*Xila.Software.Openned[i]->Handle);
        }
        Xila.Task.Delay(20);
      }
    }
  }

  Second_Sleep_Routine();
}

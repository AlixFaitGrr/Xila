#include "Arduino.h"

#include "Core.hpp"

#define INSTANCE_POINTER Xila_Class::Instance_Pointer
Xila_Class *Xila_Class::Instance_Pointer = NULL;

Xila_Class Xila;

extern Software_Handle_Class Shell_Handle;
extern Software_Handle_Class Executable_Loader_Handle;

/*char WiFi_SSID[] = "Avrupa";
char WiFi_Password [] = "0749230994";*/

extern Software_Handle_Class Shell_Handle;
extern Software_Handle_Class Oscilloscope_Handle;
extern Software_Handle_Class Paint_Handle;
extern Software_Handle_Class Calculator_Handle;
extern Software_Handle_Class TinyBasic_Handle;
extern Software_Handle_Class Internet_Browser_Handle;
extern Software_Handle_Class Music_Player_Handle;
extern Software_Handle_Class Piano_Handle;
extern Software_Handle_Class Ultrasonic_Handle;
extern Software_Handle_Class Pong_Handle;
extern Software_Handle_Class Signal_Generator_Handle;
extern Software_Handle_Class Text_Editor_Handle;
extern Software_Handle_Class Periodic_Handle;
extern Software_Handle_Class Clock_Handle;

Xila_Class::Xila_Class() : Tag(0),
                           Display(),
                           Sound(),
                           Battery(13, 47, 47),
                           Keyboard(2, 6),
                           Event_Reply(None),
                           Background_Function_Counter(0)

{
  if (Instance_Pointer != NULL)
  {
    // something went wrong
    ESP.restart();
  }
  Instance_Pointer = this;

  //strcpy(WiFi_SSID, "Avrupa");
  //strcpy(WiFi_Password, "0749230994");

#if SD_MODE == 0
  Drive = &SD_MMC;
#else
  Drive = &SD;
#endif

  memset(Current_Username, '\0', sizeof(Current_Username));

  memset(Open_Software_Pointer, NULL, sizeof(Open_Software_Pointer) / sizeof(Open_Software_Pointer[1]));
  memset(Software_Handle_Pointer, NULL, sizeof(Software_Handle_Pointer) / sizeof(Software_Handle_Pointer[1]));

  memset(Device_Name, '\0', sizeof(Device_Name));

  Dialog_Semaphore = xSemaphoreCreateMutex();

  //Core_Instruction_Queue_Handle = xQueueCreate(10, sizeof(Core_Instruction));cal
}

Xila_Class::~Xila_Class() // destructor
{
  Instance_Pointer = NULL;
}

// Idle task, lowest priority, nothing is running

void Xila_Class::Idle_Task_Software_Core(void *pvParameters)
{
  (void)pvParameters;
  while (1)
  {
    //Serial.print(F("Idle software core"));
    vTaskDelay(1);
  }
}

void Xila_Class::Idle_Task_System_Core(void *pvParameters)
{
  (void)pvParameters;
  while (1)
  {
    //Serial.print(F("Idle system core"));
    vTaskDelay(1);
  }
}

// Used to initialise the core

void Xila_Class::Start()
{
#if DEBUG_MODE == 1
  xTaskCreatePinnedToCore(Xila_Class::Core_Task, "Core Task", 4 * 1024, NULL, SYSTEM_TASK_PRIORITY, &Core_Task_Handle, SYSTEM_CORE);
#else
  esp_sleep_wakeup_cause_t Wakeup_Cause = esp_sleep_get_wakeup_cause();
  if (Wakeup_Cause == ESP_SLEEP_WAKEUP_EXT0)
  {
    xTaskCreatePinnedToCore(Xila_Class::Core_Task, "Core Task", 4 * 1024, NULL, SYSTEM_TASK_PRIORITY, &Core_Task_Handle, SYSTEM_CORE);
  }
  else
  {
    Shutdown();
  }
#endif
}

void Xila_Class::Shutdown()
{
  Close_Software(Open_Software_Pointer[2]->Handle_Pointer);
  Close_Software(Open_Software_Pointer[3]->Handle_Pointer);
  Close_Software(Open_Software_Pointer[4]->Handle_Pointer);
  Close_Software(Open_Software_Pointer[5]->Handle_Pointer);
  Close_Software(Open_Software_Pointer[6]->Handle_Pointer);
  Close_Software(Open_Software_Pointer[7]->Handle_Pointer);
  Close_Software(Open_Software_Pointer[1]->Handle_Pointer);

  // Shutdown screen
  Display.Sleep();

  // Disconnect wifi
  WiFi.disconnect(true);

  //Set deep sleep
  esp_sleep_enable_ext0_wakeup(POWER_BUTTON_PIN, LOW);
  esp_deep_sleep_start();
}

void Xila_Class::Load()
{
  Serial.begin(SERIAL_SPEED); //PC Debug UART
  Remaining_Spaces = 0;
  Horizontal_Separator();
  Print_Line();
  Print_Line(" _____       ___   _           ___  __    __       _____   _____ ", STYLE_CENTER_ALIGNMENT);
  Print_Line("|  ___|     /   | | |         /   | | |  | |      |  _  | |  ___|", STYLE_CENTER_ALIGNMENT);
  Print_Line("| |        / /| | | |        / /| |  | || |       | | | | | |___ ", STYLE_CENTER_ALIGNMENT);
  Print_Line("| |  _    / /_| | | |       / /_| |   |  |        | | | | |___  |", STYLE_CENTER_ALIGNMENT);
  Print_Line("| |_| |  / ___  | | |___   / ___  |  | || |       | |_| |  ___| |", STYLE_CENTER_ALIGNMENT);
  Print_Line("|_____| /_/   |_| |_____| /_/   |_| |_|  |_|      |_____| |_____|", STYLE_CENTER_ALIGNMENT);
  Print_Line();
  Print_Line();
  Horizontal_Separator();
  //Print_Line("Flash : 1,310,720 Bytes - EEPROM : 512 Bytes - RAM : " + char(ESP.getFreeHeap()) + "/ 327680 Bytes");
  Print_Line(F("Xila Embedded Edition - Alix ANNERAUD - Alpha - 0.1.0"));
  Horizontal_Separator();
  Print_Line(F("Starting Galax OS ..."), 0);

  // Setting interrput for power buttons

  pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(POWER_BUTTON_PIN), Power_Button_Handler, FALLING);

  // Initialize SD Card
  Print_Line(F("Mount The SD Card ..."), 0);

  // Initialize display
  Display.Set_Callback_Function_Numeric_Data(&Incomming_Numeric_Data_From_Display);
  Display.Set_Callback_Function_String_Data(&Incomming_String_Data_From_Display);
  Display.Set_Callback_Function_Event(&Incomming_Event_From_Display);
  Display.Begin();
  Display.Wake_Up();
  //Start animation
  Display.Set_Current_Page(F("Core_Load"));
  Display.Set_Value(F("LOAD_TIM"), 0);
  Display.Set_Trigger(F("LOAD_TIM"), true);

// Initialize drive
#if SD_MODE == 0
  pinMode(14, INPUT_PULLUP);
  //pinMode(15, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  Drive = &SD_MMC;
#else

  Drive = &SD;
#endif

  if (!Drive->begin() || Drive->cardType() == CARD_NONE)
  {
    Display.Set_Text(F("EVENT_TXT"), F("Please Check System Drive"));
  }
  while (!Drive->begin() || Drive->cardType() == CARD_NONE)
  {
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  //Display.Set_Text(F("EVENT_TXT"), F(""));

  File Temporary_File;

  uint8_t Installation_State;

  // Checking version / installation
  {
    Verbose_Print_Line("> Load system registry");
    Temporary_File = Drive->open(System_Registry_Path);
    if (!Temporary_File)
    {
    }
    DynamicJsonDocument System_Registry(256);
    if (deserializeJson(System_Registry, Temporary_File)) // error while deserialising
    {
      Display.Set_Text(F("EVENT_TXT"), "Damaged system registry.");
    }
    else
    {
      JsonObject Version = System_Registry["Version"];

      if (Version["Major"] != VERSION_MAJOR || Version["Minor"] != VERSION_MINOR || Version["Revision"] != VERSION_REVISION)
      {
        Display.Set_Text(F("EVENT_TXT"), "Incompatible installation.");
      }
      Installation_State = System_Registry["State"];
    }
  }

  // Initialize Virtual Memory
  /*Virtual_Memory_Semaphore = xSemaphoreCreateMutex();

  if (Virtual_Memory_Semaphore == NULL)
  {
    //error handle
  }
  else
  {
    xSemaphoreGive(Virtual_Memory_Semaphore);
  }*/

  //Testing Virtual Memory

  /*Verbose_Print_Line("> Testing Virtual Memory ...");

  uint32_t Test_Float = 123456789;
  Set_Variable('T', &Test_Float);
  Serial.println(Test_Float);
  Get_Variable('T', &Test_Float);
  Serial.println(Test_Float);

  uint16_t Test_Integer = 1543;
  Set_Variable('T', (uint32_t *)&Test_Integer);
  Serial.println(Test_Integer);
  Get_Variable('T', (uint32_t *)&Test_Integer);
  Serial.println(Test_Integer);

  uint8_t Test_Byte = 128;
  Set_Variable('T', (uint32_t *)&Test_Byte);
  Serial.println(Test_Byte);
  Get_Variable('T', (uint32_t *)&Test_Byte);
  Serial.println(Test_Byte);

  String Test_String = "test";
  Set_Variable('T', Test_String);
  Serial.println(Test_String);
  Get_Variable('T', Test_String);
  Serial.println(Test_String);/

  if (Test_Float != 123456789 || Test_Integer != 1543 || Test_Byte != 128 || Test_String == "test")
  {
    Verbose_Print("> Waring : Test failed for uint64_t");
  }*/

  // Initailize system tasks
  Verbose_Print_Line("> Loading Task ...");
  xTaskCreatePinnedToCore(Idle_Task_Software_Core, "Idle Software", 2 * 1024, NULL, IDLE_TASK_PRIORITY, NULL, SOFTWARE_CORE);
  xTaskCreatePinnedToCore(Idle_Task_System_Core, "Idle System", 2 * 1024, NULL, IDLE_TASK_PRIORITY, NULL, SYSTEM_CORE);

  // Check for a firmware switch (install/unistall software or recovery OS)
  if (Drive->exists("/XILA/XILA.XEF"))
  {
    Verbose_Print_Line("Switch firmware");
    Open_Software(&Executable_Loader_Handle);
    Temporary_File = Drive->open("/XILA/XILA.XEF");
    Open_File(Temporary_File);
  }

  // Check for a state
  Verbose_Print_Line("> Check existing of file last state ...");

  Restore_System_State();

  {
    Verbose_Print_Line("> Load display registry ...");
    Temporary_File = Drive->open(Display_Registry_Path);
    DynamicJsonDocument Display_Registry(256);
    deserializeJson(Display_Registry, Temporary_File);
    Display.Set_Brightness(100);
    //Display.Set_Baud_Rate(Display_Registry["Baud Rate"] | 921600, false);
  }

  // Sound load
  {
    Temporary_File = Drive->open(Sound_Registry_Path);
    DynamicJsonDocument Sound_Registry(256);
    deserializeJson(Sound_Registry, Temporary_File);
    Sound.Set_Volume(Sound_Registry["Volume"] | 0);
    Temporary_File.close();
  }
  Temporary_File = Drive->open("/XILA/SOUNDS/STARTUP.WAV");
  Sound.Play(Temporary_File);

  // Load network configuration
  {
    Verbose_Print_Line("> Load network registry ...");
    Temporary_File = Drive->open(Network_Registry_Path);
    if (!Temporary_File)
    {
      // error handle
    }
    DynamicJsonDocument Network_Registry(256);
    deserializeJson(Network_Registry, Temporary_File);
    // WiFi :
    WiFi.setHostname(Device_Name); // Set hostname
    JsonArray Name_Array = Network_Registry["WiFi"]["Names"];
    JsonArray Password_Array = Network_Registry["WiFi"]["Passwords"];
    char Name[32], Password[32];
    for (uint8_t i = 0; i < Name_Array.size(); i++)
    {
      strcpy(Name, Name_Array[i]);
      strcpy(Password, Password_Array[i]);
      WiFi.setAutoConnect(false);
      WiFi.begin(Name, Password);
      while (WiFi.status() != WL_CONNECTED && i < 50)
      {
        vTaskDelay(pdMS_TO_TICKS(100));
        i++;
      }
      if (WiFi.status() == WL_CONNECTED)
      {
        Verbose_Print_Line("> WiFi connected");
        break;
      }
    }
    Temporary_File.close();
  }

  // Set Regional Parameters
  {
    Verbose_Print_Line("> Load regional registry ...");
    Temporary_File = Drive->open(Regional_Registry_Path);
    if (!Temporary_File)
    {
      //error
    }
    DynamicJsonDocument Regional_Registry(256);
    deserializeJson(Regional_Registry, Temporary_File);

    strcpy(NTP_Server, Regional_Registry["Time"]["NTP Server"]);
    int32_t GMT_Offset = Regional_Registry["Time"]["GMT Offset"];
    int16_t Daylight_Offset = Regional_Registry["Time"]["Daylight Offset"];
    Serial.println(NTP_Server);
    Serial.println(GMT_Offset);
    Serial.println(Daylight_Offset);
    configTime(GMT_Offset, Daylight_Offset, NTP_Server);
    Temporary_File.close();
  }

  // Load software (including Shell UI)
  Verbose_Print_Line("> Load software ...");

  vTaskDelay(pdMS_TO_TICKS(5000)); // Waiting for software to add their
  Software_Handle_Pointer[0] = &Executable_Loader_Handle;
  Software_Handle_Pointer[1] = &Shell_Handle;
  Software_Handle_Pointer[2] = &Paint_Handle;
  Software_Handle_Pointer[3] = &Calculator_Handle;
  Software_Handle_Pointer[4] = &TinyBasic_Handle;
  Software_Handle_Pointer[5] = &Internet_Browser_Handle;
  Software_Handle_Pointer[6] = &Music_Player_Handle;
  Software_Handle_Pointer[7] = &Piano_Handle;
  Software_Handle_Pointer[8] = &Ultrasonic_Handle;
  Software_Handle_Pointer[9] = &Pong_Handle;
  Software_Handle_Pointer[10] = &Signal_Generator_Handle;
  Software_Handle_Pointer[11] = &Oscilloscope_Handle;
  Software_Handle_Pointer[12] = &Text_Editor_Handle;
  Software_Handle_Pointer[13] = &Periodic_Handle;
  Software_Handle_Pointer[14] = &Clock_Handle;

  File Software_Directory = Drive->open("/SOFTWARE");
  while (1)
  {
    Temporary_File = Software_Directory.openNextFile();
    if (!Temporary_File)
    {
      break;
    }
    else if (Temporary_File.isDirectory())
    {
      if (Drive->exists("/SOFTWARE/" + String(Temporary_File.name()) + "/REGISTRY/SOFTWARE.GRF"))
      {
        Temporary_File = Drive->open("/SOFTWARE/" + String(Temporary_File.name()) + "/REGISTRY/SOFTWARE.GRF");
        if (Temporary_File)
        {
          DynamicJsonDocument Software_Registry(256);
          if (deserializeJson(Software_Registry, Temporary_File) == DeserializationError::Ok)
          {
            Add_Software_Handle(new Software_Handle_Class(Software_Registry["Name"], Software_Registry["Picture"], NULL));
          }
        }
      }
    }
  }

  // Execute startup function of each software

  for (uint8_t i = 0; i < MAXIMUM_SOFTWARE; i++)
  {
    if (Software_Handle_Pointer[i]->Startup_Function_Pointer != NULL)
    {
      (*Software_Handle_Pointer[i]->Startup_Function_Pointer)();
    }
  }

  vTaskDelay(pdMS_TO_TICKS(3000));
  Display.Set_Value(F("STATE_VAR"), 2);
  vTaskDelay(pdMS_TO_TICKS(1200));
}

// Main task for the core
void Xila_Class::Core_Task(void *pvParameters)
{
  (void)pvParameters;
  Xila.Load();
  uint8_t Counter = 0;
  while (1)
  {

    if (ESP.getFreeHeap() < 2000)
    {
      Serial.println("Low Memory !");
    }

    Xila.Synchronise_Time(); // Time synchro

    Xila.Refresh_Header(); // Header refreshing

    Xila.Check_Power_Button();

    Xila.Execute_Background_Jobs(); // Software background function execution

#if STACK_OVERFLOW_DETECTION == 1
    Verbose_Print("> Current task high watermark :");
    Serial.println(uxTaskGetStackHighWaterMark(Xila_Class::Instance_Pointer->Open_Software_Pointer[0]->Task_Handle));
#endif
    vTaskDelay(pdMS_TO_TICKS(200));
  }
  /*xQueueReceive(Xila.Core_Instruction_Queue_Handle, Core_Instruction_Pointer, portMAX_DELAY);
    Core_Instruction_Pointer->Return_Pointer = Core_Instruction_Pointer->Function_Pointer(Core_Instruction_Pointer);
    xSemaphoreGive(Core_Instruction_Pointer->Executed);
    vTaskDelay(pdMS_TO_TICKS(5));*/
}

void Xila_Class::Save_System_State()
{
  if (Drive->exists("/XILA/SLEEP.XDF"))
  {
    if (Drive->remove("/XILA/SLEEP.XDF"))
    {
      //event handler
    }
  }
  DynamicJsonDocument System_Save_Registry(256);

  System_Save_Registry["Current Username"] = Current_Username;

  File Temporary_File = Drive->open("/XILA/GOSCUSA.XSF");
  serializeJson(System_Save_Registry, Temporary_File);
  Temporary_File.close();
}

// Interrput function

void IRAM_ATTR Xila_Class::Power_Button_Handler()
{
  vTaskEnterCritical(&Xila.Power_Button_Mutex);
  Xila.Power_Button_Counter = 1;
  vTaskExitCritical(&Xila.Power_Button_Mutex);
}

void Xila_Class::Check_Power_Button()
{
  if (Power_Button_Counter == 1)
  {
    Maximize_Software(1);
  }
}

// Callback function for screen

void Xila_Class::Incomming_String_Data_From_Display(const char *Received_Data, uint8_t Size)
{
  Serial.println(Received_Data);
  String Temporary_String;
  switch (Received_Data[0])
  {
  case Code::Close:
    Instance_Pointer->Close_Software();
    break;
  case Code::Minimize:
    Instance_Pointer->Minimize_Software();
    break;
  case Code::Command:
  case Code::Command_New:
    Xila.Open_Software_Pointer[0]->Execute(Received_Data[1], Received_Data[2]);
    break;
  case Code::Event:
    Xila.Event_Reply = (Xila_Event)Received_Data[1];
    break;
  case Code::Variable_String_Local:
    Temporary_String = String(Received_Data + 2);
    Xila.Open_Software_Pointer[0]->Set_Variable(&Temporary_String, Variable_String_Local, Received_Data[0]);
    break;
  case Code::Variable_Char_Local:
    Xila.Open_Software_Pointer[0]->Set_Variable(Received_Data + 2, Variable_Char_Local, Received_Data[1], Size);
    Xila.Tag = '\0';
    break;
  case Code::Variable_String_Global:
    Temporary_String = String(Received_Data + 2);
    Xila.Open_Software_Pointer[0]->Set_Variable(&Temporary_String, Variable_String_Local, Received_Data[0]);
    break;
  case Code::Variable_Long_Local:
    Xila.Tag = Received_Data[1];
    break;
  case Code::Variable_Long_Global:
    Xila.Tag = Received_Data[1];
    break;
  default:
    //error handle
    break;
  }
}

void Xila_Class::Incomming_Numeric_Data_From_Display(uint32_t &Received_Data)
{
  if (Xila.Tag != '\0')
  {
    Xila.Open_Software_Pointer[0]->Set_Variable(&Received_Data, Xila.Variable_Long_Local, Xila.Tag);
    Xila.Tag = '\0';
  }
  else
  {
    Xila.Event_Dialog(F("Unexpected Numeric Data Return From Display"), Error);
  }
}

void Xila_Class::Incomming_Event_From_Display(uint8_t &Event_Code)
{
}

void Xila_Class::Open_File(File &File_To_Open)
{
  if (!File_To_Open)
  {
    //error handle
    return;
  }
  else if (File_To_Open.isDirectory())
  {
    //error : directory not a file
    return;
  }
  char File_Name[14];
  strcpy(File_Name, File_To_Open.name());
  uint8_t i = 1;
  for (; i < 14; i++) //extract extension
  {
    if (File_Name[i] == '.')
    {
      break;
    }
  }
  char Extension[] = {File_Name[i + 1], File_Name[i + 2], File_Name[i + 3]};
  //pre-defined
  if (strcmp(Extension, "XEF")) // executable file : load it into flash
  {
    Open_Software(&Executable_Loader_Handle);
    for (i = 0; i < 8; i++)
    {
      if (Open_Software_Pointer[i] != NULL)
      {
        if (Open_Software_Pointer[i]->Handle_Pointer == &Executable_Loader_Handle)
        {
          Open_Software_Pointer[i]->Open_File(File_To_Open);
        }
      }
    }
  }

  else
  {
    if (Drive->open(F("/XILA/REGISTRY/EXTENSIO.GCF")))
    {
      DynamicJsonDocument Extension_Registry(256);
      File Temporary_File = Drive->open(Extension_Registry_Path);
      deserializeJson(Extension_Registry, Temporary_File);
      for (uint8_t i = 0; i < MAXIMUM_SOFTWARE; i++)
      {
        if (strcmp(Software_Handle_Pointer[i]->Name, Extension_Registry[Extension]))
        {
          Open_Software(Software_Handle_Pointer[i]);
          for (uint8_t j = 0; i < 8; i++)
          {
            if (Open_Software_Pointer[j]->Handle_Pointer == Software_Handle_Pointer[i])
            {
              Open_Software_Pointer[j]->Open_File(File_To_Open);
              File_To_Open.close();
              return;
            }
          }
        }
      }
    }
    //error handle : unknow extension
  }
  File_To_Open.close();
}

// Software management

void Xila_Class::Open_Software(Software_Handle_Class *Software_Handle_Pointer)
{
  uint8_t i = 1;
  for (; i < 8; i++) //check if software is openned already
  {
    if (Open_Software_Pointer[i] != NULL)
    {
      if (Software_Handle_Pointer == Open_Software_Pointer[i]->Handle_Pointer)
      {
        Maximize_Software(i);
        return;
      }
    }
  }
  // if not, open it in an empty place
  for (i = 1; i < 8; i++)
  {
    if (Open_Software_Pointer[i] == NULL)
    {
      Open_Software_Pointer[i] = (*Software_Handle_Pointer->Load_Function_Pointer)();
      Maximize_Software(i);
      return;
    }
  }

  // if not enough place, call event handler

  Event_Dialog(F("Too many open software, please close one before starting a software."), Error);
}

void Xila_Class::Close_Software(Software_Handle_Class *Software_Handle_To_Close)
{
  if (Software_Handle_To_Close == NULL) //by default delete current running software
  {
    for (uint8_t i = 2; i < 8; i++)
    {
      if (Open_Software_Pointer[i] != NULL)
      {
        if (Open_Software_Pointer[i]->Handle_Pointer == Open_Software_Pointer[0]->Handle_Pointer)
        {
          Open_Software_Pointer[i] = NULL;
        }
      }
    }
    Open_Software_Pointer[0]->Close();
    Open_Software_Pointer[0] = NULL;
  }
  else
  {
    for (uint8_t i = 2; i < 8; i++)
    {
      if (Open_Software_Pointer[i] != NULL)
      {
        if (Open_Software_Pointer[i]->Handle_Pointer == Software_Handle_To_Close)
        {
          Open_Software_Pointer[i]->Close();
          Open_Software_Pointer[i] = NULL;
        }
      }
    }
  }
  Maximize_Software(1); //reopen shell
}

void Xila_Class::Minimize_Software()
{
  Open_Software_Pointer[0]->Minimize();
  Open_Software_Pointer[0] = NULL;
}

void Xila_Class::Maximize_Software(uint8_t Slot)
{
  if (Open_Software_Pointer[Slot] == NULL) // Error : no software to maximize
  {
    return;
  }
  if (Open_Software_Pointer[0] != NULL)
  {
    Minimize_Software();
  }
  Open_Software_Pointer[0] = Open_Software_Pointer[Slot];
  Open_Software_Pointer[0]->Maximize();
}

void Xila_Class::Load_Software_Handle(Software_Handle_Class *Software_Handle_Pointer_To_Add, const __FlashStringHelper *Header_Path)
{
  for (uint8_t i = 0; i < MAXIMUM_SOFTWARE; i++)
  {
    if (Software_Handle_Pointer[i] != NULL) //check for doublon
    {
      if (strcmp(Software_Handle_Pointer[i]->Name, Software_Handle_Pointer_To_Add->Name) == 0)
      {
        return;
      }
    }
  }

  if (!Drive->exists(Header_Path))
  {
    return;
  }
  File Temporary_File = Drive->open(Header_Path);
  DynamicJsonDocument Software_Registry(DEFAULT_REGISTRY_SIZE);
  if (deserializeJson(Software_Registry, Temporary_File) != DeserializationError::Ok)
  {
    return;
  }
  if (Software_Registry["Type"] != 0)
  {
    return;
  }
  if (strcmp(Software_Registry["Name"], Software_Handle_Pointer_To_Add->Name) != 0)
  {
    return;
  }
  
  for (uint8_t i = 0; i < MAXIMUM_SOFTWARE; i++)
  {
    if (Software_Handle_Pointer[i] == NULL)
    {
      Software_Handle_Pointer[i] = Software_Handle_Pointer_To_Add;
    }
  }
}

// Serial communication with commputer

void Xila_Class::Horizontal_Separator()
{
  Serial.println(F("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"));
}

void Xila_Class::Print_Line(const __FlashStringHelper *Text_To_Print, uint8_t const &Alignement)
{
  Serial.print(F("||"));
  Serial.print(F(" "));
  Serial.print(Text_To_Print);
  Serial.println(F("||"));
}

void Xila_Class::Print_Line(const char *Text_To_Print, uint8_t const &Alignement)
{
  Serial.print(F("||"));

  if (sizeof(Text_To_Print) > 64)
  {
    //unsupported now
    return;
  }
  Remaining_Spaces = 64 - sizeof(Text_To_Print);
  switch (Alignement)
  {
  case 0: //left
    Serial.print(F(" "));
    Serial.print(Text_To_Print);
    break;
  case 1: //centered
    Serial.print(F(" "));
    for (uint8_t i = 0; i < Remaining_Spaces / 2; i++)
    {
      Serial.println(F(" "));
    }
    Serial.print(Text_To_Print);
    Remaining_Spaces /= 2;
  default:

    break;
  }
  for (uint8_t i = 0; i < Remaining_Spaces; i++)
  {
    Serial.println(F(" "));
  }
  Serial.println(F("||"));
}

void Xila_Class::Print_Line()
{
  Serial.print(F("||                                                                            ||"));
}

// --------------------------

void Xila_Class::Restore_System_State()
{
  if (!Drive->exists("/XILA/XILA.GRF"))
  {
    // no need to restore
    return;
  }
  File Temporary_File = Drive->open("/XILA/GOSCUSA.XSF");
  DynamicJsonDocument Temporary_Json_Document(256);
  //ReadBufferingStream Buffering_Stream(Drive->open("/"), 64); dont work, i don't know why

  if (deserializeJson(Temporary_Json_Document, Temporary_File) == true)
  {
    return;
  }

  strcpy(Current_Username, Temporary_Json_Document["Current Username"]);

  Temporary_File.close();
  Drive->remove(F("/XILA/XILA.GRF"));
}

//---------------------------------------------------------------------------//
//                                Virtual Memory Management                  //
//---------------------------------------------------------------------------//

// Char array
/*void Xila_Class::Set_Variable(char const &Tag, const char *String_To_Set, Software_Handle_Class *Software_Handle_Targeted = NULL)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL) // global scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/GLOBAL/STRING/" + Tag, FILE_WRITE);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.print(String_To_Set);
    }
  }
  else // local scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + char(Software_Handle_Targeted->Name) + "/STRING/" + Tag, FILE_WRITE);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.print(String_To_Set);
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

void Xila_Class::Get_Variable(char const &Tag, char *String_To_Set, Software_Handle_Class *Software_Handle_Targeted = NULL)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL) // global scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/GLOBAL/STRING/" + Tag, FILE_READ);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.readBytes(String_To_Set, sizeof(String_To_Set));
    }
  }
  else // local scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + char(Software_Handle_Targeted->Name) + "/STRING/" + Tag, FILE_READ);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.readBytes(String_To_Set, sizeof(String_To_Set));
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}/

// String
void Xila_Class::Set_Variable(char const &Tag, String const &String_To_Set, uint16_t Size, Software_Handle_Class *Software_Handle_Targeted)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL) // global scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/GLOBAL/STRING/" + Tag, FILE_WRITE);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.print(String_To_Set);
    }
  }
  else // local scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + String(*Software_Handle_Targeted->Name) + "/STRING/" + Tag, FILE_WRITE);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.print(String_To_Set);
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

void Xila_Class::Get_Variable(char const &Tag, String &String_To_Get, uint16_t Size, Software_Handle_Class *Software_Handle_Targeted)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL) // global scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/GLOBAL/STRING/" + Tag, FILE_READ);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      String_To_Get = Virtual_Memory_File.readString();
    }
  }
  else // local scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + String(*Software_Handle_Targeted->Name) + "/STRING/" + Tag, FILE_READ);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      String_To_Get = Virtual_Memory_File.readString();
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

// char array
void Xila_Class::Set_Variable(char const &Tag, const char *String_To_Set, uint16_t Size, Software_Handle_Class *Software_Handle_Targeted)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL) // global scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/GLOBAL/STRING/" + Tag, FILE_WRITE);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.print(String(String_To_Set));
    }
  }
  else // local scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + String(*Software_Handle_Targeted->Name) + "/STRING/" + Tag, FILE_WRITE);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.print(String(String_To_Set));
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

void Xila_Class::Get_Variable(char const &Tag, char *String_To_Get, uint16_t Size, Software_Handle_Class *Software_Handle_Targeted)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL) // global scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/GLOBAL/STRING/" + Tag, FILE_READ);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.readString().toCharArray(String_To_Get, Size);
    }
  }
  else // local scope
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + String(*Software_Handle_Targeted->Name) + "/STRING/" + Tag, FILE_READ);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.readString().toCharArray(String_To_Get, Size);
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

// 32-bit var
void Xila_Class::Set_Variable(char const &Tag, uint32_t *Number_To_Set, uint16_t Size, Software_Handle_Class *Software_Handle_Targeted)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  Split_Number[0] = (uint8_t)Number_To_Set[0];
  Split_Number[1] = (uint8_t)Number_To_Set[0] << 8;
  Split_Number[2] = (uint8_t)Number_To_Set[0] << 16;
  Split_Number[3] = (uint8_t)Number_To_Set[0] << 24;
  if (Software_Handle_Targeted == NULL)
  {
    Virtual_Memory_File = Drive->open(Virtual_Global_Memory_File, FILE_WRITE);
    Virtual_Memory_File.seek(Tag << 1);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.write(Split_Number, 4);
    }
  }
  else
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + String(*Software_Handle_Targeted->Name) + "/VARIABLE.XSF", FILE_WRITE);
    Virtual_Memory_File.seek(Tag << 1);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.write(Split_Number, 4);
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

void Xila_Class::Get_Variable(char const &Tag, uint32_t *Number_To_Get, uint16_t Size, Software_Handle_Class *Software_Handle_Targeted)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL)
  {
    Virtual_Memory_File = Drive->open(Virtual_Global_Memory_File, FILE_READ);
    Virtual_Memory_File.seek(Tag << 1);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.readBytes((char *)Split_Number, 4);
    }
  }
  else
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + String(*Software_Handle_Targeted->Name) + "/VARIABLE.XSF", FILE_READ);
    Virtual_Memory_File.seek(Tag << 1);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.readBytes((char *)Split_Number, 4);
    }
  }
  Number_To_Get[0] = (uint32_t)Split_Number << 24 | (uint32_t)Split_Number << 16 | (uint32_t)Split_Number << 8 | (uint32_t)Split_Number;
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}
/*
// 32-bit variable
void Xila_Class::Set_Variable(uint8_t const &Tag, uint32_t const &Number_To_Set, Software_Handle_Class *Software_Handle_Targeted = NULL)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  Split_Number[0] = (uint8_t)Number_To_Set;
  Split_Number[1] = (uint8_t)Number_To_Set << 8;
  Split_Number[2] = (uint8_t)Number_To_Set << 16;
  Split_Number[3] = (uint8_t)Number_To_Set << 24;
  if (Software_Handle_Targeted == NULL)
  {
    Virtual_Memory_File = Drive->open(Virtual_Global_Memory_File, FILE_WRITE);
    Virtual_Memory_File.seek(Tag << 1);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.write(Split_Number, 4);
    }
  }
  else
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + char(Software_Handle_Targeted->Name) + "/VARIABLE.XSF", FILE_WRITE);
    Virtual_Memory_File.seek(Tag << 1);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.write(Split_Number, 4);
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

void Xila_Class::Get_Variable(uint8_t const &Tag, uint32_t &Number_To_Get, Software_Handle_Class *Software_Handle_Targeted = NULL)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL)
  {
    Virtual_Memory_File = Drive->open(Virtual_Global_Memory_File, FILE_READ);
    Virtual_Memory_File.seek(Tag << 1);
    if (Virtual_Memory_File)
    {
      Number_To_Get = Virtual_Memory_File.parseFloat();
    }
  }
  else
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + char(Software_Handle_Targeted) + "/VARIABLE.XSF", FILE_READ);
    Virtual_Memory_File.seek(Tag << 1);
    if (Virtual_Memory_File)
    {
      Number_To_Get = Virtual_Memory_File.parseFloat();
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

// 16 bit variable
void Xila_Class::Set_Variable(uint8_t const &Tag, uint16_t* Number_To_Set, uint16_t const& Size = 1, Software_Handle_Class *Software_Handle_Targeted = NULL)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL)
  {
    Virtual_Memory_File = Drive->open(Virtual_Global_Memory_File, FILE_WRITE);
    if (Virtual_Memory_File)
    {
      Split_Number[0] = (char)Number_To_Set;
      Split_Number[1] = (char)Number_To_Set << 8;
      Virtual_Memory_File.write(Split_Number, 2);
    }
  }
  else
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + char(Software_Handle_Targeted->Name) + "/VARIABLE.XSF", FILE_WRITE);
    if (Virtual_Memory_File)
    {
      Split_Number[0] = (char)Number_To_Set;
      Split_Number[1] = (char)Number_To_Set << 8;
      Virtual_Memory_File.write(Split_Number, 2);
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

void Xila_Class::Get_Variable(uint8_t const &Tag, uint16_t* Number_To_Set, uint16_t const& Size = 1, Software_Handle_Class *Software_Handle_Targeted = NULL)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL)
  {
    Virtual_Memory_File = Drive->open(Virtual_Global_Memory_File, FILE_WRITE);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Number_To_Set = Virtual_Memory_File.parseInt();
    }
  }
  else
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + char(Software_Handle_Targeted->Name) + "/VARIABLE.XSF", FILE_WRITE);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Number_To_Set = Virtual_Memory_File.parseInt();
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

// 8 bit variable
void Xila_Class::Set_Variable(uint8_t const &Tag, uint8_t* Number_To_Set, uint16_t const& Size = 1, Software_Handle_Class *Software_Handle_Targeted = NULL)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  if (Software_Handle_Targeted == NULL)
  {
    Virtual_Memory_File = Drive->open(Virtual_Global_Memory_File, FILE_WRITE);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.write(Number_To_Set, Size);
    }
  }
  else
  {
    Virtual_Memory_File = Drive->open("/XILA/MEMORY/" + char(Software_Handle_Targeted->Name) + "/VARIABLE.XSF", FILE_WRTIE);
    Virtual_Memory_File.seek(0);
    if (Virtual_Memory_File)
    {
      Virtual_Memory_File.write(Number_To_Set, Size);
    }
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

void Xila_Class::Get_Variable(uint8_t const &Tag, uint8_t &Number_To_Set, Software_Handle_Class *Software_Handle_Targeted = NULL)
{
  xSemaphoreTake(Virtual_Memory_Semaphore, portMAX_DELAY);
  Virtual_Memory_File = Drive->open(Virtual_Global_Memory_File, FILE_READ);
  Virtual_Memory_File.seek(0);
  if (Virtual_Memory_File)
  {
    Number_To_Set = Virtual_Memory_File.read();
  }
  Virtual_Memory_File.close();
  xSemaphoreGive(Virtual_Memory_Semaphore);
}

void Ressource_Monitor(void *pvParameters)
{
  (void)pvParameters;
  for (;;)
  {
    if (ESP.getFreeHeap() < 2000)
    {
      Serial.println("Low Memory !");
    }
    vTaskDelay(1000);
  }
}*/

// Account management
Xila_Event Xila_Class::Add_User(const char *Username, const char *Password)
{
  if (Drive->exists("/USERS/" + String(Username)))
  {
    return Error;
  }

  Drive->mkdir("/USERS/" + String(Username) + "/REGISTRY");
  Drive->mkdir("/USERS/" + String(Username) + "/DESK");
  Drive->mkdir("/USERS/" + String(Username) + "/DOCUMENT");
  Drive->mkdir("/USERS/" + String(Username) + "/MUSIC");
  File Temporary_File = Drive->open("/USERS/" + String(Username) + "/REGISTRY/USER.XRF", FILE_WRITE);
  DynamicJsonDocument User_Registry(256);
  if (deserializeJson(User_Registry, Temporary_File) != DeserializationError::Ok)
  {
    return Error;
  }

  strcpy(User_Registry["Registry"], "User");
  strcpy(User_Registry["Password"], Password);
  serializeJson(User_Registry, Temporary_File);
  return Success;
}

Xila_Event Xila_Class::Delete_User(const char *Username_To_Check, const char *Password_To_Check)
{
  if (Check_Credentials(Username_To_Check, Password_To_Check) == Success)
  {
    Drive->remove("/USERS/" + String(Username_To_Check));
    return Success;
  }
  else
  {
    return Error;
  }
}

Xila_Event Xila_Class::Change_Password(const char *Username_To_Check, const char *Password_To_Check, const char *Password_To_Set)
{
  if (Check_Credentials(Username_To_Check, Password_To_Check) != Success)
  {
    return Error;
  }
  File Temporary_File = Drive->open("/USERS/" + String(Username_To_Check) + "/REGISTRY/USER.XRF");
  DynamicJsonDocument User_Registry(DEFAULT_REGISTRY_SIZE);
  User_Registry["Registry"] = "Registry";
  User_Registry["Password"] = Password_To_Set;
  serializeJson(User_Registry, Temporary_File);
  return Success;
}

Xila_Event Xila_Class::Logout()
{
  if (Current_Username[0] == '\0')
  {
    return Error;
  }
  memset(Current_Username, '\0', sizeof(Current_Username));
  return Success;
}

Xila_Event Xila_Class::Check_Credentials(const char *Username_To_Check, const char *Password_To_Check)
{
  if (Drive->exists("/USERS/" + String(Username_To_Check) + "/REGISTRY/USER.GRF"))
  {
    File Temporary_File = Drive->open("/USERS/" + String(Username_To_Check) + "/REGISTRY/USER.GRF");
    DynamicJsonDocument User_Registry(256);
    if (deserializeJson(User_Registry, Temporary_File) != DeserializationError::Ok)
    {
      return Error;
    }
    char Password[24];
    strcpy(Password, User_Registry["Password"]);
    Serial.println(Password);
    if (strcmp(Password_To_Check, Password) != 0)
    {
      Verbose_Print_Line("> Wrong Credentials ...");

      return Error;
    }

    Verbose_Print_Line("> Good Credentials ...");
    return Success;
  }

  Xila_Event Xila_Class::Login(const char *Username_To_Check, const char *Password_To_Check)
  {
    if (Check_Credentials(Username_To_Check, Password_To_Check) != Success)
    {
      return Error;
    }
    strcpy(Current_Username, Username_To_Check);
    return Success;
  }

  /*  switch (Type)
  {
    Nextion_Serial_Transmit(F("Event"), COMMAND_PAGE_NAME, "", 0);
  case ERROR_FAILLED_TO_INTIALIZE_SD_CARD:
    Nextion_Serial.print(F("TITLE_TXT.txt="));
    Nextion_Serial.print(F("A"));
    Nextion_Serial_Transmit(F("TITLE_TXT"), ATTRIBUTE_TXT, F("Error !"), 0);
    Nextion_Serial_Transmit(F("MESSAGE_TXT"), ATTRIBUTE_TXT, F("Failled to initialize the SD Card !"), 0);
    break;
  case ERROR_SOME_SYSTEM_FILES_ARE_MISSING:
    Nextion_Serial_Transmit(F("TITLE_TXT"), ATTRIBUTE_TXT, F("Error !"), 0);
    Nextion_Serial_Transmit(F("MESSAGE_TXT"), ATTRIBUTE_TXT, F("Some system files are missing !"), 0);
    break;
  case ERROR_SOME_USER_SETTINGS_FILES_ARE_MISSING:
    Nextion_Serial_Transmit(F("TITLE_TXT"), ATTRIBUTE_TXT, F("Error !"), 0);
    Nextion_Serial_Transmit(F("MESSAGE_TXT"), ATTRIBUTE_TXT, F("Some settings files are missing !"), 0);
    break;
  case ERROR_SOME_USER_SETTINGS_FILES_ARE_CORRUPTED:
    Nextion_Serial_Transmit(F("TITLE_TXT"), ATTRIBUTE_TXT, F("Error !"), 0);
    Nextion_Serial_Transmit(F("MESSAGE_TXT"), ATTRIBUTE_TXT, F("Some user settings files are corrupted !"), 0);
    break;
  case ERROR_THE_FILE_DO_NOT_EXIST:
    Nextion_Serial_Transmit(F("TITLE_TXT"), ATTRIBUTE_TXT, F("Error !"), 0);
    Nextion_Serial_Transmit(F("MESSAGE_TXT"), ATTRIBUTE_TXT, F("The file doesn't exist !"), 0);
    break;
  case WARNING_WRONG_PASSWORD:
    break;
  case WARNING_WRONG_USERNAME:
    Nextion_Serial_Transmit(F("WRONG_TXT"), ATTRIBUTE_TXT, F("Wrong Username !"), 0);
    Serial.println(F("Wrong Username !"));
    break;
  case WARNING_DO_YO_REALLY_WANT_TO_DELETE_THIS_ITEM:
    break;
  default:
    break;
  }*/

  Xila_Event Xila_Class::Event_Dialog(uint16_t const &Event_ID)
  {

    /*Display.Set_Current_Page(F("Event"));
  
  byte i = 0;

  while (Event_Reply == 0 && i < 100)
  {
    vTaskDelay(pdMS_TO_TICKS(50));
    i++;
  }
  if (i > 100)
  {
    Display.Set_Current_Page(Display.Page_History[1]); //go to last page
    return Default;
  }
  return Event_Reply;*/
    return 0;
  }

  Xila_Event Xila_Class::Event_Dialog(const __FlashStringHelper *Message, uint8_t Event_Type, const __FlashStringHelper *Button_Text_1, const __FlashStringHelper *Button_Text_2, const __FlashStringHelper *Button_Text_3)
  {
    xSemaphoreTake(Dialog_Semaphore, portMAX_DELAY);
    Display.Set_Current_Page(F("Shell_Event"));
    if (Button_Text_1 != NULL)
    {
      Display.Set_Text(F("BUTTON1_BUT"), Button_Text_1);
      if (Button_Text_2 != NULL)
      {
        Display.Set_Text(F("BUTTON2_BUT"), Button_Text_2);
      }
      else
      {
        Display.Set_Text(F("BUTTON2_BUT"), F(""));
      }
      if (Button_Text_3 != NULL)
      {
        Display.Set_Text(F("BUTTON3_BUT"), Button_Text_3);
      }
      else
      {
        Display.Set_Text(F("BUTTON3_BUT"), F(""));
      }
    }
    else
    {
      Display.Set_Text(F("BUTTON1_BUT"), F("Okay"));
      Display.Set_Text(F("BUTTON2_BUT"), F("Yes"));
      Display.Set_Text(F("BUTTON3_BUT"), F("No"));
    }
    Display.Set_Text(F("MESSAGE_TXT"), Message);
    switch (Event_Type)
    {
    case Error:
      Display.Set_Picture(F("EVENT_PIC"), 12);
      Display.Set_Text(F("TITLE_TXT"), F("Error"));
      break;
    case Warning:
      Display.Set_Picture(F("EVENT_PIC"), 12);
      Display.Set_Text(F("TITLE_TXT"), F("Warning"));
      break;
    case Information:
      Display.Set_Picture(F("EVENT_PIC"), 12);
      Display.Set_Text(F("TITLE_TXT"), F("Information"));
      break;
    case Question:
      Display.Set_Picture(F("EVENT_PIC"), 12);
      Display.Set_Text(F("TITLE_TXT"), F("Question"));
    default:
      break;
    }
    Display.Show(F("CLOSE_PIC"));
    while (Event_Reply == 0)
    {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    Xila_Event Event_Reply_Copy = Event_Reply;
    Event_Reply = 0;
    xSemaphoreGive(Dialog_Semaphore);
    return Event_Reply_Copy;
  }

  Xila_Event Xila_Class::Open_File_Dialog(File & File_To_Open)
  {
    Maximize_Shell();
    Execute_Shell(Open_File);

    while (!Shell_Return_Item)
    {
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    return
  }

  void Xila_Class::Maximize_Shell()
  {
    Maximize_Software(1);
  }

  void Xila_Class::Execute_Shell(uint16_t const &Command)
  {
    Open_Software_Pointer[1]->Execute(Command);
  }

  void Xila_Class::Refresh_Header()
  {
    uint32_t Update_Time = millis();
    static char Clock[5];
    Clock[0] = Time.tm_hour / 10;
    Clock[0] += 48;
    Clock[1] = Time.tm_hour % 10;
    Clock[1] += 48;
    Clock[2] = ':';
    Clock[3] = Time.tm_min / 10;
    Clock[3] += 48;
    Clock[4] = Time.tm_min % 10;
    Clock[4] += 48;
    Clock[5] += '\0';
    Display.Set_Text(F("CLOCK_TXT"), Clock);

    Clock[0] = WiFi.RSSI();
    if (WiFi.status() == WL_CONNECTED)
    {
      if (Clock[0] <= -70)
      {
        Instance_Pointer->Display.Set_Text(F("CONNEXION_BUT"), F(WiFi_Low));
      }
      if (Clock[0] <= -50 && Clock[0] > -70)
      {
        Instance_Pointer->Display.Set_Text(F("CONNEXION_BUT"), F(WiFi_Medium));
      }
      else
      {
        Instance_Pointer->Display.Set_Text(F("CONNEXION_BUT"), F(WiFi_High));
      }
    }
    else
    {
      Instance_Pointer->Display.Set_Text(F("CONNEXION_BUT"), F(' '));
    }

    Clock[0] = Instance_Pointer->Battery.Get_Charge_Level();
    if (Clock[0] <= 15)
    {
      Instance_Pointer->Display.Set_Text(F("BATTERY_BUT"), F(Battery_Empty));
    }
    else if (Clock[0] <= 30 && Clock[0] > 15)
    {
      Instance_Pointer->Display.Set_Text(F("BATTERY_BUT"), F(Battery_Low));
    }
    else if (Clock[0] <= 70 && Clock[0] > 30)
    {
      Instance_Pointer->Display.Set_Text(F("BATTERY_BUT"), F(Battery_Medium));
    }
    else // more than 70 %
    {
      Instance_Pointer->Display.Set_Text(F("BATTERY_BUT"), F(Battery_High));
    }

    Clock[0] = Instance_Pointer->Sound.Get_Volume();
    if (Clock[0] == 0)
    {
      Instance_Pointer->Display.Set_Text(F("SOUND_BUT"), F(Sound_Mute));
    }
    else if (Clock[0] < 33)
    {
      Instance_Pointer->Display.Set_Text(F("SOUND_BUT"), F(Sound_Low));
    }
    else if (Clock[0] < 66)
    {
      Instance_Pointer->Display.Set_Text(F("SOUND_BUT"), F(Sound_Medium));
    }
    else
    {
      Instance_Pointer->Display.Set_Text(F("SOUND_BUT"), F(Sound_High));
    }
    Update_Time = millis() - Update_Time;
    Verbose_Print("Update header time :");
    Verbose_Print_Line(Update_Time);
  }

  void Xila_Class::Execute_Background_Jobs()
  {
    if ((millis() - Last_Execution) > 250)
    {
      if (Software_Handle_Pointer[Background_Function_Counter] != NULL)
      {
        if (Software_Handle_Pointer[Background_Function_Counter]->Background_Function_Pointer != NULL)
        {
          (*Software_Handle_Pointer[Background_Function_Counter]->Background_Function_Pointer)();
        }
        Background_Function_Counter = millis();
      }
      else
      {
        Background_Function_Counter = 0;
      }
    }
  }

  void Xila_Class::Synchronise_Time()
  {
    time(&Now);
    localtime_r(&Now, &Time);
  }

  tm Xila_Class::Get_Time()
  {
    Synchronise_Time();
    return Time;
  }

  // Create System file at 1st boot

  /*void Xila_Class::Create_System_Files()
  {
    // Display Registry
    DynamicJsonDocument Display_Registry(256);
    Display_Registry["Backlight"] = 75;
    Display_Registry["Baud Rate"] = 921600;
    File Temporary_File = Drive->open(Display_Registry_Path, FILE_WRITE);
    serializeJson(Display_Registry, Temporary_File);
    Display_Registry.clear();
    Temporary_File.close();

    // Network registry
    DynamicJsonDocument Network_Registry(256);
    Network_Registry["Host name"] = "ESP32";
    Network_Registry["Number WiFi AP"] = 0;
    Temporary_File = Drive->open(Network_Registry_Path, FILE_WRITE);
    serializeJson(Display_Registry, Temporary_File);
    Network_Registry.clear();
    Temporary_File.close();

    // Regional registry
    DynamicJsonDocument Regional_Registry(256);
    Temporary_File = Drive->open(Regional_Registry_Path, FILE_WRITE);
    serializeJson(Regional_Registry, Temporary_File);
    Regional_Registry.clear();
    Temporary_File.close();

    // Software registry
    DynamicJsonDocument Software_Registry(256);
    Temporary_File = Drive->open(Software_Registry_Path, FILE_WRITE);
    Software_Registry.clear();
    Temporary_File.close();
  }*/

  void Pictviewer()
  {
  }
  /*Nextion_Serial.print(F("page Pictviewer\xFF\xFF\xFF"));
  Nextion_Serial.print(F("FILENAME_TXT.txt=\""));
  Nextion_Serial.print(Temporary_File_Name);
  Nextion_Serial.print(F("\"\xFF\xFF\xFF"));
  unsigned int Width;
  unsigned int Height;
  unsigned int Size;
  unsigned long Data_offset;
  unsigned int Encoding;
  unsigned int Array_Size;
  byte Red;
  byte Green;
  byte Blue;
  unsigned int Color;
  Serial.print(F("Open path :"));
  Serial.println(Temporary_File_Path);
  Temporary_File = Drive->open(Temporary_File_Path);
  if (Temporary_File)
  {
    //Read the header
    Temporary_File.seek(0);
    if (Temporary_File.read() == 66 && Temporary_File.read() == 77)
    {
      Serial.println(F("It's a Bitmap File"));
      Temporary_File.seek(2);
      Size = int(Temporary_File.read()); //in bytes
      Serial.print(F("Size :"));
      Serial.println(Size);
      Nextion_Serial.print(F("SIEZ_NUM.val="));
      Nextion_Serial.print(Size);
      Nextion_Serial.print(F("\xFF\xFF\xFF"));
      Temporary_File.seek(10);
      Data_offset = long(Temporary_File.read());
      Serial.print(F("Data Offset :"));
      Serial.println(Data_offset);
      Temporary_File.seek(18);
      Width = int(Temporary_File.read());
      Serial.print(F("Width :"));
      Serial.println(Width);
      Nextion_Serial.print(F("WIDTH_NUM.val="));
      Nextion_Serial.print(Width);
      Nextion_Serial.print(F("\xFF\xFF\xFF"));
      Temporary_File.seek(22);
      Height = long(Temporary_File.read());
      Nextion_Serial.print(F("HEIGH_NUM.val="));
      Nextion_Serial.print(Height);
      Nextion_Serial.print(F("\xFF\xFF\xFF"));
      Serial.print(F("Height :"));
      Serial.println(Height);
      Temporary_File.seek(28);
      Encoding = int(Temporary_File.read());
      Serial.print(F("Encoding :"));
      Serial.println(Encoding);
      Height += 24;
      Width += 10;
      Temporary_File.seek(Data_offset);
      Serial.println(Temporary_File.position());
      if (Encoding == 16)
      {
        //Draw picture
        Serial.println(F("16 bit encoding"));
        for (int y = Height; y > 24; y--)
        {
          for (int x = 10; x < Width; x++)
          {
            Serial.println(Temporary_File.peek());
            Blue = Temporary_File.read();
            Serial.println(Temporary_File.peek());
            Green = Temporary_File.read();
            Color = Blue;
            Color = Color << 8;
            Color = Color | Green;
            Serial.println(Color);
            Nextion_Serial.print(F("fill "));
            Nextion_Serial.print(x);
            Nextion_Serial.write(0x2c);
            Nextion_Serial.print(y);
            Nextion_Serial.write(0x2c);
            Nextion_Serial.print(1);
            Nextion_Serial.write(0x2c);
            Nextion_Serial.print(1);
            Nextion_Serial.write(0x2c);
            Nextion_Serial.print(Color);
            Nextion_Serial.write(0xff);
            Nextion_Serial.write(0xff);
            Nextion_Serial.write(0xff);
          }
        }
      }
      else if (Encoding == 24)
      {
        Serial.println(F("24 bit encoding"));
        for (int y = Height; y > 24; y--)
        {
          for (int x = 10; x < Width; x++)
          {
            Serial.println(Temporary_File.peek());
            Blue = Temporary_File.read();
            Serial.println(Temporary_File.peek());
            Green = Temporary_File.read();
            Serial.println(Temporary_File.peek());
            Red = Temporary_File.read();
            Color = Red << 6;
            Color = Color | Green;
            Color = Color << 5;
            Color = Color | Blue;
            Serial.println(Color);
            Nextion_Serial.print(F("fill "));
            Nextion_Serial.print(x);
            Nextion_Serial.write(0x2c);
            Nextion_Serial.print(y);
            Nextion_Serial.write(0x2c);
            Nextion_Serial.print(1);
            Nextion_Serial.write(0x2c);
            Nextion_Serial.print(1);
            Nextion_Serial.write(0x2c);
            Nextion_Serial.print(map(Color, 0, 16777215, 0, 65535));
            Nextion_Serial.write(0xff);
            Nextion_Serial.write(0xff);
            Nextion_Serial.write(0xff);
          }
          Temporary_File.read();
          Temporary_File.read();
        }
      }

      else
      {
        Serial.println(F("The 32 bit encoding isn't be supported now"));
      }
    }
    else
    {
      Serial.println(F("It isn't a bitmap file"));
    }
  }
  else
  {
    Serial.println(F("The File Doesn't Exist or isn't readable"));
  }*/

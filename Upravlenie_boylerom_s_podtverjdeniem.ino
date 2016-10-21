#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>

//////////////////////////////////////////////////////////////////////////
//         Назначения выводов   //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
                                                                        //
#define ONE_WIRE_BUS 10                 // Пин на датчик темпиратуры    //
#define RELAY 4                         // Переменная реле              //
                                                                        //
#define PIN_RX 2                        // Пин RX GSM                   //
#define PIN_TX 3                        // Пин TX GSM                   //
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//         Параметры программы  //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
                                                                        //
#define GSM_SPEED  19200                 // Скорость порта ардуино      //
                                                                        //
#define MIN_TEMP  27                     // Темпиратура минимум         //
#define MAX_TEMP  30                     // Темпиратура максимум        //
                                                                        //
#define NUMBER_ONE  "+79991108854"       // Первый номер телефона       //
#define NUMBER_TWO  "+79063978854"       // Второй номер телефона       //
                                                                        //
#define COMMAND_ON  "On"                 // Команда на включение        //
#define COMMAND_OFF "Off"                // Команда на выключение       //
                                                                        //
DeviceAddress Thermometer = {                                           //
  0x28, 0xFF, 0x35, 0x5D, 0x60, 0x16, 0x05, 0x87 };  // адрес DS18B20   //
                                                                        //
#define ON  HIGH                        // Состояние порта ввода/вывода //
#define OFF LOW                         // Состояние порта ввода/вывода //
                                                                        //
#define AT_SLEEP  500                   // Задержка после АТ команды    //
#define SENSOR_TIME 10000               // Интервал опроса DS18B20      //
                                                                        //
//////////////////////////////////////////////////////////////////////////

SoftwareSerial gsm(PIN_RX, PIN_TX);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float t_temp = 0, temp = 0;
unsigned long currentTime = 0, loopTime = 0;

uint8_t ch = 0;
String val = "";

boolean status_sms_min_temp = FALSE;
boolean status_sms_max_temp = FALSE;

//////////////////////////////////////////////////////////////////////////
//  Функция настройки программы //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void setup() {
    // Инициализация пинов на выходы
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, OFF); 

    Serial.begin(9600);
    
    // Инициализация GSM
    gsm.begin(GSM_SPEED); delay(AT_SLEEP);
    gsm.println("AT"); delay(AT_SLEEP);
    gsm.print("AT+CMGF=1\r"); delay(AT_SLEEP);
    gsm.print("AT+CSCS=\"GSM\"\r"); delay(AT_SLEEP);
        
    // Инициализация датчиков темпиратуры
    sensors.begin(); sensors.setResolution(Thermometer, ONE_WIRE_BUS);
}

//////////////////////////////////////////////////////////////////////////
//  Функция отправки смс    //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void sms(String text, String phone) {  
    gsm.println("AT+CMGS=\"" + phone + "\""); delay(AT_SLEEP);
    gsm.print(text); delay(AT_SLEEP);
    gsm.print((char)26); delay(AT_SLEEP);
}

//////////////////////////////////////////////////////////////////////////
//  Функция основной цикл   //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void loop()
{
    currentTime = millis(); // получаем текущее время в миллисекундах со старта программы, каждый раз перезаписывая

    if(currentTime >= (loopTime + SENSOR_TIME)) { // считываем датчик темпиратуры каждые секунду
        sensors.requestTemperatures(); 
        t_temp = sensors.getTempC(Thermometer); delay(AT_SLEEP * 2); // считываем темпиратуру
        if(t_temp != float(-127)) temp = t_temp;
        
        if(temp < MIN_TEMP) { // Если темпиратура равна или менее минимальной
            if(!status_sms_min_temp)
            {
                // Отправляем СМС оповещение TEMP
                sms("TEMP: " + String(temp) + "\n" + "Отключили горячую воду", String(NUMBER_ONE)); //отправляем СМС
                delay(AT_SLEEP * 5);
                sms("TEMP: " + String(temp) + "\n" + "Отключили горячую воду", String(NUMBER_TWO)); //отправляем СМС
                delay(AT_SLEEP * 5);
                
                // Меняем статус - на отправлено
                status_sms_min_temp = TRUE;
            } 
        }
        else if(temp >= MIN_TEMP && temp <= MAX_TEMP)
        {
            status_sms_min_temp = FALSE;
            status_sms_max_temp = FALSE;
        }
        else if(temp > MAX_TEMP) { // Если темпиратура равна или более максимальной
            if(!status_sms_max_temp)
            {
                // Отправляем СМС оповещение TEMP
                sms("TEMP: " + String(temp) + "\n" + "Включили горячую воду", String(NUMBER_ONE)); //отправляем СМС
                delay(AT_SLEEP * 5);
                sms("TEMP: " + String(temp) + "\n" + "Включили горячую воду", String(NUMBER_TWO)); //отправляем СМС
                delay(AT_SLEEP * 5);
                
                // Меняем статус на отправлено
                status_sms_max_temp = TRUE;
            }
        }
        
        loopTime = currentTime; 
    }

    
    //если GSM модуль что-то послал нам, то    
    if(gsm.available())
    {   
        delay(200); // Ожидаем заполнения буфера
        
        //сохраняем входную строку в переменную val, посимвольно в строку
        while (gsm.available()) {  
            ch = gsm.read();
            val += char(ch);
            delay(10);
        }

        Serial.println(val);
        
        //если пришло смс, читаем команду
        if (val.startsWith("+CMT") > -1)
        {
            if (val.indexOf(COMMAND_ON) > -1) 
            { 
                sms("ON", String(NUMBER_ONE)); //отправляем СМС
                delay(AT_SLEEP * 5);
                sms("ON", String(NUMBER_TWO)); //отправляем СМС
                delay(AT_SLEEP * 5);

                digitalWrite(RELAY, ON); delay(AT_SLEEP * 2); 
            }
            
            if (val.indexOf(COMMAND_OFF) > -1)
            { 
                sms("OFF", String(NUMBER_ONE)); //отправляем СМС
                delay(AT_SLEEP * 5);
                sms("OFF", String(NUMBER_TWO)); //отправляем СМС
                delay(AT_SLEEP * 5);
                
                digitalWrite(RELAY, OFF); delay(AT_SLEEP * 2);
            } 
        }
        
        val = ""; // Очищаем переменную команды

        delay(AT_SLEEP * 5);
        gsm.print("AT+CMGD=1,4\r\n"); // Удаляем все смс
        delay(AT_SLEEP * 5);
    }
    
}

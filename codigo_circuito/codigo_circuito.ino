#include <LiquidCrystal.h>
#include <AsyncTaskLib.h>
#include <Keypad.h>
#include <DHT.h>

/* Display */
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

/* Keypad setup */
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 3;
byte rowPins[KEYPAD_ROWS] = { 5, 4, 3, 2 };  // R1 = 5, R2 = 4, R3 = 3. R4 = 2
byte colPins[KEYPAD_COLS] = { A3, A2, A1 };
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

char password[5] = "1234";  // contraseña: 1234
char inputPassword[5];      // Almacena la clave ingresada
unsigned char idxPassword = 0;
unsigned char idxAsterisks = 0;
//
int intentosFallidos = 0;  // intentos fallidos 

// Pins del LED RGB
const int ledRojo = 14; 
const int ledVerde = 15; 
const int ledAzul = 16; 

// Pin del sensor de luz (fotoresistor)
const int sensorLuz = A8; 

// Pin del sensor DHT22
const int dhtPin = 28; 

// Crear un objeto DHT para el sensor DHT22
DHT dht(dhtPin, DHT22);

// Estado del sistema
enum Estado { 
  INICIO, 
  VALIDACION_CONTRASENA,
  BLOQUEADO,
  ALARMATEMPERATURA,
  MOSTRAR_VALORES  // Agregado el estado para mostrar los valores
};

Estado estadoActual = INICIO;

// Variable para almacenar el estado anterior
Estado estadoAnterior = MOSTRAR_VALORES;

// Definición de las tareas asíncronas
void valorTemperatura(void); 
AsyncTask TemperatureTask(2500, true, valorTemperatura);

void valorHumedad(void); 
AsyncTask HumidityTask(2000, true, valorHumedad);

void valorLuz(void); 
AsyncTask LightTask(1500, true, valorLuz);

// Definir una tarea asíncrona para el bloqueo
void funcionalidadLed(void); 
AsyncTask LedTask(650, true, funcionalidadLed);

void funcionalidadLed_alarma(void); 
AsyncTask LedTaskAlarm(650, true, funcionalidadLed_alarma);

void funcionalidadTiempo(void); 
AsyncTask TimeTask(10000, false, funcionalidadTiempo);

void funcionalidadTiempo_alarma(void); 
AsyncTask TimeTaskAlarm(3000, false, funcionalidadTiempo_alarma);

void funcionalidadledAlarmaBloqueo(void); 
AsyncTask LedAlarmBloq(800, true, funcionalidadledAlarmaBloqueo);

void funcionalidadledAlarmaTemperatura(void); 
AsyncTask LedAlarmTemp(500, true, funcionalidadledAlarmaTemperatura);

int ledState = LOW;

void funcionalidadLed(void) {
  ledState = !ledState;
  digitalWrite(ledRojo, ledState);
}

void funcionalidadLed_alarma(void) {
  ledState = !ledState;
  digitalWrite(ledAzul, ledState);
}

void funcionalidadledAlarmaBloqueo(void) {
  unsigned long startTime = millis();
  while (millis() - startTime < 300) {
    // Azul
    analogWrite(ledRojo, 255);
  }
  // Apagar el LED RGB
  analogWrite(ledRojo, 0);
}

void funcionalidadledAlarmaTemperatura(void) {
  unsigned long startTime = millis();
  while (millis() - startTime < 3000) {
    // Azul
    analogWrite(ledAzul, 255);
  }

  // Apagar el LED RGB
  analogWrite(ledAzul, 0);
}

void funcionalidadTiempo(void) {
  LedTask.Stop();
  analogWrite(ledRojo, ledState);
  analogWrite(ledVerde, 0);
  analogWrite(ledAzul, 0);
  lcd.clear();
  intentosFallidos = 0;
  // Verificar el estado anterior al entrar en el estado bloqueado
  if (estadoAnterior == MOSTRAR_VALORES) {
    LedAlarmBloq.Start();
    estadoActual = MOSTRAR_VALORES;  // Restablecer el estado anterior

  } else {
    // Si venimos de otro estado, mantenemos el bloqueo indefinidamente
    estadoActual = INICIO;  // Restablecer el estado anterior
  }
}

void funcionalidadTiempo_alarma(void) {
  LedTaskAlarm.Stop();
  analogWrite(ledAzul, ledState);
  analogWrite(ledVerde, 0);
  analogWrite(ledAzul, 0);
  lcd.clear();
  // Verificar el estado anterior al entrar en el estado bloqueado
  if (estadoAnterior == MOSTRAR_VALORES) {
    LedAlarmTemp.Start();
    estadoActual = MOSTRAR_VALORES;  // Restablecer el estado anterior
  }
}

unsigned long lastKeyPressTime = 0;  // Variable para almacenar el tiempo del último ingreso del teclado
unsigned long startTime = 0;         // Variable para almacenar el tiempo de inicio de la visualización

unsigned char flagInicio = 0;

void setup() {
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAzul, OUTPUT);

  // Encender solo el color azul al inicio
  analogWrite(ledRojo, 255);
  analogWrite(ledVerde, 255);
  analogWrite(ledAzul, 255);
  startTime = millis();  // Inicializa el tiempo de inicio
  delay(2000);           // Mantener encendido durante 3 segundos

  // Apagar todos los LED RGB
  analogWrite(ledRojo, 0);
  analogWrite(ledVerde, 0);
  analogWrite(ledAzul, 0);

  lcd.begin(16, 2);
  dht.begin();
  estadoActual = INICIO;
}

void loop() {
  LedTask.Update();
  LedAlarmBloq.Update();
  LedAlarmTemp.Update();
  TimeTask.Update();
  TimeTaskAlarm.Update();
  TemperatureTask.Update();
  HumidityTask.Update();
  LightTask.Update();

  char key = keypad.getKey();

  switch (estadoActual) {
    case INICIO:
      analogWrite(ledRojo, 0);
      if (idxPassword == 0 && flagInicio == 0) {
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("Contraseña:");
        lcd.setCursor(6, 1);  // ***
        flagInicio = 1;
      }  //end if
      if (key) {
        if (idxPassword < 4) {
          inputPassword[idxPassword++] = key;
          lcd.print("*");
          idxAsterisks++;
        }
        if (idxPassword == 4) {
          inputPassword[idxPassword] = '\0';
          estadoActual = VALIDACION_CONTRASENA;
          estadoAnterior = INICIO;
        }
      }  //fin if
      break;

    case VALIDACION_CONTRASENA:
      if (strcmp(inputPassword, password) == 0) {
        analogWrite(ledVerde, 255);
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Contraseña");
        lcd.setCursor(4, 1);
        lcd.print("correcta");
        estadoActual = MOSTRAR_VALORES;  // Cambiado a MOSTRAR_VALORES
        estadoAnterior = VALIDACION_CONTRASENA;
      } else {
        analogWrite(ledRojo, 255);
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Contraseña");
        lcd.setCursor(4, 1);
        lcd.print("Incorrecta");
        intentosFallidos++;
        flagInicio = 0;
        if (intentosFallidos >= 3) {
          estadoActual = BLOQUEADO;
          estadoAnterior = VALIDACION_CONTRASENA;
        } else {
          estadoActual = INICIO;
          estadoAnterior = VALIDACION_CONTRASENA;
        }
      }
      delay(1000);
      idxPassword = 0;
      idxAsterisks = 0;  // Reiniciar el contador de asteriscos
      break;

    case BLOQUEADO:
      if (!TimeTask.IsActive()) {
        // Verificar el estado anterior al entrar en el estado bloqueado
        if (estadoAnterior == MOSTRAR_VALORES) {
          TimeTask.Start();
          LedAlarmBloq.Start();
          TemperatureTask.Stop();
          HumidityTask.Stop();
          LightTask.Stop();
          lcd.clear();
          lcd.setCursor(5, 0);
          lcd.print("Bloqueado");
          lcd.setCursor(3, 2);
          lcd.print("L <40 T >30 ");
        } else {
          // Si venimos de otro estado, mantenemos el bloqueo indefinidamente
          LedTask.Start();
          TimeTask.Start();
          TemperatureTask.Stop();
          HumidityTask.Stop();
          LightTask.Stop();
          lcd.clear();
          lcd.setCursor(4, 0);
          lcd.print("Blocked");
          lcd.setCursor(4, 2);
          lcd.print("System");
        }
      }
      break;

    case ALARMATEMPERATURA:
      if (!TimeTaskAlarm.IsActive()) {
        if (estadoAnterior == MOSTRAR_VALORES) {
          TimeTaskAlarm.Start();
          LedAlarmTemp.Start();
          TemperatureTask.Stop();
          HumidityTask.Stop();
          LightTask.Stop();
          lcd.clear();
          lcd.setCursor(5, 0);
          lcd.print("ALARMA");
          lcd.setCursor(3, 2);
          lcd.print(" Temp > 25 ");
        }

        break;

        case MOSTRAR_VALORES:
          LedAlarmBloq.Stop();
          LedAlarmTemp.Stop();
          analogWrite(ledVerde, 0);
          // Lógica para mostrar valores
          if (!TemperatureTask.IsActive()) {
            lcd.clear();
            TemperatureTask.Start();
            HumidityTask.Start();
            LightTask.Start();
          }
          if (analogRead(sensorLuz) <= 40 && dht.readTemperature() >= 27) {  // Si los valores de luz y temperatura cumplen con las condiciones
            lcd.clear();
            estadoActual = BLOQUEADO;
            estadoAnterior = MOSTRAR_VALORES;
          }
          if (dht.readTemperature() > 28) {  // si los valores de temperatura cuymplen las condiciones
            lcd.clear();
            estadoActual = ALARMATEMPERATURA;
            estadoAnterior = MOSTRAR_VALORES;
          }
          break;
      }
  }
}

void valorTemperatura(void) {
  float temperatureValue = dht.readTemperature();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperatureValue);
  lcd.print(" C");
}

void valorHumedad(void) {
  float humidityValue = dht.readHumidity();
  lcd.setCursor(0, 1);
  lcd.print("Humedad: ");
  lcd.print(humidityValue);
  lcd.print(" %");
}

void valorLuz(void) {
  int lightValue = analogRead(sensorLuz);
  lcd.setCursor(0, 2);
  lcd.print("Luz: ");
  lcd.print(lightValue);
}

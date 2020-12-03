/*
  Programa para controlar las 2 ventanas y seguros de una Yukon 94 version Esclava (sin bluethooth:

  Este programa es para la primer version de tarjeta fabricada en china en la empresa JLCPCB, esta version fue diseñada para trabajar como independiente o sclava
  sin modulo bluethoot es esclava y con modulo bluethoot independiente.

  En el modo esclava recive instrucciones desde un modulo principal (Power Interface Board EL JEFE) que contiene el modulo bluethooth y procesa los comando de voz

  En el modo independiente el modulo bluethooth esta incluido y las instrucciones las recibe directamente en ella desde un smartphone.

  Esta tarjeta esta compuesta por un arduino nano 6 relevadores (4 reles para 2 ventanas y 2 para seguros) el arreglo de 2 relevadores hace posible el cambio de
  polaridad en los motores de ventanas y actuadores de seguros estos son activados por las salidas D3,D5,D7,D9,D12,D13.
  Como circuito de acoplamiento se utilizan 6 optoacopladores que reciben +12 o -12 de los botones de las ventanas y los convierten a 1 o 0's que se introducen al arduino
  por medio de las entradas D2,D4,D6,D8,D10,D11

  Tambien esta incluida en esta tarjeta el sensor de corriente ACS712 que es quien se encarga de enviar al arduino por la entrada analogica A3 los datos de consumo de corriente
  de los motores de seguros y ventanas para que el arduino los use para calculos y mandar detener los motores cuando sense corriente excesiva y proteja los componentes
  cuando una ventana llegue a tope superior o inferior se da un incremento de corriente que el arduino interpretara que debe detener los motores.
  el sistema debe instalarse entre los controles de apertura y el motor para que se pueda dar  el control.

*/

int CSens = A3;                   //Sensor de corriente ACS712
int PupIn = 2;                    //Señal que indica que se presiono la tecla ventana PILOTO ARRIBA, "0" presionada, "1" no presionada.
int PdnIn = 4;                    //Señal que indica que se presiono la tecla ventana PILOTO ABAJO, "0" presionada, "1" no presionada.
int CupIn = 6;                    //Señal que indica que se presiono la tecla ventana COPILOTO ARRIBA, "0" presionada, "1" no presionada.
int CdnIn = 8;                    //Señal que indica que se presiono la tecla ventana COPILOTO ABAJO, "0" presionada, "1" no presionada.
int LockIn = 10;                  //Señal que indica que se presiono la tecla LOCK, "0" presionada, "1" no presionada.
int UnlockIn = 11;                //Señal que indica que se presiono la tecla UNLOCK, "0" presionada, "1" no presionada.
int relayCdn = 5;                 //Salida para relay que baja la ventana del copiloto
int relayCup = 3;                 //Salida para relay que sube la ventana del copiloto
int relayPdn = 9;                 //Salida para relay que activa que baja la ventana del piloto
int relayPup = 7;                 //Salida para relay que activa que sube la ventana del copiloto
int relayLock = 13;               //Salida para relay que activa el seguro de las puertas
int relayUnlock = 12;             //Salida para relay que activa la desactivacion de seguros


unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;
int lastButtonState = LOW;
int buttonState;
int Count_Mode = 0;
String comando;                      //variable para almacenar el texto proveniente de Power Interface Board EL JEFE


//Variables para controles de las ventanas y seguros
int SWcenter = A7;                   //Entrada analogica para seguro
int Ampmax = 750;                    //740 Valor maximo de corriente proporcional en binario (valor del sensor sin consumo es de 440 y 441).
int SWstate;                         //Almacena el estado del switch
int SensLectura;                     //Variable que almacena el valor leido del sensor de corriente;
int updnmaxtime = 10000;             //Tiempo maximo limite en miliseg para que suba o baje el vidrio, despues de transcurrir este tiempo el motor se detendra en caso de que exceda
//este limite y no reciba señal de paro por el sensor de corriente.
unsigned long T7 = 0;                //Variable para almacenar el tiempo transcurrido de la rutina de ventana up & down.
unsigned long TmotorStartPup = 0;    //Variable para almacenar el tiempo transcurrido desde que se arranco el motor para Piloto arriba, el motor al arranque consume mucha corriente por lo que le daremos 300 segundos para que se estabilize y despues de eso se empezaran a hacer las lecturas de corriente para el paro automatico del motor de la ventana
unsigned long TmotorStartPdn = 0;    //Variable para almacenar el tiempo transcurrido desde que se arranco el motor para Piloto abajo, el motor al arranque consume mucha corriente por lo que le daremos 300 segundos para que se estabilize y despues de eso se empezaran a hacer las lecturas de corriente para el paro automatico del motor de la ventana
unsigned long TmotorStartCup = 0;    //Variable para almacenar el tiempo transcurrido desde que se arranco el motor para Copiloto arriba, el motor al arranque consume mucha corriente por lo que le daremos 300 segundos para que se estabilize y despues de eso se empezaran a hacer las lecturas de corriente para el paro automatico del motor de la ventana
unsigned long TmotorStartCdn = 0;    ////Variable para almacenar el tiempo transcurrido desde que se arranco el motor para Copiloto abajo, el motor al arranque consume mucha corriente por lo que le daremos 300 segundos para que se estabilize y despues de eso se empezaran a hacer las lecturas de corriente para el paro automatico del motor de la ventana

int FRPup = 0;                       //Variables para controlar los vidrios remotamente de manera parcial
unsigned long TFRPup = 0;            //Variable que almacena el tiempo transcurrido desde que se llamo la rutina de subir el vidiro con tecla de app del cel
int FRPdw = 0;
unsigned long TFRPdw = 0;
int FRCup = 0;
unsigned long TFRCup = 0;
int FRCdw = 0;
unsigned long TFRCdw = 0;
unsigned long Tt6 = 0;
int ontime = 600;                    //Tiempo en miliseg que estara el motor de la ventana encendido en parcialidad, cuando se use la tecla de la app
int swemul = 0;
int FLSW = 0;                        //Bandera de rutina que procesa lock/unlock
int LockRutine = 0;                  //Bandera de rutina lock
int UnlockRutine = 0;                //Bandera de rutina unlock
unsigned long TimerLSW = 0;          //Variable donde se alamcenara el tiempo transcurrido al arrancar el reloj de la rutina lock.
unsigned long Tt5 = 0;               //Tiempo transcurrido (Tt5) alamcenara el tiempo transcurrido.
int SLSW = 0;                        //Variable que almacena el ultimo estado del switch (para que pueda hacer el toggle con un solo switch)
int LSWread = 0;                     //Variable para almacenar estado de switch center
int FGesto = 0;                      //Bandera de rutina llamada gesto.
int FChecaSw = 0;                    //Bandera de rutina llamada ChecaSw.
int FProcesando = 0;                 //Bandera de rutina llamada ChecaSw.
unsigned long timestart = 0;         //Variable donde se alamcenara el tiempo transcurrido al arrancar el reloj.
unsigned long Tt = 0;                //Tiempo transcurrido (Tt) alamcenara el tiempo transcurrido.
unsigned long GTupdncount = 0;       //Variable que contara el tiempo subiendo o bajando de la ventana como medida de seguridad para que no quede encendido por siempre y se queme.
int Pupminon = 780;                  //Valor minimo que toma para considerar que la tecla Pup fue presionada 10.
int Pupmaxon = 880;                  //Valor maximo que toma para considerar que la tecla Pup fue presionada 20.
int F2Gesto = 0;                     //Bandera de rutina llamada gesto del switch2.
int F2ChecaSw = 0;                   //Bandera de rutina llamada ChecaSw del switch2.
int F2Procesando = 0;                //Bandera de rutina llamada ChecaSw del switch2.
unsigned long timestart2 = 0;        //Variable donde se alamcenara el tiempo transcurrido al arrancar el reloj del switch2.
unsigned long Tt2 = 0;               //Tiempo transcurrido (Tt) alamcenara el tiempo transcurrido del switch2.
unsigned long G2Tupdncount = 0;      //Variable que contara el tiempo subiendo o bajando de la ventana como medida de seguridad para que no quede encendido por siempre y se queme.
int Pdnminon = 510;                  //Valor minimo que toma para considerar que la tecla Pdn fue presionada 510.
int Pdnmaxon = 650;                  //Valor maximo que toma para considerar que la tecla Pdn fue presionada 650.
int F3Gesto = 0;                     //Bandera de rutina llamada gesto del switch2.
int F3ChecaSw = 0;                   //Bandera de rutina llamada ChecaSw del switch2.
int F3Procesando = 0;                //Bandera de rutina llamada ChecaSw del switch2.
unsigned long timestart3 = 0;        //Variable donde se alamcenara el tiempo transcurrido al arrancar el reloj del switch2.
unsigned long Tt3 = 0;               //Tiempo transcurrido (Tt) alamcenara el tiempo transcurrido del switch2.
unsigned long G3Tupdncount = 0;      //Variable que contara el tiempo subiendo o bajando de la ventana como medida de seguridad para que no quede encendido por siempre y se queme.
int Cupminon = 220;                  //Valor minimo que toma para considerar que la tecla + fue presionada 220.
int Cupmaxon = 350;                  //Valor maximo que toma para considerar que la tecla + fue presionada 350.
int F4Gesto = 0;                     //Bandera de rutina llamada gesto del switch2.
int F4ChecaSw = 0;                   //Bandera de rutina llamada ChecaSw del switch2.
int F4Procesando = 0;                //Bandera de rutina llamada ChecaSw del switch2.
unsigned long timestart4 = 0;        //Variable donde se alamcenara el tiempo transcurrido al arrancar el reloj del switch2.
unsigned long Tt4 = 0;               //Tiempo transcurrido (Tt) alamcenara el tiempo transcurrido del switch2.
unsigned long G4Tupdncount = 0;      //Variable que contara el tiempo subiendo o bajando de la ventana como medida de seguridad para que no quede encendido por siempre y se queme.
int Cdnminon = 5;                    //Valor minimo que toma para considerar que la tecla + fue presionada 465.
int Cdnmaxon = 130;                  //Valor maximo que toma para considerar que la tecla + fue presionada 485.
//int SWminoff = 950;                  //Valor minimo que toma para considerar que ningun switch fue presionado 1000.
//int SWmaxoff = 1050;                 //Valor maximo que toma para considerar que ningun switch fue presionado 1020.
int PFGesto = 1;                     //Permiso Piloto arriba,Permiso para entrar a rutina de gestos y ayudar a que se suban ventanas 1x1.
int PF2Gesto = 1;                    //Permiso Piloto abajo,Permiso para entrar a rutina de gestos y ayudar a que se suban ventanas 1x1.
int PF3Gesto = 1;                    //Permiso Copiloto arriba,Permiso para entrar a rutina de gestos y ayudar a que se suban ventanas 1x1.
int PF4Gesto = 1;                    //Permiso Copiloto abajo,Permiso para entrar a rutina de gestos y ayudar a que se suban ventanas 1x1.
int Pup_Cupmaxon = 0;        //Colocar valor que debe ser//Valor maximo cuando se presionan las teclas de ventanas de piloto y copiloto arriba.
int Pup_Cupminon = 0;        //Colocar valor que debe ser//Valor minimo cuando se presionan las teclas de ventanas de piloto y copiloto arriba.
int Pdn_Cdnmaxon = 0;        //Colocar valor que debe ser//Valor maximo cuando se presionan las teclas de ventanas de piloto y copiloto abajo.
int Pdn_Cdnminon = 0;        //Colocar valor que debe ser//Valor minimo cuando se presionan las teclas de ventanas de piloto y copiloto abajo.
int FChecaSWPup_Cup = 0;             //Bandera de rutina para cuando se presionan las dos teclas arriba piloto y copiloto (cerrar ventanas).
int FChecaSWPdn_Cdn = 0;             //Bandera de rutina para cuando se presionan las dos teclas abajo piloto y copiloto (abrir ventanas).
unsigned long G5Tupcount = 0;        //Variable que contara el tiempo subiendo o bajando de la ventana como medida de seguridad para que no quede encendido por siempre y se queme.
unsigned long G6Tdncount = 0;         //Variable que contara el tiempo subiendo o bajando de la ventana como medida de seguridad para que no quede encendido por siempre y se queme.

void setup() {
  Serial.begin(9600);                 //inicializacion de comunicacion serial con power interface board EL JEFE
  //entradas de optoacopladores, que estan conectados al master switch
  pinMode(PupIn, INPUT);              //PILOTO ARRIBA, como entrada.
  pinMode(PdnIn, INPUT);              //PILOTO ABAJO, como entrada.
  pinMode(CupIn, INPUT);              //COPILOTO ARRIBA, como entrada.
  pinMode(CdnIn, INPUT);              //COPILOTO ABAJO, como entrada.
  pinMode(LockIn, INPUT);             //LOCK, como entrada.
  pinMode(UnlockIn, INPUT);           //UNLOCK, como entrada.
  //salidas de relay para ventanas
  pinMode(relayCdn, OUTPUT);          //Copiloto abajo como salida
  pinMode(relayCup, OUTPUT);          //Copiloto arriba como salida
  pinMode(relayPup, OUTPUT);          //Piloto arriba como salida
  pinMode(relayPdn, OUTPUT);          //Piloto abajo como salida
  //salidas de relay para seguros
  pinMode(relayLock, OUTPUT);         //Lock como salida
  pinMode(relayUnlock, OUTPUT);       //Unlock como salida
  //Inicializacion de salidas de relay
  digitalWrite(relayPup, HIGH);
  digitalWrite(relayPdn, HIGH);
  digitalWrite(relayCup, HIGH);
  digitalWrite(relayCdn, HIGH);
  digitalWrite(relayLock, HIGH);
  digitalWrite(relayUnlock, HIGH);
}

void loop() {
Inicio:                                            // etiqueta para regresar a inicio
continua: if (Serial.available())                  // Serial communication
  {
    while (Serial.available())
    {
      delay(10);
      char c = Serial.read();
      if (c == '#')
      {
        break;
      }
      += c;
    }
    if (comando.length() > 0)
    {
      if (comando == "sierra" || comando == "Sierra")
      {
        //Serial.println("Cerrando Ventanas");
        FGesto = 1;
        GTupdncount = millis();
        F3Gesto = 1;
        G3Tupdncount = millis();
        goto sigue;
      }
      else if (comando == "abre" || comando == "Abre")
      {
        //Serial.println("Abriendo Ventanas");
        F2Gesto = 1;
        G2Tupdncount = millis();
        F4Gesto = 1;
        G4Tupdncount = millis();
        goto sigue;
      }
      if (comando == "Seguros" || comando == "seguros")
      {
        //Serial.println("asegurando");
        LockRutine = 1;
        TimerLSW = millis();
        goto sigue;
      }
      else if (comando == "Seguros fuera" || comando == "seguros fuera")
      {
        //Serial.println("Sin Seguros");
        UnlockRutine = 1;
        TimerLSW = millis();
        goto sigue;
      }
      if (comando == "Pup")
      {
        FRPup = 1;
        TFRPup = millis();
        goto sigue;
      }
      else if (comando == "Pdw")
      {
        FRPdw = 1;
        TFRPdw = millis();
        goto sigue;
      }
      if (comando == "Cup")
      {
        FRCup = 1;
        TFRCup = millis();
        goto sigue;
      }
      else if (comando == "Cdw")
      {
        FRCdw = 1;
        TFRCdw = millis();
        goto sigue;
      }
      //Serial.println("Comando incorrecto");
    }
sigue: comando = "";
  }

  // "Codigo que controla las ventanas del jefe"
next_step8:  SensLectura = analogRead(CSens);
  if (FGesto == 1)        //inicia control para la tecla ventana piloto arriba
  {
    if (PFGesto == 1)
    {
      F2Gesto = 0;
      F4Gesto = 0;
      PF2Gesto = 0;
      PF3Gesto = 0;
      PF4Gesto = 0;
      digitalWrite(relayPup, HIGH);
      digitalWrite(relayPdn, LOW);
      //int SWstate = analogRead(SWKey); borrar si no se requiere
      if (digitalRead(PdnIn) == LOW)        //Se detiene el motor si se presiona la tecla bajar vidrio piloto (operacion contraria)
      {
        digitalWrite(relayPup, HIGH);
        digitalWrite(relayPdn, HIGH);
        FGesto = 0;
        PFGesto = 1;
        PF2Gesto = 1;
        PF3Gesto = 1;
        PF4Gesto = 1;
        FProcesando = 0;
        F2Procesando = 0;
      }

      T7 = (millis() - GTupdncount);                        //Operacion que checa si ya pasaron 300mS para empezar a sensar la corriente, para evitar paro anticipado de motor
      if (T7 > 300) {                                       //debido a que los motores al arranque consumen corriente excesiva para salir de la inercia
        SensLectura = analogRead(CSens);
        if (SensLectura > Ampmax || T7 >= updnmaxtime )     //Detiene el motor si la demanda de corriente excedio el limite (llego a tope) o excede el tiempo de 10 segundos (updnmaxtime)
        {
          digitalWrite(relayPup, HIGH);
          digitalWrite(relayPdn, HIGH);
          FGesto = 0;
          PFGesto = 1;
          PF2Gesto = 1;
          PF3Gesto = 1;
          PF4Gesto = 1;
          Serial.println("Ventana del piloto cerrada");
        }
      }
    }
    FProcesando = 0;                                      //Detiene el proceso "FProcesando"
    F2Procesando = 0;
  }


  if (FRPup == 1)                     //rutina para procesar control de vidrio up parcial 0.6 segundos, con tecla del cel (app), preparativos para subir la ventana durante 0.6 seg
  {
    Tt6 = millis() - TFRPup;
    if (Tt6 < ontime)
    {
      swemul = 1;
      FChecaSw = 1;
      goto A;
    }
    swemul = 0;
    FRPup = 0;
  }                                 //fin rutina para procesar control de vidrio up parcial 0.6 segundos

A: if (FChecaSw == 1)               //Verifica permiso para entrar a rutina que sube la ventana mientras este presionada el piloto arriba en master switch.
  {
    if (swemul == 1)                //Verifica si se requirio subir la ventana desde comando serial bluetooth (emulador de switch)
    {
      goto H;
    }
    if (digitalRead(PupIn) == LOW)   //Verifica si la tecla Piloto arriba fue presionada
    {
H:    digitalWrite(relayPup, HIGH);
      digitalWrite(relayPdn, LOW);
      if (TmotorStartPup == 0) {
        TmotorStartPup = millis();
      }
      if (millis() - TmotorStartPup > 300) {         //Empieza a monitorear demanda de corriente asta pasados los 300mS
        SensLectura = analogRead(CSens);
        if (SensLectura > Ampmax)
        {
          FChecaSw = 0;
          digitalWrite(relayPup, HIGH);              //Para el motor si la demanda de corriente fue excedida (indica que ventana llego a tope)
          digitalWrite(relayPdn, HIGH);
          //Serial.println("Punto C");
          TmotorStartPup = 0;                        //Inicializa timer de arranque de motor
        }
      }
      FProcesando = 0;                               //Cancela rutina procesando
      F2Procesando = 0;
      goto next;
    }
    FChecaSw = 0;                                    //Para motor de ventana si Piloto arriba se dejo de presionar
    digitalWrite(relayPup, HIGH);
    digitalWrite(relayPdn, HIGH);
    FProcesando = 0;
    F2Procesando = 0;
  }

  if (FProcesando == 1)                             //Esta rutina es para verificar el modo de presionar las teclas para subir ventana mientras se presiona Piloto arriba
  { // dos pulsaciones seguidas en un intervalo de 1 segundo para que suba automatico asta tope
    Tt = millis() - timestart;
    if (60 < Tt && Tt < 70)
    {
      if (digitalRead(PupIn) == LOW)
      {
        digitalWrite(relayPup, HIGH);
        digitalWrite(relayPdn, LOW);
        goto next;
      }
      FProcesando = 0;
    }
    Tt = millis() - timestart;
    if (180 < Tt && Tt < 200)
    {
      SensLectura = analogRead(CSens);
      if (SensLectura > Ampmax)
      {
        //Serial.println("Punto A");
        digitalWrite(relayPup, HIGH);
        digitalWrite(relayPdn, HIGH);
        FProcesando = 0;
        goto next;
      }
      if (digitalRead(PupIn) == HIGH && digitalRead(PdnIn) == HIGH  && digitalRead(CupIn) == HIGH  && digitalRead(CdnIn) == HIGH)) //checa si no se presiono ninguna tecla del master-switch
      {
        goto next;
      }
      FChecaSw = 1;
      FProcesando = 0;
      goto next;
    }
    Tt = millis() - timestart;
    if (280 < Tt && Tt < 380)
    {
      if (digitalRead(PupIn) == LOW)      //Verifica si la tecla ventana piloto arriba fue presionada, "LOW" significa presionada "HIGH" no presionada
      {
        FGesto = 1;
        GTupdncount = millis();
        goto next;
      }
      digitalWrite(relayPup, HIGH);
      digitalWrite(relayPdn, HIGH);
      FProcesando = 0;
      goto next;
    }
    Tt = millis() - timestart;
    if (Tt < 400)
    {
      goto next;
    }
    digitalWrite(relayPup, HIGH);
    digitalWrite(relayPdn, HIGH);
    FProcesando = 0;
    goto next;
  }
  //Termina seccion procesando Pup

  //Inicia Seccion de verificacion de tecla presionada Pup para activar rutina procesando
  if (digitalRead(PupIn) == LOW))         //Verifica si la tecla ventana piloto arriba fue presionada, "LOW" significa presionada "HIGH" no presionada  
  {
    timestart = millis();
    FProcesando = 1;
  }
  //Termina Seccion de verificacion de tecla presionada Pup para activar rutina procesando

  //"Termina control para tecla channel up o vidrio arriba"

next: if (F2Gesto == 1)                   //inicia control para la tecla PdnIn o vidrio piloto abajo
{

  if (PF2Gesto == 1)
    {
      FGesto = 0;
      F3Gesto = 0;
      PFGesto = 0;
      PF3Gesto = 0;
      PF4Gesto = 0;
      digitalWrite(relayPup, LOW);
      digitalWrite(relayPdn, HIGH);
      if (digitalRead(PupIn) == LOW))     //Verifica si la tecla PupIN se presiono, si asi fue detiene el proceso de bajar vidirio o abrir ventana de piloto
      {
        digitalWrite(relayPup, HIGH);
        digitalWrite(relayPdn, HIGH);
        F2Gesto = 0;
        PFGesto = 1;
        PF2Gesto = 1;
        PF3Gesto = 1;
        PF4Gesto = 1;
        FProcesando = 0;
        F2Procesando = 0;
      }
      T7 = (millis() - G2Tupdncount);
      if (T7 > 300) {
      SensLectura = analogRead(CSens);
        if (SensLectura > Ampmax || T7 >= updnmaxtime )
        {
          digitalWrite(relayPup, HIGH);
          digitalWrite(relayPdn, HIGH);
          Serial.println(Ampmax);
          F2Gesto = 0;
          PFGesto = 1;
          PF2Gesto = 1;
          PF3Gesto = 1;
          PF4Gesto = 1;
          Serial.println("Ventana del piloto abierta");
        }
      }
    }
    FProcesando = 0;
    F2Procesando = 0;
  }

  if (FRPdw == 1)                     //rutina para procesar control de vidrio down parcial 0.6 segundos
{
  Tt6 = millis() - TFRPdw;
    if (Tt6 < ontime)
    {
      swemul = 1;
      F2ChecaSw = 1;
      goto B;
    }
    swemul = 0;
    FRPdw = 0;
  }                                 //fin rutina para procesar control de vidrio dw parcial 0.6 segundos

B:  if (F2ChecaSw == 1)
{
  if (swemul == 1)
    {
      goto G;
    }
    if (digitalRead(PdnIn) == LOW)
    {
G: digitalWrite(relayPup, LOW);
      digitalWrite(relayPdn, HIGH);
      SensLectura = analogRead(CSens);
      if (SensLectura > Ampmax)
      {
        F2ChecaSw = 0;
        digitalWrite(relayPup, HIGH);
        digitalWrite(relayPdn, HIGH);
      }
      FProcesando = 0;
      F2Procesando = 0;
      goto next2;
    }
    F2ChecaSw = 0;
    digitalWrite(relayPup, HIGH);
    digitalWrite(relayPdn, HIGH);
    FProcesando = 0;
    F2Procesando = 0;
  }

  if (F2Procesando == 1)
{
  Tt2 = millis() - timestart2;
    if (60 < Tt2 && Tt2 < 70)
    {
      if (digitalRead(PdnIn) == LOW)
      {
        digitalWrite(relayPup, LOW);
        digitalWrite(relayPdn, HIGH);
        goto next2;
      }
      F2Procesando = 0;
    }
    Tt2 = millis() - timestart2;
    if (180 < Tt2 && Tt2 < 200)
    {
      SensLectura = analogRead(CSens);
      if (SensLectura > Ampmax)
      {
        digitalWrite(relayPup, HIGH);
        digitalWrite(relayPdn, HIGH);
        F2Procesando = 0;
        goto next2;
      }
      if (digitalRead(PupIn) == HIGH && digitalRead(PdnIn) == HIGH  && digitalRead(CupIn) == HIGH  && digitalRead(CdnIn) == HIGH)
      {
        goto next2;
      }
      F2ChecaSw = 1;
      F2Procesando = 0;
      goto next2;
    }
    Tt2 = millis() - timestart2;
    if (280 < Tt2 && Tt2 < 380)
    {
      if (digitalRead(PdnIn) == LOW )
      {
        F2Gesto = 1;
        G2Tupdncount = millis();
        goto next2;
      }
      digitalWrite(relayPup, HIGH);
      digitalWrite(relayPdn, HIGH);
      F2Procesando = 0;
      goto next2;
    }
    Tt2 = millis() - timestart2;
    if (Tt2 < 400)
    {
      goto next2;
    }
    digitalWrite(relayPup, HIGH);
    digitalWrite(relayPdn, HIGH);
    F2Procesando = 0;
    goto next2;
  }
  if (digitalRead(PdnIn) == LOW)
  {
    timestart2 = millis();
    F2Procesando = 1;
  }                                                       //Termina control para tecla channel down o vidrio abajo
next2: if (F3Gesto == 1)                                  //inicia control para la tecla vol up o vidrio copiloto arriba
{

  if (PF3Gesto == 1)
    {
      F2Gesto = 0;
      F4Gesto = 0;
      PFGesto = 0;
      PF2Gesto = 0;
      PF4Gesto = 0;
      digitalWrite(relayCup, HIGH);
      digitalWrite(relayCdn, LOW);
      if (digitalRead(CdnIn) == LOW)
      {
        digitalWrite(relayCup, HIGH);
        digitalWrite(relayCdn, HIGH);
        F3Gesto = 0;
        PFGesto = 1;
        PF2Gesto = 1;
        PF3Gesto = 1;
        PF4Gesto = 1;
        F3Procesando = 0;
        F4Procesando = 0;
      }
      T7 = (millis() - G3Tupdncount);
      if (T7 > 300) {
        SensLectura = analogRead(CSens);
        if (SensLectura > Ampmax || T7 >= updnmaxtime )
        {
          digitalWrite(relayCup, HIGH);
          digitalWrite(relayCdn, HIGH);
          Serial.println(Ampmax);
          F3Gesto = 0;
          PFGesto = 1;
          PF2Gesto = 1;
          PF3Gesto = 1;
          PF4Gesto = 1;
          Serial.println("Ventana del piloto cerrada");
        }
      }
    }
    F3Procesando = 0;
    F4Procesando = 0;
  }

  if (FRCup == 1)                     //rutina para procesar control de vidrio down parcial 0.6 segundos
{
  Tt6 = millis() - TFRCup;
    if (Tt6 < ontime)
    {
      swemul = 1;
      F3ChecaSw = 1;
      goto C;
    }
    swemul = 0;
    FRCup = 0;
  }                                 //fin rutina para procesar control de vidrio dw parcial 0.6 segundos

C: if (F3ChecaSw == 1)
{
  if (swemul == 1)
    {
      goto F;
    }
    if (digitalRead(CupIn) == LOW)
    {
F:    digitalWrite(relayCup, HIGH);
      digitalWrite(relayCdn, LOW);

      SensLectura = analogRead(CSens);
      if (SensLectura > Ampmax)
      {
        F3ChecaSw = 0;
        digitalWrite(relayCup, HIGH);
        digitalWrite(relayCdn, HIGH);
      }
      F3Procesando = 0;
      F4Procesando = 0;
      goto next3;
    }
    F3ChecaSw = 0;
    digitalWrite(relayCup, HIGH);
    digitalWrite(relayCdn, HIGH);
    F3Procesando = 0;
    F4Procesando = 0;
  }

  if (F3Procesando == 1)   // tecla + de control
{
  Tt3 = millis() - timestart3;
    if (60 < Tt3 && Tt3 < 70)
    {
      if (digitalRead(CupIn) == LOW)
      {
        digitalWrite(relayCup, HIGH);
        digitalWrite(relayCdn, LOW);
        goto next3;
      }
      F3Procesando = 0;
    }
    Tt3 = millis() - timestart3;
    if (180 < Tt3 && Tt3 < 200)
    {
      SensLectura = analogRead(CSens);
      if (SensLectura > Ampmax)
      {
        digitalWrite(relayPdn, HIGH);
        digitalWrite(relayPup, HIGH);
        F3Procesando = 0;
        goto next3;
      }
      int SWstate = analogRead(SWKey);
      if (digitalRead(PupIn) == HIGH && digitalRead(PdnIn) == HIGH  && digitalRead(CupIn) == HIGH  && digitalRead(CdnIn) == HIGH))
      {
        goto next3;
      }
      F3ChecaSw = 1;
      F3Procesando = 0;
      goto next3;
    }
    Tt3 = millis() - timestart3;
    if (280 < Tt3 && Tt3 < 380)
    {
      int SWstate = analogRead(SWKey);
      if (digitalRead(CupIn) == LOW)
      {
        F3Gesto = 1;
        G3Tupdncount = millis();
        goto next3;
      }
      digitalWrite(relayCup, HIGH);
      digitalWrite(relayCdn, HIGH);
      F3Procesando = 0;
      goto next3;
    }
    Tt3 = millis() - timestart3;
    if (Tt3 < 400)
    {
      goto next3;
    }
    digitalWrite(relayCup, HIGH);
    digitalWrite(relayCdn, HIGH);
    F3Procesando = 0;
    goto next3;
  }
  if (digitalRead(CupIn) == LOW)
  {
    timestart3 = millis();
    F3Procesando = 1;
  }                                                         //  Termina control para tecla vol up o vidrio copiloto arriba
next3: if (F4Gesto == 1)                                    //inicia control para la tecla vol dn o vidrio copiloto abajo
{

  if (PF4Gesto == 1)
    {
      FGesto = 0;
      F3Gesto = 0;
      PFGesto = 0;
      PF2Gesto = 0;
      PF3Gesto = 0;
      digitalWrite(relayCup, LOW);
      digitalWrite(relayCdn, HIGH);
      int SWstate = analogRead(SWKey);
      if (digitalRead(CupIn) == LOW)
      {
        digitalWrite(relayCup, HIGH);
        digitalWrite(relayCdn, HIGH);
        F4Gesto = 0;
        PFGesto = 1;
        PF2Gesto = 1;
        PF3Gesto = 1;
        PF4Gesto = 1;
        F3Procesando = 0;
        F4Procesando = 0;
      }
      T7 = (millis() - G4Tupdncount);
      if (T7 > 300) {
        SensLectura = analogRead(CSens);
        if (SensLectura > Ampmax || T7 >= updnmaxtime )
        {
          digitalWrite(relayCup, HIGH);
          digitalWrite(relayCdn, HIGH);
          F4Gesto = 0;
          PFGesto = 1;
          PF2Gesto = 1;
          PF3Gesto = 1;
          PF4Gesto = 1;
          Serial.println("Ventana del piloto abierta");
        }
      }
    }
    F3Procesando = 0;
    F4Procesando = 0;
  }

  if (FRCdw == 1)                     //rutina para procesar control de vidrio down parcial 0.6 segundos
{
  Tt6 = millis() - TFRCdw;
    if (Tt6 < ontime)
    {
      swemul = 1;
      F4ChecaSw = 1;
      goto D;
    }
    swemul = 0;
    FRCdw = 0;
  }                                 //fin rutina para procesar control de vidrio dw parcial 0.6 segundos

D: if (F4ChecaSw == 1)
{
  if (swemul == 1)
    {
      goto E;
    }
    if (digitalRead(CdnIn) == LOW)
    {
E:    digitalWrite(relayCup, LOW);
      digitalWrite(relayCdn, HIGH);

      SensLectura = analogRead(CSens);
      if (SensLectura > Ampmax)
      {
        F4ChecaSw = 0;
        digitalWrite(relayCup, HIGH);
        digitalWrite(relayCdn, HIGH);
      }
      F3Procesando = 0;
      F4Procesando = 0;
      goto next4;
    }
    F4ChecaSw = 0;
    digitalWrite(relayCup, HIGH);
    digitalWrite(relayCdn, HIGH);
    F3Procesando = 0;
    F4Procesando = 0;
  }

  if (F4Procesando == 1)  //tecla - de control
{
  Tt4 = millis() - timestart4;
    if (60 < Tt4 && Tt4 < 70)
    {
      if (digitalRead(CdnIn) == LOW)
      {
        digitalWrite(relayCup, LOW);
        digitalWrite(relayCdn, HIGH);
        goto next4;
      }
      F4Procesando = 0;
    }
    Tt4 = millis() - timestart4;
    if (180 < Tt4 && Tt4 < 200)
    {
      SensLectura = analogRead(CSens);
      if (SensLectura > Ampmax)
      {
        digitalWrite(relayPdn, HIGH);
        digitalWrite(relayPup, HIGH);
        FProcesando = 0;
        goto next4;
      }
      int SWstate = analogRead(SWKey);
      if (digitalRead(PupIn) == HIGH && digitalRead(PdnIn) == HIGH  && digitalRead(CupIn) == HIGH  && digitalRead(CdnIn) == HIGH)
      {
        goto next4;
      }
      F4ChecaSw = 1;
      F4Procesando = 0;
      goto next4;
    }
    Tt4 = millis() - timestart4;
    if (280 < Tt4 && Tt4 < 380)
    {
      if (digitalRead(CdnIn) == LOW)
      {
        F4Gesto = 1;
        G4Tupdncount = millis();
        goto next4;
      }
      digitalWrite(relayCup, HIGH);
      digitalWrite(relayCdn, HIGH);
      F4Procesando = 0;
      goto next4;
    }
    Tt4 = millis() - timestart4;
    if (Tt4 < 400)
    {
      goto next4;
    }
    digitalWrite(relayCup, HIGH);
    digitalWrite(relayCdn, HIGH);
    F4Procesando = 0;
    goto next4;

  }
  if (digitalRead(CdnIn) == LOW)
  {
    timestart4 = millis();
    F4Procesando = 1;
  }                                        //Termina control para tecla vol down o vidrio abajo copiloto

next4:
    if (digitalRead(PupIn) == LOW)
{
  FChecaSWPup_Cup = 1;
  FChecaSw = 0;
  F2ChecaSw = 0;
  F3ChecaSw = 0;
  F4ChecaSw = 0;
  FChecaSWPdn_Cdn = 0;
  FProcesando = 0;
  F2Procesando = 0;
  F3Procesando = 0;
  F4Procesando = 0;
  FGesto = 0;
  F2Gesto = 0;
  F3Gesto = 0;
  F4Gesto = 0;
}
if (digitalRead(PdnIn) == LOW)
{
  FChecaSWPdn_Cdn = 1;
  FChecaSw = 0;
  F2ChecaSw = 0;
  F3ChecaSw = 0;
  F4ChecaSw = 0;
  FChecaSWPup_Cup = 0;
  FProcesando = 0;
  F2Procesando = 0;
  F3Procesando = 0;
  F4Procesando = 0;
  FGesto = 0;
  F2Gesto = 0;
  F3Gesto = 0;
  F4Gesto = 0;
}
if (FChecaSWPup_Cup == 1)
{
     if (digitalRead(PupIn) == LOW)
    {
      digitalWrite(relayPup, HIGH);
      digitalWrite(relayPdn, LOW);
      digitalWrite(relayCup, HIGH);
      digitalWrite(relayCdn, LOW);
      T7 = (millis() - G5Tupcount);
      if (T7 > 300) {
        SensLectura = analogRead(CSens);
        if (SensLectura > Ampmax || T7 >= updnmaxtime )
        {
          FChecaSWPup_Cup = 0;
          digitalWrite(relayPup, HIGH);
          digitalWrite(relayPdn, HIGH);
          digitalWrite(relayCup, HIGH);
          digitalWrite(relayCdn, HIGH);
        }
      }
      goto siguiente;
    }
    FChecaSWPup_Cup = 0;
    digitalWrite(relayPup, HIGH);
    digitalWrite(relayPdn, HIGH);
    digitalWrite(relayCup, HIGH);
    digitalWrite(relayCdn, HIGH);
  }

siguiente:
  if (FChecaSWPdn_Cdn == 1)
{
      if (digitalRead(PupIn) == LOW && digitalRead(CupIn) == LOW)
    {
      digitalWrite(relayPup, LOW);
      digitalWrite(relayPdn, HIGH);
      digitalWrite(relayCup, LOW);
      digitalWrite(relayCdn, HIGH);
      T7 = (millis() - G6Tdncount);
      if (T7 > 300) {
        SensLectura = analogRead(CSens);
        if (SensLectura > Ampmax || T7 >= updnmaxtime )
        {
          FChecaSWPdn_Cdn = 0;
          digitalWrite(relayPup, HIGH);
          digitalWrite(relayPdn, HIGH);
          digitalWrite(relayCup, HIGH);
          digitalWrite(relayCdn, HIGH);
        }
      }
      goto siguiente2;
    }
    FChecaSWPdn_Cdn = 0;
    digitalWrite(relayPup, HIGH);
    digitalWrite(relayPdn, HIGH);
    digitalWrite(relayCup, HIGH);
    digitalWrite(relayCdn, HIGH);
  }

siguiente2:
  
  if (LockRutine == 0 && UnlockRutine == 0)
{
  if (digitalRead(LockIn) == LOW)
    {
      digitalWrite(relayLock, LOW);
      digitalWrite(relayUnlock, HIGH);
    }
    if (digitalRead(UnlockIn) == LOW
    {
      digitalWrite(relayLock, HIGH);
      digitalWrite(relayUnlock, LOW);
    }
    if (digitalRead(LockIn) == HIGH && digitalRead (UnlockIn) == HIGH)
    {
      digitalWrite(relayLock, HIGH);
      digitalWrite(relayUnlock, HIGH);
    }
  }

  if (UnlockRutine == 1)
{
  //Serial.println("Punto C");
  LockRutine = 0;
  digitalWrite(relayLock, HIGH);
    digitalWrite(relayUnlock, LOW);
    Tt5 = millis() - TimerLSW;
    if (Tt5 < 600)
    {
      goto Inicio;
    }
    digitalWrite(relayLock, HIGH);
    digitalWrite(relayUnlock, HIGH);
    Tt5 = millis() - TimerLSW;
    if (Tt5 < 1200)
    {
      goto Inicio;
    }
    digitalWrite(relayLock, HIGH);
    digitalWrite(relayUnlock, LOW);
    Tt5 = millis() - TimerLSW;
    if (Tt5 < 1800)
    {
      goto Inicio;
    }
    digitalWrite(relayLock, HIGH);
    digitalWrite(relayUnlock, HIGH);
    UnlockRutine = 0;
    goto Inicio;
  }

  if (LockRutine == 1)
{
  //Serial.println("Punto D");
  UnlockRutine = 0;
  digitalWrite(relayLock, LOW);
    digitalWrite(relayUnlock, HIGH);
    Tt5 = millis() - TimerLSW;
    if (Tt5 < 600)
    {
      goto Inicio;
    }
    digitalWrite(relayLock, HIGH);
    digitalWrite(relayUnlock, HIGH);
    Tt5 = millis() - TimerLSW;
    if (Tt5 < 1200)
    {
      goto Inicio;
    }
    digitalWrite(relayLock, LOW);
    digitalWrite(relayUnlock, HIGH);
    Tt5 = millis() - TimerLSW;
    if (Tt5 < 1800)
    {
      goto Inicio;
    }
    digitalWrite(relayLock, HIGH);
    digitalWrite(relayUnlock, HIGH);
    LockRutine = 0;
    goto Inicio;
  }

  LSWread = analogRead(SWcenter);
  if (digitalRead(LockIn) == HIGH && digitalRead(UnlockIn) == HIGH)
{
  TimerLSW = millis();
    FLSW = 1;
  }                                              //Termina control de seguros de las puertas
}

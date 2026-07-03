/* ======================= VARIATEUR ==========================

CARASTERISTIQUES
Marche avant marche arrière avec mémorisation du neutre
A la première utilisation ou un changement de télécommande une initialisation est nécessaire. 
Cette opération n’est à faire qu’une fois si nous utilisons toujours la même radio.


INITIALISATION
Ne pas mettre le Cavalier du récepteur en place (pin 2)
Brancher le variateur au récepteur
Allumer l’émetteur 
Mettre le manche et le trim au neutre
Allumer le récepteur ( l’ATTiny mémorise le neutre, mais le variateur ne fonctionne pas encore)
Mettre le Cavalier en place (le variateur est maintenant en état de marche)
Tant que le Cavalier reste en place (sur le pin 2) le neutre reste en mémoire. 

BRANCHEMENT
Moteur sur pin 0 
Relais sur pin 1 
Récepteur sur pin 4 
Cavalier  sur pin 2 
Led power replacée sur pin 3
==============================================================================     */

/*
Use ONLY Attiny85 or Uno for Serial checks
*/

/*
 *                         ATtiny85
 *                      -------u-------
 *  RST - A0 - (D 5) --| 1 PB5   VCC 8 |-- +5V
 *                     |               |
 *       LED - (D 3) --| 2 PB3   PB2 7 |-- (D 2) - Cavalier
 *                     |               | 
 *  Input RC - (D 4) --| 3 PB4   PB1 6 |-- (D 1) - relais
 *                     |               |      
 *              Gnd ---| 4 GND   PB0 5 |-- (D 0) - motor
 *                     -----------------
 */
 
#include <avr/eeprom.h>  // Appel de la  librairie avr /eeprom
#include <Rcul.h>
#include <SoftRcPulseIn.h>
SoftRcPulseIn PwmRcInput;
uint32_t RxPulseStartMs = millis();
bool ledState = false;
#define PULSE_MAX_PERIOD_MS   30

#define AVERAGE_LEVEL         2  /* Choose here the average level among the above listed PwmRcInputUsues (0 to 4) */
#define AVERAGE(PwmRcInputUsueToAverage,LastReceivedPwmRcInputUsue,AverageLevelInPowerOf2)  PwmRcInputUsueToAverage=(((PwmRcInputUsueToAverage)*((1<<(AverageLevelInPowerOf2))-1)+(LastReceivedPwmRcInputUsue))/(1<<(AverageLevelInPowerOf2)))

#if defined(__AVR_ATtiny85__)
#define M_AV                  0 // Cde vitesse
#define M_AR                  1 // Cde relais
#else
#define M_AV                  5 // Cde vitesse
#define M_AR                  6 // Cde relais
#endif
#define signal                4 // Signal radio attaché au Pin 4
#define Cavalier              2 // Cavalier attaché au Pin 2
#define LED_PIN               3         

uint16_t  neutre         = 0; // PwmRcInputUseur du neutre
uint16_t  plage_neutre   = 65;   // Plage du neutre, ne pas descendre en dessous de 10 par securité passé à 65 comme origine rbase du IRRL?
uint16_t  PwmRcInputUs;        // PwmRcInputUs, signal récepteur
bool      inAlert        = false;
bool      alarmLaunched  = false;
int       Vitesse        = 0;

#define SIGNAL_TIMEOUT_MS  500
struct AlertStatus {
  bool signalLost        = false;
  unsigned long lastRxMs = false;
};
AlertStatus alert;

#include <elapsedMillis.h>
struct Timers {
  elapsedMillis SignalLost;
  elapsedMillis ledInfo;
  elapsedMillis alert;
};
Timers timers;

//=========================================================================
void setup()
{
#if !defined(__AVR_ATtiny85__)
    Serial.begin(115200);
    delay(500);
    Serial.println(F("Loyal Mediator ESC"));
#endif

  pinMode(Cavalier, INPUT_PULLUP);// Déclare le pin "detecteur" en entrée Activation du pull-up interne
  pinMode(M_AR, OUTPUT);          //Déclare le relais en sortie

  pinMode(LED_PIN, OUTPUT);
  for(uint8_t i = 0; i < 5; i++)
  {
    analogWrite(LED_PIN, 200);
    delay(125);
    analogWrite(LED_PIN, 0);
    delay(125);
  }

  PwmRcInput.attach(signal);

  //=========================================================================
  if (digitalRead(Cavalier) == HIGH) 
  { 
    delay(500);
#if !defined(__AVR_ATtiny85__)
    Serial.print("Neutral check is actived !");
#endif
    if (PwmRcInput.available()){
      analogWrite(LED_PIN, 255);
      AVERAGE(PwmRcInputUs, PwmRcInput.width_us(), AVERAGE_LEVEL);

      eeprom_write_word(0,PwmRcInputUs); // Sauvegarde Valeur dans l’eeprom à l’adresse 0 
      delay(500);
      for(uint8_t i = 0; i < 3; i++)
      {
        analogWrite(LED_PIN, 200);
        delay(250);
        analogWrite(LED_PIN, 0);
        delay(250);
      }
    } 
  }
  delay(250);
  neutre = eeprom_read_word(0);           // Recupere le neutre mis en memoire
  if(neutre < 800 || neutre > 2200) neutre = 1500;//FRSKY or FUTABA

}


//=========================================================================
void loop()
{ 
  if (PwmRcInput.available()) {
    AVERAGE(PwmRcInputUs, PwmRcInput.width_us(), AVERAGE_LEVEL);
    alert.lastRxMs = millis();
    alert.signalLost = false;
  }
  if (!PwmRcInput.available() && (millis() - alert.lastRxMs > SIGNAL_TIMEOUT_MS))
  {
    alert.signalLost = true;
    PwmRcInputUs = 1500;
  }

  if (timers.SignalLost > 1000)
  {
    timers.SignalLost = 0;
    SignalLost();
#if !defined(__AVR_ATtiny85__)
    Serial.print("Signal Lost is actived !");
#endif
  }

  if (digitalRead(Cavalier) == HIGH) { PwmRcInputUs = neutre; }//mise au neutre durant la calibration
#if !defined(__AVR_ATtiny85__)
  Serial.print("Input Rc: "); Serial.print(PwmRcInputUs); Serial.print(" ");
  if (digitalRead(Cavalier) == HIGH) Serial.print("Neutral check is actived, please add Bridge !");
#endif
    //.............................. ARRET ............................................  
  if ( (PwmRcInputUs > (neutre - plage_neutre)) && (PwmRcInputUs < (neutre + plage_neutre)))
  { 
    analogWrite(M_AV, 0);                 // Vitesse avant= 0
    digitalWrite(M_AR, LOW);              // Vitesse arriere= 0
#if !defined(__AVR_ATtiny85__)
    Serial.print("Mode = NEUTRAL ");
#endif
  }
    //............................ MARCHE AR..............................................  
  if ( PwmRcInputUs < (neutre - plage_neutre))
  {   
    digitalWrite(M_AR, HIGH);             // relais travail              
    Vitesse = map(PwmRcInputUs, (neutre-500), (neutre - plage_neutre), 250, 50); //calibrage pour le transistor
    Vitesse = constrain(Vitesse, 50, 250);     
    analogWrite(M_AV, Vitesse);           // vitesse avant=x
#if !defined(__AVR_ATtiny85__)
    Serial.print("Mode = BACK ");
#endif
  }
    //............................ MARCHE AV..............................................
  if ( PwmRcInputUs > (neutre + plage_neutre))
  {
    digitalWrite(M_AR, LOW);              // relais repos   
    Vitesse = map(PwmRcInputUs, (neutre + plage_neutre), (neutre+500), 50, 250); //calibrage pour le transistor
    Vitesse = constrain(Vitesse, 50, 250);     
    analogWrite(M_AV, Vitesse);           // vitesse avant=x
#if !defined(__AVR_ATtiny85__)
    Serial.print("Mode = BEFORE ");
#endif
  }
#if !defined(__AVR_ATtiny85__)
  Serial.print("Speed: "); Serial.println(Vitesse);
#endif
}

void SignalLost() {
  if (alert.signalLost) {

    blinkLED();
    
    if (!inAlert) {
      inAlert = true;
      alarmLaunched = false;
      timers.alert = 0;
    }

    if (!alarmLaunched && timers.alert >= 2000) {
      alarmLaunched = true;
    }
  } else {
    analogWrite(LED_PIN, 0);
    inAlert = false;
    alarmLaunched = false;
    timers.alert = 0;
  }

}

void blinkLED()
{
  // Check if it's time to change the state of the LED
  if (timers.ledInfo > 1000)
  {
    timers.ledInfo = false;  // restart the chrono so that it triggers again later
    // toggle the stored state
    ledState = !ledState;
    if (ledState)
      analogWrite(LED_PIN, 200);
    else
      analogWrite(LED_PIN, 0);
  }
}


//========================== FIN DE PROGRAMME ============================================

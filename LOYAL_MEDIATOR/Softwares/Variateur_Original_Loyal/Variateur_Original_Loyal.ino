/* ======================= VARIATEUR ==========================

CARASTERISTIQUES
Marche avant marche arrière avec mémorisation du neutre
A la première utilisation ou un changement de télécommande une initialisation est nécessaire. 
Cette opération n’est à faire qu’une fois si nous utilisons toujours la même radio.


INITIALISATION
Ne pas mettre le cavalier du récepteur en place (pin 2)
Brancher le variateur au récepteur
Allumer l’émetteur 
Mettre le manche et le trim au neutre
Allumer le récepteur ( l’ATTiny mémorise le neutre, mais le variateur ne fonctionne pas encore)
Mettre le cavalier en place (le variateur est maintenant en état de marche)
Tant que le cavalier reste en place (sur le pin 2) le neutre reste en mémoire. 

BRANCHEMENT
Moteur sur pin 0 
Relais sur pin 1 
Récepteur sur pin 4 
Cavalier  sur pin 2 
==============================================================================     */
#include <avr/eeprom.h>  // Appel de la  librairie avr /eeprom
#define MOY_SUR_2_VALEURS       1
#define MOY_SUR_4_VALEURS       2
#define MOY_SUR_8_VALEURS       3
#define MOY_SUR_16_VALEURS      4
#define MOY_SUR_32_VALEURS      5

#define TAUX_DE_MOYENNAGE       3  /* Choisir ici le taux de moyennage parmi les valeurs precedentes possibles listees ci-dessus */
                                   /* Plus le taux est élevé, plus le système est stable (diminution de la gigue), mais moins il est réactif */

#define MOYENNE(Valeur_A_Moyenner,DerniereValeurRecue,TauxDeMoyEnPuissanceDeDeux)  Valeur_A_Moyenner=(((Valeur_A_Moyenner)*((1<<(TauxDeMoyEnPuissanceDeDeux))-1)+(DerniereValeurRecue))/(1<<(TauxDeMoyEnPuissanceDeDeux)))

#define M_AV                  0 // Cde vitesse
#define M_AR                  1 // Cde relais
#define signal                4 // Signal radio attaché au Pin 4
#define cavalier              2 // Cavalier attaché au Pin 2
int neutre;                     // Valeur du neutre
int plage_neutre = 65;          // Plage du neutre, ne pas descendre en dessous de 10 par securité passé à 65 comme origine rbase du IRRL?
int val;                        // Valeur du signal récepteur

//=========================================================================
void setup()
{
  pinMode(cavalier, INPUT_PULLUP);// Déclare le pin "detecteur" en entrée Activation du pull-up interne
  pinMode(M_AR, OUTPUT);          //Déclare le relais en sortie
  //=========================================================================
  if (digitalRead(cavalier) == HIGH) 
  { 
    delay(1000);  
    neutre = pulseIn(signal, HIGH);  
    eeprom_write_word(0,neutre);             // Sauvegarde Valeur dans l’eeprom à l’adresse 0 
  }
  delay(1000);
  neutre = eeprom_read_word(0);           // Recupere le neutre mis en memoire

  if (neutre == 0)
    neutre = 1500;//FRSKY or FUTABA

}
//=========================================================================
void loop()
{
  static int ValMoyennee;
  val = pulseIn(signal, HIGH);            //lit le signal récepteur et le stocke dans val
  ValMoyennee=MOYENNE(ValMoyennee,val,TAUX_DE_MOYENNAGE);
  val = ValMoyennee;
  if (digitalRead(cavalier) == HIGH) { val = neutre; }//mise au neutre durant la calibration

    //.............................. ARRET ............................................  
  if ( (val > (neutre - plage_neutre)) && (val < (neutre + plage_neutre)))
  { 
    analogWrite(M_AV, 0);                 // Vitesse avant= 0
    digitalWrite(M_AR, LOW);              // Vitesse arriere= 0
    delay(40);
  }
    //............................ MARCHE AR..............................................  
  if ( val < (neutre - plage_neutre))
  {   
    digitalWrite(M_AR, HIGH);             // relais travail              
    int Vitesse = map(val, (neutre-500), (neutre - plage_neutre), 250, 50); //calibrage pour le transistor
    Vitesse=constrain(Vitesse, 50, 250);     
    analogWrite(M_AV, Vitesse);           // vitesse avant=x

  }
    //............................ MARCHE AV..............................................
  if ( val > (neutre + plage_neutre))
  {
    digitalWrite(M_AR, LOW);              // relais repos   
    int Vitesse = map(val, (neutre + plage_neutre), (neutre+500), 50, 250); //calibrage pour le transistor
    Vitesse=constrain(Vitesse, 50, 250);     
    analogWrite(M_AV, Vitesse);           // vitesse avant=x

  }
}
//========================== FIN DE PROGRAMME ============================================

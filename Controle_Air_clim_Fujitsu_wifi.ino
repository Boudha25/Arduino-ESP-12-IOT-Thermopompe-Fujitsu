//* Auteur Stéphane April. *
//* stephaneapril@hotmail.com *

// Charge les bibliothèques.
// Load library
#include <Arduino.h>
#include <FujitsuHeatpumpIR.h>

// Charge la bibliothèque Wi-Fi.
// Load Wi-Fi library
#include <ESP8266WiFi.h>

//*****Section AC choix du board*****//
//#ifndef ESP8266
//IRSenderPWM irSender(9);       // IR led on Arduino digital pin 9, using Arduino PWM
//IRSenderBlaster irSender(3); // IR led on Arduino digital pin 3, using IR Blaster (generates the 38 kHz carrier)
//#else
IRSenderBitBang irSender(D2);  // IR led on Wemos D1 mini, connect between D2 and G
//#endif

/*
// Codes pour thermopompe Fujitsu.
// Fujitsu codes in the librairy.
#define FUJITSU_AIRCON1_MODE_AUTO   0x00 // Operating mode
#define FUJITSU_AIRCON1_MODE_HEAT   0x04
#define FUJITSU_AIRCON1_MODE_COOL   0x01
#define FUJITSU_AIRCON1_MODE_DRY    0x02
#define FUJITSU_AIRCON1_MODE_FAN    0x03
#define FUJITSU_AIRCON1_MODE_OFF    0xFF // Power OFF - not real codes, but we need something...
#define FUJITSU_AIRCON1_FAN_AUTO    0x00 // Fan speed
#define FUJITSU_AIRCON1_FAN1        0x04
#define FUJITSU_AIRCON1_FAN2        0x03
#define FUJITSU_AIRCON1_FAN3        0x02
#define FUJITSU_AIRCON1_FAN4        0x01
#define FUJITSU_AIRCON1_VDIR_MANUAL 0x00 // Air direction modes
#define FUJITSU_AIRCON1_VDIR_SWING  0x10
#define FUJITSU_AIRCON1_HDIR_MANUAL 0x00
#define FUJITSU_AIRCON1_HDIR_SWING  0x20
#define FUJITSU_AIRCON1_ECO_OFF     0x20
#define FUJITSU_AIRCON1_ECO_ON      0x00
*/

// Déclarer une structure pour stocker les paramètres de la commande IR.
// Declare a structure to store the parameters of the IR command.
struct IRCommand {
  uint8_t power;
  uint8_t mode;
  uint8_t fan;
  uint8_t temperature;
  uint8_t vdir;
  uint8_t hdir;
};

// Tableau pour thermopompe Fujitsu.
// Array supported heatpumps Fujitsu
HeatpumpIR *heatpumpIR[] = {new FujitsuHeatpumpIR(), NULL};

// Remplacer par vos informations de réseau sans-fil.
// Replace with your network credentials
const char* ssid = "VIDEOTRON0055"; // wifi SSID
const char* password = "Y7YPAWX7W3Y7T"; // wifi password

// Port du serveur web 80.
// Set web server port number to 80
WiFiServer server(80);

// Variable pour enregistrer les requêtes HTTP.
// Variable to store the HTTP request
String header;

// Variable pour enregistrer l'état de la thermopompe.
// Auxiliar variables to store the current output state
String output5State = "OFF";
String lastSelectedoutput5State = "OFF";
// Variables pour le controle de la temperature.
// Variables for temperature control.
int temperature = 20;
int lastSelectedTemperature = 20;
// Variable pour le controle du mode de fonctionnement.
// Variable for operating mode control.
String mode = "Faire une s&eacutelection";
String lastSelectedMode = "MODE_HEAT";
// Variable pour le controle de la vitesse du ventillateur.
// Variable for fan speed control.
String fan = "FAN4";
String lastSelectedFan = "FAN4";
// Variable pour le controle des pales directrices vertical.
// Variable for controlling vertical guide vanes.
String vdir = "VDIR_MANUAL";
String lastSelectedVdir = "VDIR_MANUAL";

// Temp présent.
// Current time.
unsigned long currentTime = millis();
// Temp précédent.
// Previous time
unsigned long previousTime = 0; 
// Défini une pause en millisecondes.(example: 2000ms = 2s)
// Define timeout time in milliseconds. (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting"));

  // Connexion au réseau wifi avec le SSID et le mot de passe.
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Affiche l'adresse IP du serveur web sur le port série.
  // Print local IP address and start web server.
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){

  // Définir les paramètres de la commande IR par défault.
  // Set default parameters for the IR command.
  IRCommand command;
  command.power = POWER_ON;
  command.mode = MODE_HEAT;
  command.fan = FAN_AUTO;
  command.temperature = temperature;
  command.vdir = VDIR_UP;
  command.hdir = HDIR_AUTO;


  WiFiClient client = server.available();   // Vérifie si des clients se connecte // Listen for incoming clients.

  if (client) {                             // Si il y a un nouveau client. // If a new client connects,
    Serial.println("New Client.");          // Affiche ce message sur le port série. // print a message out in the serial port
    String currentLine = "";                // Fait une "String" avec les données reçus. // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // Loop tant que le client est connecté. // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // Tant qu'il y a des données du client, // if there's bytes to read from the client,
        char c = client.read();             // Lire bit à bit, // read a byte, then
        Serial.write(c);                    // Affiche les sur le port série. // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // si l'octet est un caractère de nouvelle ligne. // if the byte is a newline character.
          // si la ligne actuelle est vide, vous avez deux caractères de nouvelle ligne d'affilée.
          // c'est la fin de la requête HTTP du client, donc envoyez une réponse :
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // Les en-têtes HTTP commencent toujours par un code de réponse (par exemple HTTP/1.1 200 OK)
            // et un type de contenu pour que le client sache ce qui s'en vient, puis une ligne vide :
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Écoute et analise des requetes HTTP.
            // Listen and analyze HTTP requests.
            if (header.indexOf("GET /on") >= 0) {
              Serial.println("Pompe  ON");
              output5State = "ON";
              lastSelectedoutput5State = output5State;
            // Exécute la fonction qui met l'air climatisée à OFF.
            // Execute the function that turns off the air conditioner.
            command = updatePower(command, POWER_ON);
            sendIRCommand(command);

            } else if (header.indexOf("GET /off") >= 0) {
              Serial.println("Pompe  OFF");
              output5State = "OFF";
              lastSelectedoutput5State = output5State;
            command = updatePower(command, POWER_OFF);
            sendIRCommand(command);

            } else if (header.indexOf("GET /mode") >= 0) {
              Serial.print("Changement de mode de fonctionnement:");
              // Trouver la position de "mode" dans la requête HTTP.
              // Find the position of 'mode' in the HTTP request.
              int modePosition = header.indexOf("mode=");         
              // Si "mode" est trouvé dans la requête HTTP.
              // If 'mode' is found in the HTTP request.
              if (modePosition != -1) {
                // Extraire la valeur du mode choisi.
                // Extract the value of the chosen mode.
                String modeValue = header.substring(modePosition + 5); // 5 est la longueur de "mode=". // The length of 'mode=' is 5.
                // Supprimer la partie " HTTP/1.1" de la chaîne.
                // Remove the ' HTTP/1.1' part from the string.
                String inputString = modeValue;
                int index = inputString.indexOf("HTTP/1.1");
                if (index != -1) {
                    mode = inputString.substring(0, index);
                     }                    
                // Mettre à jour la structure IRCommand avec la nouvelle température.
                // Update the IRCommand structure with the new temperature.
                command = updateMode(command, getModeValue(mode));
                sendIRCommand(command);
                // Enregistre la dernière valeur sélectionnée.
                // Save the last selected value.
                lastSelectedMode = mode;
                Serial.print("Mode de fonctionnement: ");
                Serial.println(mode);
                }

              } else if (header.indexOf("GET /temp") >= 0) {
              Serial.print("Changement de temperature");
              // Trouver la position de "temperature" dans la requête HTTP.
              // Find the position of 'temperature' in the HTTP request.
              int temperaturePosition = header.indexOf("temperature=");           
              // Si "temperature" est trouvé dans la requête HTTP.
              // If 'temperature' is found in the HTTP request.
              if (temperaturePosition != -1) {
                // Extraire la valeur de la température.
                // Extract the value of the temperature.
                String temperatureValue = header.substring(temperaturePosition + 12); // 12 est la longueur de "temperature=" // The length of 'temperature=' is 12.      
                // Convertir la chaîne en entier.
                // Convert the string to an integer.
                temperature = temperatureValue.toInt();
                // Mettre à jour la structure IRCommand avec la nouvelle température.
                // Update the IRCommand structure with the new temperature.
                command = updateTemperature(command, temperature);
                sendIRCommand(command);
                // Enregistre la dernière valeur sélectionnée.
                // Save the last selected value.
                lastSelectedTemperature = temperature;
                Serial.print("Temperature: ");
                Serial.println(temperature);
                }
            
              } else if (header.indexOf("GET /fan") >= 0) {
              Serial.print("Changement de vitesse du ventillateur");
              // Trouver la position de "fan" dans la requête HTTP.
              // Find the position of 'fan' in the HTTP request.
              int fanPosition = header.indexOf("fan=");           
              // Si "fan" est trouvé dans la requête HTTP.
              // If 'fan' is found in the HTTP request.
              if (fanPosition != -1) {
                // Extraire la valeur de la vitesse du ventillateur.
                // Extract the value of the fan speed.
                String fanValue = header.substring(fanPosition + 4); // 4 est la longueur de "fan=".
                // Supprimer la partie " HTTP/1.1" de la chaîne.
                // Remove the ' HTTP/1.1' part from the string.
                String inputString = fanValue;
                int index = inputString.indexOf("HTTP/1.1");
                if (index != -1) {
                    fan = inputString.substring(0, index);
                     }           
                // Mettre à jour la structure IRCommand avec la nouvelle vitesse.
                // Update the IRCommand structure with the new fan speed.
                command = updateFan(command, getFanValue(fan));
                sendIRCommand(command);
                // Enregistre la dernière valeur sélectionnée.
                // Save the last selected value.
                lastSelectedFan = fan;
                Serial.print("Vitesse du ventillateur: ");
                Serial.println(fan);
                }             
              } else if (header.indexOf("GET /vdir") >= 0) {
              Serial.print("Changement d'orientation des pales:");
              // Trouver la position de "vdir" dans la requête HTTP.
              // Find the position of 'vdir' in the HTTP request.
              int vdirPosition = header.indexOf("vdir=");           
              // Si "vdir" est trouvé dans la requête HTTP.
              // If 'vdir' is found in the HTTP request.
              if (vdirPosition != -1) {
                // Extraire la valeur de vdir.
                // Extract the value of the vdir.
                String vdirValue = header.substring(vdirPosition + 5); // 5 est la longueur de "vdir=". // The length of 'vdir=' is 5.
                // Supprimer la partie " HTTP/1.1" de la chaîne.
                // Remove the ' HTTP/1.1' part from the string.
                String inputString = vdirValue;
                int index = inputString.indexOf("HTTP/1.1");
                if (index != -1) {
                    vdir = inputString.substring(0, index);
                     }           
                // Mettre à jour la structure IRCommand avec la nouvelle position.
                // Update the IRCommand structure with the new position.
                command = updateVdir(command, getVdirValue(vdir));
                sendIRCommand(command);
                // Enregistre la dernière valeur sélectionnée.
                // Save the last selected value.
                lastSelectedVdir = vdir;
                Serial.print("Orientation des pales: ");
                Serial.println(vdir);
              }
            }
            // Création de la page web.
            // Display the HTML web page.
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS du bouton ON/OFF.
            // CSS to style the on/off buttons.
            // N'hésitez pas à modifier les attributs background-color et font-size en fonction de vos préférences.
            // Feel free to change the background-color and font-size attributes to fit your preferences.
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}");

            // CSS pour le choix de la temperature.
            // CSS for temperature selection.
            client.println("label {display: block;");
            client.println("font: 1rem 'Fira Sans', sans-serif;}");
            client.println("input, label {margin: 0.4rem 0;}");
            // CSS pour le choix du mode de fonctionnement.
            // CSS for operating mode selection.
            client.println(".combo {padding: 8px;}");
            
            client.println("</style></head>");

            // Entête de la page Web.
            // Web Page Heading
            client.println("<body><h1>Contr&ocircle air climatis&eacutee Wifi</h1>");
            // Affiche l'état du bouton ON/Off.
            // Display current state, and ON/OFF buttons.
            client.println("<p>&Eacutetat de la thermopompe: " + output5State + "</p>");
            // Si output5State est désactivée, affiche le bouton ON.
            // If the output5State is off, it displays the ON button.      
            if (output5State=="OFF") {
              client.println("<p><a href=\"/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 

            // Formulaire pour envoyer la température.
            // Form to submit the temperature.
            client.println("<form id=\"temperature\" action=\"/temp\" method=\"GET\">");
            client.println("<label for=\"temperature\">Temp&eacuterature d&eacutesir&eacutee (10-30):</label>");
            client.println("<input type=\"number\" value=\"" + String(lastSelectedTemperature) + "\" name=\"temperature\" min=\"10\" max=\"30\" />");
            client.println("<input type=\"submit\" value=\"Valider\" />");
            client.println("</form>");

            // Formulaire pour selectionner le mode d'operation.
            // Form to submit the operating mode.
            client.println("<form id=\"mode\" action=\"/mode\" method=\"GET\">");
            client.println("<label for=\"mode\">Choisir un mode d'op&eacuteration:</label>");
            client.println("<select name=\"mode\" class=\"combo\">");
            client.println("<option value=\"Select\">" + lastSelectedMode + "</option>");
            client.println("<option value=MODE_AUTO>Mode AUTO</option>");
            client.println("<option value=MODE_HEAT>Mode HEAT</option>");
            client.println("<option value=MODE_COOL>Mode COOL</option>");
            client.println("<option value=MODE_DRY>Mode DRY</option>");
            client.println("<option value=MODE_FAN>Mode FAN</option></select>");
            client.println("<input type=\"submit\" value=\"Valider\" />");
            client.println("</form>");

            // Formulaire pour selectionner la vitesse du ventillateur.
            // Form to submit the fan speed.
            client.println("<form id=\"fan\" action=\"/fan\" method=\"GET\">");
            client.println("<label for=\"fan\">Choisir la vitesse du ventillateur:</label>");
            client.println("<select name=\"fan\" class=\"combo\">");
            client.println("<option value=\"Select\">" + String(lastSelectedFan) + "</option>");
            client.println("<option value=FAN_AUTO>Mode AUTO</option>");
            client.println("<option value=FAN1>FAN1</option>");
            client.println("<option value=FAN2>FAN2</option>");
            client.println("<option value=FAN3>FAN3</option>");
            client.println("<option value=FAN4>FAN4</option></select>");
            client.println("<input type=\"submit\" value=\"Valider\" />");
            client.println("</form>");

            // Formulaire pour selectionner le mode d'orientation des pales de ventillateur.
            // Form to select the fan blade orientation mode.
            client.println("<form id=\"vdir\" action=\"/vdir\" method=\"GET\">");
            client.println("<label for=\"vdir\">Choisir le mode d'orientation des pales:</label>");
            client.println("<select name=\"vdir\" class=\"combo\">");
            client.println("<option value=\"Select\">" + String(lastSelectedVdir) + "</option>");
            client.println("<option value=VDIR_MANUAL>VDIR_MANUAL</option>");
            client.println("<option value=VDIR_SWING>VDIR_SWING</option></select>");
            client.println("<input type=\"submit\" value=\"Valider\" />");
            client.println("</form>");

            client.println("</body></html>");
            // La réponse HTTP se termine avec une ligne vide.
            // The HTTP response ends with another blank line.
            client.println();
            // Met fin à la loop.
            // Break out of the while loop.
            break;
          } else { // Si il y a une nouvelle ligne met fin à la ligne actuelle. // if you got a newline, then clear currentLine.
            currentLine = "";
          }
        } else if (c != '\r') {  // si vous avez autre chose qu'un caractère de retour chariot, // if you got anything else but a carriage return character,
          currentLine += c;      // ajoute le à la ligne actuelle. // add it to the end of the currentLine.
        }
      }
    }
    // Efface les variables d'entêtes.
    // Clear the header variable.
    header = "";
    // Fermer la connexion.
    // Close the connection.
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

// Fonction pour envoyer la commande IR.
// Function to send the IR command.
void sendIRCommand(IRCommand command) {
  int i = 0;
  const char* buf;
  informationThermopompe();
  // Commande Ir envoyé.
  // IR command sent.
  heatpumpIR[i]->send(irSender, command.power, command.mode, command.fan, command.temperature, command.vdir, command.hdir);
  Serial.println("Commande IR envoyée: - Power:" + String(command.power) + " Mode:" + String(command.mode) + " fan:" + String(command.fan) + " Température:" + String(command.temperature) + "°C" + " vdir:" + String(command.vdir));
  delay(2000);
}

// Fonction pour mettre à jour la température dans la structure IRCommand.
// Function to update the temperature in the IRCommand structure.
IRCommand updateTemperature(IRCommand command, uint8_t newTemperature) {
  command.temperature = newTemperature;
  return command;
}

// Fonction pour mettre à jour le mode de fonctionnement dans la structure IRCommand.
// Function to update the operating mode in the IRCommand structure.
IRCommand updateMode(IRCommand command, uint8_t newMode) {
  command.mode = newMode;
  return command;
}

// Fonction pour mettre à jour la vitesse du ventillateur dans la structure IRCommand.
// Function to update the fan speeed in the IRCommand structure.
IRCommand updateFan(IRCommand command, uint8_t newFan) {
  command.fan = newFan;
  return command;
}

// Fonction pour mettre à jour la position du ventillateur dans la structure IRCommand.
// Function to update the pale position in the IRCommand structure.
IRCommand updateVdir(IRCommand command, uint8_t newVdir) {
  command.vdir = newVdir;
  return command;
}

// Fonction pour obtenir la valeur numérique du mode à partir de la chaîne.
// Function to obtain the numerical value of the mode from the string.
uint8_t getModeValue(String modeString) {
  modeString.trim(); // Enlève les espace blancs si il en a dans la chaine de caratère modeString. // Remove white spaces if any in the modeString character string.
  if (modeString == "MODE_AUTO") {
    return MODE_AUTO;
  } else if (modeString == "MODE_HEAT") {
    return MODE_HEAT;
  } else if (modeString == "MODE_COOL") {
    return MODE_COOL;
  } else if (modeString == "MODE_DRY") {
    return MODE_DRY;
  } else if (modeString == "MODE_FAN") {
    return MODE_FAN;
  } 
  else {
    // Gérer le cas où le mode n'est pas reconnu, peut-être retourner une valeur par défaut.
    // Handle the case where the mode is not recognized, perhaps return a default value.
    return MODE_HEAT; // Utiliser MODE_HEAT comme valeur par défaut.  // Use MODE_HEAT as the default value.
  }
}

// Fonction pour obtenir la valeur numérique de la vitesse du ventillateur à partir de la chaîne.
// Function to obtain the numerical value of the fan speed from the string.
uint8_t getFanValue(String modeString) {
  modeString.trim(); // Enlève les espace blancs si il en a dans la chaine de caratère modeString. // Remove white spaces if any in the modeString character string.
  if (modeString == "FAN_AUTO") {  
    return FAN_AUTO;
  } else if (modeString == "FAN1") {
    return FAN_1;
  } else if (modeString == "FAN2") {
    return FAN_2;
  } else if (modeString == "FAN3") {
    return FAN_3;
  } else if (modeString == "FAN4") {
    return FAN_4;
  } 
  else {
    // Gérer le cas où la vitesse du fan n'est pas reconnu, peut-être retourner une valeur par défaut.
    // Handle the case where the fan speed is not recognized, perhaps return a default value.
    return FAN_AUTO; // Utiliser FAN_AUTO comme valeur par défaut. // Use FAN_AUTO as the default value.
  }
}

// Fonction pour obtenir la valeur numérique de la vitesse du ventillateur à partir de la chaîne.
// Function to obtain the numerical value of the fan speed from the string.
uint8_t getVdirValue(String modeString) {
  modeString.trim(); // Enlève les espace blancs si il en a dans la chaine de caratère modeString. // Remove white spaces if any in the modeString character string.
  if (modeString == "VDIR_MANUAL") {  
    return VDIR_MANUAL;
  } else if (modeString == "VDIR_SWING") {
    return VDIR_SWING;
  } 
  else {
    // Gérer le cas où le mode n'est pas reconnu, peut-être retourner une valeur par défaut.
    // Handle the case where the fan speed is not recognized, perhaps return a default value.
    return VDIR_MANUAL; // Utiliser VDIR_MANUAL comme valeur par défaut. // Use VDIR_MANUAL as the default value.
  }
}

// Fonction pour allumer ou éteindre la thermopompe dans la structure IRCommand.
// Function to turn on or off the heat pump in the IRCommand structure.
IRCommand updatePower(IRCommand command, uint8_t PowerSet) {
  command.power = PowerSet;
  return command;
}
void informationThermopompe() {
  int i = 0;
  const char* buf;
  // Envoyez la même commande IR à toutes les thermopompes prises en charge.
  // Send the same IR command to all supported heatpumps.
  Serial.print(F("Sending IR to "));

  buf = heatpumpIR[i]->model();
  // 'model' est un pointeur PROGMEM, il faut donc écrire un octet à la fois.
  // 'model' is a PROGMEM pointer, so need to write a byte at a time.
  while (char modelChar = pgm_read_byte(buf++))
  {
    Serial.print(modelChar);
  }
  Serial.print(F(", info: "));

  buf = heatpumpIR[i]->info();
  // 'info' est un pointeur PROGMEM, il faut donc écrire un octet à la fois.
  // 'info' is a PROGMEM pointer, so need to write a byte at a time
  while (char infoChar = pgm_read_byte(buf++))
  {
    Serial.print(infoChar);
  }
  Serial.println();
}
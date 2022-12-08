#include <ESP8266WiFi.h>      //Biblioteca utilizada para conexão Wi-Fi.
#include <ESPAsyncTCP.h>      //Comunicação TCP
#include <ESPAsyncWebServer.h>//Utilizada para criar o webserver.
#include <FS.h>               //Responsável pelo acesso ao SPIFFS.
#include <SPI.h>              //Responsável pela comunicação com o leitor RFID.
#include <MFRC522.h>          //RFID
#include <vector>             //Vetor

#define SS_PIN D2
#define RST_PIN D1
#define FILENAME "/Cadastro.txt"
#define LED_RED D3
#define LED_GREEN D4
#define LED_BLUE D8

using namespace std;
//const char* ssid = "SATC IOT";
//const char* password = "IOT2022@";
//const char* ssid = "Samsung";
//const char* password = "igor1234";
const char* ssid = "Igor";
const char* password = "10042002";
//const char* ssid = "Teclenet_Valdenir";
//const char* password = "ir101823";

String info_data; //Informação sobre o usuario.Ex nome, cpf, etc.
String id_data;   //Id para o usuario.
String credit_data;   //Crédito para o usuario.
int index_user_for_removal = -1;

String rfid_card = ""; //Codigo RFID obtido pelo Leitor
String sucess_msg = "";
String failure_msg = "";
String monitoring_msg = "PEDÁGIO FECHADO";
bool monitoring = true;

// Cria um objeto  MFRC522.
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Cria um objeto AsyncWebServer que usará a porta 80
AsyncWebServer server(80);

// Protótipo da função
char* string_substring(char str[], int start, int end);

//Inicializa o sistema de arquivos.
bool initFS() {
  if (!SPIFFS.begin()) {
    Serial.println("Erro ao abrir o sistema de arquivos");
    return false;
  }
  Serial.println("Sistema de arquivos carregado com sucesso!");
  return true;
}
//Lista todos os arquivos salvos na flash.
void listAllFiles() {
  String str = "";
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    str += dir.fileName();
    str += " / ";
    str += dir.fileSize();
    str += "\r\n";

    Serial.println(str);
  }
  Serial.print(str);
}
//Faça a leitura de um arquivo e retorne um vetor com todas as linhas.
vector <String> readFile(String path) {
  vector <String> file_lines;
  String content;
  File myFile = SPIFFS.open(path.c_str(), "r");
  if (!myFile) {
    myFile.close();
    return {};
  }

  while (myFile.available()) {
    content = myFile.readStringUntil('\n');
    file_lines.push_back(content);
    Serial.println(content);
  }

  myFile.close();
  return file_lines;
}
//Faça a busca de um usuario pelo ID e pela INFO.
int findUser(vector <String> users_data, String id, String nome) {

  String newID = id + "\n";
  String newinfo = nome;

  Serial.println("antes -" + newID + "-");
  Serial.println("busca user nome" + newinfo);

  for (int i = 0; i < users_data.size(); i++) {
    return 0;
    Serial.println("-" + users_data[i] + "-");
    if (String(users_data[i]).indexOf(newID) > 0 || String(users_data[i]).indexOf(newinfo) > 0) {
      return i;
    }
  }
  return -1;
}
//Adiciona um novo usuario ao sistema
bool addNewUser(String id, String data, String credit) {
  File myFile = SPIFFS.open(FILENAME, "a+");
  if (!myFile) {
    Serial.println("Erro ao abrir arquivo!");
    myFile.close();
    return false;
  } else {
    myFile.printf("%s\n", id.c_str());
    myFile.printf("%s\n", data.c_str());
    myFile.printf("%s\n", credit.c_str());
    Serial.println("Arquivo gravado");
  }
  myFile.close();
  return true;
}
//Remove um usuario do sistema
bool removeUser(int user_index) {
  vector <String> users_data = readFile(FILENAME);
  if (user_index == -1)//Caso usuário não exista retorne falso
    return false;

  File myFile = SPIFFS.open(FILENAME, "w");
  if (!myFile) {
    Serial.println("Erro ao abrir arquivo!");
    myFile.close();
    return false;
  } else {

    Serial.println("Usuário " + String(user_index));

    for (int i = 0; i < users_data.size(); i++) {
      if (i != user_index && i != user_index + 1 && i != user_index + 2)
        myFile.println(users_data[i]);
    }
    Serial.println("Usuário removido");
  }
  myFile.close();
  return true;
}

//Adiciona crédito para um usuario do sistema
bool addCreditUser(int user_index, bool removeCred) {
  vector <String> users_data = readFile(FILENAME);
  if (user_index == -1)//Caso usuário não exista retorne falso
    return false;

  File myFile = SPIFFS.open(FILENAME, "w");
  if (!myFile) {
    Serial.println("Erro ao abrir arquivo!");
    myFile.close();
    return false;
  } else {

    for (int i = 0; i < users_data.size(); i++) {
      if (user_index + 2 == i)
      {
        String aux = users_data[i].c_str();
        int valueCred = aux.toInt();

        //Lógica para incluir crédito
        if (removeCred)
          valueCred = valueCred - 5;
        else
          valueCred = valueCred + 10;

        myFile.println(String(valueCred));
      }
      else
        myFile.println(users_data[i]);
    }
    Serial.println("Crédito adicionado");
  }
  myFile.close();
  return true;
}

//Esta função substitui trechos de paginas html marcadas entre %
String processor(const String& var) {
  String msg = "";
  if (var == "TABLE") {
    Serial.println("TABLE");
    msg = "<table><tr><td>RFID</td><td>Nome</td><td>Crédito Disponível (R$)</td><td>Adicionar</td><td>Deletar</td></tr>";
    vector <String> lines = readFile(FILENAME);
    for (int i = 0; i < lines.size(); i++) {
      msg += "<tr><td>" + lines[i] + "</td><td>" + lines[i + 1] + "</td><td>" + lines[i + 2] + "</td>";
      Serial.println("valores: " + lines[i] + ", " + lines[i + 1] + ", " + lines[i + 2]);
      msg += "<td><a href=\"get?add=" + String(i + 1) + "\"><button>+ R$10,00</button></a></td><td><a href=\"get?remove=" + String(i + 1) + "\"><button>Excluir</button></a></td></tr>"; //Adiciona um botão com um link para o indice do usuário na tabela
      i += 2;
    }
    msg += "</table>";
  }
  else if (var == "MESSAGE")
    msg = monitoring_msg;
  else if (var == "SUCESS_MSG")
    msg = sucess_msg;
  else if (var == "FAILURE_MSG")
    msg = failure_msg;
  return msg;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_BLUE, LOW);

  SPI.begin();
  mfrc522.PCD_Init();   // Inicia MFRC522

  // Inicialize o SPIFFS
  if (!initFS())
    return;

  listAllFiles();
  readFile(FILENAME);

  // Conectando ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando WiFi..");
  }

  Serial.println(WiFi.localIP());

  //Rotas do servidor
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    monitoring = true;
    rfid_card = "";
    request->send(SPIFFS, "/monitoring.html", String(), false, processor);
  });
  server.on("/monitoring", HTTP_GET, [](AsyncWebServerRequest * request) {
    monitoring = true;
    rfid_card = "";
    request->send(SPIFFS, "/monitoring.html", String(), false, processor);
  });
  server.on("/userRegistration", HTTP_GET, [](AsyncWebServerRequest * request) {
    monitoring = false;
    rfid_card = "";
    request->send(SPIFFS, "/userRegistration.html", String(), false, processor);
  });
  server.on("/userView", HTTP_GET, [](AsyncWebServerRequest * request) {
    rfid_card = "";
    request->send(SPIFFS, "/userView.html", String(), false, processor);
  });
  server.on("/sucess", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/sucess.html", String(), false, processor);
  });
  server.on("/warning", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/warning.html");
  });
  server.on("/failure", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/failure.html");
  });
  server.on("/deleteuser", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (removeUser(index_user_for_removal)) {
      sucess_msg = "Usuário excluido do registro.";
      request->send(SPIFFS, "/sucess.html", String(), false, processor);
    }
    else
      request->send(SPIFFS, "/failure.html", String(), false, processor);
    return;
  });
  server.on("/stylesheet.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/stylesheet.css", "text/css");
  });
  server.on("/rfid", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", rfid_card.c_str());
  });
  server.on("/message", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", monitoring_msg.c_str());
  });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    vector <String> users_data = readFile(FILENAME);

    if (request->hasParam("credit")) {
      credit_data = request->getParam("credit")->value();
      Serial.printf("Credit: %s\n", credit_data.c_str());
    }
    if (request->hasParam("info")) {
      info_data = request->getParam("info")->value();
      info_data.toUpperCase();
      Serial.printf("info: %s\n", info_data.c_str());
    }
    if (request->hasParam("rfid")) {
      id_data = request->getParam("rfid")->value();
      Serial.printf("ID: %s\n", id_data.c_str());
    }

    if (request->hasParam("remove")) {
      String user_removed = request->getParam("remove")->value();
      Serial.printf("Remover o usuário da posição : %s\n", user_removed.c_str());
      index_user_for_removal = user_removed.toInt();
      index_user_for_removal -= 1;
      request->send(SPIFFS, "/warning.html");
      return;
    }

    if (request->hasParam("add")) {
      String user_add = request->getParam("add")->value();
      Serial.printf("Adicionar crédito para o usuário da posição : %s\n", user_add.c_str());
      int indexAdd = user_add.toInt() - 1;
      addCreditUser(indexAdd, false);
      request->send(SPIFFS, "/userView.html", String(), false, processor);
      return;
    }

    if (id_data == "" || info_data == "" || credit_data == "") {
      failure_msg = "Informações de usuário estão incompletas.";
      request->send(SPIFFS, "/failure.html", String(), false, processor);
      return;
    }

    int user_index = -1;//findUser(users_data, id_data, info_data);

    if (user_index < 0) {
      Serial.println("Cadastrando novo usuário");
      addNewUser(id_data, info_data, credit_data);
      sucess_msg = "Novo usuário cadastrado.";
      request->send(SPIFFS, "/sucess.html", String(), false, processor);
    }
    else {
      Serial.printf("Usuário numero %d ja existe no banco de dados\n", user_index);
      failure_msg = "Ja existe um usuário cadastrado.";
      request->send(SPIFFS, "/failure.html", String(), false, processor);
    }
  });
  server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/logo.png", "image/jpg");
  });

  // Inicia o serviço
  server.begin();
}

void loop() {

  // Procure por novos cartões.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  //Faça a leitura do ID do cartão
  if (mfrc522.PICC_ReadCardSerial()) {
    Serial.print("UID da tag :");

    String rfid_data = "";
    for (uint8_t i = 0; i < mfrc522.uid.size; i++)
    {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      rfid_data.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      rfid_data.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.println();
    rfid_data.toUpperCase();
    rfid_card = rfid_data;

    //Carregue o arquivo de cadastro
    vector <String> users_data = readFile(FILENAME);

    //Faça uma busca pelo id
    int user_index = findUser(users_data, rfid_data, "");

    if (user_index < 0) {

      Serial.printf("Nenhum usuário encontrado\n");

      if (monitoring)
      {
        monitoring_msg = "USUÁRIO NÃO ENCONTRADO";
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_BLUE, HIGH);
        delay(5000);
        monitoring_msg = "PEDÁGIO FECHADO";
        digitalWrite(LED_BLUE, LOW);
        digitalWrite(LED_RED, HIGH);
        rfid_card = "";
      }
    }
    else {

      Serial.printf("Usuário %d encontrado\n", user_index);

      if (monitoring)
      {
        addCreditUser(user_index, true);
        monitoring_msg = "PEDÁGIO ABERTO";
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, HIGH);
        delay(5000);
        monitoring_msg = "PEDÁGIO FECHADO";
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, HIGH);
        rfid_card = "";
      }
    }

    Serial.println();
  }
}

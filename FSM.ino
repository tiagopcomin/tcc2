//Bibliotecas:
#include "Arduino.h" //Biblioteca padrão do Arduino
#include <Wire.h> //Biblioteca necessária para comunicação I2C, usando SDA e SCL
#include <LiquidCrystal_I2C.h> //Biblioteca para funcionamento do Display LCD por comunicação I2C
#include "I2CKeyPad.h" //Biblioteca para funcionamento do teclado matricial por comunicação I2C 
#include <Ultrasonic.h> //Biblioteca para funcionamento do sensor ultrassônico
#include "SoftwareSerial.h" //Biblioteca usada para criação de portas seriais virtuais (UART)
#include "DFRobotDFPlayerMini.h" //Biblioteca para funcionamento do módulo MP3
#include <CD74HC4067.h> //Biblioteca para funcionamento do multiplexador para os LEDs

//Definições
#define pinRx 11 //Pino Rx para o módulo MP3
#define pinTx 10 //Pino Tx para o módulo MP3
#define volumeMP3 22 //Volume de reprodução do módulo MP3
#define trigPin 13 //Definição do pino Trigger do sensor ultrassônico
#define echoPin 12 //Definição do pino Echo do sensor ultrassônico
#define button_reset 3 //Definição da porta do botão de pressão
#define button_return 2 //Definição da porta do botão de pressão
#define button_train 4 //Definição da porta do botão de pressão

//Variáveis globais:
void(*PonteiroDeFuncao) ();  //Ponteiro de função da máquina de estados
const int common_pin = A0; //Pino comum que compartilha de suas funções com os outros canais do CH74HC4067
unsigned long lastButtonCheckTime = 0;
const unsigned long buttonCheckInterval = 100;

//Definição da posição no array das teclas
const int TECLA_1 = 0; 
const int TECLA_2 = 1; 
const int TECLA_3 = 2; 
const int TECLA_7 = 8; 
const int TECLA_0 = 13;

const int MAX_KEYS = 7; //Máximo de teclas para o modo de treino
const int numReadings = 10; //Definição da quantidade de leituras necessárias

char keymap[19] = "123N456N789N*0#F"; //Mapeamento das teclas [0][1][2][4][5][6][8][9][10][12][13][14]
char notas[][3] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}; //Notas para exibição no display

//Definição da sequência de músicas a ser tocada de acordo com a escala
int scaleCM[] = {1, 3, 5, 6, 8, 10, 12}; 
int scaleCsM[] = {2, 4, 6, 7, 9, 11, 1};
int scaleDM[] = {3, 5, 7, 8, 10, 12, 2};
int scaleDsM[] = {4, 7, 8, 9, 11, 1, 3};
int scaleEM[] = {5, 7, 9, 10, 12, 2, 4};
int scaleFM[] = {6, 8, 10, 11, 1, 3, 5};
int scaleFsM[] = {7, 9, 11, 12, 2, 4, 6};
int scaleGM[] = {8, 10, 12, 1, 3, 5, 7};
int scaleGsM[] = {9, 11, 1, 2, 3, 6, 8};
int scaleAM[] = {10, 12, 2, 3, 5, 7, 9};
int scaleAsM[] = {11, 1, 3, 4, 6, 8, 10};
int scaleBM[] = {12, 2, 4, 5, 7, 9, 11};
int scaleCm[] = {1, 3, 4, 6, 8, 9, 11};
int scaleCsm[] = {2, 4, 5, 7, 9, 10, 12};
int scaleDm[] = {3, 5, 6, 8, 10, 11, 1};
int scaleDsm[] = {4, 6, 7, 9, 11, 12, 2};
int scaleEm[] = {5, 7, 8, 10, 12, 1, 3};
int scaleFm[] = {6, 8, 9, 11, 1, 2, 4};
int scaleFsm[] = {7, 9, 10, 12, 2, 3, 5};
int scaleGm[] = {8, 10, 11, 1, 3, 4, 6};
int scaleGsm[] = {9, 11, 12, 2, 4, 5, 7};
int scaleAm[] = {10, 12, 1, 3, 5, 6, 8};
int scaleAsm[] = {11, 1, 2, 4, 6, 7, 11};
int scaleBm[] = {12, 2, 3, 5, 7, 8, 10};

int escolha_escala = 0; //Variável de controle de escolha da escala
int escolha_nota = 0; //Variável de controle de escolha da nota
int order = 0; //Variável de controle para ordenar corretamente
int treino = 0; //Variável de controle para modo de treino
int keyCounter = 0; //Variável de controle para contagem de teclas
int pressedKeys[8]; //Variável de controle para comparar esse array com a escala correta

//Inicialização das bibliotecas
LiquidCrystal_I2C mylcd(0x27,20,4); //Inicialização do objeto mylcd no endereço para o módulo I2C e do tamanho do display
I2CKeyPad keyPad(0x20); //Inicialização do objeto keyPad no endereço para o módulo I2C do teclado
SoftwareSerial playerMP3Serial(pinRx, pinTx); //Inicialização do objeto playerMP3Serial nos pinos designados
DFRobotDFPlayerMini playerMP3; //Inicialização do playerMP3 que controlará as funções do módulo
Ultrasonic ultrasonic(trigPin, echoPin); //Inicialização do objeto ultrasonic nos pinos designados
CD74HC4067 my_mux(5, 6, 7, 8); //Inicialização do objeto my_mux nas portas conectadas para controle dos LEDs

//Prototipos de função
void Inicio(void); //Função inicial com a mensagem de apresentação
void EscolhaModo(void); //Função para escolha do modo de utilização
void EcoMusical(void); //Função para interação com o sensor ultrassônico
void TeclaMusical(void); //Função para interação com o teclado
void EscalaSonora(void); //Função para escolha da escala para reprodução
void EscolhaNotas(void); //Função para escolha da nota para reprodução
void selecaoEscala(void); //Função para seleção da escala de acordo com as escolhas
void ReproducaoEscala(int* sequence); //Função que reproduz a escala da nota de acordo com as escolhas
void TreinoEscala(void); //Função para seleção de como tratar a escala reproduzida
void ResultadoTreino(int* sequence); //Função para lidar com o resultado do treino
void checkResetButton();
void checkReturnButton();

void checkResetButton() { //Função para reset das funções
  if (digitalRead(button_reset) == LOW) {
    PonteiroDeFuncao = Inicio;
    mylcd.clear();
  }
}

void checkReturnButton() { //Função para retorno das funções
  if (digitalRead(button_return) == LOW) {
    PonteiroDeFuncao = EscolhaModo;
    mylcd.clear();
  }
}

void setup() {
	
  mylcd.init(); //Inicialização do LCD
  mylcd.backlight(); //Inicialização do backlight do LCD
  mylcd.clear(); //Limpeza da mensagem do LCD
  mylcd.setCursor(2,0); //Posicionamento da mensagem
  mylcd.print("Inicializando..."); //Mensagem enquanto as funções são inicializadas

  playerMP3Serial.begin(9600); //Inicialização da serial virtual do módulo MP3
  playerMP3.begin(playerMP3Serial); //Inicialização do objeto que controla o módulo MP3
  playerMP3.volume(volumeMP3); //Inicialização do volume do MP3

  pinMode(common_pin, OUTPUT); //Inicialização do pino comum aos LEDs como saída
  pinMode(trigPin, OUTPUT); //Inicialização do pino trigger do sensor ultrassônico como saída
  pinMode(echoPin, INPUT); //Inicialização do pino echo do sensor ultrassônico como entrada
  pinMode(button_reset, INPUT); //Inicialização do botão de pressão como entrada
  pinMode(button_train, INPUT); //Inicialização do botão de pressão como entrada
  pinMode(button_return, INPUT); //Inicialização do botão de pressão como entrada

  mylcd.clear();

  PonteiroDeFuncao = Inicio; //Aponta para o estado inicial
}

void loop() {
	
  (*PonteiroDeFuncao)(); //Chama a função apontada pelo ponteiro de função, chamando assim o estado atual
}


void Inicio() {
  
  mylcd.setCursor(5,0); //Posicionamento da mensagem no LCD
  mylcd.print("TMSP 3.1.7"); //Nome e versionamento do sistema
  mylcd.setCursor(6,2); //Posicionamento da mensagem no LCD
  mylcd.print("Aperte 0"); //Instrução para iniciar
  mylcd.setCursor(4,3); //Posicionamento da mensagem no LCD
  mylcd.print("para iniciar"); //Instrução para iniciar

  if (keyPad.isPressed()) { //Função da biblioteca do teclado que garante que uma tecla foi realmente pressionada
    int key = keyPad.getKey(); // Função da biblioteca do teclado que identifica qual foi a tecla pressionada
    if (key == TECLA_0) { //Caso a tecla 0, que está na posição 13 do arry seja pressionada
      PonteiroDeFuncao = EscolhaModo; //
      mylcd.clear(); //Limpeza da tela
    }
  }
}

void EscolhaModo() {
  
  mylcd.setCursor(0,0);
  mylcd.print("Escolha o modo: ");
  mylcd.setCursor(0,1);
  mylcd.print("1. Eco Musical");
  mylcd.setCursor(0,2);
  mylcd.print("2. Tecla Musical");
  mylcd.setCursor(0,3);
  mylcd.print("3. Escala Sonora");

  if (keyPad.isPressed() || digitalRead(button_reset) == LOW || digitalRead(button_return) == LOW || digitalRead(button_train) == LOW) {
  //Verificação se alguma tecla ou botão foram pressionados
    int key = keyPad.getKey();
    if (key == TECLA_1) { 
      PonteiroDeFuncao = EcoMusical; //O ponteiro aponta para a função de interação com o ultrassom
      mylcd.clear();
    }
    else if (key == TECLA_2) { 
      PonteiroDeFuncao = TeclaMusical; //O ponteiro aponta para a função de interação com o teclado
      mylcd.clear();
    }
    else if (key == TECLA_3) {
      PonteiroDeFuncao = EscalaSonora; //O ponteiro aponta para a função de escolha da escala desejada
      mylcd.clear();
    }
    else if (digitalRead(button_reset) == LOW) {
      PonteiroDeFuncao = Inicio; //O ponteiro aponta para a função inicial
      mylcd.clear();
    }
    else if (digitalRead(button_return) == LOW) {
      PonteiroDeFuncao = Inicio; //O ponteiro aponta para a função inicial
      mylcd.clear();
    }
    else if (digitalRead(button_train) == LOW) { //O ponteiro leva para o modo de treino
      PonteiroDeFuncao = EscalaSonora;
      treino = 1;
      mylcd.clear();
      mylcd.setCursor(0, 3);
      mylcd.print("MODO DE TREINO");
    }
  }
}

void EcoMusical() {
  
  unsigned long currentTime = millis();
  if (currentTime - lastButtonCheckTime >= buttonCheckInterval) {
    checkResetButton();
    checkReturnButton();
    lastButtonCheckTime = currentTime;
  }

  digitalWrite(trigPin, LOW); //Manipulação do trigger para emitir um pulso rápido
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(25);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH); //Função da biblioteca que
  // Aproximação derivada da velocidade do som (340m/s) e considerando que é uma onda que vai e volta, então seria 0,0343/2 = 0,0171 
  float averageDistance = duration / 58.00; 

   //Se a distância for válida toque a música correspondente. Os números correspondem a distância estabelecida na referência no protótipo
  if (averageDistance >= 0 && averageDistance <= 22.3) {
    int fileNumber;
    if (averageDistance <= 2.3) {
      fileNumber = 1;
    }
    else if (averageDistance > 2.3 && averageDistance <=4.1) {
      fileNumber = 2;
    }
    else if (averageDistance > 4.1 && averageDistance <=5.9) {
      fileNumber = 3;
    }
    else if (averageDistance > 5.9 && averageDistance <=7.7) {
      fileNumber = 4;
    }
    else if (averageDistance > 7.7 && averageDistance <=9.5) {
      fileNumber = 5;
    }
    else if (averageDistance > 9.5 && averageDistance <=11.3) {
      fileNumber = 6;
    }
    else if (averageDistance > 11.3 && averageDistance <=13.1) {
      fileNumber = 7;
    }
    else if (averageDistance > 13.1 && averageDistance <=14.9) {
      fileNumber = 8;
    }
    else if (averageDistance > 14.9 && averageDistance <=16.7) {
      fileNumber = 9;
    }
    else if (averageDistance > 16.7 && averageDistance <=18.5) {
      fileNumber = 10;
    }
    else if (averageDistance > 18.5 && averageDistance <=20.3) {
      fileNumber = 11;
    }
    else if (averageDistance > 20.3) {
      fileNumber = 12;
    }

    playerMP3.playFolder(1, fileNumber); //Execução da música de acordo com a distância
    digitalWrite(common_pin, HIGH); //Definição do pino comum aos LEDs como ligados
    my_mux.channel(fileNumber - 1); //Acesso ao canal relacionado ao LED

    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Eco Musical");
    mylcd.setCursor(0, 2);
    mylcd.print("Nota: ");
    mylcd.print(notas[fileNumber - 1]);
    mylcd.setCursor(0, 3);
    mylcd.print("Distancia: ");
    mylcd.print(averageDistance);
    mylcd.print(" cm");

    delay(1000);
  }
}

void TeclaMusical() { //De acordo com a tecla pressionada, uma nota diferente é executada

  if (digitalRead(button_reset) == LOW || digitalRead(button_return) == LOW) {
    if (digitalRead(button_reset) == LOW) {
      PonteiroDeFuncao = Inicio; //O ponteiro aponta para a função inicial
      mylcd.clear();
    }
    else if (digitalRead(button_return) == LOW) {
      PonteiroDeFuncao = EscolhaModo; //O ponteiro aponta para a função inicial
      mylcd.clear();
    }
  }

  else if (keyPad.isPressed()) {
    int key = keyPad.getKey();
    if (key >= 0 && key <= 14) { //O teclado reconhece a posição em um array da tecla pressionada
                                 // e assim, é preciso reorganizar a posição, com a ordem da música e a tecla
      if (key >= 0 && key <= 2) {
        order = key + 1;
      }
      else if (key >= 4 && key <= 6) {
        order = key;
      }
      else if (key >= 8 && key <= 10) {
        order = key - 1;
      }
      else if (key >= 12 && key <= 14) {
        order = key - 2;
      }

      playerMP3.playFolder(1, order);
      digitalWrite(common_pin, HIGH);
      my_mux.channel(order-1);
      
      mylcd.clear();
      mylcd.setCursor(0, 0);
      mylcd.print("Tecla Musical");
      mylcd.setCursor(0, 2);
      mylcd.print("Nota: ");
      mylcd.print(notas[order - 1]);
      delay(1000);
    }
  }
}

void EscalaSonora() { //Escolha por meio de teclas da escala desejada
  
  mylcd.setCursor(0,0);
  mylcd.print("Escolha a escala: ");
  mylcd.setCursor(0,1);
  mylcd.print("1. Escala Maior");
  mylcd.setCursor(0,2);
  mylcd.print("2. Escala Menor");

  if (digitalRead(button_reset) == LOW || digitalRead(button_return) == LOW) {
    if (digitalRead(button_reset) == LOW) {
      PonteiroDeFuncao = Inicio; //O ponteiro aponta para a função inicial
      mylcd.clear();
    }
    else if (digitalRead(button_return) == LOW) {
      PonteiroDeFuncao = EscolhaModo; //O ponteiro aponta para a função inicial
      mylcd.clear();
    }
  }
  else if (keyPad.isPressed()) {
    int key = keyPad.getKey();
    if (key == TECLA_1) { //Caso a tecla 1 seja pressionada, que é a tecla na posição 0 do array
      escolha_escala = 1;
      PonteiroDeFuncao = EscolhaNotas; //O ponteiro aponta para a função de interação com o ultrassom
      mylcd.clear();
    }
    else if (key == TECLA_2) { //Caso a tecla 2 seja pressionada, que é a tecla na posição 1 do array
      escolha_escala = 2;
      PonteiroDeFuncao = EscolhaNotas; //O ponteiro aponta para a função de interação com o teclado
      mylcd.clear();
    }
  }
}

void EscolhaNotas() { //Escolha por meio de teclas da nota desejada

  mylcd.setCursor(0,0);
  mylcd.print("Escolha a nota: ");
  
  mylcd.setCursor(1,1);
  mylcd.print("1.C");
  mylcd.setCursor(5,1);
  mylcd.print("2.C#");
  mylcd.setCursor(10,1);
  mylcd.print("3.D");
  mylcd.setCursor(14,1);
  mylcd.print("4.D#");

  mylcd.setCursor(2,2);
  mylcd.print("5.E");
  mylcd.setCursor(6,2);
  mylcd.print("6.F");
  mylcd.setCursor(10,2);
  mylcd.print("7.F#");
  mylcd.setCursor(15,2);
  mylcd.print("8.G");

  mylcd.setCursor(1,3);
  mylcd.print("9.G#");
  mylcd.setCursor(6,3);
  mylcd.print("*.A");
  mylcd.setCursor(10,3);
  mylcd.print("0.A#");
  mylcd.setCursor(15,3);
  mylcd.print("#.B");

  if (digitalRead(button_reset) == LOW || digitalRead(button_return) == LOW) {
    if (digitalRead(button_reset) == LOW) {
      PonteiroDeFuncao = Inicio; //O ponteiro aponta para a função inicial
      mylcd.clear();
    }
    else if (digitalRead(button_return) == LOW) {
      PonteiroDeFuncao = EscalaSonora; //O ponteiro aponta para a função inicial
      mylcd.clear();
    }
  }
  else if (keyPad.isPressed()) {
    int key = keyPad.getKey();
    // Checa se a nota está dentro de um intervalo válido
    if (key >= 0 && key <= 14) {
      if (key >= 0 && key <= 2) {
        escolha_nota = key + 1;
        if (treino == 1) {
          PonteiroDeFuncao = TreinoEscala;
          mylcd.clear();
        }
        else {
          PonteiroDeFuncao = SelecaoEscala; //O ponteiro aponta para a função de interação com o ultrassom
          mylcd.clear();
        }
      }
      else if (key >= 4 && key <= 6) {
        escolha_nota = key;
        if (treino == 1) {
          PonteiroDeFuncao = TreinoEscala;
          mylcd.clear(); 
        }
        else {
          PonteiroDeFuncao = SelecaoEscala; //O ponteiro aponta para a função de interação com o ultrassom
          mylcd.clear();
        }
      }
      else if (key >= 8 && key <= 10) {
        escolha_nota = key - 1;
        if (treino == 1) {
          PonteiroDeFuncao = TreinoEscala;
          mylcd.clear(); 
        }
        else {
          PonteiroDeFuncao = SelecaoEscala; //O ponteiro aponta para a função de interação com o ultrassom
          mylcd.clear();
        }
      }
      else if (key >= 12 && key <= 14) {
        escolha_nota = key - 2;
        if (treino == 1) {
          PonteiroDeFuncao = TreinoEscala;
          mylcd.clear(); 
        }
        else {
          PonteiroDeFuncao = SelecaoEscala; //O ponteiro aponta para a função de interação com o ultrassom
          mylcd.clear();
        }
      }
    }
  }
}

void SelecaoEscala() { //Definição da escala a ser reproduzida de acordo com as escolhas

  if (escolha_escala == 1 && escolha_nota == 1) {
    ReproducaoEscala(scaleCM);
  }
  else if (escolha_escala == 1 && escolha_nota == 2) {
    ReproducaoEscala(scaleCsM);
  }
  else if (escolha_escala == 1 && escolha_nota == 3) {
    ReproducaoEscala(scaleDM);
  }
  else if (escolha_escala == 1 && escolha_nota == 4) {
    ReproducaoEscala(scaleDsM);
  }
  else if (escolha_escala == 1 && escolha_nota == 5) {
    ReproducaoEscala(scaleEM);
  }
  else if (escolha_escala == 1 && escolha_nota == 6) {
    ReproducaoEscala(scaleFM);
  }
  else if (escolha_escala == 1 && escolha_nota == 7) {
    ReproducaoEscala(scaleFsM);
  }
  else if (escolha_escala == 1 && escolha_nota == 8) {
    ReproducaoEscala(scaleGM);
  }
  else if (escolha_escala == 1 && escolha_nota == 9) {
    ReproducaoEscala(scaleGsM);
  }
  else if (escolha_escala == 1 && escolha_nota == 10) {
    ReproducaoEscala(scaleAM);
  }
  else if (escolha_escala == 1 && escolha_nota == 11) {
    ReproducaoEscala(scaleAsM);
  }
  else if (escolha_escala == 1 && escolha_nota == 12) {
    ReproducaoEscala(scaleBM);
  }

  else if (escolha_escala == 2 && escolha_nota == 1) {
    ReproducaoEscala(scaleCm);
  }
  else if (escolha_escala == 2 && escolha_nota == 2) {
    ReproducaoEscala(scaleCsm);
  }
  else if (escolha_escala == 2 && escolha_nota == 3) {
    ReproducaoEscala(scaleDm);
  }
  else if (escolha_escala == 2 && escolha_nota == 4) {
    ReproducaoEscala(scaleDsm);
  }
  else if (escolha_escala == 2 && escolha_nota == 5) {
    ReproducaoEscala(scaleEm);
  }
  else if (escolha_escala == 2 && escolha_nota == 6) {
    ReproducaoEscala(scaleFm);
  }
  else if (escolha_escala == 2 && escolha_nota == 7) {
    ReproducaoEscala(scaleFsm);
  }
  else if (escolha_escala == 2 && escolha_nota == 8) {
    ReproducaoEscala(scaleGm);
  }
  else if (escolha_escala == 2 && escolha_nota == 9) {
    ReproducaoEscala(scaleGsm);
  }
  else if (escolha_escala == 2 && escolha_nota == 10) {
    ReproducaoEscala(scaleAm);
  }
  else if (escolha_escala == 2 && escolha_nota == 11) {
    ReproducaoEscala(scaleAsm);
  }
  else if (escolha_escala == 2 && escolha_nota == 12) {
    ReproducaoEscala(scaleBm);
  }
}

void ReproducaoEscala(int* sequence) {
  
  unsigned long currentTime = millis();
  if (currentTime - lastButtonCheckTime >= buttonCheckInterval) {
    checkResetButton();
    checkReturnButton();
    lastButtonCheckTime = currentTime;
  }

  for (int i = 0; i < 7; i++) {
      int fileNumber = sequence[i];

      playerMP3.playFolder(1, fileNumber);
      digitalWrite(common_pin, HIGH);
      my_mux.channel(fileNumber - 1);
      
      mylcd.clear();
      mylcd.setCursor(0, 0);
      if (escolha_escala == 1){
        mylcd.print("Escala Maior");
      }
      else if (escolha_escala == 2) {
        mylcd.print("Escala Menor");
      }
      mylcd.setCursor(0, 2);
      mylcd.print("Nota: ");
      mylcd.print(notas[fileNumber - 1]);
      delay(1000);
    }
}

void TreinoEscala() { //Função para gravar as teclas pressionadas

  if (digitalRead(button_reset) == LOW) {
    if (digitalRead(button_reset) == LOW) {
      PonteiroDeFuncao = Inicio; //O ponteiro aponta para a função inicial
      mylcd.clear();
    }
  }

  else if (keyPad.isPressed()) {
    int key = keyPad.getKey();
    int trainingOrder=0;
    if (key >= 0 && key <= 14) {

      if (key >= 0 && key <= 2) {
        trainingOrder = key + 1;
      }
      else if (key >= 4 && key <= 6) {
        trainingOrder = key;
      }
      else if (key >= 8 && key <= 10) {
        trainingOrder = key - 1;
      }
      else if (key >= 12 && key <= 14) {
        trainingOrder = key - 2;
      }

      pressedKeys[keyCounter] = trainingOrder;
      keyCounter++;

      playerMP3.playFolder(1, trainingOrder);
      digitalWrite(common_pin, HIGH);
      my_mux.channel(trainingOrder-1);
      
      mylcd.clear();
      mylcd.setCursor(0, 0);
      mylcd.print("Modo de Treino");
      mylcd.setCursor(0, 1);
      mylcd.print("Pressione  as teclas:");
      mylcd.setCursor(0, 2);
      mylcd.print("Notas: ");
      mylcd.setCursor(0, 3);
      
      for (int i = 1; i < keyCounter; ++i) {
        mylcd.print(notas[pressedKeys[i] - 1]);
        if (i < keyCounter - 1) {
          mylcd.print(","); // Adicionando uma vírgula entre as notas
        }
      }
      delay(1000);

      if (keyCounter == 8) { //São 7 teclas pressionadas, mas o array é populado naturalmente com a tecla atual
        // Chamada da função quando chegamos ao número desejado de teclas pressionadas
        PonteiroDeFuncao = ComparaEscala;
        mylcd.clear();
      }
    }
  }
}

void ComparaEscala() { //Definição da escala a ser comparada de acordo com as escolhas
  
  if (escolha_escala == 1 && escolha_nota == 1) {
    ResultadoTreino(scaleCM);
  }
  else if (escolha_escala == 1 && escolha_nota == 2) {
    ResultadoTreino(scaleCsM);
  }
  else if (escolha_escala == 1 && escolha_nota == 3) {
    ResultadoTreino(scaleDM);
  }
  else if (escolha_escala == 1 && escolha_nota == 4) {
    ResultadoTreino(scaleDsM);
  }
  else if (escolha_escala == 1 && escolha_nota == 5) {
    ResultadoTreino(scaleEM);
  }
  else if (escolha_escala == 1 && escolha_nota == 6) {
    ResultadoTreino(scaleFM);
  }
  else if (escolha_escala == 1 && escolha_nota == 7) {
    ResultadoTreino(scaleFsM);
  }
  else if (escolha_escala == 1 && escolha_nota == 8) {
    ResultadoTreino(scaleGM);
  }
  else if (escolha_escala == 1 && escolha_nota == 9) {
    ResultadoTreino(scaleGsM);
  }
  else if (escolha_escala == 1 && escolha_nota == 10) {
    ResultadoTreino(scaleAM);
  }
  else if (escolha_escala == 1 && escolha_nota == 11) {
    ResultadoTreino(scaleAsM);
  }
  else if (escolha_escala == 1 && escolha_nota == 12) {
    ResultadoTreino(scaleBM);
  }

  else if (escolha_escala == 2 && escolha_nota == 1) {
    ResultadoTreino(scaleCm);
  }
  else if (escolha_escala == 2 && escolha_nota == 2) {
    ResultadoTreino(scaleCsm);
  }
  else if (escolha_escala == 2 && escolha_nota == 3) {
    ResultadoTreino(scaleDm);
  }
  else if (escolha_escala == 2 && escolha_nota == 4) {
    ResultadoTreino(scaleDsm);
  }
  else if (escolha_escala == 2 && escolha_nota == 5) {
    ResultadoTreino(scaleEm);
  }
  else if (escolha_escala == 2 && escolha_nota == 6) {
    ResultadoTreino(scaleFm);
  }
  else if (escolha_escala == 2 && escolha_nota == 7) {
    ResultadoTreino(scaleFsm);
  }
  else if (escolha_escala == 2 && escolha_nota == 8) {
    ResultadoTreino(scaleGm);
  }
  else if (escolha_escala == 2 && escolha_nota == 9) {
    ResultadoTreino(scaleGsm);
  }
  else if (escolha_escala == 2 && escolha_nota == 10) {
    ResultadoTreino(scaleAm);
  }
  else if (escolha_escala == 2 && escolha_nota == 11) {
    ResultadoTreino(scaleAsm);
  }
  else if (escolha_escala == 2 && escolha_nota == 12) {
    ResultadoTreino(scaleBm);
  }
}

void ResultadoTreino(int* sequence) { //Mostra no display LCD as mensagens de acordo
  
  unsigned long currentTime = millis();
  if (currentTime - lastButtonCheckTime >= buttonCheckInterval) {
    mylcd.clear();
    checkResetButton();
    lastButtonCheckTime = currentTime;
  }

  // Criação de um array com 7 elementos para comparação
  int trimmedPressedKeys[7]; //

  // Ignorar o primeiro elemento que é atribuido automaticamente
  for (int i = 0; i < 7; i++) {
    trimmedPressedKeys[i] = pressedKeys[i + 1];
  }
  
  //Comparação das notas pressionadas e da sequência pré-estabelecida
  bool match = true;
  for (int i = 0; i < 7; ++i) {
      if (trimmedPressedKeys[i] != sequence[i]) {
          match = false;
          break;
      }
  }

  //Se forem iguais exibe uma mensagem
  if (match) {
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Sucesso");
  }
  //Se forem diferentes mostra o que foi pressionado e como seria o correto
  else {
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Treino: ");
    mylcd.setCursor(0, 1);

    for (int i = 1; i < 8; ++i) {
      int noteIndex = (pressedKeys[i] - 1) % 12;   // Garantia de que esteja no tamanho correto analisado
      mylcd.print(notas[noteIndex]);
      mylcd.print(" ");
    }

    mylcd.setCursor(0, 2);
    mylcd.print("Correto: ");
    mylcd.setCursor(0, 3);
    
    for (int i = 0; i < MAX_KEYS; ++i) {
      int adjustedIndex = (sequence[i] - 1) % 12; 
      mylcd.print(notas[adjustedIndex]);
      mylcd.print(" ");
    }
  }

  //Resetando o counter para novas informações
  keyCounter = 0;

  delay(1000);
  mylcd.clear();
}
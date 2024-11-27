    #include <Servo.h>
    #include <Wire.h>
    #include <LiquidCrystal_I2C.h>
    #include <Ultrasonic.h>

    // Pinos do sensor Ultrassônico
    const int trigger = A0;
    const int echo = A1;

    // Pinos do sensor de refletância
    const int sensorPin1 = A3; //Direita
    const int sensorPin2 = A2; //Esquerda

    // Pinos do sensor RGB
    const int OUT = 2;
    const int S2 = 3;
    const int S3 = 4;
    const int S0 = 12;
    const int S1 = 13;

    // Pinos dos motores
    const int motor1 = 5;
    const int motor2 = 6;

    // Pino do servo motor 
    const int servoPin = 9;

    //Pino Led vermelho
    const int pinoLed = 10;

    //Enums
    enum CoresDisponiveis {
        VERMELHO,
        VERDE,
        AMARELO,
        OUTRA
    };

    enum AnguloServo {
        ESQUERDA = 115,
        CENTRAL = 90,
        DIREITA = 65
    };

    enum EstadoMotor {
        DESLIGADO = 0, // Associando ao valor de LOW (0)
        LIGADO = 120    // Associando ao valor de HIGH (1)
    };

    class SensorDeCores {
      private:
          int pinoOUT, pinoS2, pinoS3, pinoS0, pinoS1;
          CoresDisponiveis corAtual;
          
          bool amareloRepetido;             // Controla a passagem do amarelo
          bool primeiraPassagemAmarelo;   //Primeira passagem Amarelo
          bool bloqueado;                 //Controle Verde-Vermelho

      public:
          // Construtor
          struct EstadosMemoria {
              bool amareloRepetido;
              bool primeiraPassagemAmarelo;
              bool bloqueado;
          };

          SensorDeCores(int out, int s2, int s3, int s0, int s1)
              : pinoOUT(out), pinoS2(s2), pinoS3(s3), corAtual(CoresDisponiveis::OUTRA),
              amareloRepetido(false), 
              primeiraPassagemAmarelo(false), 
              bloqueado(true) {}

          void setup() {
              pinMode(pinoS2, OUTPUT);
              pinMode(pinoS3, OUTPUT);
              pinMode(pinoOUT, INPUT);
              pinMode(S1, OUTPUT);
              pinMode(S2, OUTPUT);
          }     

          void definirFrequencia(int estado1, int estado2){
              digitalWrite(S1, estado1);
              digitalWrite(S0, estado2);
          }

          int lerCor(int S2State, int S3State) {
              digitalWrite(pinoS2, S2State);
              digitalWrite(pinoS3, S3State);
              delay(50);
              return pulseIn(pinoOUT, LOW);
          }

          int* lerCoresRGB(int multiplicador = 1) {
              static int rgb[4];
              rgb[0] = lerCor(LOW, LOW) * multiplicador;      // Vermelho
              rgb[1] = lerCor(HIGH, HIGH) * multiplicador;    // Verde
              rgb[2] = lerCor(LOW, HIGH) * multiplicador;     // Azul
              rgb[3] = lerCor(HIGH, LOW) * multiplicador;     // No filter
              // Aplica o mapeamento invertido para cada valor
              for (int i = 0; i < 4; i++) {
                rgb[i] = mapInverted(rgb[i], 0, 255, 255, 0);
              }
              return rgb;
          }
          // Função para mapear valores invertidos
          int mapInverted(int cor, int original_min, int original_max, int inverted_min, int inverted_max) {
              cor = constrain(cor, original_min, original_max);
              return inverted_max - ((cor - original_min) * (inverted_max - inverted_min) / (original_max - original_min));
          }

        CoresDisponiveis definirCor(int* rgb, int valorSemFiltroAmbiente, int valorSomaRGBAmbiente ) {
            const int getR = rgb[0];
            const int getG = rgb[1];
            const int getB = rgb[2];
            const int getN = rgb[3];

            // Verifica se os valores são fora do padrão ou muito baixos
            if (getR + getG + getB > valorSomaRGBAmbiente || getN > valorSemFiltroAmbiente) {
              Serial.println("Outra");
                return CoresDisponiveis::OUTRA; // Baixa luz ou nenhuma cor detectada
            }
            if ((getR + getG < valorSomaRGBAmbiente) && (getR < getB && getG < getB) && getB > 20) {
                Serial.println("Amarelo");
                return CoresDisponiveis::AMARELO;
            }
            // Detecta vermelho: é o valor mínimo entre R, G e B
            if (getR < getG && getR < getB) {
                Serial.println("Vermelho");
                return CoresDisponiveis::VERMELHO;
            }
            // Detecta verde: é o valor mínimo entre G, R e B
            if (getG < getR && getG <= getB) {
                Serial.println("Verde");
                return CoresDisponiveis::VERDE;
            }
        }

        EstadosMemoria getEstadosMemoria() const {
            return {amareloRepetido, primeiraPassagemAmarelo, bloqueado};
        }
        void atualizarMemoria(CoresDisponiveis novaCor) {
            switch (novaCor) {
                case CoresDisponiveis::AMARELO:
                    gerenciarAmarelo();
                    break;

                case CoresDisponiveis::VERDE:
                case CoresDisponiveis::VERMELHO:
                    gerenciarVerdeVermelho(novaCor);
                    resetarAmarelo();
                    break;

                default:
                    resetarAmarelo();
                    break;
            }
            corAtual = novaCor;
        }

        void gerenciarVerdeVermelho(CoresDisponiveis cor){
            if (cor == CoresDisponiveis::VERMELHO) {
                bloqueado = true;      // Ativa o bloqueio
            }else if (cor == CoresDisponiveis::VERDE) {
                bloqueado = false;     // Libera o bloqueio
            }
        }

        void resetarAmarelo() {
           amareloRepetido = false;
        } 
        
        void gerenciarAmarelo(){
            if (amareloRepetido) {
                return;
            }
            primeiraPassagemAmarelo = !primeiraPassagemAmarelo;
            return;
        }
    };
    class SensoresArduino{
      private:
          int sensorPin1, sensorPin2, trigger, echo;
          Ultrasonic ultrasonic;
      public:
          SensoresArduino(int refle1, int refle2, int dTrigger, int dEcho)
              : sensorPin1(refle1),  sensorPin2(refle2), trigger(dTrigger), echo(dEcho), ultrasonic(dTrigger, dEcho) {}
          void setup() {
              pinMode(sensorPin1, INPUT);
              pinMode(sensorPin2, INPUT);
          }
          int medirDistancia() {
              return ultrasonic.read(CM);  
          }

          int* lerSensoresRefletancia() {
              static int leituras[2];
              leituras[0] = analogRead(sensorPin1);
              leituras[1] = analogRead(sensorPin2);
              return leituras;
          }
      };
      class DisplayLCD{
      private:
          LiquidCrystal_I2C& lcd;
      public:
          DisplayLCD(LiquidCrystal_I2C& lcdInstance) : lcd(lcdInstance) {}

          void setup(){
              // Configuração do LCD
              lcd.init();
              lcd.backlight();
          }
          void exibirNoLCD(int distancia, int* leiturasRefletancia, int* rgb) {
              lcd.clear();

              lcd.setCursor(0, 0);
              lcd.print("E");
              lcd.print(leiturasRefletancia[1]);
              lcd.print(" D");
              lcd.print(leiturasRefletancia[0]);
              lcd.print(" Dt:");
              lcd.print(distancia);

              lcd.setCursor(0, 1);
              lcd.print("RGB: ");
              lcd.print(rgb[0] > 999 ? 999 : rgb[0]);
              lcd.print(" ");
              lcd.print(rgb[1] > 999 ? 999 : rgb[1]);
              lcd.print(" ");
              lcd.print(rgb[2] > 999 ? 999 : rgb[2]);
          }
          void lcdInit(int tempoInit, bool comportamentoCor = false, CoresDisponiveis cor = CoresDisponiveis::OUTRA, String msg = "") {
      
              lcd.clear();

              if(comportamentoCor){
                  switch (cor)
                  {
                  case CoresDisponiveis::VERMELHO:
                      printMensagemLDC("Cor: VERMELHO");
                      printMensagemLDC(msg, 1);
                      break;

                  case CoresDisponiveis::VERDE:
                      printMensagemLDC("Cor: VERDE");
                      contagemRegressiva(tempoInit, msg);
                      break;

                  case CoresDisponiveis::AMARELO:
                      printMensagemLDC("Cor: AMARELO");
                      printMensagemLDC(msg, 1);
                      break;

                  default:
                      break;
                  }
              }else{
                  printMensagemLDC("Iniciando...");
                  contagemRegressiva(tempoInit, "Iniciando em: ");
              }
          }
          void contagemRegressiva(int tempoInit, String msg = "") {
              for (int i = tempoInit; i >= 0; i--) {
                  lcd.setCursor(0, 1);
                  lcd.print(msg + i + "   ");
                  delay(1000);
              }
          }
          void printMensagemLDC(String msg, int linha = 0, int coluna = 0){
              lcd.setCursor(coluna, linha);
              lcd.print(msg);
          }
          void lcdClear(){
              lcd.clear();
          }
      };
    class ComandosMotor{
    private:
        int motor1, motor2;
        int servoPin;
        Servo servoMotor;
        AnguloServo ultimoAngulo;
      unsigned long ultimoTempoMudanca;

    public:
        ComandosMotor(int motor1Pin, int motor2Pin, int servoPin)
            :motor1(motor1Pin), motor2(motor2Pin), servoPin(servoPin),
            ultimoAngulo(AnguloServo::CENTRAL), ultimoTempoMudanca(0) {};
    
        void setup(){
            servoMotor.attach(servoPin);
            servoMotor.write(AnguloServo::CENTRAL);
            pinMode(motor1, OUTPUT);
            pinMode(motor2, OUTPUT);
        }

        void estadoMotores(EstadoMotor estado) {
            analogWrite(motor1, estado);
            analogWrite(motor2, estado);
        }
        void definirParada(int distancia, int* leiturasRefletancia, int limiteRefle, bool bloqueado) {
            if (distancia <= 6) {
                estadoMotores(EstadoMotor::DESLIGADO);  
            }
            else if(leiturasRefletancia[0] > limiteRefle && leiturasRefletancia[1] > limiteRefle){
                estadoMotores(EstadoMotor::DESLIGADO);
            }
            else if(bloqueado){
                estadoMotores(EstadoMotor::DESLIGADO);
            }
        }

        void definirRotacao(AnguloServo rotacao) {
            unsigned long tempoAtual = millis();
            // Verifica se já passou o tempo limite antes de permitir nova mudança
            if (rotacao != ultimoAngulo && (tempoAtual - ultimoTempoMudanca >= 500)) {
                servoMotor.write(rotacao);
                ultimoAngulo = rotacao;
                ultimoTempoMudanca = tempoAtual;
            }
        }

        AnguloServo definirAngulo(int* leiturasRefletancia, int limiteRefletancia) {
            if(leiturasRefletancia[1] < limiteRefletancia && leiturasRefletancia[0] > limiteRefletancia){
                return AnguloServo::ESQUERDA;
            }else if(leiturasRefletancia[1] > limiteRefletancia && leiturasRefletancia[0] < limiteRefletancia){
                return AnguloServo::DIREITA;
            }else{
                return AnguloServo::CENTRAL;
            }
        }
    };


    // Instâncias
    SensorDeCores sensorDeCor(OUT, S2, S3, S0, S1);
    SensoresArduino sensoresArduino(sensorPin1, sensorPin2, trigger, echo);
    LiquidCrystal_I2C lcdInstance(0x27, 16, 2);
    DisplayLCD lcd(lcdInstance);
    ComandosMotor motores(motor1, motor2, servoPin);

    //Logica central
    void comandosArduino(CoresDisponiveis cor, bool primeiraPassagemAmarelo, bool bloqueado = false ){
        if(bloqueado){
            lcd.lcdClear();
            lcd.printMensagemLDC("Aguardando");
            lcd.printMensagemLDC("Liberacao", 1, 0);
            delay(1000);
        }
        const int tempoInit = 3;
        switch (cor)
        {   
           case CoresDisponiveis::OUTRA:
                motores.estadoMotores(EstadoMotor::LIGADO);
                break;

            case CoresDisponiveis::VERMELHO:
                lcd.lcdInit(0, true, CoresDisponiveis::VERMELHO, "Aguardar liberar");
                motores.estadoMotores(EstadoMotor::DESLIGADO);
                delay(tempoInit * 1000);
                break;

            case CoresDisponiveis::VERDE: 
                lcd.lcdInit(5, true, CoresDisponiveis::VERDE, "Libera em: ");
                motores.estadoMotores(EstadoMotor::LIGADO);
                break;

            case CoresDisponiveis::AMARELO:
                digitalWrite(pinoLed, HIGH);
                motores.estadoMotores(EstadoMotor::DESLIGADO);
                lcd.lcdInit(0, true, 
                    CoresDisponiveis::AMARELO,
                    primeiraPassagemAmarelo ? "Descarregando..." : "Salvando..."
                );
                delay(tempoInit * 1000);
                motores.estadoMotores(EstadoMotor::LIGADO);
                digitalWrite(pinoLed, LOW);
                break;

            default:
                motores.estadoMotores(EstadoMotor::DESLIGADO);
                break;
        }
    }

    //Variaveis de Operacao
        //Limite Refletancia
        const int limiteRefletancia = 400;
        //Variaveis de Luz Ambiente
        const int valorSemFiltroAmbiente = 15;
        const int valorSomaRGBAmbiente = 90;

    void setup() {
        Serial.begin(9600);
        sensorDeCor.setup();
        sensoresArduino.setup();
        lcd.setup();
        motores.setup();
        sensorDeCor.definirFrequencia(HIGH, HIGH);
        lcd.lcdInit(5);
        //PINO LED
        pinMode(pinoLed, OUTPUT);
    }

    void loop() {
        int distancia = sensoresArduino.medirDistancia();
        int* rgb = sensorDeCor.lerCoresRGB();
        int* leiturasRefletancia = sensoresArduino.lerSensoresRefletancia();
        const auto anguloDefinido = motores.definirAngulo(leiturasRefletancia, limiteRefletancia);
        CoresDisponiveis corIdentificada = sensorDeCor.definirCor(rgb, valorSemFiltroAmbiente, valorSomaRGBAmbiente);

        SensorDeCores::EstadosMemoria estadosMemoriaDeCores = sensorDeCor.getEstadosMemoria();
        bool primeiraPassagemAmarelo = estadosMemoriaDeCores.primeiraPassagemAmarelo;
        bool bloqueado = estadosMemoriaDeCores.bloqueado;

        motores.definirRotacao(anguloDefinido);
        comandosArduino(corIdentificada, primeiraPassagemAmarelo, bloqueado);
        motores.definirParada(distancia, leiturasRefletancia, limiteRefletancia, bloqueado);
        
        sensorDeCor.atualizarMemoria(corIdentificada);

        lcd.exibirNoLCD( 
            distancia,
            leiturasRefletancia,
            rgb
        );
        printSerial(leiturasRefletancia, rgb, distancia);
    }

    void printSerial(int* leiturasRefletancia, int* rgb, int distancia){
        // Teste no Serial Monitor
        Serial.print("Refletancia Direita A3: ");
        Serial.print(leiturasRefletancia[0]);
        Serial.print(" Esquerda A2: ");
        Serial.print(leiturasRefletancia[1]);
        Serial.print(" | Distancia: ");
        Serial.print(distancia);
        Serial.print(" cm | RGB: [");
        Serial.print(rgb[0]); // Vermelho
        Serial.print(", ");
        Serial.print(rgb[1]); // Verde
        Serial.print(", ");
        Serial.print(rgb[2]); // Azul
        Serial.print(", ");
        Serial.print(rgb[3]); // NF
        Serial.println("]");
    }



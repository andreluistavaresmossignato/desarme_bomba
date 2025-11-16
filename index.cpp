#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int buzzerPin = 11;
const int ledPins[] = {10, 12, 13}; // Pinos para os LEDs (um por dígito)
const int tamanhoSenha = sizeof(ledPins) / sizeof(ledPins[0]); // 3 aqui

int fase = 1;
unsigned long inicioTempo;
unsigned long tempoFases[] = {30000UL, 20000UL, 10000UL}; // milissegundos por fase
String senha = "";
String tentativa = "";
bool perdeu = false;
bool faseVencida = false;

void setup() {
  Serial.begin(9600); //  Serial para debug/testes
  lcd.init();
  lcd.backlight();
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // garante o buzzer desligado no início
  randomSeed(analogRead(A1)); // semente aleatória

  // Define os LEDs como saída e garante que começam apagados
  for (int i = 0; i < tamanhoSenha; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  Serial.println("=== Simulador (modo de testes) ===");
  iniciarFase();
}

void loop() {
  // mesmo se perdeu, deixamos o loop rodando para possíveis futuras ações
  if (perdeu) {
    // não processar entradas depois de perder, mas mantém o display como está
    return;
  }

  unsigned long faseTempo = tempoFases[fase - 1];
  unsigned long elapsed = millis() - inicioTempo;
  long remainingSigned = (long)faseTempo - (long)elapsed;
  unsigned long restante = (remainingSigned > 0) ? (unsigned long)remainingSigned : 0UL;

  // Atualiza linha do tempo com valor correto (evita negativo)
  lcd.setCursor(0, 1);
  lcd.print("Tempo:");
  lcd.print(restante / 1000);
  lcd.print("s     ");

  if (restante == 0 && !faseVencida) {
    explodirBomba();
    return;
  }

  char tecla = keypad.getKey();
  if (tecla) {
    Serial.print("Tecla: "); Serial.println(tecla); // debug de teclas
    if (tecla == '*') {
      // limpa tentativa e apaga LEDs (pois LEDs representam acertos de posição)
      tentativa = "";
      apagarTodosLeds();
      atualizarLCD();
      Serial.println("Tentativa limpa.");
    } else if (tecla == '#') {
      // avançar fase só se já venceu essa fase
      if (faseVencida) {
        fase++;
        if (fase > 3) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("VENCEDOR!");
          digitalWrite(buzzerPin, LOW);
          perdeu = true; // fim do jogo
          Serial.println("Jogo finalizado: VENCEDOR!");
        } else {
          iniciarFase();
        }
      } else {
        Serial.println("Botao # pressionado sem vencer a fase.");
      }
    } else if (!faseVencida && tentativa.length() < tamanhoSenha) {
      // adiciona dígito à tentativa
      tentativa += tecla;
      Serial.print("Tentativa atual: "); Serial.println(tentativa);
      atualizarLCD();

      if (tentativa.length() == tamanhoSenha) {
        verificarTentativa();
      }
    }
  }
}

// === FUNÇÕES INTELIGENTES ===
void iniciarFase() {
  // Mensagem de introdução antes da fase
  digitalWrite(buzzerPin, LOW);  
  lcd.clear();
  lcd.setCursor(0, 0);
  // Fases diferentes, mensagens diferentes
  if (fase == 1) {
    lcd.print("Ha bombas nesse");
    lcd.setCursor(0, 1);
    lcd.print("aviao? Desarme!");
  } else if (fase == 2) {
    lcd.print("Dinamites nesse");
    lcd.setCursor(0, 1);
    lcd.print("banco? Evacue!");
  } else if (fase == 3) {
    lcd.print("Bomba nuclear!");
    lcd.setCursor(0, 1);
    lcd.print("Desarme ja!");
  }
  delay(2000);  // Espera 2 segundos

  // Mostra o número da fase
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Fase ");
  lcd.print(fase);
  delay(1000);  // Espera 1 segundo

  // Geração da senha aleatória
  senha = "";
  for (int i = 0; i < tamanhoSenha; i++) {
    senha += String(random(0, 10));
  }

  // LOG no Serial Monitor (modo teste)
  Serial.print(">>> Fase "); Serial.print(fase);
  Serial.print(" - senha (TEST): "); Serial.println(senha);

  tentativa = "";
  apagarTodosLeds();
  inicioTempo = millis();
  faseVencida = false;

  delay(800); // pequeno delay para leitura final
  atualizarLCD();  // Exibe a senha em branco e o tempo
}


void atualizarLCD() {
  unsigned long faseTempo = tempoFases[fase - 1];
  long remainingSigned = (long)faseTempo - (long)(millis() - inicioTempo);
  unsigned long restante = (remainingSigned > 0) ? (unsigned long)remainingSigned : 0UL;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Senha: ");
  for (int i = 0; i < tamanhoSenha; i++) {
    if (i < tentativa.length()) {
      lcd.print(tentativa.charAt(i));
    } else {
      lcd.print("_");
    }
    if (i < tamanhoSenha - 1) lcd.print(" ");
  }
  lcd.setCursor(0, 1);
  lcd.print("Tempo:");
  lcd.print(restante / 1000);
  lcd.print("s     ");
}

void verificarTentativa() {
  // Apaga todos os LEDs antes de avaliar (cada LED corresponde à posição)
  apagarTodosLeds();

  bool correto = true;

  // Verifica posição por posição; acende LED i apenas se o dígito i estiver correto
  for (int i = 0; i < tamanhoSenha; i++) {
    if (tentativa.charAt(i) == senha.charAt(i)) {
      digitalWrite(ledPins[i], HIGH);  // Acende LED da posição correta
    } else {
      digitalWrite(ledPins[i], LOW);   // Garante LED apagado se errado
      correto = false;
    }
  }

  // LOG: imprime comparação no Serial para debug
  Serial.print("Verificando tentativa: "); Serial.print(tentativa);
  Serial.print("  | senha: "); Serial.print(senha);
  Serial.print("  | resultado: ");
  Serial.println(correto ? "CORRETO" : "ERRADO");

  if (correto) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Correto!Aperte #");
    lcd.setCursor(0, 1);
    lcd.print("");
    faseVencida = true;
    Serial.println("Fase vencida. Aperte # para ir pra proxima.");
  } else {
    int foraLugar = contarMisplaced();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Errado");
    lcd.setCursor(0, 1);
    lcd.print(foraLugar);
    lcd.print(" fora pos");
    delay(1000);
    // limpa tentativa e apaga LEDs (já apagados para posições incorretas, mas garantimos)
    tentativa = "";
    apagarTodosLeds();
    atualizarLCD();
    Serial.print("Resposta errada. Digitos fora de posicao: ");
    Serial.println(foraLugar);
  }
}

int contarMisplaced() {
  // Conta quantos dígitos da tentativa existem na senha, mas em posições diferentes.
  int count = 0;
  bool usadoSenha[10]; // marca dígitos já contabilizados na senha (por posição)
  for (int i = 0; i < 10; i++) usadoSenha[i] = false;

  for (int i = 0; i < tamanhoSenha; i++) {
    char t = tentativa.charAt(i);
    if (t == senha.charAt(i)) continue; // posicionamento correto não conta aqui
    for (int j = 0; j < tamanhoSenha; j++) {
      if (j == i) continue;
      if (!usadoSenha[j] && senha.charAt(j) == t) {
        count++;
        usadoSenha[j] = true; // marca essa posição da senha como usada
        break;
      }
    }
  }
  return count;
}

void explodirBomba() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EXPLODIU!");
  Serial.println(">>> EXPLODIU! (tempo esgotado)");
  for (int i = 0; i < 6; i++) {
    tone(buzzerPin, 200 + i * 100);
    for (int j = 0; j < tamanhoSenha; j++) {
      digitalWrite(ledPins[j], HIGH); // LED sempre aceso durante o apito
    }
    delay(400); // dobra o tempo de apito
    noTone(buzzerPin); 
    apagarTodosLeds(); 
    delay(100); // breve pausa entre apitos
  }

    noTone(buzzerPin);
    // garante todos LEDs apagados após efeito
    apagarTodosLeds();
    perdeu = true;
}

void apagarTodosLeds() {
  for (int i = 0; i < tamanhoSenha; i++) {
    digitalWrite(ledPins[i], LOW);
  }
}
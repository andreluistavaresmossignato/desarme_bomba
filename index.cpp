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
const int ledPins[] = {12, 13, A0}; // LEDs para cada posição da senha


int fase = 1;
unsigned long inicioTempo;
int tempoFases[] = {30000, 20000, 10000}; // milissegundos por fase
String senha = "";
String tentativa = "";
bool perdeu = false;
bool faseVencida = false;
const int tamanhoSenha = 3;

void setup() {
  lcd.init();
  lcd.backlight();
  pinMode(buzzerPin, OUTPUT);
  randomSeed(analogRead(A0));

  // Definindo os pinos dos LEDs como saída
  for (int i = 0; i < tamanhoSenha; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  iniciarFase();

}

void loop() {
    if (perdeu) return;

  long temp = (long)tempoFases[fase - 1] - (long)(millis() - inicioTempo);
  if (temp < 0) temp = 0;
  unsigned long restante = (unsigned long) temp;

  lcd.setCursor(0, 1);
  lcd.print("Tempo:");
  lcd.print(restante / 1000);
  lcd.print("s     ");

  if (restante == 0) {
    explodirBomba();
    return;
  }
  char tecla = keypad.getKey();
  if (tecla) {
    if (tecla == '*') {
      tentativa = "";
      atualizarLCD();
    } else if (tecla == '#') {
      if (faseVencida) {
        fase++;
        if (fase > 3) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("VENCEDOR!");
          digitalWrite(buzzerPin, LOW);
          perdeu = true;
        } else {
          iniciarFase();
        }
      }
    } else if (tentativa.length() < tamanhoSenha && !faseVencida) {
      tentativa += tecla;
      atualizarLCD();

      if (tentativa.length() == tamanhoSenha) {
        verificarTentativa();
      }
    }
  }
}

void iniciarFase() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Fase ");
  lcd.print(fase);
  senha = "";
  for (int i = 0; i < tamanhoSenha; i++) {
    senha += String(random(0, 10));
  }
  tentativa = "";
  inicioTempo = millis();
  faseVencida = false;
  delay(1000);
  atualizarLCD();
}

void atualizarLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Senha: ");
  for (int i = 0; i < tamanhoSenha; i++) {
    if (i < tentativa.length()) {
      lcd.print(tentativa.charAt(i));
    } else {
      lcd.print("_");
    }
    lcd.print(" ");
  }
  lcd.setCursor(0, 1);
  lcd.print("Tempo:");
  lcd.print((tempoFases[fase - 1] - (millis() - inicioTempo)) / 1000);
  lcd.print("s     ");
}

void verificarTentativa() {
  bool correto = true;

  for (int i = 0; i < tamanhoSenha; i++) {
    // Verifica se o dígito está correto
    if (tentativa.charAt(i) == senha.charAt(i)) {
      // Acende o LED correspondente
      digitalWrite(ledPins[i], HIGH);
    } else {
      // Apaga o LED correspondente
      digitalWrite(ledPins[i], LOW);
      correto = false;
    }
  }

  if (correto) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Correto!");
    lcd.setCursor(0, 1);
    lcd.print("Aperte #");
    tone(buzzerPin, 1000, 200);
    faseVencida = true;
  } else {
    int foraLugar = contarMisplaced();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Errado");
    lcd.setCursor(0, 1);
    lcd.print(foraLugar);
    lcd.print(" fora pos");
    tone(buzzerPin, 300, 200);
    delay(1500);
    tentativa = "";
    atualizarLCD();
  }
}

int contarMisplaced() {
  int count = 0;
  for (int i = 0; i < tamanhoSenha; i++) {
    char c = tentativa.charAt(i);
    for (int j = 0; j < tamanhoSenha; j++) {
      if (c == senha.charAt(j) && i != j) {
        count++;
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
  for (int i = 0; i < 3; i++) {
    tone(buzzerPin, 200 + i * 100);
    delay(400);
  }
  noTone(buzzerPin);
  perdeu = true;
}
